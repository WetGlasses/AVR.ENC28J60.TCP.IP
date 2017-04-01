/*
 * TCP_IP.h
 *
 * Created: 7/2/2016 12:56:57 PM
 *  Author: ASUS
 */ 


	
char TCP_IP_packet[1600];
// Change it according to your need

void Make_IP_packet(char raw_data[], uint16_t data_length, char Destination_IP[], char My_IP[], char protocol, char result[]);		// Rearranges the input array and put the headers
void version_and_header();
void Type_of_service();
void Total_length(uint16_t length_of_data);
void Fragment_number();
void Flag_and_fragment_offset();
void Time_to_live();
void Protocol();	
void IP_Checksum();
void Source_IP(char My_IP[]);
void Target_IP(char Destination_IP[]);




void Make_IP_packet(char raw_data[], uint16_t data_length, char Destination_IP[],char My_IP[], char protocol, char result[])
{
	version_and_header();
	Type_of_service();
	Total_length(data_length);
	Fragment_number();
	Flag_and_fragment_offset();
	Time_to_live();
	Protocol();
	IP_Checksum();
	Source_IP(My_IP);
	Target_IP(Destination_IP);
	
	for (uint16_t x = 0;x<data_length;x++)
	{
		TCP_IP_packet[20+x] = raw_data[x];
	}
	
	for (uint16_t x = 0;x<(data_length+20);x++)
	{
		result[x] = TCP_IP_packet[x];
	}
	
	
}


void version_and_header()
{
	TCP_IP_packet[0]= 0x45;
}

void Type_of_service()
{
	TCP_IP_packet[1]= 0x00;
}

void Total_length(uint16_t length_of_data)
{
	length_of_data+=20;
	
	TCP_IP_packet[2]= (length_of_data>>8);
	TCP_IP_packet[3] = (length_of_data & 0xFF);
}

void Fragment_number()						//	Give a Random number for small data.
{
	TCP_IP_packet[4]= 0x00;
	TCP_IP_packet[5] = 0x00;
}

void Flag_and_fragment_offset()
{
	TCP_IP_packet[6]= 0x40;
	TCP_IP_packet[7] = 0x00;
}

void Time_to_live()
{
	TCP_IP_packet[8]= 0x80;
}

void Protocol()						
{
	TCP_IP_packet[9]= 0x06;					//	For TCP. Change will occure for UDP and others.
}

void IP_Checksum()
{
	TCP_IP_packet[10]= 0x00;
	TCP_IP_packet[11]= 0x25;
}

void Source_IP(char My_IP[])
{
	TCP_IP_packet[12]= My_IP[0];
	TCP_IP_packet[13]= My_IP[1];
	TCP_IP_packet[14]= My_IP[2];
	TCP_IP_packet[15]= My_IP[3];
}
void Target_IP(char Destination_IP[])
{
	TCP_IP_packet[16]= Destination_IP[0];
	TCP_IP_packet[17]= Destination_IP[1];
	TCP_IP_packet[18]= Destination_IP[2];
	TCP_IP_packet[19]= Destination_IP[3];
}
