/* created by Yuxuan Zeng for homework05, ECE 497, RHIT, 2013-10-07 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <signal.h>
#include <poll.h>
#include "beaglebone_gpio.h"

/****************************************************************
 * Global variables
 ****************************************************************/
int keepgoing = 1;    // Set to 0 when ctrl-c is pressed

/****************************************************************
 * signal_handler
 ****************************************************************/
void signal_handler(int sig);
// Callback called when SIGINT is sent to the process (Ctrl-C)
void signal_handler(int sig)
{
	printf( "\nCtrl-C pressed, cleaning up and exiting...\n" );
	keepgoing = 0;
}

int main(int argc, char *argv[]) {
	volatile void *gpio_addr;
	volatile unsigned int *gpio_oe_addr;
	volatile unsigned int *gpio_setdataout_addr;
	volatile unsigned int *gpio_cleardataout_addr;
	volatile unsigned int *gpio_datain_addr;
	unsigned int reg = 0;
	unsigned int LED1 = 0;
	unsigned int LED2 = 0;
    
	// Set the signal callback for Ctrl-C
	signal(SIGINT, signal_handler);

	int fd = open("/dev/mem", O_RDWR);

	printf("Mapping %X - %X (size: %X)\n", GPIO1_START_ADDR, GPIO1_END_ADDR, GPIO1_SIZE);

	gpio_addr = mmap(0, GPIO1_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO1_START_ADDR);

	gpio_oe_addr           = gpio_addr + GPIO_OE;
	gpio_setdataout_addr   = gpio_addr + GPIO_SETDATAOUT;
	gpio_cleardataout_addr = gpio_addr + GPIO_CLEARDATAOUT;
	gpio_datain_addr   = gpio_addr + GPIO_DATAIN;
	

	if(gpio_addr == MAP_FAILED) {
		printf("Unable to map GPIO\n");
		exit(1);
	}
	printf("GPIO mapped to %p\n", gpio_addr);
	printf("GPIO OE mapped to %p\n", gpio_oe_addr);
	printf("GPIO SETDATAOUTADDR mapped to %p\n", gpio_setdataout_addr);
	printf("GPIO CLEARDATAOUT mapped to %p\n", gpio_cleardataout_addr);

	// Set led1 and led2 to be an output pin
	reg |= ~(GPIO_48|GPIO_60);  // Set led1 and led2 bit to 0
	reg = reg & (*gpio_oe_addr);
	*gpio_oe_addr = reg;
    
	printf("GPIO1 configuration: %X\r\n", reg);

		
	printf("Start blinking LED USR3\n");
	while(keepgoing) {
		printf("GPIO DATAIN is %X\r\n",*gpio_datain_addr);
		printf("GPIO_49 = %X\r\n", GPIO_49);
		printf("GPIO_51 = %X\r\n", GPIO_51);
		printf("(*gpio_datain_addr)&GPIO_49 = %X\r\n", (*gpio_datain_addr)&GPIO_49);
		printf("(*gpio_datain_addr)&GPIO_51 = %X\r\n", (*gpio_datain_addr)&GPIO_51);
		if (0 != ((*gpio_datain_addr)&GPIO_49))
		{
			LED1 = ~LED1;
			printf("Switch 1 has been pressed! LED1 = %X\r\n", LED1);
		}
		if (0 != ((*gpio_datain_addr)&GPIO_51))
		{
			LED2 = ~LED2;
			printf("Switch 2 has been pressed! LED2 = %X\r\n", LED2);
		}
		printf("LED1&LED2 = %X\r\n", LED1&LED2);
		if (0 != (LED1&LED2))
		{
        		*gpio_setdataout_addr = ((GPIO_48|GPIO_60));
		}
		else if (0 != LED1)
		{
			*gpio_setdataout_addr = GPIO_60;
			*gpio_cleardataout_addr = GPIO_48;
		}
		else if (0 != LED2)
		{
			*gpio_setdataout_addr = GPIO_48;
			*gpio_cleardataout_addr = GPIO_60;
		}
		else
		{
			*gpio_cleardataout_addr = (GPIO_48|GPIO_60);
		}
        
        	usleep(200000);
    	}

    	munmap((void *)gpio_addr, GPIO1_SIZE);
	close(fd);
	return 0;
}
