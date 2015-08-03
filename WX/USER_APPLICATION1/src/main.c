/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to board_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */


#define F_CPU	16000000UL
#define FOSC	1843200UL
#define BAUD	9600					// This has to be defined before including setbaud.h
#define MYUBRR	(F_CPU / 16 / BAUD-1)
#define START	0x08
#define RESTART	0x10
#define SL_ACK	0x18
#define MT_DATA_ACK 0x28
#define MR_SLA_ACK	0x40



#include <asf.h>
#include <avr/io.h>
#include <avr/delay.h>
#include <util/setbaud.h>
#include <string.h>
#include <stdio.h>

bool isLED = false;
uint8_t seconds;

uint8_t BMP_SLA_W = 0xee;
uint8_t BMP_SLA_R = 0xef;

char data[128] = "";

struct datetime {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t days;
	uint8_t months;
	uint16_t years;
	
	} datetime;

struct bmpcal {
	uint16_t ac1;
	uint16_t ac2;
	uint16_t ac3;
	uint16_t ac4;
	uint16_t ac5;
	uint16_t ac6;
	uint16_t b1;
	uint16_t b2;
	uint16_t mb;
	uint16_t mc;
	uint16_t md;
	
	} bmp;
	
inline void twiError(uint8_t);
inline void getBMP_ID(void);
inline void getBMPcoefficients(void);
inline void init_USART0(uint8_t);
inline void sendUART0data(char strData[], uint8_t size);
inline void toggleLED(void);


inline void twiError(uint8_t code) {
	
	memset(data, 0, 128);
	sprintf(data, "TWI Error Code %d\r\n", code);
	sendUART0data(data, sizeof(data));
}

inline void getBMP_ID(void) {
	
	uint8_t MSB, LSB;
	uint16_t bmpid;
	
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
	while ( !(TWCR & (1<<TWINT) ) );						// Wait for Start to be transmitted
			
	if ( (TWSR & 0xf8) != START)
	twiError(0);
			
	TWDR = BMP_SLA_W;							 // Send the slave module address + write bit
	TWCR = (1<<TWINT) | (1<<TWEN);			 // Transmit the address
	while (!(TWCR & (1<<TWINT)));
		
	if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
		twiError(1);
			
	TWDR = 0xd0;							 // Address of the BMP ID register which should contain 0x55
	TWCR = (1<<TWINT) | (1<<TWEN );
	while (!(TWCR & (1<<TWINT)));
			
	if ( (TWSR & 0xf8) != MT_DATA_ACK)		 // Look for slave ACK
		twiError(2);
			
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send Repeated Start and wait...
	while (!(TWCR & (1<<TWINT)));
			
		TWDR = BMP_SLA_R;						 // Send the slave module address + read bit
		//		TWCR = (1<<TWINT) | (1<<TWEN);			 // Transmit the address
		TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
		twiError(4);

		//		while ( ( (TWSR & 0xf8) != 0x50) && ( (TWSR & 0xf8) != 0x58) );			// Wait for 1st byte (MSB) from slave
		while (!(TWCR & (1<<TWINT)));
		
		MSB = TWDR;
		
		TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);
		
		//		while ( ( (TWSR & 0xf8) != 0x50) && ( (TWSR & 0xf8) != 0x58) );		// Wait for 2nd byte (LSB) from slave
		while (!(TWCR & (1<<TWINT)));

		LSB = TWDR;
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
	
		bmpid = (MSB << 8) | LSB;
		memset (data, 0, 128);
		sprintf(data, "BMP ID: %4x \r\n", bmpid);
		sendUART0data(data, sizeof(data));
	
}

inline void getBMPcoefficients(void) {
	
	uint8_t MSB, LSB, i;
	uint16_t coeffData;
	
	for (i = 0xaa; i <= 0xbe; i += 2) {
		
		TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);	// send Start
		while ( !(TWCR & (1<<TWINT) ) );						// Wait for Start to be transmitted 
	
		if ( (TWSR & 0xf8) != START)
			twiError(0);
		
		TWDR = BMP_SLA_W;							 // Send the slave module address + write bit
		TWCR = (1<<TWINT) | (1<<TWEN);			 // Transmit the address
		while (!(TWCR & (1<<TWINT)));
	
		if ( (TWSR & 0xf8) != SL_ACK)			// Look for slave ACK
			twiError(1);							
		
		TWDR = i;							 // Send the EEPROM coefficient address to read
		TWCR = (1<<TWINT) | (1<<TWEN );
		while (!(TWCR & (1<<TWINT)));
		
		if ( (TWSR & 0xf8) != MT_DATA_ACK)		 // Look for slave ACK
			twiError(2);	
				
		TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); // send Repeated Start and wait...
		while (!(TWCR & (1<<TWINT)));	
	
//		if ( (TWSR & 0xf8) != RESTART)
//			twiError(3);		
	
		TWDR = BMP_SLA_R;						 // Send the slave module address + read bit
//		TWCR = (1<<TWINT) | (1<<TWEN);			 // Transmit the address	
		TWCR = (1<<TWINT) | (0<<TWSTA) | (0<<TWSTO) | (1<<TWEA) | (1<<TWEN);				
		while (!(TWCR & (1<<TWINT)));	
		
		if ( (TWSR & 0xf8) != MR_SLA_ACK)			 // Look for slave ACK
			twiError(4);

//		while ( ( (TWSR & 0xf8) != 0x50) && ( (TWSR & 0xf8) != 0x58) );			// Wait for 1st byte (MSB) from slave
		while (!(TWCR & (1<<TWINT)));	
	
		MSB = TWDR;
	
		TWCR = (0<<TWSTA) | (0<<TWSTO) | (1<<TWINT) | (0<<TWEA) | (1<<TWEN);
	
//		while ( ( (TWSR & 0xf8) != 0x50) && ( (TWSR & 0xf8) != 0x58) );		// Wait for 2nd byte (LSB) from slave
		while (!(TWCR & (1<<TWINT)));

		LSB = TWDR;
		
		TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);  // Send a Stop
		
		
		coeffData = (MSB << 8) | LSB;
		memset (data, 0, 128);
		sprintf(data, "Coefficient at %2x: %4x \r\n", i, coeffData);
		sendUART0data(data, sizeof(data));
		
	
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



inline void init_USART0(uint8_t ubrr){
	
	UBRR0H = (ubrr >> 8);
	UBRR0L = ubrr;
	
	//enable RX and TX
	UCSR0B = 0x18;
	
	//set frame parameters
	UCSR0C = 0x06;
	
}


inline void sendUART0data(char strData[], uint8_t size) {
	
	int i;
	
	for (i=0; i<=size; i++) {
	
		while( !(UCSR0A & 0x20) ); // Wait for TX buffer to be ready
		UDR0 = data[i];
	}	
}

inline void toggleLED(void) {
		
	if (isLED) {
		isLED = false;
		PORTB |= 0x80;
		} else {
		isLED = true;
		PORTB &= 0x7f;
	}
}

// Will eventually handle the rain gauge
ISR(INT0_vect) {
	toggleLED();
}

// Will eventually handle the anemometer magnetic reed switch
ISR(INT1_vect) {
	
}

// Will eventually handle the AS3935
ISR(INT2_vect) {
	
}

// Compare ISR for 8 bit Timer 0
ISR(TIMER0_COMPA_vect) {
	
}

// Compare ISR for 16 bit Timer 1
// Will be used as an accurate seconds counter for timestamping and reading sensor data 
ISR(TIMER1_COMPA_vect) {
	
	toggleLED();
	
	datetime.seconds++;
	
	if (datetime.seconds > 59) {
		datetime.seconds = 0;
		datetime.minutes++;
	
		if (datetime.minutes > 59) {
			datetime.seconds = 0;
			datetime.minutes = 0;
			datetime.hours++;
			
			if (datetime.hours > 23) {
				datetime.seconds = 0;
				datetime.minutes = 0;
				datetime.hours = 0;
			}	
		}
	}
	
//	memset(data, 0, 128);
//	sprintf(data, "Hello world: %d:%d:%d\r\n", datetime.hours, datetime.minutes, datetime.seconds);
//	sendUART0data(data, sizeof(data));
}

int main (void)
{
//	board_init();

// Set up clock
//	CLKPR = 0x80;
//	CLKPR = 0x00;

// Set up Port B	
	DDRB = 0xff;	
	PORTB &= 0x7f; // Turn off LED
	
	cli();
	
//Set up external interrupts

	// INT0 for rain gauge
	EICRA = 0x02;	// Enable falling edge interrupt on INT0 (pin 43 or pin 21 on Arduino header)
	EIMSK = 0x01;	// Mask INT0 
	
	// INT1 for anemometer

	// INT2 for the AS3935
	
// Set up the TWI
	TWSR = (TWPS1 << 0) | (TWPS0 << 0);
	TWBR = 0x06;

// Set up TC0
	TCCR0A = 0x0;
	TCCR0B = 0x0;
	TCNT0 = 0x0;
	OCR0A = 0x80;
	OCR0B = 0x0;
	TCCR0A |= (WGM01 << 1);
	TCCR0B  = 0x00;	// Set prescaler value to 0 to disable timer
	TIMSK0 |= (OCIE0A << 1);
	
// Set up TC1
	TCCR1A = 0x0;
	TCCR1B = 0x0;
	TCCR1C = 0x0;
	TCNT1 = 0x0;
	OCR1A = 0x3d09; // with 16 MKz clk / 1024 prescaler... gives a 1 sec. compare period
	TCCR1A = 0x0;
	TCCR1B  = 0x0D; // Set prescaler value to 0 to disable timer
	TCCR1C = 0x80;
	TIMSK1 |= (OCIE1A << 1);
	
// Set up UART0
	init_USART0(MYUBRR);

	
	sei(); // May want to move this down later on
	
	getBMP_ID();
	getBMPcoefficients();

	
	while(1) {
		
//		_delay_ms(1000);
//		memset(data, 0, 128);
//		sprintf(data, "Hello world: %d:%d:%d\r\n", datetime.hours, datetime.minutes, datetime.seconds);
//		sendUART0data(data, sizeof(data));
		 		
	}
}
