/*
 * lcd-74hc595.c
 *
 *  Created on: May 7, 2011
 *      Author: bgouveia
 */

#include <avr/io.h>
#include <util/delay.h>
#include "lcd-74hc595.h"

void LCD_init(){
	//set lcd pins to output
	LCDENABLEREGISTER |= (1<<LCDENABLE);
	LCDDATAREGISTER |= (1<<LCDDATA)|(1<<LCDCLOCK);

	_delay_ms(50);
	LCD_writeByte(0x20,0); // Wake-Up Sequence
	_delay_ms(50);
	LCD_writeByte(0x20,0);
	_delay_ms(50);
	LCD_writeByte(0x20,0);
	_delay_ms(50);
	LCD_writeByte(0x28,0); // 4-bits, 2 lines, 5x7 font
	_delay_ms(50);
	LCD_writeByte(0x0C,0); // Display ON, No cursors
	_delay_ms(50);
	LCD_writeByte(0x06,0); // Entry mode- Auto-increment, No Display shifting
	_delay_ms(50);
	LCD_writeByte(0x01,0);
	_delay_ms(50);
}


void LCD_writeByte(uint8_t byte,uint8_t is_Data){
	LCD_writeNibble(byte>>4,is_Data); //high nibble
	LCD_writeNibble(byte&0xf,is_Data); //low nibble
}

void LCD_writeNibble(uint8_t nibble,uint8_t RS){
	LCDENABLEPORT &= ~(1<<LCDENABLE); //enable = 0

	// ****** Write RS *********
	LCDDATAPORT &= ~(1<<LCDCLOCK); //clock = 0
	LCDDATAPORT ^= (-RS ^LCDDATAPORT) & (1<<LCDDATA);  //set or clear rs (http://graphics.stanford.edu/~seander/bithacks.html)
	LCDDATAPORT |= (1<<LCDCLOCK); //clock = 1
	LCDDATAPORT &= ~(1<<LCDCLOCK); //clock = 0
	// ****** End RS Write

	//shift 4 bits
	for (int i=0; i < 4 ; i++){
		uint8_t flag = nibble & (8>>i);
		LCDDATAPORT^=(-(flag!=0)^LCDDATAPORT) & (1<<LCDDATA); //set or clear data
		LCDDATAPORT |= (1<<LCDCLOCK); //clock = 1
		LCDDATAPORT &= ~(1<<LCDCLOCK); //clock = 0
	}

	// One more clock because SC and ST clks are tied
	LCDDATAPORT |= (1<<LCDCLOCK); 		//clock = 1
	LCDDATAPORT &= ~(1<<LCDCLOCK); 		//clock = 0
	LCDDATAPORT &= ~(1<<LCDDATA); 		//data = 0
	LCDENABLEPORT |= (1<<LCDENABLE); 	//enable = 1
	LCDENABLEPORT &= ~(1<<LCDENABLE); 	//enable = 0
}

void LCD_writeText(char * text){
	while (*text != '\0')
		LCD_writeByte(*text++,1);
}

void LCD_moveCursor(uint8_t x, uint8_t y){
	uint8_t temp = 127 + y;
	if (x == 2) temp += 64;
	LCD_writeByte(temp,0);
}
