/*
*   Created by Yuxuan Zeng, Sep.22 2013, for ECE497, RHIT
*   1. Set TM=1,and read gpio interruption.
*   2. Print the temperature in F.
*   3. Input i2cbus, i2caddress, low temperature and high temperature
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "i2c-dev.h"
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include "gpio-utils.c"

/****************************************************************
 * Constants
 ****************************************************************/
#define POLL_TIMEOUT (100) /* 0.1 seconds */
#define GPIO0_7      7     /* GPIO0_7 */

/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;	// Set to 0 when ctrl-c is pressed


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
 * ReadTemp,read temperature in C on register0 of TMP101
 ****************************************************************/
int ReadTemp(int i2cbus, int address);
int ReadTemp(int i2cbus, int address)
{
	char *end;
	int res,size, file;
	int daddress = 0;
	char filename[20];

	size = I2C_SMBUS_BYTE;

	sprintf(filename, "/dev/i2c-%d", i2cbus);
	file = open(filename, O_RDWR);
	if (file < 0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
		return -1;
	}

	if (ioctl(file, I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		return -1;
	}

	res = i2c_smbus_read_byte_data(file, daddress);
	close(file);

	if (res < 0) {
		fprintf(stderr, "Error: Read failed, res=%d\n", res);
		return -1;
	}

	return res;
}


/***************************************************************
* Main
***************************************************************/
int main(int argc, char **argv)
{
	int a = 0x02;
	int b = 0x01;
	struct pollfd fdset[2];
	int nfds = 2;
	int timeout, rc;
	int gpio_fd;
	char buf[MAX_BUF];
	int len;
	int inter_count = 0;
	int res = 0; //the temperature on register0 of TMP101
	//int i2cbus;
	//char i2caddress = 0;
	//int Tlow,Thigh;

	/*if (argc < 5) {
		printf("Usage: i2cbus <0 1> i2caddress <hex> Tlow <Celsius> Thigh <Celsius>\n\n");
		//printf("Waits for a change in the GPIO pin voltage level or input on stdin\n");
		exit(-1);
	}*/

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	/*i2cbus = atoi(argv[1]);
	printf("\n i2cbus = %d\n",i2cbus);
	//i2caddress = atoi(argv[2]);
	//strlcpy(&i2caddress, &argv[2],sizeof(char));
	printf("\n i2caddress = %d\n",i2caddress);
	Tlow = atoi(argv[3]);
	printf("\n Tlow = %d\n",Tlow);
	Thigh = atoi(argv[4]);
	printf("\n Thigh = %d\n",Thigh);*/

	system("clear");
	//set TM=1
	system("i2cset -y 1 0x4a 1 0x02");
	system("i2cset -y 1 0x4a 2 0x19");
	//set low temperature
	//system("i2cset -y $i2cbus $i2caddress 2 $Tlow");
	//set high temperature
	system("i2cset -y 1 0x4a 3 0x1a");

	//configure PIN42, which is gpio0_7. Let PIN41 fall into interrupt when ALERT becomes active
	gpio_export(GPIO0_7);
	gpio_set_dir(GPIO0_7, "in");
	gpio_set_edge(GPIO0_7, "falling");
	gpio_fd = gpio_fd_open(GPIO0_7, O_RDONLY);

	timeout = POLL_TIMEOUT;

	while (keepgoing) {
		memset((void*)fdset, 0, sizeof(fdset));

		fdset[0].fd = STDIN_FILENO;
		fdset[0].events = POLLIN;

		fdset[1].fd = gpio_fd;
		fdset[1].events = POLLPRI;
	
		//system("i2cget -y 1 0x4a 0");
		rc = poll(fdset, nfds, timeout); 

		//printf("rc = %d",rc);
		if (rc < 0) {
			printf("\npoll() failed!\n");
			return -1;
		}
      
		else if (rc == 0) {
			printf(".");
		}
		
			
            
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("\ninterrupt occurred %d times!\n",inter_count);
			inter_count ++;
			res = ReadTemp(1, 0x4a);
			printf("\n res = %d\n",res);
			//system("i2cget -y 1 0x4a 0");
			if (res < 0){
				printf("\nError to read temperature, res = %d\n",res);
			}
			else {
				printf("\nThe temperature is %d F\n",(32+res*18/10));
			}
			
		}
		fflush(stdout);
	}

	gpio_fd_close(gpio_fd);
	return;
}
