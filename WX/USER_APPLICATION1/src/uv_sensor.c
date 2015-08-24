
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

//Si1145 Slave Address is 0x60
#define SI_SLA_W	0xc0
#define SI_SLA_R	0xc1

uint8_t getSI_PartID(void);
uint8_t getSI_PartID(void);
uint8_t getSI_RevID(void);
uint8_t getSI_SeqID(void);


uint8_t getSI_PartID(void) {
	
	uint8_t id;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
	
	if ( (TWSR & 0xf8) != START)
	return 0xaa;
	
	TWDR = 0xc0;						// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
	return 0xab;
	
	TWDR = 0x00;							// Load and send address of SI ID register 0x0 which should contain teh PART_ID
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)		// Look for slave ACK
	return 0xac;

	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = SI_SLA_R;							// Send the slave module address + read bit and wait
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
	return 0xae;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
	
	while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

	id = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	return id;
	
}

uint8_t getSI_RevID(void) {
	
	uint8_t id;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
	
	if ( (TWSR & 0xf8) != START)
	return 0xaa;
	
	TWDR = 0xc0;						// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
	return 0xab;
	
	TWDR = 0x01;							// Load and send address of SI register
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)		// Look for slave ACK
	return 0xac;

	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = SI_SLA_R;							// Send the slave module address + read bit and wait
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
	return 0xae;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
	
	while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

	id = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	return id;
	
}

uint8_t getSI_SeqID(void) {
	
	uint8_t id;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
	
	if ( (TWSR & 0xf8) != START)
	return 0xaa;
	
	TWDR = 0xc0;						// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
	return 0xab;
	
	TWDR = 0x02;							// Load and send address of SI register
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)		// Look for slave ACK
	return 0xac;

	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = SI_SLA_R;							// Send the slave module address + read bit and wait
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
	return 0xae;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
	
	while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

	id = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	return id;
	
}

