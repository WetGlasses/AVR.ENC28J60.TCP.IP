/*

Library file for SPI ENC memory.
Written by Shadman Sakib
2ra Technology Ltd.
 */ 

/*

Needs the following libraries to work properly:

#define F_CPU 

#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#include "hd44780_settings.h"
#include "hd44780.h"
#include "hd44780.c"

*/

#define ENC_SS PB0				// PB4 is connected to the device's SS pin. To pull it on and off. 									
#define IPv4 0x08


///////////////////////////////////////		Commands for ENC	////////////////////////////////////////

#define RCR 0x00	//	Read Control Register
#define RBM 0x3A	//	Read Buffer Memory
#define WCR 0x40	//	Write Control Register
#define WBM 0x7A	//	Write Buffer Memory (Constant argument. Added here already)
#define BFS 0x80	//	Bit Field Set
#define BFC 0xA0	//	Bit Field Clear
#define Reset 0xFF	//	System Reset command (Constant argument. Added here already)
							
///////////////////////////////////////		Registers Needed	////////////////////////////////////////



// Common Bank

#define ECON1 0x1F			//	Bank SElection Register

#define ECON2 0x1E
#define ESTAT 0x1D
#define EIE 0x1B
#define EIR 0x1C

// Bank 0

#define ERDPTL 0x00
#define ERDPTH 0x01
#define EWRPTL 0x02
#define EWRPTH 0x03
#define ERXSTL 0x08
#define ERXSTH 0x09
#define ERXNDL 0x0A
#define ERXNDH 0x0B
#define ERXRDPTL 0x0C
#define ERXRDPTH 0x0D
#define ERXWRPTL 0x0E
#define ERXWRPTH 0x0F

#define ETXSTL 0x04
#define ETXSTH 0x05
#define ETXNDL 0x06
#define ETXNDH 0x07


// Bank 1

#define ERXFCON 0x18
#define EPKTCNT 0x19

// Bank 2

#define MACON1 0x00
#define MACON3 0x02
#define MACON4 0x03
#define MABBIPG 0x04
#define MAIPGL 0x06
#define MAIPGH 0x07
#define MACLCON1 0x08
#define MACLCON2 0x09
#define MAMXFLL 0x0A
#define MAMXFLH 0x0B

#define MIREGADR 0x14
#define MICMD 0x12
#define MIRDH 0x19
#define MIRDL 0x18
#define MIWRH 0x17
#define MIWRL 0x16


// Bank 3

#define MAADR5 0x00
#define MAADR6 0x01
#define MAADR3 0x02
#define MAADR4 0x03
#define MAADR1 0x04
#define MAADR2 0x05

#define MISTAT 0x0A
#define REVID 0x12

//	PHY Registers

#define PHCON1 0x00
#define PHCON2 0x10
#define PHLCON 0x14


///////////////////////////////////////		MAC ADDRESS		////////////////////////////////////////

char MAC[6] = {0x08,0x62,0x66,0xD7,0x3B,0xAF};
char Remote_mac[6]= {0x15,0xCC,0x20,0xE2,0xEF,0xFE};

///////////////////////////////////////		IP ADDRESS		////////////////////////////////////////	
	
char Dest_IP[4] = {192,168,0,150};
char My_IP[4] = {192,168,0,30};
	
///////////////////////////////////////		Variables		////////////////////////////////////////	

char packet_save_loc_high = 0x00;
char packet_save_loc_low = 0x00;

/////////////////////////////////////////////		Functions	////////////////////////////////////////////

void ENC_MasterInit(void);
void ENC_Write(char data);
void ENC_Double_Write(char data1, char data2);
void ENC_Continuous_Write(char data);
char ENC_Continuous_Read(void);
char ENC_Write_and_Read(char data);
char ENC_Write_and_Read_dummy(char data);

void ENC_init();
bool ENC_Transmit(char data[], uint16_t data_length, char protocol);  // returns True if successful, False if not.
int ENC_Receive(char data[]);  // Returns the length of received data, Fill the argument array with that data
char NewPacket();

uint16_t PHY_read(char PHY_addr);
void PHY_Write(char PHY_addr, char datah, char datal);

///////////////////////////////////////		SPI Related Functions	////////////////////////////////////////



void ENC_MasterInit(void)
{
														/* Set MOSI and SCK output, all others input */
	DDRB |= (1<<DDB2)|(1<<DDB1)|(1<<PB0)|(1<<ENC_SS);				
														/* Enable SPI, Master, set clock rate fck/4 */
	SPCR = (1<<SPE)|(1<<MSTR);							// Minimum prescaler. 2MHz speed B-|
	
														// SPSR register. Nothing to do.... ^_^
}


void ENC_Write(char data)
{
	PORTB |= (1<<ENC_SS);
	_delay_ms(1);
	PORTB &=~(1<<ENC_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));						// SPI data transmission completion checking
	PORTB |= (1<<ENC_SS); 
}

char ENC_Write_and_Read_dummy(char data)
{
	
	// MAC and MII registers transmits a dummy byte first
	
	PORTB |= (1<<ENC_SS);
	_delay_ms(1);
	PORTB &=~(1<<ENC_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));						// SPI data transmission completion checking
	SPDR = 0xFF;
	while(!(SPSR & (1<<SPIF)));
	data = SPDR;
	PORTB |= (1<<ENC_SS);
	return data;
}

void ENC_Double_Write(char data1, char data2)
{
	PORTB |= (1<<ENC_SS);
	_delay_ms(1);
	PORTB &=~(1<<ENC_SS);
	SPDR = data1;
	while(!(SPSR & (1<<SPIF)));						// SPI data transmission completion checking
	SPDR = data2;
	while(!(SPSR & (1<<SPIF)));						// SPI data transmission completion checking
	PORTB |= (1<<ENC_SS);
}

void ENC_Continuous_Write(char data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));		
				
}

char ENC_Continuous_Read(void)
{
	SPDR = 0xFF;
	while(!(SPSR & (1<<SPIF)));
	return SPDR;
}
///////////////////////////////////////////////	ENC related functions	//////////////////////////////////////////////////////


/*

Initialization Needs: (According to Page 33 of the datasheet)

Setting up the following Registers:
 (1st High byte, then Low byte)

	//	Recieve Pointers
1.	ERXST
2.	ERXND (Program with even number)
3.	ERXWRPT (Auto update. But slow)
4.	ERXRDPT (First Low, then High)


	//	Filter
5.	ERXFCON (Enable CRC Check with others)

	// Oscillator Start-up Time (OST)
6.	ESTAT.CLKRDY	(Read to determine the time for proceeding to MAC address change) 


	//	MAC address initialization
7.	MACON1 ----> MARXEN >> 1
8.	MACON1 ----> TXPAUS (Optional. Full DUplex flow control)
9.	MACON1 ----> RXPAUS (Optional. Full DUplex flow control)
10.	MACON3 ----> PADCFG, TXCRCEN, FULDPX, FRMLNEN
11. MACON4. (DEFER>>1 for IEEE conformance. Only for Half duplex mode) 
12.	MAMXFL (Normal nodes are designed to handle 1518 bytes or less)
13.	MABBIPG. (15h for full duplex, 12h for half-duplex)
14.	MAIPGH (for half duplex mode. Typically 0x0C)
15. MACLCON1 (If Half Duplex. Keep default) 
16. MACLCON2 (If Half Duplex. Keep default. For extremely long cable, increase) 
17.	MACON3.PADCFG<2:0> for auto padding and CRC enabling.
18. SET MAC ADDRESS:	MAADR1 : MAADR6

	//	Auto update of pointers.
19.	ECON2.AUTOINC

	//	PHY initialization
20.	PHCON1.PDPXMD (Initialized correctly by external circuitry, which is the LED Configuration. 
	But might be needed to be programed if not present or incorrect )
21.	PDPXMD bit may be read and FULDPX bit be programmed for externally configurable system.
22. PHCON1.PDPXMD bit must match the value of MACON3.FULDPX bit for proper duplex operation.
23. For half duplex, PHCON2.HDLDIS bit might be set to prevent auto loopback.
24. PHLCON controls LED A & B. If need anything special, change it from default. Otherwise leave it.
25.	Enable receive Using ECON1.

*/

uint16_t PHY_read(char PHY_addr)
{
	ENC_Double_Write((WCR|ECON1), 0x02);	//	Set bank 2 
	ENC_Double_Write((WCR|MIREGADR), PHY_addr);
	char past = ENC_Write_and_Read(RCR|MICMD);
	ENC_Double_Write((WCR|MICMD), (past | 0x01));
	_delay_us(11);
	ENC_Double_Write((WCR|ECON1), 0x03);	//	Set bank 3
	
	// Busy checking
	past = ENC_Write_and_Read(RCR|MISTAT);
	past = past & 0x01;
	while (past==0x01)
	{
		past = ENC_Write_and_Read(RCR|MISTAT);
		past = past & 0x01;
	}
	
	//	Free
	ENC_Double_Write((WCR|ECON1), 0x02);	//	Set bank 2 
	
	// Clear MICMD.MIIRD
	past = ENC_Write_and_Read(RCR|MICMD);
	ENC_Double_Write((WCR|MICMD), ~(past & 0x01));
	
	//	Read Data
	char datah = ENC_Write_and_Read(RCR|MIRDH);
	char datal = ENC_Write_and_Read(RCR|MIRDL);
	int result = datal+ (8<<datah);
	return result;
	
}


void PHY_Write(char PHY_addr, char datah, char datal)
{
	ENC_Double_Write((WCR|ECON1), 0x02);	//	Set bank 2
	ENC_Double_Write((WCR|MIREGADR), PHY_addr);
	ENC_Double_Write((WCR|MIWRL), datal);
	ENC_Double_Write((WCR|MIWRH), datah);
	_delay_us(11);
	ENC_Double_Write((WCR|ECON1), 0x03);	//	Set bank 3
	
	// Busy checking
	char past = ENC_Write_and_Read(RCR|MISTAT);
	past = past & 0x01;
	while (past==0x01)
	{
		past = ENC_Write_and_Read(RCR|MISTAT);
		past = past & 0x01;
	}
}


void ENC_init()
{
	//	Soft Reset
	ENC_Write(Reset);
	_delay_ms(1);
	
	char CLKRDY = ENC_Write_and_Read(RCR|ESTAT);
	CLKRDY = CLKRDY & 0x01;
	while (CLKRDY==0x00)
	{
		CLKRDY = ENC_Write_and_Read(RCR|ESTAT);
		CLKRDY = CLKRDY & 0x01;
	}
	
	//	Define MAC
	
	
	//	Set Bank first. Have to send 2 bytes.
	ENC_Double_Write((WCR|ECON1), 0x00); //	Everything is at halt now. Must enable RX and TX after initialization
	packet_save_loc_high= 0x00;
	packet_save_loc_low= 0x00;
	ENC_Double_Write((WCR|ERXSTL), 0x00);
	ENC_Double_Write((WCR|ERXSTH), 0x00);
	ENC_Double_Write((WCR|ERXNDL), 0x70);
	ENC_Double_Write((WCR|ERXNDH), 0x17);	
	ENC_Double_Write((WCR|ERXRDPTL), 0x6F);
	ENC_Double_Write((WCR|ERXRDPTH), 0x17);
	
	//	Set Bank 1
	ENC_Double_Write((WCR|ECON1), 0x01);
	
	ENC_Double_Write((WCR|ERXFCON), 0x81);		// Unicast Or Broadcast
	
	
	// Wait for clock to be ready
	CLKRDY = ENC_Write_and_Read(RCR|ESTAT);
	CLKRDY = CLKRDY & 0x01;
	while (CLKRDY==0x00)
	{
		CLKRDY = ENC_Write_and_Read(RCR|ESTAT);
		CLKRDY = CLKRDY & 0x01;
	}
	
	// Clock is ready now
	
	//	Set Bank 2
	ENC_Double_Write((WCR|ECON1), 0x02);
	
	ENC_Double_Write((WCR|MACON1), 0x0D);
	ENC_Double_Write((WCR|MACON3), 0x33);		// Enable CRC generation
	ENC_Double_Write((WCR|MAMXFLL), 0xEE);
	ENC_Double_Write((WCR|MAMXFLH), 0x05);
	ENC_Double_Write((WCR|MABBIPG), 0x15);
	ENC_Double_Write((WCR|MAIPGL), 0x12);
	
	//Set Bank 3
	ENC_Double_Write((WCR|ECON1), 0x03);
	
	//	Writing MAC
	
	ENC_Double_Write((WCR|MAADR1), MAC[0]);
	ENC_Double_Write((WCR|MAADR2), MAC[1]);
	ENC_Double_Write((WCR|MAADR3), MAC[2]);
	ENC_Double_Write((WCR|MAADR4), MAC[3]);
	ENC_Double_Write((WCR|MAADR5), MAC[4]);
	ENC_Double_Write((WCR|MAADR6), MAC[5]);
	
	//	Auto Pointer Update
	ENC_Double_Write((WCR|ECON2), 0x80);
	
	//	Receive Interrupt
	ENC_Double_Write((WCR|EIE), 0xC0);
	
	//PHY Register update
	PHY_Write(PHCON1,0x01,0x00);
	PHY_Write(PHLCON,0x15,0x76);
	
	ENC_Double_Write((WCR|ECON1), 0x04);	//	Bank 0, RX Enabled
	
}



/*

Transmitting Package: (Page 39)

The MAC inside the ENC28J60 will automatically generate the preamble and Start-Of-Frame delimiter fields
when transmitting. Additionally, the MAC can generate any padding (if needed) and the CRC if configured to do
so. The host controller must generate and write all other frame fields into the buffer memory for transmission.

Additionally, the ENC28J60 requires a single per packet control byte to precede the packet for transmission. The
per packet control byte is organized as shown in Figure 7-1 of the data sheet. (Page 39)

Steps for transmission:

1.	Program ETXST to point to an unused location in the memory. Use even address.
2.	use Write Buffer Memory (WBM) SPI command to write:
							A.	The per packet control byte.
							B.	Destination address.
							C.	Source MAC address.
							D.	Type/ Length of the data.
							E.	Data payload.
							
	*** For Packet format, See Page 31. Fig 5-1
	
3.	Program ETXND to point the last byte of the data payload.
4.	Clear EIR.TXIF
5.	Set EIE.TXIE
6.	Set EIE.INTIE to enable interrupt (Optional)
7.	Start transmission by setting ECON1.TXRTS
8.	ECON1.TXRTS will be cleared when transmission is finished.
9.	Transmit status vector of 7 bytes will be written after finishing in ETXND+1 location. EIR.TXIF will be set, interrupt will fire (if enabled)
10.	ETXST and ETXND will not be modified automatically. Change them if you need.
11.	Read ESTAT.TXABRT bit to check if the data was transmitted successfully. (clear if OK.) If not, check Transmit status vectors and ESTAT.LATECOL bit.

	**Details of the Transmit Status Vector is in page 41


*/



/*


	Normal IEEE 802.3 data format:
	==============================
	
	Field						Byte			Duty
	-------------------------------------------------------
	Preamble					7				Chip
	
	SFD	(State of Frame)		1				Chip
	
	Destination Address(MAC)	6				Controller (See page 32. for Multicasting and Broadcasting. FF-FF-FF-FF-FF-FF for broadcasting)
	
	Source Address				6				Controller (**Chip will not automatically add it)
	
	Type/Length					2				Controller (Length of the data without padding)
	
	Data					46-1500				Controller
	
	Padding				with Data (Optional)	Controller/Host (Extra bytes added to data to fulfill minimum data length of IEEE standard)
	
	FCS (Frame check sequence)	4				Chip


	Packet format for Host controller:
	----------------------------------
	A.	Destination MAC address.
	B.	Source MAC address.
	C.	Type/ Length of the data.
	D.	Data payload.	
	E.	Padding. (Controller may manually add it or chip can do, if MACON3.PADCFG<2:0> bits are configured for that)
	
*/

bool ENC_Transmit(char data[], uint16_t data_length, char protocol)
{
	// Setup remote mac before calling this function
	
	//	Data length breaking up
	char high_end ,low_end ;

	
	ENC_Double_Write((WCR|ECON1), 0x04);	//	Bank 0, RX Enabled
	ENC_Double_Write((WCR|ETXSTL), 0x71);
	ENC_Double_Write((WCR|ETXSTH), 0x17);
	ENC_Double_Write((WCR|EWRPTL), 0x71);
	ENC_Double_Write((WCR|EWRPTH), 0x17);
	
	//	Write buffer
	PORTB &=~(1<<ENC_SS);
	
	//	Entering Buffer writing mode
	ENC_Continuous_Write(WBM);
	
	//	Per Packet Control Byte
	ENC_Continuous_Write(0x00);		
	
	// Destination MAC	
	for(uint8_t DM=0;DM<6;DM++)
	{
		ENC_Continuous_Write(Remote_mac[DM]);
	}	
	
	// Source MAC
	for(uint8_t SM=0;SM<6;SM++)
	{
		ENC_Continuous_Write(MAC[SM]);
	}
	
	//	Length/ Type of Data
	
	if (protocol=='I')		//	IP
	{
		ENC_Continuous_Write(IPv4); // Or High_end
		ENC_Continuous_Write(0x00); // Or Low_end
	} 
	else if (protocol=='A')		//	ARP
	{
		ENC_Continuous_Write(0x08); // Or High_end
		ENC_Continuous_Write(0x06); // Or Low_end
	}
	
	
	//	Data
	for (uint16_t x=0;x<data_length;x++)
	{
		ENC_Continuous_Write(data[x]);
	}
	
	//	End	
	
	data_length = 0x1771 + 14 + data_length;		// Pointer update
	high_end =(data_length>>8);
	low_end = (data_length & 0xFF);
	
	ENC_Double_Write((WCR|ETXNDL), low_end);
	ENC_Double_Write((WCR|ETXNDH), high_end);
	
	ENC_Double_Write((WCR|EIR), 0x00);
	ENC_Double_Write((WCR|EIE), 0x88);
	ENC_Double_Write((WCR|ECON1), 0x1C);
	
	char TX_complete = ENC_Write_and_Read(RCR|ECON1);
	TX_complete = TX_complete & 0x08;
	while (TX_complete!=0x00)
	{
		TX_complete = ENC_Write_and_Read(RCR|ECON1);
		TX_complete = TX_complete & 0x08;
	}
	
	// Checking if all went OK..
	TX_complete = ENC_Write_and_Read(RCR|ESTAT);
	TX_complete = TX_complete & 0x02;
	
	if (TX_complete== 0x00)
	{
		return true;
	} 
	else
	{
		return false;
	}
	
}


/*

Receiving
==========================================================================================================
Data will automatically be saved in the buffer memory by the chip.
Filters can be implemented for specific receiving.

Received data format:
---------------------

1.	Next Packet Pointer	[Low, High]			2 Byte
2.	Status vector		-----------			4 Byte
3.	Packet Data			[*********]			Variable. Calculated from Next Packet Pointer
						Destination MAC		6 Byte
						Source MAC			6 Byte
						Type [IP/ Other]	2 Byte
						Data				Variable (Need to break for further encapsulation)
						Padding			Variable
						CRC

4.	Unused				Optional. For even addressing of next packet's start point


Things to do in the initialization process:
-------------------------------------------

1.	Receive buffer initialization
2.	Configure MAC
3.	Configure filter if necessary
4.	Set ECON2.AUTOINC to enable continuous reading from the buffer
5.	Set EIE.PKTIE
6.	Set EIE.INTIE
7.	Set ECON1.RXEN

When packet is received: (You know it from interrupt)
-----------------------------------------------------

The process of reading the buffer memory don't have the option of sending memory location before reading.
Data saved in the address pointed by ERDPT will be transmitted by the chip after RMB command. So, the
controller must set it before reading a packet. AUTOINC option increments the ERDPT by one after each
read operation. But will not tell where the packet starts. This is indicated in the "Next Packet Pointer"
So, the controller must set the ERDPT after reading each packet for reading the next packet.

So, The process becomes:
------------------------

1.	Start reading buffer memory using RBM command.
2.	Read The first two bytes and save in two variable- A and B. But at this point, DO NOT set ERDPT.
3.	Read as many bytes you need. If you want to skip some bytes, You can either use a loop
	reading dummy bytes or change the ERDPT. I used loop method.
4.	When you are done, Set ERDPT registers. (Using A and B)

*/


int ENC_Receive(char data[])
{
	char dummy;
	uint16_t length, length_u, Previous_pos, New_pos;	// Previous pos= Position at this moment. New Pos = Next packet position
	ENC_Double_Write((WCR|ECON1), 0x04); // Bank 0
	
	Previous_pos = ((packet_save_loc_high<<4)|packet_save_loc_low);

	ENC_Double_Write((WCR|ERDPTL), packet_save_loc_low);
	ENC_Double_Write((WCR|ERDPTH), packet_save_loc_high);
	
	//	Reading present memory address from pointer register
	//	Don't get confused because of the Name of the variables. We are reading Present address
	
	PORTB |= (1<<ENC_SS);
	_delay_ms(1);
	PORTB &=~(1<<ENC_SS);
	
	//	Enter Memory Reading Mode
	ENC_Continuous_Write(RBM);
	
	//	Next packet pointer
	packet_save_loc_low = ENC_Continuous_Read();
	packet_save_loc_high = ENC_Continuous_Read();
	
	New_pos = ((packet_save_loc_high<<4)|(packet_save_loc_low & 0x00FF));

	//	Getting the length of Ethernet Frame
	char size_of_frame_A, size_of_frame_B;
	
	size_of_frame_A = ENC_Continuous_Read();
	size_of_frame_B = ENC_Continuous_Read();
	
	//	Little endian format. So, need to swap
	length_u = (size_of_frame_A<<8)|(size_of_frame_B & 0x00FF);
	length = ((length_u<<8)|(length_u>>8));
	length-=4;
	
		
	//	Other status vector
	for (char x = 0;x<2;x++)
	{
		dummy = ENC_Continuous_Read();
	}
	
	if (length>500)
	{
		length = 500;
	}
	for (uint16_t x = 0; x<length;x++)
	{
		data[x]= ENC_Continuous_Read();
	}
	
	PORTB |= (1<<ENC_SS);
	
	//	Set Pointer
	
	if (New_pos<Previous_pos)
	{
		// 		VLAN_ENC_Double_Write((WCR|ERXRDPTL), (0x6F));
		// 		VLAN_ENC_Double_Write((WCR|ERXRDPTH), (0x17));
		ENC_init();
	}
	else
	{
		ENC_Double_Write((WCR|ERXRDPTL), (packet_save_loc_low-1));
		ENC_Double_Write((WCR|ERXRDPTH), (packet_save_loc_high-1));
	}

	ENC_Double_Write((WCR|ECON2), 0xC0);		// PKTCNT decremented	
	return length;
}

char NewPacket()
{
	ENC_Double_Write((WCR|ECON1), 0x05);	//	Bank 1, RX Enabled
	char X = ENC_Write_and_Read(RCR|EPKTCNT);
	return X;
}

char ENC_Write_and_Read(char data)
{
	PORTB |= (1<<ENC_SS);
	_delay_ms(1);
	PORTB &=~(1<<ENC_SS);
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));						// SPI data transmission completion checking
	SPDR = 0xFF;
	while(!(SPSR & (1<<SPIF)));
	data = SPDR;
	PORTB |= (1<<ENC_SS);
	return data;
}