/*
 * linefollower.c
 *
 *  Created on: May 7, 2011
 *      Author: bgouveia
 */


//Pololu sensors code
/*
 * Written by Ben Schmidel et al., May 28, 2008.
 * Copyright (c) 2008 Pololu Corporation. For more information, see
 *
 *   http://www.pololu.com
 *   http://forum.pololu.com
 *   http://www.pololu.com/docs/0J18
 *
 * You may freely modify and share this code, as long as you keep this
 * notice intact (including the two links above).  Licensed under the
 * Creative Commons BY-SA 3.0 license:
 *
 *   http://creativecommons.org/licenses/by-sa/3.0/
 *
 * Disclaimer: To the extent permitted by law, Pololu provides this work
 * without any warranty.  It might be defective, in which case you agree
 * to be responsible for all resulting costs and damages.
 */


#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <stdlib.h>
#include <stdio.h>
#include "USARTatmega328.h"
#include "lcd-74hc595.h"

#define DEBUG


const uint8_t sensorPins[]={PC0,PC1,PC2,PC3,PC4,};
unsigned int sensorValues[5];
uint8_t sensorValuesMin[5]={255,255,255,255,255};
uint8_t sensorValuesMax[5]={0,0,0,0,0};

#define ALLSENSORS ((1<<sensorPins[0]) | (1<<sensorPins[1]) | (1<<sensorPins[2]) |(1<<sensorPins[3]) |(1<<sensorPins[4]))
#define SENSORSPORT PORTC
#define SENSORSREGISTER DDRC
#define SENSORSSTATE PINC

//motor pins
#define MOTORPORT PORTB
#define MOTORREGISTER DDRB
#define LEFTA 0
#define LEFTB 1
#define RIGHTA 2
#define RIGHTB 3

//pushbuttons
#define BUTTONPORT PORTD
#define BUTTONREGISTER DDRD
#define BUTTONSTATE PIND
#define BUTTON1 2
#define BUTTON2 3

//leds
#define LEDPORT PORTD
#define LEDREGISTER DDRD
#define LEDPIN 4

//lcd
#define LCDENABLEPORT PORTD
#define LCDDATAPORT PORTB

#define LCDENABLEREGISTER DDRD
#define LCDDATAREGISTER DDRB

#define LCDENABLE 7
#define LCDDATA 4
#define LCDCLOCK 5

const uint8_t symbols[] PROGMEM ={
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b00000,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111,
		0b11111
};

const char ready[] PROGMEM = "Ready...";
const char push[] PROGMEM = "Push B1";
const char start[] PROGMEM = "Start in";
const char gogo[] PROGMEM = "Go Go Go !!!";
const char sensorsgraph[] PROGMEM ="1 2 3 4 5  Pos";
const char mintext[] PROGMEM ="MIN";
const char maxtext[] PROGMEM ="MAX";

const char Message1[] PROGMEM = "3-Wire LCD";
const char Message2[] PROGMEM = "using 74HC595";


void IO_init(){
	//set motor pins to output
	MOTORREGISTER |=  (1<<LEFTA) | (1<<LEFTB) | (1<<RIGHTA) | (1<<RIGHTB);

	//set led pin to output
	LEDREGISTER |= (1<<LEDPIN);

	//set button pins to input
	BUTTONREGISTER &= ~((1<<BUTTON1) | (1<< BUTTON2));
	BUTTONPORT |= (1<<BUTTON1) | (1<<BUTTON2); //pullup

}


void PWM_init(){

	//set to output
	DDRD |= (1<<PD5) | (1<<PD6);

	//Clear OC0A/OC0B on Compare Match when up-counting. Set OC0A/OC0B on Compare Match when down-counting.
	TCCR0A |= (1<<COM0A1) | (1<<COM0B1);

	//mode 1 (pwm phase correct)
	TCCR0A |= (1<<WGM00);

	//prescaler to 64 
	TCCR0B |=  (1<<CS01) | (1<<CS00);


	OCR0A = 0; // set pwm duty
	OCR0B = 0; // set pwm duty

}

void Sensors_read(){
	sensorValues[0]=0;
	sensorValues[1]=0;
	sensorValues[2]=0;
	sensorValues[3]=0;
	sensorValues[4]=0;

	//turn on leds
	LEDPORT |= (1<<LEDPIN);

	SENSORSPORT |= ALLSENSORS;
	SENSORSREGISTER |= ALLSENSORS;
	_delay_us(15);

	SENSORSREGISTER &= ~ALLSENSORS;
	SENSORSPORT &= ~ALLSENSORS;

	while(1){
		uint8_t sensors= SENSORSSTATE;

		//if all sensors are low
		if (!(sensors & ALLSENSORS))
			break;

		for (int i=0; i< 5 ; i++){
			if(sensors & (1<<i))
				sensorValues[i]++;
		}
	}
	//turn off leds
	LEDPORT &= ~(1<<LEDPIN);
}

void Sensors_reset(){
	sensorValuesMin[0]=255;
	sensorValuesMin[1]=255;
	sensorValuesMin[2]=255;
	sensorValuesMin[3]=255;
	sensorValuesMin[4]=255;

	sensorValuesMax[0]=0;
	sensorValuesMax[1]=0;
	sensorValuesMax[2]=0;
	sensorValuesMax[3]=0;
	sensorValuesMax[4]=0;

}

void Sensors_calibrate(){
	for (int i =0 ; i < 10 ; i++){
		Sensors_read();

		for (int j=0; j < 5; j++){
			if(sensorValuesMax[j]<sensorValues[j])
				sensorValuesMax[j]=sensorValues[j];
			if(sensorValuesMin[j]>sensorValues[j])
				sensorValuesMin[j]=sensorValues[j];
		}

	}
}

// Returns values calibrated to a value between 0 and 1000, where
// 0 corresponds to the minimum value read by calibrate() and 1000
// corresponds to the maximum value.  Calibration values are
// stored separately for each sensor, so that differences in the
// sensors are accounted for automatically.
void Sensors_readCalibrated()
{
	int i;

	// read the needed values
	Sensors_read();

	for(i=0;i<5;i++)
	{
		unsigned int calmin,calmax;
		unsigned int denominator;

		calmax = sensorValuesMax[i];
		calmin = sensorValuesMin[i];

		denominator = calmax - calmin;

		signed int x = 0;
		if(denominator != 0)
			x = (((signed long)sensorValues[i]) - calmin)
			* 1000 / denominator;
		if(x < 0)
			x = 0;
		else if(x > 1000)
			x = 1000;
		sensorValues[i] = x;
	}

}


// Operates the same as read calibrated, but also returns an
// estimated position of the robot with respect to a line. The
// estimate is made using a weighted average of the sensor indices
// multiplied by 1000, so that a return value of 0 indicates that
// the line is directly below sensor 0, a return value of 1000
// indicates that the line is directly below sensor 1, 2000
// indicates that it's below sensor 2000, etc.  Intermediate
// values indicate that the line is between two sensors.  The
// formula is:
//
//    0*value0 + 1000*value1 + 2000*value2 + ...
//   --------------------------------------------
//         value0  +  value1  +  value2 + ...
//
// By default, this function assumes a dark line (high values)
// surrounded by white (low values).  If your line is light on
// black, set the optional second argument white_line to true.  In
// this case, each sensor value will be replaced by (1000-value)
// before the averaging.
int Sensors_readLine(char white_line)
{
	unsigned char i, on_line = 0;
	unsigned long avg; // this is for the weighted total, which is long
	// before division
	unsigned int sum; // this is for the denominator which is <= 64000
	static int last_value=0; // assume initially that the line is left.

	Sensors_readCalibrated();

	avg = 0;
	sum = 0;

	for(i=0;i<5;i++) {
		int value = sensorValues[i];
		if(white_line)
			value = 1000-value;

		// keep track of whether we see the line at all
		if(value > 200) {
			on_line = 1;
		}

		// only average in values that are above a noise threshold
		if(value > 50) {
			avg += (long)(value) * (i * 1000);
			sum += value;
		}
	}

	if(!on_line)
		return (last_value < (4)*1000/2)?0:4000;

	last_value = avg/sum;

	return last_value;
}


void Motors_set(int left,int right){
	if (left>=0){
		MOTORPORT |= (1<<LEFTA);
		MOTORPORT &= ~(1<<LEFTB);
	}else{
		MOTORPORT &= ~(1<<LEFTA);
		MOTORPORT |= (1<<LEFTB);
	}

	if (right>=0){
		MOTORPORT |= (1<<RIGHTA);
		MOTORPORT &= ~(1<<RIGHTB);
	}else{
		MOTORPORT &= ~(1<<RIGHTA);
		MOTORPORT |= (1<<RIGHTB);
	}

	left = abs(left);
	right = abs(right);
	OCR0A =  (left>255)?255:left;
	OCR0B =  (right>255)?255:right;

}

void initalCalibration(){
	for(int counter=0;counter<80;counter++)
	{
		if(counter < 20 || counter >= 50)
			Motors_set(60,-60);
		else
			Motors_set(-60,60);

		Sensors_calibrate();

		_delay_ms(20);
	}
	Motors_set(0,0);
}

void loadLCDSymbols(){
	for (int i=0;i<8;i++){
		LCD_defineSymbolp(i,symbols+i);
	}
}

void displayLCDMaxValues(){
	char temp[4];
	LCD_clear();

	LCD_moveCursor(1,1);
	LCD_writeTextp(maxtext);

	for (int i=0;i<5;i++){
		div_t res = div(5+i*4,14);
		LCD_moveCursor(1+res.quot,res.rem);
		itoa(sensorValuesMax[i],temp,10);
		LCD_writeText(temp);
	}
}

void displayLCDMinValues(){
	char temp[4];
	LCD_clear();

	LCD_moveCursor(1,1);
	LCD_writeTextp(mintext);

	for (int i=0;i<5;i++){
		div_t res = div(5+i*4,14);
		LCD_moveCursor(1+res.quot,res.rem);
		itoa(sensorValuesMin[i],temp,10);
		LCD_writeText(temp);
	}

}

void startLCD(){
	LCD_clear();
	LCD_moveCursor(1,4);
	LCD_writeTextp(ready);
	LCD_moveCursor(2,4);
	LCD_writeTextp(push);
}

void displayLCDcurrentValues(int pos){
	LCD_moveCursor(1,1);
	LCD_writeTextp(sensorsgraph);
	for (int i = 0; i < 5 ; i++){
		LCD_moveCursor(2,1+i*2);
		if(sensorValues[i] < 143)
			LCD_writeByte(1,1);
		else if (sensorValues[i] < 285)
			LCD_writeByte(2,1);
		else if (sensorValues[i] < 428)
			LCD_writeByte(3,1);
		else if (sensorValues[i] < 571)
			LCD_writeByte(4,1);
		else if (sensorValues[i] < 714)
			LCD_writeByte(5,1);
		else if (sensorValues[i] < 857)
			LCD_writeByte(6,1);
		else
			LCD_writeByte(7,1);
	}

	LCD_moveCursor(2,12);
	char temp[5];
	//TODO: replace sprintf
	sprintf(temp,"%04d",pos);
	LCD_writeText(temp);

}


int main (){

	IO_init();
	PWM_init();
	LCD_init();
	loadLCDSymbols();

#ifdef DEBUG
	USART_Init(25);
#endif

	Sensors_reset();


	startLCD();
	while(BUTTONSTATE & (1<<BUTTON1));
	_delay_ms(500);

	initalCalibration();

	displayLCDMaxValues();

	while(BUTTONSTATE & (1<<BUTTON1));
	_delay_ms(500);

	displayLCDMinValues();

	while(BUTTONSTATE & (1<<BUTTON1));
	_delay_ms(500);

	LCD_clear();

	while(BUTTONSTATE & (1<<BUTTON1)){
		int pos= Sensors_readLine(0);
		displayLCDcurrentValues(pos);
		_delay_ms(100);
	}

	LCD_clear();
	LCD_moveCursor(1,4);
	LCD_writeTextp(start);
	LCD_moveCursor(2,7);
	LCD_writeText(" s");
	LCD_moveCursor(2,6);
	LCD_writeText("3");
	_delay_ms(1000);
	LCD_moveCursor(2,6);
	LCD_writeText("2");
	_delay_ms(1000);
	LCD_moveCursor(2,6);
	LCD_writeText("1");
	_delay_ms(1000);
	LCD_clear();
	LCD_moveCursor(1,2);
	LCD_writeTextp(gogo);


	for (;;){

		unsigned int position = Sensors_readLine(0);

		if(position < 1000)
		{
			Motors_set(0,100);
		}
		else if(position < 3000)
		{
			Motors_set(100,100);
		}
		else
		{
			Motors_set(100,0);
		}

		/*for (int i=0;i<8;i++){
			LCD_moveCursor(1,4);
			LCD_writeByte(i,1);
			_delay_ms(100);
		}
		_delay_ms(1500);
		LCD_moveCursor(2,2);
		LCD_writeTextp(Message2);
		_delay_ms(1500);
		LCD_clear();*/

	}

	return 0;
}
