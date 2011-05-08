/*
 * lcd-74hc595.c
 *
 *  Created on: May 7, 2011
 *      Author: bgouveia
 */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "lcd-74hc595.h"

void LCD_init(){
	LCDENABLEREGISTER |= (1<<LCDENABLE);
	LCDDATAREGISTER |= ((1<<LCDDATA)|(1<<LCDCLOCK));

	LCDENABLEPORT &= ~(1<<LCDENABLE);
	LCDDATAPORT &= ~((1<<LCDDATA)|(1<<LCDCLOCK));

	_delay_ms(50);

	LCD_writeNibble(0x3,0);
	_delay_us(4500);

	LCD_writeNibble(0x3,0);
	_delay_us(4500);

	LCD_writeNibble(0x3,0);
	_delay_us(150);

	LCD_writeNibble(0x2,0);


	LCD_writeByte(0x28,0);
	_delay_us(4500);

	LCD_writeByte(0xC,0);

	//clear
	LCD_writeByte(0x01,0);
	_delay_us(2000);

	LCD_writeByte(0x06,0);
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
	_delay_us(1);
	LCDENABLEPORT &= ~(1<<LCDENABLE); 	//enable = 0
	_delay_us(40);
}

void LCD_writeText(char * text){
	while (*text != '\0')
		LCD_writeByte(*text++,1);
}

//program space
void LCD_writeTextp(char * text){
	char c;
	while ((c=pgm_read_byte(text++))!= '\0'){
		LCD_writeByte(c,1);
	}
}

void LCD_moveCursor(uint8_t x, uint8_t y){
	uint8_t temp = 127 + y;
	if (x == 2) temp += 64;
	LCD_writeByte(temp,0);
}

void LCD_defineSymbol(uint8_t location,const uint8_t * symbol)
{
	location &= 0x7;
	LCD_writeByte(0x40 | (location<<3),0);
	for (int i=0; i<8 ; i++){
		LCD_writeByte(symbol[i],1);
	}
}

void LCD_defineSymbolp(uint8_t location,const uint8_t * symbol)
{
	location &= 0x7;
	LCD_writeByte(0x40 | (location<<3),0);
	for (int i=0; i<8 ; i++){
		LCD_writeByte(pgm_read_byte(symbol+i),1);
	}
}
