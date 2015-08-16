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


#define FOSC	1843200UL
#define BAUD	9600					// This has to be defined before including setbaud.h
#define MYUBRR	(F_CPU / 16 / BAUD-1)

#include <asf.h>
#include <avr/io.h>
#include <avr/delay.h>
#include <util/setbaud.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <math.h>

bool isLED = false;
uint8_t seconds;

static char data[128] = "";
static double temperature;
static double pressure;
static uint16_t count;
static uint16_t elevation = 155.5; // Location elevation in meters

struct datetime {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t days;
	uint8_t months;
	uint16_t years;
	
	} datetime;
	
uint8_t getBMP_ID(void);
void getBMPcoefficients(void);
double getBMPtemp(void);
double getBMPpressure(uint16_t);
void BMPreset(void);
void init_USART0(uint8_t);
void sendUART0data(char strData[], uint8_t size);
void toggleLED(void);


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
	
	count++;
	
	if (count >= 900) { //Every 15 minutes
		
		temperature = getBMPtemp();
		pressure = getBMPpressure(elevation);
			
		memset(data, 0, 128);
		sprintf(data, "Temperature AND Pressure @time: %d:%d:%d	%.1f	%.2f\r\n", datetime.hours, datetime.minutes, datetime.seconds, temperature, pressure);
		sendUART0data(data, sizeof(data));
	
		count = 0;
	}	
}

int main (void)
{
	
	uint8_t bmpID;
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
	EICRA = 0x20;	// Enable falling edge interrupt on INT2
	EIMSK = 0x04;	// Mask INT2
	
	// INT1 for anemometer

	// INT2 for the AS3935
	
// Set up the TWI
	TWSR = (TWPS1 << 0) | (TWPS0 << 0);
	TWBR = 0x0b;

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
	
	bmpID = getBMP_ID();
	memset (data, 0, 128);
	sprintf(data, "BMP ID: %4x \r\n", bmpID);
	sendUART0data(data, sizeof(data));
	
	getBMPcoefficients();
	
	temperature = getBMPtemp();	
	pressure = getBMPpressure(elevation);
		
	memset(data, 0, 128);
	sprintf(data, "Initial Temperature AND Pressure: %.1f	%.2f\r\n", temperature, pressure);
	sendUART0data(data, sizeof(data));

	while(1) {
		 		
	}
}
