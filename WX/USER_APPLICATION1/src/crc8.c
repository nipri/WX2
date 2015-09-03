 /*  
 * crc8.c
* 
* Computes a 8-bit CRC 
* 
*/

#include <stdio.h>

//#define DI  0x07	//Poly X8+X2+X+1
#define DI 0x19		//Poly X8+X5+X4+1

uint8_t crc8(uint8_t data[], uint8_t);
void init_crc8(void);

uint8_t crc8_table[256];     /* 8-bit table */
static uint8_t made_table = 0;

void init_crc8(void) {

	uint16_t i;
	uint8_t j, crc;
  
		for (i=0; i<256; i++) {

			crc = i;

			for (j=0; j<8; j++)
				crc = (crc << 1) ^ ((crc & 0x80) ? DI : 0);

			crc8_table[i] = crc & 0xFF;
		}

	made_table=1;
}

uint8_t crc8(uint8_t data[], uint8_t length) {
/*
* For a byte array whose accumulated crc value is stored in *crc, computes
* resultant crc obtained by appending m to the byte array
*/

	uint8_t i;
	uint8_t crc = 0x0;
 
	if (made_table == 0)
		init_crc8();

	for (i=0; i<length; i++) {
		crc = crc8_table[crc ^ data[i]];
	}
	return (crc & 0xff);
}


