/*
 * lcd-74hc595.h
 *
 *  Created on: May 7, 2011
 *      Author: bgouveia
 */

#ifndef LCD_74HC595_H_
#define LCD_74HC595_H_


//lcd
#define LCDENABLEPORT PORTD
#define LCDDATAPORT PORTB

#define LCDENABLEREGISTER DDRD
#define LCDDATAREGISTER DDRB

#define LCDENABLE 7
#define LCDDATA 4
#define LCDCLOCK 5

void LCD_init();
void LCD_writeByte(uint8_t byte,uint8_t is_Data);
void LCD_writeNibble(uint8_t nibble,uint8_t RS);
void LCD_writeText(char * text);
void LCD_writeTextp(char * text);
void LCD_moveCursor(uint8_t x, uint8_t y);
void LCD_defineSymbol(uint8_t location,const uint8_t * symbol);
void LCD_defineSymbolp(uint8_t location,const uint8_t * symbol);
void LCD_clear();




#endif /* LCD_74HC595_H_ */
