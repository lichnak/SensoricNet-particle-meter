/*
 * This file is part of the SenoricNet project, https://sensoricnet.cz
 *
 * Copyright (C) 2017 Pavel Polach <ivanahepjuk@gmail.com>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include "functions.h"
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/spi.h>

//#define SPI_CR1_DFF_8BIT  	(0 << 11)


float calculate_float(uint8_t val0, uint8_t val1, uint8_t val2, uint8_t val3)
{
	// Return an IEEE754 float from an array of 4 bytes
	union u_tag {
		uint8_t b[4];
		float val;
	} u;

	u.b[0] = val0;
	u.b[1] = val1;
	u.b[2] = val2;
	u.b[3] = val3;

	return u.val;
}

void flash(uint8_t loop)
{
	for (uint8_t i = 0; i < loop; i++)
	{
		gpio_set(GPIOA, GPIO11); 
		wait(100000); 
		gpio_clear(GPIOA, GPIO11); 
		wait(100000);
	}
}



/* 		void spi_setup(void)
 * 
 * This functions does setting-up of SPI peripheral. 
 * (1) Configuring GPIO's
 * (2) Configuring SPI block
 * (3) Enabling spi
 * 
 */
void spi_setup(void)
{
	// gpio setting for SS
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO8);
	
	// gpio setting for SDI SDO SCK
	gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,  GPIO3 | GPIO4 | GPIO5);

	// gpio alternative function SPI 1
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO3  | GPIO4 | GPIO5);

	//Reset SPI
	spi_reset(SPI1);

	/* Set up SPI in Master mode with:
	* Clock baud rate: 1/32? of peripheral clock frequency
	* Clock polarity: Idle High
	* Clock phase: Data valid on 2nd clock pulse
	* Data frame format: 8-bit
	* Frame format: MSB First
	* Setting NSS high (Even though I am using gpio as SS)
	*/
	
	//bitstream for register settinggs according to datasheet
	SPI_CR1(SPI1) |= 0b0000001100100101;  
	//bitstream for register settinggs according to datasheet
	SPI_CR2(SPI1) |= 0b0000011100000000; 
	 
	//this is super important for reliable reading 8-bit packets from bus   
	spi_fifo_reception_threshold_8bit(SPI1);

	//Enable SPI1 peripherals
	spi_enable(SPI1);
	
}
  



char hexDigit(unsigned n)
{
	if (n < 10) {
        return n + '0';
    } else {
        return (n - 10) + 'A';
    }
}

void charToHex(char c, char hex[3])
{
    hex[0] = hexDigit(c / 0x10);
    hex[1] = hexDigit(c % 0x10);
    hex[2] = '\0';
}

void wait(uint32_t usec)
{
	for (uint32_t i=0; i<usec; i++)
	{
		__asm__("NOP");
	}		
}

/* 		void usartSend(char *phrase, uint8_t usart)
 * 
 * This functions sends string to one of four usarts.
 * Note: chosen usart must be allready configured in usart_setup! 
 */
void usartSend(char *phrase, uint8_t usart)
{

	uint32_t i=0;                    

	while(phrase[i] != '\0')
	{
		if(usart==1)
		{
			usart_send_blocking(USART1, phrase[i]);
		}
		if(usart==2)
		{
			usart_send_blocking(USART2, phrase[i]);
		}
		if(usart==3)
		{
			usart_send_blocking(USART3, phrase[i]);
		}
		if(usart==4)
		{
			usart_send_blocking(USART4, phrase[i]);
		}
		i++;
	}
}


void clock_setup(void)
{
    //clk for gsm,leds,
    rcc_periph_clock_enable(RCC_GPIOA);

	//clk for spi and FIXME i2c
    rcc_periph_clock_enable(RCC_GPIOB);
	
	// Enable GPIOC clock for LED
	rcc_periph_clock_enable(RCC_GPIOC);
	
	//clk for SPI1
	rcc_periph_clock_enable(RCC_SPI1);
	
	// clk for USART4 (quectel)  PC10 tx PC11 rx
	rcc_periph_clock_enable(RCC_USART4);

    // clk for USART2 (gsm) PA2 tx PA3 rx
	rcc_periph_clock_enable(RCC_USART2);
}


void i2c_setup(void)
{
	//clocks
	rcc_periph_clock_enable(RCC_I2C2);
	rcc_periph_clock_enable(RCC_GPIOB);
	
	i2c_reset(I2C2);
	
	// gpio setting for data and clock
	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO13 | GPIO14);
	// gpio setting to alternative function I2C2
	gpio_set_af(GPIOB, GPIO_AF5, GPIO13 | GPIO14);
	//disable i2c2 before configuration
	i2c_peripheral_disable(I2C2);
	//configure ANFOFF DNF[3:0] in CR1
	i2c_enable_analog_filter(I2C2);
	//digital filter
	//i2c_set_digital_filter(I2C1, I2C_CR1_DNF_DISABLED);
	//speed
	i2c_set_speed(I2C2, i2c_speed_sm_100k, 8);
	//configure No-Stretch CR1 (only relevant in slave mode)
	i2c_enable_stretching(I2C2);
	//addressing mode
	i2c_set_7bit_addr_mode(I2C2);
	//After setting it up, it must be enabled.
	i2c_peripheral_enable(I2C2);
}


void usart_setup(void)
{
	// setup gsm USART2 parameters
	usart_set_baudrate(USART2, 9600);
	//lora
	//usart_set_baudrate(USART2, 57600);
	usart_set_databits(USART2, 8);
	usart_set_parity(USART2, USART_PARITY_NONE);
	usart_set_stopbits(USART2, USART_STOPBITS_1);
	usart_set_mode(USART2, USART_MODE_TX_RX);
	usart_set_flow_control(USART2, USART_FLOWCONTROL_NONE);
	// enable the USART2
	usart_enable(USART2);

        // setup quectel USART4 parameters
	usart_set_baudrate(USART4, 57600);
	usart_set_databits(USART4, 8);
	usart_set_parity(USART4, USART_PARITY_NONE);
	usart_set_stopbits(USART4, USART_STOPBITS_1);
	usart_set_mode(USART4, USART_MODE_TX_RX);
	usart_set_flow_control(USART4, USART_FLOWCONTROL_NONE);
	// enable the USART4
	usart_enable(USART4);
}	


void gpio_setup(void)
{

	//gpio LEDs setup
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO10);
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11);
	
	//wireless reset
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, GPIO9);

	// USART2 GPIO pins 
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO2);//tx
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO3);//rx

	// USART2 setup pins as alternate function AF0
	gpio_set_af(GPIOA, GPIO_AF1, GPIO2);
	gpio_set_af(GPIOA, GPIO_AF1, GPIO3);
    
    // USART4 GPIO pins 
    gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO10);//tx
	gpio_mode_setup(GPIOC, GPIO_MODE_AF, GPIO_PUPD_PULLDOWN, GPIO11);//rx

	// USART4 setup pins as alternate function AF0
	gpio_set_af(GPIOC, GPIO_AF0, GPIO10);
	gpio_set_af(GPIOC, GPIO_AF0, GPIO11);
}








