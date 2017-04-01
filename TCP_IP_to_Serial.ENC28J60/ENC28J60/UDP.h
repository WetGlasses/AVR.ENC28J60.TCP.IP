/*
 * UDP.h
 *
 * Created: 7/30/2016 10:28:58 AM
 *  Author: ASUS
 */ 


#ifndef UDP_H_
#define UDP_H_

#define PORTH	0x00
#define PORTL	0x64		// PORT 100
int UDP_check(char data[], int length);
//	Returns the length of necessary data in the UDP packet. If the packet is not valid, returns 0
//	rewrite the data[] array with only the main data beneath the UDP packet.
// length is the length of the incoming frame

bool UDP_send(char data[], uint16_t length, int number);
// number = serial number of the packet
// returns true or false depending on success.


int UDP_check(char data[], int length)
{
	uint16_t data_length;
	if (length<42)
	{
		return 0;
	}
	
	else if ((data[12]==0x08)&&(data[13]==0x00)&&(data[23]==0x11)&&(data[30]==My_IP[0])&&(data[31]==My_IP[1])&&(data[32]==My_IP[2])&&(data[33]==My_IP[3])&&(data[36]== PORTH)&&(data[37]==PORTL))
	{
		data_length = ((data[38]<<8) | ( 0xFF & data[39])) -8 ;
		for (int x = 0; x<length;x++)
		{
			data[x]=data[42+x];
		}
		return data_length;
	}
	else
	return 0;
}

bool UDP_send(char data[], uint16_t length, int number)		// Use number = 10 
{
	uint16_t total_data = length+28;
	uint16_t checksum_value, x =0x00;
	char new_data[1600];
	
	char data_high, data_low;
	data_high= (total_data>>8);
	data_low= (total_data & 0xFF);
	
	total_data = length+8;	// For data length field
	
	for (x = 0;x<length;x++)
	{
		new_data[28+x]=data[x];
	}	
	new_data[28+x]= 0x00;	//Extra one byte for CRC checking
	
	
	
	//	IP header starts
	
	new_data[0]= 0x45;
	new_data[1]= 0x00;
	new_data[2]=data_high;
	new_data[3]=data_low;
	new_data[4]=0x1A;				//	Arbitrary ID= 0x1010
	new_data[5]=0x30;				//	
	new_data[6]=0x00;
	new_data[7]=0x00;
	new_data[8]=0x80;			//	Time to Live
	new_data[9]=0x11;			// UDP
	new_data[10]=0x00;			// IP checksum 10,11
	new_data[11]=0x00;
	
	//	UDP header starts
	
	new_data[12]=My_IP[0];
	new_data[13]=My_IP[1];
	new_data[14]=My_IP[2];
	new_data[15]=My_IP[3];
	new_data[16]=Dest_IP[0];
	new_data[17]=Dest_IP[1];
	new_data[18]=Dest_IP[2];
	new_data[19]=Dest_IP[3];	
				
	// IP header finish
	
	new_data[20]=0xF6;			// ID
	new_data[21]=0x30;
	new_data[22]=PORTH;
	new_data[23]=PORTL;
	new_data[24]=((total_data>>8) & 0xFF);
	new_data[25]=(total_data & 0xFF);
	new_data[26]=0x00;			//	UDP Checksum 26, 27
	new_data[27]=0x00;
	
	//	UDP header finish
	
	total_data = length+28;
	
	// IP checksum calculation
	checksum_value= checksum(new_data,0,19);			// Minus korsi milanor jonno. jani na keno always ek beshi ashe
	
	new_data[10]= (checksum_value>>8) & 0xFF;
	new_data[11]= (checksum_value & 0xFF);
	
	// UDP checksum calculation
	// Optional for IPv4
	
	
	//	Transmit
	bool Ans = ENC_Transmit(new_data, total_data ,'I');
	return Ans;
}
#endif /* UDP_H_ */