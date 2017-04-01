/*
 * checksum.h
 *
 * Created: 7/30/2016 3:25:41 PM
 *  Author: ASUS
 */ 


#ifndef CHECKSUM_H_
#define CHECKSUM_H_

uint16_t checksum(char data[],uint16_t start_pos, uint16_t end_pos);
// Must Give even number of data points.

uint16_t checksum(char data[],uint16_t start_pos, uint16_t end_pos)
{
	uint16_t result, x = 0;
	uint32_t Big_boss = 0;
	for (x=start_pos;x<= end_pos;x+=2)
	{
		 if (x == end_pos)
		 {
			 Big_boss+= (((data[x]<<8) & 0xFF00) | 0x00 );
		 }
		 else
		 {
			 Big_boss+= (((data[x]<<8) & 0xFF00) | (data[x+1] & 0xFF));
		 }

		 while (Big_boss>0xFFFF)
		 {
			 Big_boss = ((Big_boss & 0xFFFF) + ((Big_boss>>16) & 0xFFFF)) ;
		 }
	}
	

	result = ~(Big_boss & 0xFFFF);

	return result;
}


#endif /* CHECKSUM_H_ */