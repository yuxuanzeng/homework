/*
    hw3-3.c  Created by Yuxuan Zeng for ECE479 homework03, RHIT, Sep 24, 2013
*/
/*
    to do some extra fun on my beaglebone
    1 display 0-9 on LED matrix
    2 display 2-digit number on LED matrix
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

// the number 0-9
static __u16 bmp_0[]=
	{0x0f, 0x09, 0x09, 0x09, 0x09, 0x09, 0x0f, 0x00};
static __u16 bmp_1[]=
	{0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00};
static __u16 bmp_2[]=
	{0x0f, 0x08, 0x08, 0x0f, 0x01, 0x01, 0x0f, 0x00};
static __u16 bmp_3[]=
	{0x0f, 0x08, 0x08, 0x0f, 0x08, 0x08, 0x0f, 0x00};
static __u16 bmp_4[]=
	{0x09, 0x09, 0x09, 0x0f, 0x08, 0x08, 0x08, 0x00};
static __u16 bmp_5[]=
	{0x0f, 0x01, 0x01, 0x0f, 0x08, 0x08, 0x0f, 0x00};
static __u16 bmp_6[]=
	{0x0f, 0x01, 0x01, 0x0f, 0x09, 0x09, 0x0f, 0x00};
static __u16 bmp_7[]=
	{0x0f, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00};
static __u16 bmp_8[]=
	{0x0f, 0x09, 0x09, 0x0f, 0x09, 0x09, 0x0f, 0x00};
static __u16 bmp_9[]=
	{0x0f, 0x09, 0x09, 0x0f, 0x08, 0x08, 0x0f, 0x00};

static __u16 *bmp_all[]=
	{bmp_0, bmp_1, bmp_2, bmp_3, bmp_4, bmp_5, bmp_6, bmp_7, bmp_8, bmp_9};

//read temperature
int read_temp(void);
int read_temp(void)
{
	char *end;
	int res, i2cbus, address, size, file;
	int daddress;
	char filename[20];

	i2cbus   = 1;
	address  = 0x48;
	daddress = 0;
	size = I2C_SMBUS_BYTE;

	sprintf(filename, "/dev/i2c-%d", i2cbus);
	file = open(filename, O_RDWR);
	if (file<0) {
		if (errno == ENOENT) {
			fprintf(stderr, "Error: Could not open file "
				"/dev/i2c-%d: %s\n", i2cbus, strerror(ENOENT));
		} else {
			fprintf(stderr, "Error: Could not open file "
				"`%s': %s\n", filename, strerror(errno));
			if (errno == EACCES)
				fprintf(stderr, "Run as root?\n");
		}
		exit(1);
	}

	if (ioctl(file, I2C_SLAVE, address) < 0) {
		fprintf(stderr,
			"Error: Could not set address to 0x%02x: %s\n",
			address, strerror(errno));
		return -errno;
	}

	res = i2c_smbus_read_byte_data(file, daddress);
	close(file);

	if (res < 0) {
		fprintf(stderr, "Error: Read failed, res=%d\n", res);
		exit(2);
	}

	//printf("0x%02x (%d)\n", res, res);

	return res;
}


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
	char filename[20];
	int force = 0;

	char buf[MAX_BUF];
	int len;
	int i = 0;
	int a = 36;
	int aq = 0;
	int ar = 0;

	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	
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

	//display 27
	/*for (i = 0; i < 8; i++) {
		zero_bmp[i] = bmp_2[i] | (bmp_7[i] << 4);
	}
	write_block(file, zero_bmp);*/

	//display 2-digit a
	while (keepgoing) {
		a = read_temp();
		aq = a/10;
		ar = a%10;
		for (i = 0; i< 8; i++) {
			zero_bmp[i] = bmp_all[aq][i] | (bmp_all[ar][i] << 4);
		}
		write_block(file, zero_bmp);
		sleep(1);
	}
	
	exit(0);
}
