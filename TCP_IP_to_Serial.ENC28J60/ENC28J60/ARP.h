/*
 * ARP.h
 *
 * Created: 7/21/2016 3:51:12 PM
 *  Author: ASUS
 */ 

char ARP_IP[4];
char Source_MAC[6];

char ARP_check(char data[]);
void ARP_reply(char dest_IP[] , char dest_MAC[]);

char ARP_check(char data[])
{
		if ((data[12]==0x08)&&(data[13]==0x06)&&(data[14]==0x00)&&(data[38]==My_IP[0])&&(data[39]==My_IP[1])&&(data[40]==My_IP[2])&&(data[41]==My_IP[3]))
		{
			Source_MAC[0]= data[6];
			Source_MAC[1]= data[7];
			Source_MAC[2]= data[8];
			Source_MAC[3]= data[9];
			Source_MAC[4]= data[10];
			Source_MAC[5]= data[11];
			ARP_IP[0]= data[28];
			ARP_IP[1]= data[29];
			ARP_IP[2]= data[30];
			ARP_IP[3]= data[31];
			
			//	Yes, Its ARP for you
			ARP_reply(ARP_IP ,Source_MAC);
			return 'N';
		}
		else
		{
			if (data[0]==0xFF)
			{
				return 'N';				//	No, ignore it
			}
			else 
			{
				return 'O';				//	Other packet for you
			}
		}

}

void ARP_reply(char dest_IP[] , char dest_MAC[] )
{
	char ARP_Request[29] = {0x00,0x01,0x08,0x00,0x06,0x04,0x00,0x02, 
							MAC[0],MAC[1],MAC[2],MAC[3],MAC[4],MAC[5],
							My_IP[0],My_IP[1],My_IP[2],My_IP[3],
							dest_MAC[0],dest_MAC[1],dest_MAC[2],dest_MAC[3],dest_MAC[4],dest_MAC[5],
							dest_IP[0],dest_IP[1],dest_IP[2],dest_IP[3], 
							0x00};
							
	ENC_Transmit(ARP_Request,29,'A');
}