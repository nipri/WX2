
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

//HTU21DF Default Slave Address is 0x40
#define HTU_SLA_W	0x80
#define HTU_SLA_R	0x81

uint8_t HTU_readI2Cbyte(uint8_t whichReg);
uint8_t HTU_writeI2Cbyte(uint8_t whichReg, uint8_t data);
double HTU_getData(uint8_t command);


uint8_t HTU_readI2Cbyte(uint8_t whichReg) {
	
	uint8_t retValue;
		
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
		
	if ( (TWSR & 0xf8) != START) {
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xaa;
	}
		
	TWDR = HTU_SLA_W;						// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
	while (!(TWCR & (1<<TWINT)));
		
	if ( (TWSR & 0xf8) != SL_ACK) {		// Look for slave ACK
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xab;
	}
		
	TWDR = whichReg;							// Load and send address of SI ID register 0x0 which should contain teh PART_ID
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
		
	if ( (TWSR & 0xf8) != MT_DATA_ACK)	{	// Look for slave ACK
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xac;
	}

	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
		
	TWDR = HTU_SLA_R;							// Send the slave module address + read bit and wait
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
		
	if ( (TWSR & 0xf8) != MR_SLA_ACK)	{		 // Look for slave ACK
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xae;
	}
		
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
		
	while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

	retValue = TWDR;
		
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		
	return retValue;	
}


uint8_t HTU_writeI2Cbyte(uint8_t whichReg, uint8_t data) {
	
		uint8_t retValue;
		
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
		while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
		
		if ( (TWSR & 0xf8) != START) {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xaa;
		}
		
		TWDR = HTU_SLA_W;						// Send the slave module address + write bit
		TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != SL_ACK) {		// Look for slave ACK
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xab;
		}
		
		TWDR = whichReg;							// Load and send address of SI ID register 0x0 which should contain teh PART_ID
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)	{	// Look for slave ACK
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xac;
		}
		
		TWDR = data;							// Load and send address of SI ID register 0x0 which should contain teh PART_ID
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)	{	// Look for slave ACK
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xad;
		}
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		
		_delay_ms(30);
		
		// Verify that the data got written
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
		while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
		
		if ( (TWSR & 0xf8) != START) {
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xaa;
		}
		
		TWDR = HTU_SLA_W;							// Send the slave module address + read bit and wait
		TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != SL_ACK) {		 // Look for slave ACK
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xaf;
		}
		
		TWDR = whichReg;							// Load and send address of SI ID register 0x0 which should contain teh PART_ID
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)	{	// Look for slave ACK
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xaf;
		}
		
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
		while (!(TWCR & (1<<TWINT)));
		
		TWDR = HTU_SLA_R;						// Send the slave module address + write bit
		TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MR_SLA_ACK) {		// Look for slave ACK
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
			return 0xac;
		}
		
		TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
		
		while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

		retValue = TWDR;
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		
		return retValue;
	
}

double HTU_getData(uint8_t command) {
	
	uint16_t MSB, LSB, rawData;
	uint8_t crc;
	double retValue;

	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		// send Start
	while ( !(TWCR & (1<<TWINT) ) );			// Wait for Start to be transmitted
	
	TWDR = HTU_SLA_W;							// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);				// Transmit the address
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK) {				// Look for slave ACK
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xaa;
	}			
	
	TWDR = command;								// Send the address of the AD MSB
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK) {			// Look for slave ACK
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xab;
	}
	
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = HTU_SLA_R;							// Send the slave module address + read bit
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK) {			// Look for slave ACK
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		return 0xac;
	}
	
	_delay_ms(100);

	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);   // Send a RESTART and an ACK after MSB is received
	while (!(TWCR & (1<<TWINT)));
	
	MSB = TWDR;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);   // Send a RESTART and an ACK after MSB is received
	while (!(TWCR & (1<<TWINT)));

	LSB = (TWDR & 0xfc);
	LSB = LSB >> 2;
//	LSB = TWDR;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);   // Send a RESTART and a NACK after LSB is received
	while (!(TWCR & (1<<TWINT)));
	
	crc = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	rawData = (MSB << 8) | LSB;
	
	if (command == 0xe3) {	// Temp		
		retValue = -46.85 + 175.72 * (double)(rawData / pow(2, 16));
	}
	else if (command == 0xe5) { // humidity
		retValue = 	 -6 + 125 * (double)(rawData / pow(2,16));
	}
	else {
	}
	
	return retValue;
}
