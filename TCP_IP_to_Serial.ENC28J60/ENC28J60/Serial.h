/*


Library for Serial Communication

Written By Shadman Sakib
2RA Technology Ltd.

This is a minimalistic library and not depended on any other library.
Just include this header file only. There is no .c file. So, dont worry. Tested with Atmel studio 7.
This library is built using the raw informations from the datasheet and documentations on Serial.
All the mechanisms, methodologies and references from datasheet are cited as much as possible.
So, one can use this file as a tutorial too ^_^
Have fun.


Need these:
-----------
#define F_CPU

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

*/


////////////////////////////////////////////////// Serial definitions	///////////////////////////////////////////////



# define USART_BAUDRATE 9600
# define BAUD_PRESCALE (((( F_CPU / 16) + ( USART_BAUDRATE / 2)) / ( USART_BAUDRATE )) - 1)



/////////////////////////////////////////////	Functions for  Serial comm	///////////////////////////////////////

void serial0_init(void);
void serial_init(void);
void USART_Flush( bool Serial_line );

//////////////////////////////////////////////	Serial related functions	//////////////////////////////////////////////////////////////////////////////


void serial0_init(void)
{
	UCSR0B = (1 << RXEN0 ) | (1 << TXEN0 );								// Turn on the transmission and reception circuitry
	UCSR0C = (1 << UCSZ00 ) | (1 << UCSZ01 );							// Use 8- bit character sizes. Parity NONE, Stop bit 1
	UBRR0H = ( BAUD_PRESCALE >> 8);										// Load upper 8- bits of the baud rate value into the high byte of the UBRR register
	UBRR0L = BAUD_PRESCALE ;											// Load lower 8- bits of the baud rate value into the low byte of the UBRR register
	UCSR0B |= (1 << RXCIE0 );											// Receive Interrupt enable
	//sei();																// Global interrupt routine
}

void serial_init(void)
{
	UCSR1B = (1 << RXEN1 ) | (1 << TXEN1 );								// Turn on the transmission and reception circuitry
	UCSR1C = (1 << UCSZ10 ) | (1 << UCSZ11 );							// Use 8- bit character sizes. Parity NONE, Stop bit 1
	UBRR1H = ( BAUD_PRESCALE >> 8);										// Load upper 8- bits of the baud rate value into the high byte of the UBRR register
	UBRR1L = BAUD_PRESCALE ;											// Load lower 8- bits of the baud rate value into the low byte of the UBRR register
	
	//sei();																// Global interrupt routine
}


void USART_Flush( bool Serial_line )
{
	unsigned char dummy;
	
	
	if (Serial_line==true)							// That means 1
	{
		while ( UCSR1A & (1<<RXC1) ) dummy = UDR1;
	}
	else
	{
		while ( UCSR0A & (1<<RXC0) ) dummy = UDR0;
	}
	
}
//////////////////////////////////////////////////// Interrupt Service Routine for Serial	///////////////////////////////////////////////////////////



