/*
    hw3-2.c  Created by Yuxuan Zeng for ECE479 homework03, RHIT, Sep 23, 2013
*/
/*
    i2cset.c - A user-space program to write an I2C register.
    Copyright (C) 2001-2003  Frodo Looijaard <frodol@dds.nl>, and
                             Mark D. Studebaker <mdsxyz123@yahoo.com>
    Copyright (C) 2004-2010  Jean Delvare <khali@linux-fr.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA 02110-1301 USA.
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "i2c-dev.h"
#include "i2cbusses.c"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include "gpio-utils.c"

/****************************************************************
* button  PIN     16     23     41      42
*         GPIO    51     49     20      7
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

#define BICOLOR		// undef if using a single color display

/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;	// Set to 0 when ctrl-c is pressed
int buttoncnt = 0;      // Added when button pressed

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


// The upper btye is RED, the lower is GREEN.
// The single color display responds only to the lower byte
static __u16 smile_bmp[]=
	{0x3c, 0x42, 0xa9, 0x85, 0x85, 0xa9, 0x42, 0x3c};
static __u16 frown_bmp[]=
	{0x3c00, 0x4200, 0xa900, 0x8500, 0x8500, 0xa900, 0x4200, 0x3c00};
static __u16 neutral_bmp[]=
	{0x3c3c, 0x4242, 0xa9a9, 0x8989, 0x8989, 0xa9a9, 0x4242, 0x3c3c};

static void help(void) __attribute__ ((noreturn));

static void help(void) {
	fprintf(stderr, "Usage: matrixLEDi2c (hardwired to bus 3, address 0x70)\n");
	exit(1);
}

static int check_funcs(int file) {
	unsigned long funcs;

	/* check adapter functionality */
	if (ioctl(file, I2C_FUNCS, &funcs) < 0) {
		fprintf(stderr, "Error: Could not get the adapter "
			"functionality matrix: %s\n", strerror(errno));
		return -1;
	}

	if (!(funcs & I2C_FUNC_SMBUS_WRITE_BYTE)) {
		fprintf(stderr, MISSING_FUNC_FMT, "SMBus send byte");
		return -1;
	}
	return 0;
}

// Writes block of data to the display
static int write_block(int file, __u16 *data) {
	int res;
#ifdef BICOLOR
	res = i2c_smbus_write_i2c_block_data(file, 0x00, 16, 
		(__u8 *)data);
	return res;
#else
/*
 * For some reason the single color display is rotated one column, 
 * so pre-unrotate the data.
 */
	int i;
	__u16 block[I2C_SMBUS_BLOCK_MAX];
//	printf("rotating\n");
	for(i=0; i<8; i++) {
		block[i] = (data[i]&0xfe) >> 1 | 
			   (data[i]&0x01) << 7;
	}
	res = i2c_smbus_write_i2c_block_data(file, 0x00, 16, 
		(__u8 *)block);
	return res;
#endif
}

int main(int argc, char *argv[])
{
	int res, i2cbus, address, file;
	__u16 zero_bmp[8] = {0};
	__u16 zero_zero[8] = {0};
	char filename[20];
	int force = 0;

	struct pollfd fdset[5];
	int nfds = 5;
	int timeout, rc;
	int gpiob1_fd,gpiob2_fd,gpiob3_fd,gpiob4_fd;
	char buf[MAX_BUF];
	int len;

	//
	int x,y;
	int loop = 0;
	//

	//
	//x = atoi(argv[1]);
	//y = atoi(argv[2]);
	x = 0;
	y = 0;
	//

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	/* export and configure button gpio */
	gpio_export(GPIOB1);
	gpio_export(GPIOB2);
	gpio_export(GPIOB3);
	gpio_export(GPIOB4);
	gpio_set_dir(GPIOB1, "in");
	gpio_set_dir(GPIOB2, "in");
	gpio_set_dir(GPIOB3, "in");
	gpio_set_dir(GPIOB4, "in");
	gpio_set_edge(GPIOB1, "falling");  // Can be rising, falling or both
	gpio_set_edge(GPIOB2, "rising"); 
	gpio_set_edge(GPIOB3, "rising"); 
	gpio_set_edge(GPIOB4, "rising"); 
	gpiob1_fd = gpio_fd_open(GPIOB1, O_RDONLY);
	gpiob2_fd = gpio_fd_open(GPIOB2, O_RDONLY);
	gpiob3_fd = gpio_fd_open(GPIOB3, O_RDONLY);
	gpiob4_fd = gpio_fd_open(GPIOB4, O_RDONLY);

	i2cbus = lookup_i2c_bus("1");
	printf("i2cbus = %d\n", i2cbus);
	if (i2cbus < 0)
		help();

	address = parse_i2c_address("0x70");
	printf("address = 0x%2x\n", address);
	if (address < 0)
		help();

	file = open_i2c_dev(i2cbus, filename, sizeof(filename), 0);
//	printf("file = %d\n", file);
	if (file < 0
	 || check_funcs(file)
	 || set_slave_addr(file, address, force))
		exit(1);

	// Check the return value on these if there is trouble
	i2c_smbus_write_byte(file, 0x21); // Start oscillator (p10)
	i2c_smbus_write_byte(file, 0x81); // Disp on, blink off (p11)
	i2c_smbus_write_byte(file, 0xe7); // Full brightness (page 15)

//	Display a series of pictures
	write_block(file, frown_bmp);
	sleep(1);
	write_block(file, neutral_bmp);
	sleep(1);
	write_block(file, smile_bmp);
	sleep(1);
	zero_bmp[x] = zero_bmp[x] | ((0x00 | 0x01) << y);
	write_block(file, zero_bmp);
	sleep(1);


	// register poll 
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
            	
		loop++;
		// press the first button, move left, x++
		if (fdset[1].revents & POLLPRI) {
			lseek(fdset[1].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[1].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB1, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
			
			if (1 < loop){
				if (x == 7){
					// reach the boundary, do nothing
				}
				else {
					x++;
					zero_bmp[x] = zero_bmp[x] | ((0x00 | 0x01) << y);
					write_block(file, zero_bmp);
				}
			}
		}

		// press the second button, move right, x--
		if (fdset[2].revents & POLLPRI) {
			lseek(fdset[2].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[2].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB2, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			if (1 < loop){
				if (x == 0){
					// reach the boundary, do nothing
				}
				else {
					x--;
					zero_bmp[x] = zero_bmp[x] | ((0x00 | 0x01) << y);
					write_block(file, zero_bmp);
				}
			}
		}

		// press the third button, move down, y++
		if (fdset[3].revents & POLLPRI) {
			lseek(fdset[3].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[3].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB3, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);                        
			
			if (1 < loop){
				if (y == 7){
					// reach the boundary, do nothing
				}
				else {
					y++;
					zero_bmp[x] = zero_bmp[x] | ((0x00 | 0x01) << y);
					write_block(file, zero_bmp);
				}
			}
		}

		// press the fourth button, move up, y--
		if (fdset[4].revents & POLLPRI) {
			lseek(fdset[4].fd, 0, SEEK_SET);  // Read from the start of the file
			len = read(fdset[4].fd, buf, MAX_BUF);
			printf("\npoll() GPIO %d interrupt occurred, value=%c, len=%d\n",
				 GPIOB4, buf[0], len);
			buttoncnt++;
			printf("\nbutton has been pressed %d times.",buttoncnt);
                        
			if (1 < loop){
				if (y == 0){
					// reach the boundary, do nothing
				}
				else {
					y--;
					zero_bmp[x] = zero_bmp[x] | ((0x00 | 0x01) << y);
					write_block(file, zero_bmp);
				}
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
	
// Fade the display
	/*int daddress;
	for(daddress = 0xef; daddress >= 0xe0; daddress--) {
//	    printf("writing: 0x%02x\n", daddress);
	    res = i2c_smbus_write_byte(file, daddress);
	    usleep(100000);	// Sleep 0.1 seconds
	}

	if (res < 0) {
		fprintf(stderr, "Error: Write failed\n");
		close(file);
		exit(1);
	}*/
	exit(0);
}
