/* Copyright (c) 2011, RidgeRun
 * All rights reserved.
 *
From https://www.ridgerun.com/developer/wiki/index.php/Gpio-int-test.c

 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    This product includes software developed by the RidgeRun.
 * 4. Neither the name of the RidgeRun nor the
 *    names of its contributors may be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY RIDGERUN ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL RIDGERUN BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>	// Defines signal-handling functions (i.e. trap Ctrl-C)
#include "gpio-utils.c"


/****************************************************************
* button  PIN     16     23     41      42
*         GPIO    51     49     20      7
* led     PIN     11     12     13      15
*         GPIO    30     60     31      48
*****************************************************************/


 /****************************************************************
 * Constants
 ****************************************************************/
 
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64
/***define button gpio***/
#define GPIOB1  51
#define GPIOB2  49
#define GPIOB3  20
#define GPIOB4  7
/***define led gpio***/
#define GPIOL1  30
#define GPIOL2  60
#define GPIOL3  31
#define GPIOL4  48

/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;	// Set to 0 when ctrl-c is pressed
int buttoncnt = 0;      // Added when button pressed
//Values controls led
unsigned int l1value  = 1; 
unsigned int l2value  = 1;
unsigned int l3value  = 1;
unsigned int l4value  = 1;     

/****************************************************************
 * signal_handler
 ****************************************************************/
void signal_handler(int sig);
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "Ctrl-C pressed, cleaning up and exiting..\n" );
	keepgoing = 0;
}

/****************************************************************
 * Main
 ****************************************************************/
int main(int argc, char **argv, char **envp)
{
	struct pollfd fdset[5];
	int nfds = 5;
	int timeout, rc;
	int gpiob1_fd,gpiob2_fd,gpiob3_fd,gpiob4_fd;
	int gpiol1_fd,gpiol2_fd,gpiol3_fd,gpiol4_fd;
	char buf[MAX_BUF];
	//unsigned int gpioint, gpioled; //gpioint: interrupt gpio, gpioled: led gpio
	int len;

	/*if (argc < 2) {
		printf("Usage: gpio-int <gpio-pin> gpio-led <gpio-pin>\n\n");
		printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	}*/

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	//gpioint = atoi(argv[1]);
	//gpioled = atoi(argv[2]);

	/* export and configure led gpio */
	gpio_export(GPIOL1);
	gpio_export(GPIOL2);
	gpio_export(GPIOL3);
	gpio_export(GPIOL4);
	gpio_set_dir(GPIOL1,"out");
	gpio_set_dir(GPIOL2,"out");
	gpio_set_dir(GPIOL3,"out");
	gpio_set_dir(GPIOL4,"out");
	gpiol1_fd = gpio_fd_open(GPIOL1, O_WRONLY);
	gpiol2_fd = gpio_fd_open(GPIOL2, O_WRONLY);
	gpiol3_fd = gpio_fd_open(GPIOL3, O_WRONLY);
	gpiol4_fd = gpio_fd_open(GPIOL4, O_WRONLY);

	/* export and configure button gpio */

	gpio_export(GPIOB1);
	gpio_export(GPIOB2);
	gpio_export(GPIOB3);
	gpio_export(GPIOB4);
	gpio_set_dir(GPIOB1, "in");
	gpio_set_dir(GPIOB2, "in");
	gpio_set_dir(GPIOB3, "in");
	gpio_set_dir(GPIOB4, "in");
	gpio_set_edge(GPIOB1, "rising");  // Can be rising, falling or both
	gpio_set_edge(GPIOB2, "rising"); 
	gpio_set_edge(GPIOB3, "rising"); 
	gpio_set_edge(GPIOB4, "rising"); 
	gpiob1_fd = gpio_fd_open(GPIOB1, O_RDONLY);
	gpiob2_fd = gpio_fd_open(GPIOB2, O_RDONLY);
	gpiob3_fd = gpio_fd_open(GPIOB3, O_RDONLY);
	gpiob4_fd = gpio_fd_open(GPIOB4, O_RDONLY);


	timeout = POLL_TIMEOUT;
 
	while (keepgoing) {
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;
      
		fdset[1].fd = gpiob1_fd;
		fdset[1].events = POLLPRI;

		fdset[2].fd = gpiob2_fd;
		fdset[2].events = POLLPRI;

		fdset[3].fd = gpiob3_fd;
		fdset[3].events = POLLPRI;

		fdset[4].fd = gpiob4_fd;
		fdset[4].events = POLLPRI;

		rc = poll(fdset, nfds, timeout);      

		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}
      
		if (rc == 0) {
			printf(".");
		}
            
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB1, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			l1value = !l1value;
			if (gpio_set_value(GPIOL1, l1value)){
				printf("\r\n failed to light led!");
			}
		}

		if (fdset[2].revents & POLLPRI) {
			lseek(fdset[2].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[2].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB2, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			l2value = !l2value;
			if (gpio_set_value(GPIOL2, l2value)){
				printf("\r\n failed to light led!");
			}
		}

		if (fdset[3].revents & POLLPRI) {
			lseek(fdset[3].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[3].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB3, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			l3value = !l3value;
			if (gpio_set_value(GPIOL3, l3value)){
				printf("\r\n failed to light led!");
			}
		}

		if (fdset[4].revents & POLLPRI) {
			lseek(fdset[4].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[4].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB4, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			l4value = !l4value;
			if (gpio_set_value(GPIOL4, l4value)){
				printf("\r\n failed to light led!");
			}
		}

		if (fdset[0].revents & POLLIN) {
			(void)read(fdset[0].fd, buf, 1);
			printf("\npoll() stdin read 0x%2.2X\n", (unsigned int) buf[0]);
		}

		fflush(stdout);
	}

	gpio_fd_close(gpiob1_fd);
	gpio_fd_close(gpiob2_fd);
	gpio_fd_close(gpiob3_fd);
	gpio_fd_close(gpiob4_fd);

	gpio_fd_close(gpiol1_fd);
	gpio_fd_close(gpiol2_fd);
	gpio_fd_close(gpiol3_fd);
	gpio_fd_close(gpiol4_fd);
	return 0;
}

