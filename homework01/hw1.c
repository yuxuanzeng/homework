/* created by Yuxuan Zeng for homework01, ECE 497, RHIT,09/19/2013*/
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
* etch-a-sketch display
*   0 1 2 3 4 5 6 7
* 0     x x x x
* 1   x         x
* 2 x   x     x   x
* 3 x             x
* 4 x   x     x   x
* 5 x     x x     x
* 6   x         x
* 7     x x x x
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
unsigned int M[8][8] = {0};
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

/***************************************************************
* display function
****************************************************************/
void display(int x, int y);
void display(int x, int y)
{
	system("clear");
	//printf("\r\n x = %d, y = %d",x,y);
	M[x][y] = M[x][y]||1;
	printf("\r\n %d %d %d %d %d %d %d %d",M[0][0],M[0][1],M[0][2],M[0][3],M[0][4],M[0][5],M[0][6],M[0][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[1][0],M[1][1],M[1][2],M[1][3],M[1][4],M[1][5],M[1][6],M[1][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[2][0],M[2][1],M[2][2],M[2][3],M[2][4],M[2][5],M[2][6],M[2][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[3][0],M[3][1],M[3][2],M[3][3],M[3][4],M[3][5],M[3][6],M[3][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[4][0],M[4][1],M[4][2],M[4][3],M[4][4],M[4][5],M[4][6],M[4][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[5][0],M[5][1],M[5][2],M[5][3],M[5][4],M[5][5],M[5][6],M[5][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[6][0],M[6][1],M[6][2],M[6][3],M[6][4],M[6][5],M[6][6],M[6][7]);
	printf("\r\n %d %d %d %d %d %d %d %d",M[7][0],M[7][1],M[7][2],M[7][3],M[7][4],M[7][5],M[7][6],M[7][7]);
	return;
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

	int x = 0;
	int y = 0;

	/*if (argc < 2) {
		printf("Usage: x position <0-7> y positon <0-7>\n\n");
		printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	}*/

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	//gpioint = atoi(argv[1]);
	//gpioled = atoi(argv[2]);

	/* export and configure led gpio */
	/*gpio_export(GPIOL1);
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
	gpiol4_fd = gpio_fd_open(GPIOL4, O_WRONLY);*/

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
            
		// x = x+1 when button 1 is pressed
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			//printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
			//	 GPIOB1, buf[0], len);
			buttoncnt++;
			//printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//display x+1
			if (6 < x){
				printf("\r\n hit the left boundary!");
			}
			else {
				x++;
				//printf("\r\nx = %d, y = %d",x,y);
				display(x,y);
			}
		}

		// x = x-1 when button 2 is pressed
		if (fdset[2].revents & POLLPRI) {
			lseek(fdset[2].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[2].fd, buf, MAX_BUF);
			//printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
			//	 GPIOB2, buf[0], len);
			buttoncnt++;
			//printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			if (x < 1){
				printf("\r\n hit the right boundary!");
			}
			else {
				x--;
				//printf("\r\n x = %d, y = %d",x,y);
				display(x,y);
			}
		}

		// y = y+1 when button 3 is pressed
		if (fdset[3].revents & POLLPRI) {
			lseek(fdset[3].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[3].fd, buf, MAX_BUF);
			//printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
			//	 GPIOB3, buf[0], len);
			buttoncnt++;
			//printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			if (y > 6){
				printf("\r\n hit the bottom!");
			}
			else {
				y++;
				//printf("\r\n x = %d, y = %d",x,y);
				display(x,y);
			}
		}

		// y=y-1 when button 4 is pressed
		if (fdset[4].revents & POLLPRI) {
			lseek(fdset[4].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[4].fd, buf, MAX_BUF);
			//printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
			//	 GPIOB4, buf[0], len);
			buttoncnt++;
			//printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			//turn on or turn off led
			if (y < 1){
				printf("\r\n hit the upper boundary!");
			}
			else {
				y--;
				//printf("\r\n x = %d, y = %d",x,y);
				display(x,y);
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

	/*gpio_fd_close(gpiol1_fd);
	gpio_fd_close(gpiol2_fd);
	gpio_fd_close(gpiol3_fd);
	gpio_fd_close(gpiol4_fd);
		system("clear");
		scanf("%d %d",&x,&y);
		printf("x = %d, y = %d\r",x,y);*/
	//}
	return 0;
}

