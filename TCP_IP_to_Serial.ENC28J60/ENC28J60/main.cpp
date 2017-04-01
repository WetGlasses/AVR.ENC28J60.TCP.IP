
/*

Shadman Sakib
2RA Technology Limited

Driver for ENC28J60 Ethernet Controller IC

23 June, 2016

*/


#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "Serial.h"

#include "checksum.h"
#include "ENC_Ethernet.h"
#include "TCP.h"
#include "IP.h"
#include "ARP.h"
#include "UDP.h"

#define heartbit_LED_DATA					DDRA	
#define heartbit_LED_PORT					PORTA
#define heartbit_LED						PA7

#define Connection_LED_DATA					DDRD
#define Connection_LED_PORT					PORTD
#define Connection_LED						PD7

#define Settings_LED_DATA					DDRC
#define Settings_LED_PORT					PORTC
#define Settings_LED						PC7

#define Settings_switch_DATA					DDRE			// Sends Data at its Falling edge (Interrupt)
#define Settings_switch_PORT					PORTE
#define Settings_switch_PIN						PINE
#define Settings_switch							PE6



////////////////////////////////	Command For The Module	//////////////////////////////////////////

#define MAC_Change		0x10
#define IP_Change		0x20
#define PORT_Change		0x30
#define Module_Restart	0x40

char ENC_Data[500];
char Serial_Data[500];
uint16_t Packet_length, Core_Packet =0x00;
uint16_t Outgoing_byte_count = 0x00;
bool Auto_Send = true;
bool pre_connection = false;
void change_settings(bool Method);		// Method = True for Serial, False for UDP

int main()
{
	heartbit_LED_DATA |= (1<<heartbit_LED);
	Connection_LED_DATA |= (1<<Connection_LED);
	Settings_LED_DATA |= (1<< Settings_LED);	
	Settings_switch_DATA &=~(1<< Settings_switch);
	
	heartbit_LED_PORT |= (1<<heartbit_LED);
	Connection_LED_PORT |= (1<<Connection_LED);
	Settings_LED_PORT |= (1<< Settings_LED);
	Settings_switch_PORT &= ~ (1<< Settings_switch);					// Makes it Low. So that rising can be detected
	
	// Heart Beat LED
	TCCR1B |= 1<<CS10 | 1<<CS12 | 1<<WGM12; // Heart beat LED
	OCR1A= 14000;  // 10 millisecond interval.
	TIMSK |= 1<< OCIE1B;    //	Channel 1, Heart beat LED Enabled.
	
	if ((eeprom_read_byte(0x00)==0xFF)|(eeprom_read_byte(0x00)==0x00))		// First time. Write default
	{
		for ( char x=0;x<6;x++)				// MAC Write
		{
			eeprom_write_byte(( uint8_t *)x,MAC[x]);
		}
		
		for (char x=0;x<4;x++)				// IP Write
		{
			eeprom_write_byte(( uint8_t *)(6+x),My_IP[x]);
		}
	} 
	else									// Load from EEPROM
	{
		for ( uint8_t x=0;x<6;x++)				// MAC Read
		{
			MAC[x] = eeprom_read_byte(( uint8_t *)x);
		}
		for (uint8_t x=0;x<4;x++)				// IP Read
		{
			My_IP[x] = eeprom_read_byte(( uint8_t *)6+ x);
		}
	}
	ENC_MasterInit();
	ENC_init();
	serial_init();
	
	// Interrupt on IN6 for EMU input
	EICRB |= 1<<ISC61 | 1<<ISC60;		// Rising edge
	EIMSK |= 1<<INT6;				// Enable INT6
	sei();
	while (1)
	{
		while (NewPacket()>0x00)
			{
			// Read the frame
			Packet_length = ENC_Receive(ENC_Data);	
			Core_Packet = Packet_length;
			// Check if ARP			
			char IsARP=ARP_check(ENC_Data);  // Auto reply ARP		
			if (IsARP=='O')		// Other. But for me.
				{
				// Check if TCP
				Packet_length=TCP_check(ENC_Data,Packet_length);	// Handles sending ACK and SYN+ACK 								
				if (Packet_length>0)
				{
					//Make_TCP_Packet(ENC_Data,Packet_length,false,true,true);
					for (uint16_t x=0;x<Packet_length;x++)
					{
						UDR1= ENC_Data[x];
						while( !( UCSR1A & (1<<UDRE1)) );
						
					}
					Packet_length=0;
				}
				else
				{
					
				Packet_length = UDP_check(ENC_Data,Core_Packet);
				
				if (Packet_length>8)
				{
					for (char x=0;x<20;x++)
					{
						Serial_Data[x]= ENC_Data[x];
					}
					change_settings(false);
				}
				Packet_length=0;
										
				}
				
			}
			
			if (pre_connection != connected)
			{
				if (connected==true)
				{
					OCR1A= 2800;
					Outgoing_byte_count=0;
					USART_Flush(true);
					UCSR1B |= (1 << RXCIE1 );			// Turn on Interrupt
					Connection_LED_PORT &=~(1<< Connection_LED);
				}
				else
				{
					OCR1A= 14000;
					Outgoing_byte_count=0;
					USART_Flush(true);
					UCSR1B &= ~(1 << RXCIE1 );			// Turn on Interrupt
					Connection_LED_PORT |= (1<< Connection_LED);
				}
			}
			pre_connection= connected;
			
		}
	}

}

ISR(TIMER1_COMPB_vect)
{
	heartbit_LED_PORT ^=(1<<heartbit_LED);
	if ((Outgoing_byte_count>0)& (connected==true))
	{
		Make_TCP_Packet(Serial_Data,Outgoing_byte_count,false,true,true);
		Outgoing_byte_count=0;
	}
}

ISR(INT6_vect)
{
	cli();				// Turn off Interrupt
	
	Settings_LED_PORT &= ~(1<<Settings_LED);	// Indicator of settings change
	
	Outgoing_byte_count=0;
	while (Settings_switch_PIN & (1<< Settings_switch))
	{
		while( !( UCSR1A & (1<<RXC1)) );		// Wait until comes
		Serial_Data[Outgoing_byte_count]= UDR1;	// Shove it into the Buffer
		Outgoing_byte_count++;
	}
	change_settings(true);
	Outgoing_byte_count=0;
	
	Settings_LED_PORT |= (1<<Settings_LED);		// Indicator of change finish.
	
	sei();				// Turn on Interrupt
}


void change_settings(bool Method)
{
	if (Serial_Data[1]== MAC_Change)
	{
		Settings_LED_PORT &= ~(1<<Settings_LED);
		
		// send "OK"  Ack sending 1st, because after changing, It won't communicate.
		if (Method==true)			// Serial
		{
			UDR1= MAC_Change;
			while( !( UCSR1A & (1<<UDRE1)) );
			_delay_ms(20);
		}
		else
		{
			UDP_send("Changed MAC",11,10);
		}
		
		for (char x=0;x<6;x++)
		{
			MAC[x]= ENC_Data[2+x];
		}
		
		for (uint8_t x=0;x<6;x++)				// MAC Write
		{
			eeprom_write_byte(( uint8_t *)x,MAC[x]);
		}
		
		ENC_init();
		
		Settings_LED_PORT |= (1<<Settings_LED);
		
	}
	else if (Serial_Data[1]== IP_Change)
	{
		
		Settings_LED_PORT &= ~(1<<Settings_LED);
		
		// send "OK"  Ack sending 1st, because after changing, It won't communicate.
		if (Method==true)			// Serial
		{
			UDR1= IP_Change;
			while( !( UCSR1A & (1<<UDRE1)) );
			_delay_ms(20);
		}
		else
		{
			UDP_send("Changed IP",10,10);
		}
		
		for (char x=0;x<4;x++)
		{
			My_IP[x]= ENC_Data[2+x];
		}
		
		
		for (uint8_t x=0;x<4;x++)				// IP Write
		{
			eeprom_write_byte((( uint8_t *)6+x),My_IP[x]);
		}
		
		Settings_LED_PORT |= (1<<Settings_LED);
	}
	
	else if (Serial_Data[1]== Module_Restart)
	{
		
		Settings_LED_PORT &= ~(1<<Settings_LED);
		// send "OK"
		if (Method==true)			// Serial
		{
			UDR1= Module_Restart;
			while( !( UCSR1A & (1<<UDRE1)) );
			_delay_ms(20);
		}
		else
		{
			UDP_send("Module is restarted",19,10);
		}
		ENC_init();
		Settings_LED_PORT |= (1<<Settings_LED);
	}
	
	else
	{	
		Settings_LED_PORT &= ~(1<<Settings_LED);
		
		// send "??"
		if (Method==true)			// Serial
		{
			UDR1= 0x00;
			while( !( UCSR1A & (1<<UDRE1)) );
			_delay_ms(20);
		}
		else
		{
			UDP_send("Unknown Command",15,10);
		}
		Settings_LED_PORT |= (1<<Settings_LED);
	}
}


ISR(USART1_RX_vect)
{
	TIMSK &= ~(1<< OCIE1B);
	
	if (Outgoing_byte_count<500 & ((UCSR1A & (1<<RXC1))>0))
	{
		Serial_Data[Outgoing_byte_count] = UDR1;
		Outgoing_byte_count++;
	}
	
	TIMSK |= (1<< OCIE1B);
}