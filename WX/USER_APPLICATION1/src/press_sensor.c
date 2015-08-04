
#include <asf.h>
#include <avr/delay.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>


#define START	0x08
#define RESTART	0x10
#define SL_ACK	0x18
#define MT_DATA_ACK 0x28
#define MR_SLA_ACK	0x40

uint8_t BMP_SLA_W = 0xee;
uint8_t BMP_SLA_R = 0xef;

struct bmpcal {
	int16_t ac1;
	int16_t ac2;
	int16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	int16_t b1;
	int16_t b2;
	int16_t mb;
	int16_t mc;
	int16_t md;
	
} bmp;

inline void twiError(uint8_t);
uint8_t getBMP_ID(void);
void getBMPcoefficients(void);
int16_t getBMPtemp(void);
int16_t calculateBMPtemp(uint32_t);

inline void twiError(uint8_t code) {
	
//	memset(data, 0, 128);
//	sprintf(data, "TWI Error Code %d\r\n", code);
//	sendUART0data(data, sizeof(data));
}

uint8_t getBMP_ID(void) {
	
	uint8_t id;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
	
	if ( (TWSR & 0xf8) != START)
	twiError(0);
	
	TWDR = BMP_SLA_W;						// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
	twiError(1);
	
	TWDR = 0xd0;							// Load and send address of the BMP ID register which should contain 0x55
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)		// Look for slave ACK
	twiError(2);
	
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = BMP_SLA_R;							// Send the slave module address + read bit and wait
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
	twiError(4);
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
	
	while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

	id = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	return id;
	
}

inline void getBMPcoefficients(void) {
	
	uint8_t MSB, LSB, i;
	int16_t coeffData;
	
	for (i = 0xaa; i <= 0xbe; i += 2) {
		
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		// send Start
		while ( !(TWCR & (1<<TWINT) ) );			// Wait for Start to be transmitted
		
		if ( (TWSR & 0xf8) != START)
		twiError(0);
		
		TWDR = BMP_SLA_W;							// Send the slave module address + write bit
		TWCR = (1<<TWINT) | (1<<TWEN);				// Transmit the address
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != SL_ACK)				// Look for slave ACK
		twiError(1);
		
		TWDR = i;									// Send the EEPROM coefficient address to read
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
		twiError(2);
		
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send Repeated Start and wait...
		while (!(TWCR & (1<<TWINT)));
		
		TWDR = BMP_SLA_R;							// Send the slave module address + read bit
		TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MR_SLA_ACK)			// Look for slave ACK
		twiError(4);

		TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);   // Send a RESTART and an ACK after MSB is received
		while (!(TWCR & (1<<TWINT)));
		
		MSB = TWDR;
		
		TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);   // Send a RESTART and a NACK after LSB is received
		while (!(TWCR & (1<<TWINT)));

		LSB = TWDR;
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		
		
		coeffData = (MSB << 8) | LSB;
		
		switch (i) {
			
			case 0xaa:
			bmp.ac1 = (MSB << 8) | LSB;
			break;
			
			case 0xac:
			bmp.ac2 = (MSB << 8) | LSB;
			break;
			
			case 0xae:
			bmp.ac3 = (MSB << 8) | LSB;
			break;
			
			case 0xb0:
			bmp.ac4 = (MSB << 8) | LSB;
			break;
			
			case 0xb2:
			bmp.ac5 = (MSB << 8) | LSB;
			break;
			
			case 0xb4:
			bmp.ac6 = (MSB << 8) | LSB;
			break;
			
			case 0xb6:
			bmp.b1 = (MSB << 8) | LSB;
			break;
			
			case 0xb8:
			bmp.b2 = (MSB << 8) | LSB;
			break;
			
			case 0xba:
			bmp.mb = (MSB << 8) | LSB;
			break;
			
			case 0xbc:
			bmp.mc = (MSB << 8) | LSB;
			break;
			
			case 0xbe:
			bmp.md = (MSB << 8) | LSB;
			break;
			
			default:
			break;
		}
		
	}
}


int16_t getBMPtemp(void) {
	
	uint8_t MSB, LSB;
	uint32_t rawTempData;
	int16_t calcTemp;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		// send Start
	while ( !(TWCR & (1<<TWINT) ) );			// Wait for Start to be transmitted
	
	if ( (TWSR & 0xf8) != START)
	twiError(5);
	
	TWDR = BMP_SLA_W;							// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);				// Transmit the address
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)				// Look for slave ACK
	twiError(6);
	
	TWDR = 0xf4;								// Send the control register address
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
	twiError(7);
	
	TWDR = 0x2e;								// Send the control register data
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
	twiError(8);
	
	TWDR = 0xf6;								// Send the address of the AD MSB
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
	twiError(8);
	
	_delay_ms(5);
	
	
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = BMP_SLA_R;							// Send the slave module address + read bit
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			// Look for slave ACK
	twiError(8);

	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);   // Send a RESTART and an ACK after MSB is received
	while (!(TWCR & (1<<TWINT)));
	
	MSB = TWDR;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);   // Send a RESTART and a NACK after LSB is received
	while (!(TWCR & (1<<TWINT)));

	LSB = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	rawTempData = (MSB << 8) | LSB;
	calcTemp = calculateBMPtemp(rawTempData);
	
	return calcTemp;
	
}

int16_t calculateBMPtemp(uint32_t rawData) {
	
	int32_t x1, x2, b5;
	int16_t t;
	
	x1=(rawData - bmp.ac6) * bmp.ac5 /pow(2, 15);
	x2 = bmp.mc * pow(2, 11)/(x1 + bmp.md);
	b5 = x1 + x2;
	t = (b5 + 8)/pow(2, 4);
	
	return t;
}