

#define F_CPU 16000000UL									/* Define CPU clock Frequency e.g. here its 8MHz */
#include <avr/io.h>										/* Include AVR std. library file */
#include <util/delay.h>									/* Include delay header file */
#include <inttypes.h>									/* Include integer type header file */
#include <stdlib.h>										/* Include standard library file */
#include <stdio.h>										/* Include standard library file */
#include "MPU6050_def.h"							/* Include MPU6050 register define file */
#include "i2c_master.h"							/* Include I2C Master header file */
#include "uart.h"							/* Include USART header file */
#include "lcd.h"

float Acc_x,Acc_y,Acc_z,Temperature,Gyro_x,Gyro_y,Gyro_z;

void MPU6050_Init()										/* Gyro initialization function */
{
	_delay_ms(150);										/* Power up time >100ms */
	I2C_Start_Wait(0xD0);								/* Start with device write address */
	I2C_Write(SMPLRT_DIV);								/* Write to sample rate register */
	I2C_Write(0x07);									/* 1KHz sample rate */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(PWR_MGMT_1);								/* Write to power management register */
	I2C_Write(0x01);									/* X axis gyroscope reference frequency */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(CONFIG);									/* Write to Configuration register */
	I2C_Write(0x00);									/* Fs = 8KHz */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(ACCEL_CONFIG);								/* Write to Gyro configuration register */
	I2C_Write(0x18);									/* Full scale range +/- 16g*/
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(GYRO_CONFIG);								/* Write to Gyro configuration register */
	I2C_Write(0x18);									/* Full scale range +/- 2000 degree/C */
	I2C_Stop();

	I2C_Start_Wait(0xD0);
	I2C_Write(INT_ENABLE);								/* Write to interrupt enable register */
	I2C_Write(0x01);
	I2C_Stop();
}

void MPU_Start_Loc()
{
	I2C_Start_Wait(0xD0);								/* I2C start with device write address */
	I2C_Write(0x3B);							/* Write start location address from where to read */
	I2C_Repeated_Start(0xD1);							/* I2C start with device read address */
}

void Read_RawValue()
{
	MPU_Start_Loc();									/* Read Gyro values */
	Acc_x = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Acc_y = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Acc_z = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Temperature = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Gyro_x = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Gyro_y = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Ack());
	Gyro_z = (((int)I2C_Read_Ack()<<8) | (int)I2C_Read_Nack());
	I2C_Stop();
}




int main()
{
	char buffer[20], float_[10];
	float t;

	I2C_Init();											/* Initialize I2C */
	MPU6050_Init();										/* Initialize MPU6050 */
	USART_Init(9600);									/* Initialize USART with 9600 baud rate */
	
	TCCR1A|=(1<<COM1A1)|(1<<COM1B1)|(1<<WGM11);        //NON Inverted PWM
	TCCR1B|=(1<<WGM13)|(1<<WGM12)|(1<<CS11)|(1<<CS10);
	
	ICR1=4999;  //fPWM=50Hz (Period = 20ms Standard).

	DDRB|=(1<<PORTB1);   //PWM Pins as Out
	
	 DDRC &= ~(1 << PORTC1); // Set BUTTON_PIN as input
	 PORTC |= (1 << PORTC1);  // Enable pull-up resistor
	 DDRC &= ~(1 << PORTC2); // Set BUTTON_PIN as input
	 PORTC |= (1 << PORTC2);  // Enable pull-up resistor
	 DDRC &= ~(1 << PORTC3); // Set BUTTON_PIN as input
	 PORTC |= (1 << PORTC3);  // Enable pull-up resistor
	
	int val = 0;
	
lcd_init(LCD_DISP_ON);
lcd_clrscr();
lcd_set_contrast(0x00);
lcd_gotoxy(4,1);
lcd_charMode(DOUBLESIZE);
lcd_gotoxy(0,4);



#ifdef GRAPHICMODE
lcd_display();
#endif
	
	while(1)
	{
		Read_RawValue();

		t = (Temperature/340.00)+36.53;					/* Convert temperature in °/c using formula */


		t = (t * 1.8) + 32;								//converts the celsius to farenheight
		dtostrf( t, 3, 2, float_ );
		sprintf(buffer,"%s, %i\r\n",float_, val);
		USART_SendString(buffer);
		
		//OCR1A
		if(t > 90){
			OCR1A = 490;						//guessed number for "very hot"
		} else if(t > 70 && t < 90){
			OCR1A = 350;						//guessed number for "hot"
		}else if(t > 50 && t < 70){
			OCR1A =  200;						//guessed number for "Nice"
		}else if(t < 50){
			OCR1A = 100;						//guessed number for "Cold!"
		}
		
		while(!(PINC & (1<<PINC1))){
			OCR1A = 100;
			t = 50;
			//_delay_ms(1000);
		}
		while(!(PINC & (1<<PINC2))){
			OCR1A = 200;
			t = 60;
			//_delay_ms(1000);
		}
		while(!(PINC & (1<<PINC3))){
			OCR1A = 350;
			t= 80;
		//	_delay_ms(1000);
		}
		

		
		
		val = OCR1A;
		lcd_clrscr();
		lcd_puts("  Temp  \r\n");
		 lcd_puts(float_);  // put string from RAM to display (TEXTMODE) or buffer (GRAPHICMODE)
		 _delay_ms(1000);
		 //lcd_gotoxy(0,2);
		 
		
	}
}