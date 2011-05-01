/*
 * USARTatmega328.c
 *
 *  Created on: Feb 5, 2011
 *      Author: bgouveia
 */
#include <avr/io.h>
#include "USARTatmega328.h"

void USART_Init(uint16_t ubrr)
{
/*Set baud rate */
UBRR0H = (unsigned char)(ubrr>>8);
UBRR0L = (unsigned char)ubrr;

/*
>> Asynchronous mode
>> No Parity
>> 1 StopBit
*/

/* Set frame format: 8data, 1stop bit */
UCSR0C = (1<<UCSZ01)| (1<<UCSZ00);

/*Enable receiver and transmitter */
UCSR0B = (1<<RXEN0)|(1<<TXEN0);
}

//This function is used to read the available data
//from USART. This function will wait untill data is
//available.
char USARTReadChar()
{
   //Wait untill a data is available

   while(!(UCSR0A & (1<<RXC0)))
   {
      //Do nothing
   }

   //Now USART has got data from host
   //and is available is buffer

   return UDR0;
}

//This fuction writes the given "data" to
//the USART which then transmit it via TX line
void USARTWriteChar(char data)
{
   //Wait untill the transmitter is ready

   while(!(UCSR0A & (1<<UDRE0)))
   {
      //Do nothing
   }

   //Now write the data to USART buffer

   UDR0=data;
}


void print_string(const unsigned char *data) {
	while (*data != '\0') {
		USARTWriteChar(*data++);
	}
}
