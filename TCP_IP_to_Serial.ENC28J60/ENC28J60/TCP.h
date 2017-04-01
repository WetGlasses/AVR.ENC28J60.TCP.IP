/*
 * TCP.h
 *
 * Created: 7/10/2016 12:22:47 PM
 *  Author: ASUS
 */ 

/*

[Ethernet Frame[IP packet[TCP Data]]]

Data to be sent --->> TCP format data --->> Payload of IP Packet --->> Payload of Ethernet frame --->> Transmit through wire

*/

#include "checksum.h"

char PORTH	= 0x00;
char PORTL	= 0x64;		// PORT 100

uint16_t Dest_port = 100;

unsigned long Squnc = 12458;		// My sequence number
unsigned long Next_Ack = 0 ;		// My Ack number
unsigned long last_data_seq_num = 0; // sequence number of the last data packet. New must be greater than that
char ACK_Data[600];
uint16_t ACK_length =0;
bool connected = false;

int TCP_check(char data[], int length);
void Make_TCP_Packet(char raw_data[], int data_length, bool SYN, bool ACK, bool psh);


int TCP_check(char data[], int length)
{
	uint16_t data_length =0 ;
	if (length<50)
	{
		return 0;
	}
	
	else if ((data[12]==0x08)&&(data[13]==0x00)&&(data[23]==0x06)&&(data[30]==My_IP[0])&&(data[31]==My_IP[1])&&(data[32]==My_IP[2])&&(data[33]==My_IP[3]))
	{
		
		data_length = (   (((data[16]<<8) & 0xFF00) | data[17] ) - ((data[14] & 0x0F)*4) - (( (data[46]>>4) & 0x0F)*4)  ) ; 
			
		if (((data[47] & 0b00000010)>0)& (connected==false))				// check SYN
		{
			Next_Ack = ((data[38] * 16777216UL ) + (data[39] * 65536UL) + (data[40] * 256UL ) + data[41] + 1 ) ;
			// Destination IP set
			for(char x =0;x<4;x++)
			{
				Dest_IP[x]=data[26+x];
			}
			 
			// Destination MAC set
			
			for(char x =0;x<6;x++)
			{
				Remote_mac[x]=data[6+x];
			}
			Dest_port = ((data[34]<<8)| data[35]);  // Destination Port set
			Make_TCP_Packet("",0, true,true, false);			// send SYN+ACK
			Squnc++;
			connected= true;
		}
		
		else if (((data[47] & 0b00000100)>0) | ((data[47] & 0b00000001)>0))				// check FIN and RST
		{
			Next_Ack = ((data[38] * 16777216UL ) + (data[39] * 65536UL) + (data[40] * 256UL ) + data[41] + 1 ) ;
			Make_TCP_Packet("",0, false,true, false);
			Squnc = 12458;
			last_data_seq_num =0;
			Next_Ack = 0 ;
			ACK_length = 0;
			connected= false;
		}
		else if((data_length>0) & (connected==true))				//	This part will forward Data to the upper layer
		{
			if (((data[38] * 16777216UL ) + (data[39] * 65536UL) + (data[40] * 256UL ) + data[41] )> last_data_seq_num)		// Detect if not retransmission. If not retransmission, forward data and increase Ack.
			{
				Next_Ack+=data_length;
				last_data_seq_num=((data[38] * 16777216UL ) + (data[39] * 65536UL) + (data[40] * 256UL ) + data[41]);
				
				for (uint16_t X = 0;X<data_length;X++)			// Extract Data-Only
				{
					data[X] = data[length-data_length + X];
				}
			} 
			else
			{
				data_length=0;
			}	
			Make_TCP_Packet("",0, false,true, false);				// General Ack
			return data_length;
		}
	}
	return 0;
}



void Make_TCP_Packet(char raw_data[], int data_length, bool SYN, bool ACK, bool psh)
{
	
	char TCP_packet[600];
	uint16_t checksum_value;

	//	IP header starts
	
	TCP_packet[0]= 0x45;
	TCP_packet[1]= 0x00;
	
	TCP_packet[2]= (((data_length+40)>>8) & 0xFF);
	TCP_packet[3] = ((data_length+40) & 0xFF);	
	
	TCP_packet[4]=0x1A;			//	Arbitrary ID= 0x1010
	TCP_packet[5]=0x30;			//
	TCP_packet[6]=0x00;
	TCP_packet[7]=0x00;
	TCP_packet[8]=0x80;			//	Time to Live
	TCP_packet[9]=0x06;			// TCP
	
	TCP_packet[10]=0x00;		// IP checksum 10,11
	TCP_packet[11]=0x00;
	
	TCP_packet[12]=My_IP[0];
	TCP_packet[13]=My_IP[1];
	TCP_packet[14]=My_IP[2];
	TCP_packet[15]=My_IP[3];
	TCP_packet[16]=Dest_IP[0];
	TCP_packet[17]=Dest_IP[1];
	TCP_packet[18]=Dest_IP[2];
	TCP_packet[19]=Dest_IP[3];
	
	// Checksum update
	checksum_value = checksum(TCP_packet,0,19);
	TCP_packet[10]= ((checksum_value>>8) & 0xFF);		// IP checksum 10,11
	TCP_packet[11]= (checksum_value & 0xFF);
	
	
	//	TCP_port
	
	TCP_packet[20]= PORTH;
	TCP_packet[21]= PORTL;
	
	TCP_packet[22]= ((Dest_port>>8) & 0xFF);
	TCP_packet[23]= (Dest_port & 0xFF);
	
	
	//	TCP_Sequence_number
	TCP_packet[24]= ((Squnc>>24) & 0xFF);
	TCP_packet[25]= ((Squnc>>16) & 0xFF);
	TCP_packet[26]= ((Squnc>>8) & 0xFF);
	TCP_packet[27]= ( Squnc & 0xFF);
	
	
	//	TCP_Ack_number
	TCP_packet[28]= ((Next_Ack>>24) & 0xFF);
	TCP_packet[29]= ((Next_Ack>>16) & 0xFF);
	TCP_packet[30]= ((Next_Ack>>8) & 0xFF);
	TCP_packet[31]= ( Next_Ack & 0xFF);
	
	
	//	TCP_data_offset_and_control_bit
	TCP_packet[32]= (0x05 << 4);
	TCP_packet[33]= 0x00;
	
	if (psh== true)
	{
		TCP_packet[33]|= (0x01 << 3);
	}
	if (SYN==true)
	{
		TCP_packet[33] |= (0x01 << 1); 
	}
	if (ACK==true)
	{
		TCP_packet[33] |= (0x01 << 4);
	}
		
	//	TCP_window
	TCP_packet[34]= 0x05;
	TCP_packet[35]= 0xDC;
	
	//	checksum
	TCP_packet[36]= 0x00;
	TCP_packet[37]= 0x00;
	
	//	TCP_pointers
	TCP_packet[38]= 0x00;
	TCP_packet[39]= 0x00;
	
	for (int x = 0;x<data_length;x++)
	{
		TCP_packet[40+x] = raw_data[x];
	}
	
	
	//	Checksum part Starts
	
	char TCP_checksum[1500];
	
	// Pseudo header
	TCP_checksum[0]=My_IP[0];
	TCP_checksum[1]=My_IP[1];
	TCP_checksum[2]=My_IP[2];
	TCP_checksum[3]=My_IP[3];
	TCP_checksum[4]=Dest_IP[0];
	TCP_checksum[5]=Dest_IP[1];
	TCP_checksum[6]=Dest_IP[2];
	TCP_checksum[7]=Dest_IP[3];
	TCP_checksum[8]=0x00;
	TCP_checksum[9]=0x06;
	TCP_checksum[10]= (((data_length+20)>>8)& 0xFF);
	TCP_checksum[11]= ((data_length+20) & 0xFF);

	
	//	TCP Header & Data
 	for (uint16_t data=0;data<(20+data_length);data++)
 	{ 		
		TCP_checksum[12 + data]= TCP_packet[data+20];
	}
	
	
	//	TCP_checksum
	checksum_value= checksum(TCP_checksum,0,(data_length + 12 + 20-1 ));
	
	TCP_packet[36]= ((checksum_value>>8) & 0xFF);
	TCP_packet[37]= (checksum_value & 0xFF);
	
	//	Checksum part End
	bool is_OK = false;
	while(is_OK==false)
	{
		is_OK = ENC_Transmit(TCP_packet, (data_length+40) ,'I');
	}
	// ACK checking and retransmitting
	
	bool Got_ACK = false;
	char retransmit = 0x01;
	if (!((ACK==true)&&(SYN==false)&&(psh==false)))
	{
		while (Got_ACK==false)
		{
			if (retransmit>0x0A)
			{
				connected = false;
				break;
			}
			_delay_ms(2);
			while ((NewPacket()>0))
			{
				ACK_length = ENC_Receive(ACK_Data);
				if ((ACK_Data[12]==0x08)&&(ACK_Data[13]==0x00)&&(ACK_Data[23]==0x06)&&(ACK_Data[30]==My_IP[0])&&(ACK_Data[31]==My_IP[1])&&(ACK_Data[32]==My_IP[2])&&(ACK_Data[33]==My_IP[3])&&((ACK_Data[47] & 0b00010000)>0))
				{
					Got_ACK=true;
					if (((ACK_Data[47] & 0b00000100)>0) | ((ACK_Data[47] & 0b00000001)>0))
					{
						Next_Ack = ((ACK_Data[38] * 16777216UL ) + (ACK_Data[39] * 65536UL) + (ACK_Data[40] * 256UL ) + ACK_Data[41] + 1 ) ;
						Make_TCP_Packet("",0, false,true, false);
						connected= false;
					}
					break;
				}
			}
			if (Got_ACK==false)
			{
				is_OK = false;
				while(is_OK==false)
				{
					is_OK = ENC_Transmit(TCP_packet, (data_length+40) ,'I');
				}
			}
			retransmit++;
		}
	}
	Squnc+=data_length;
}