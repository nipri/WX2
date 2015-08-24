
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

#define BMP_SLA_W	0xee
#define BMP_SLA_R	0xef

static int32_t b5_2;

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

void twiError(uint8_t);
uint8_t getBMP_ID(void);
void getBMPcoefficients(void);
double getBMPtemp(void);
void BMPreset(void);

static uint8_t oss = 3; //Need to change this along with the 2 oss bits in the control register

double getBMPpressure(uint16_t);

void twiError(uint8_t code) {
	
//	memset(data, 0, 128);
//	sprintf(data, "TWI Error Code %d\r\n", code);
//	sendUART0data(data, sizeof(data));
}

uint8_t getBMP_ID(void) {
	
	uint8_t id;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );		// Wait for Start to be transmitted
	
	if ( (TWSR & 0xf8) != START)
	return 0xba;
	
	TWDR = BMP_SLA_W;						// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			// Transmit the address and wait
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
	return 0xbb;
	
	TWDR = 0xd0;							// Load and send address of the BMP ID register which should contain 0x55
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)		// Look for slave ACK
	return 0xbc;
	
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);		// send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = BMP_SLA_R;							// Send the slave module address + read bit and wait
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
	return 0xbd;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);		// send a RESTART and return a NACK
	
	while (!(TWCR & (1<<TWINT))); // Wait for slave to return a byte

	id = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
	return id;
	
}

void getBMPcoefficients(void) {
	
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
			bmp.ac1 = coeffData;
			break;
			
			case 0xac:
			bmp.ac2 = coeffData;
			break;
			
			case 0xae:
			bmp.ac3 = coeffData;
			break;
			
			case 0xb0:
			bmp.ac4 = coeffData;
			break;
			
			case 0xb2:
			bmp.ac5 = coeffData;
			break;
			
			case 0xb4:
			bmp.ac6 = coeffData;
			break;
			
			case 0xb6:
			bmp.b1 = coeffData;
			break;
			
			case 0xb8:
			bmp.b2 = coeffData;
			break;
			
			case 0xba:
			bmp.mb = coeffData;
			break;
			
			case 0xbc:
			bmp.mc = coeffData;
			break;
			
			case 0xbe:
			bmp.md = coeffData;
			break;
			
			default:
			break;
		}
		
	}
}


double getBMPtemp(void) {
	
	uint8_t MSB, LSB;
	uint32_t rawTempData;
	double calcTemp;
	
	int32_t x1 = 0;
	int32_t x2 = 0;
	int32_t b5 = 0;
	int16_t t = 0;
	
// Get raw temperature
	
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
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop	
	
	
	_delay_ms(10);
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		// send Start
	while ( !(TWCR & (1<<TWINT) ) );			// Wait for Start to be transmitted
	
	TWDR = BMP_SLA_W;							// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);				// Transmit the address
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)				// Look for slave ACK
	twiError(6);
	
	TWDR = 0xf6;								// Send the address of the AD MSB
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
	twiError(8);
	
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

// Calculate the temperature using the coefficients
	x1=(rawTempData - bmp.ac6) * bmp.ac5 /pow(2, 15);
	x2 = bmp.mc * pow(2, 11)/(x1 + bmp.md);
	b5 = x1 + x2;

// Save this to a global for use in calculating the pressure	
	b5_2 = b5;
	
	t = (b5 + 8)/pow(2, 4);

// Returns temperature in deg.C
	calcTemp = (double)t / 10;	
	return calcTemp;
}

double getBMPpressure(uint16_t height) {
	
	uint32_t MSB2, LSB2, XLSB;
	uint32_t rawPressureData;
	double a, pressure, inHg;
	
	int32_t b3, b6;
	uint32_t b4, b7;
	int32_t x1, x2, x3;
	int32_t p;	
	
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
	
//	TWDR = 0x34;								// Send the control register data
	TWDR = 0xf4;								// Send the control register data
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
	twiError(8);
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop	

//	_delay_ms(10);	
	_delay_ms(100);
	
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		// send Start
	while ( !(TWCR & (1<<TWINT) ) );			// Wait for Start to be transmitted	
	
	TWDR = BMP_SLA_W;							// Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);				// Transmit the address
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != SL_ACK)				// Look for slave ACK
	twiError(6);
	
	TWDR = 0xf6;								// Send the address of the AD MSB
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
	twiError(8);
	
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
	
	TWDR = BMP_SLA_R;							// Send the slave module address + read bit
	TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
	while (!(TWCR & (1<<TWINT)));
	
	if ( (TWSR & 0xf8) != MR_SLA_ACK)			// Look for slave ACK
	twiError(8);

	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);   // Send a RESTART and an ACK after MSB is received
	while (!(TWCR & (1<<TWINT)));
	
	MSB2 = TWDR;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (1<<TWEA) | (1<<TWEN);   // Send a RESTART and an ACK after LSB is received
	while (!(TWCR & (1<<TWINT)));
	
	LSB2 = TWDR;
	
	TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);   // Send a RESTART and a NACK after LSB2 is received
	while (!(TWCR & (1<<TWINT)));

	XLSB = TWDR;
//	LSB2 = TWDR;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
//	rawPressureData = ( (MSB2 << 8) & 0xff00) | LSB2;
	rawPressureData = (((MSB2 << 16) & 0xFF0000) + ((LSB2 << 8) & 0xFF00) + (XLSB & 0xFF)) >> (8 - oss);


// Calculate the absolute pressure 
	b6 = b5_2 - 4000;
	x1 = (bmp.b2 * (b6 * (b6/pow(2,12)) ) ) / pow(2, 11);
	x2 = bmp.ac2 * (b6 / pow(2, 11));
	x3 = x1 + x2;
	b3 = ( ( (bmp.ac1 * 4 + x3) << oss) + 2) / 4;
	x1 = bmp.ac3 * (b6 / pow(2, 13));
	x2 = (bmp.b1 * (b6 * (b6 / pow(2, 12)) ) ) / pow(2, 16);
	x3 = ( (x1 + x2) + 2) / pow (2,2);
	b4 = bmp.ac4 * (uint32_t)(x3 + 32768) / pow(2, 15);
	b7 = ( (uint32_t)rawPressureData - b3) * (50000 >> oss);
	
	if (b7 < 0x80000000) {
		p = (b7 *2) / b4;
		} else {
		p = (b7 / b4) * 2;
	}
	
	x1 = (p / pow(2,8)) * (p / pow(2,8));
	x1 = (x1 * 3038) / pow(2, 16);
	x2 = (-7357 * p) / pow(2, 16);
	p = p + (x1 + x2 + 3791) / pow(2, 4);


// Calculate pressure @ sea level & return this
	a = 1 - (double)height/44330;
	a = pow(a, 5.255);
		
	pressure = p / a;
	inHg = pressure * 0.0002953; // 1 inHg = 0.0002953 Pa
	
	return inHg;	
}

void BMPreset(void) {
	
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		// send Start
		while ( !(TWCR & (1<<TWINT) ) );			// Wait for Start to be transmitted
		
		if ( (TWSR & 0xf8) != START)
		twiError(5);
		
		TWDR = BMP_SLA_W;							// Send the slave module address + write bit
		TWCR = (1<<TWINT) | (1<<TWEN);				// Transmit the address
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != SL_ACK)				// Look for slave ACK
		twiError(6);
		
		TWDR = 0xe0;								// Send the control register address
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
		twiError(7);
		
		TWDR = 0xb6;								// Send the control register data
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)			// Look for slave ACK
		twiError(8);
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop	
}