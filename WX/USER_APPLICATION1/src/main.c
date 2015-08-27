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

#include "uv_sensor.h"

static bool isLED = false;
static bool isPressureSensorPresent = false;
static bool isLightSensorPresent = false;
uint8_t seconds;

static char data[128] = "";
static double temperature;
static double pressure;
static uint16_t count;
static uint16_t elevation = 155.5; // Location elevation in meters
static uint16_t rawLightData, uvIndex;

struct datetime {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t days;
	uint8_t months;
	uint16_t years;
	
	} datetime;
	
extern uint8_t getBMP_ID(void);
extern void getBMPcoefficients(void);
extern double getBMPtemp(void);
extern double getBMPpressure(uint16_t);
extern void BMPreset(void);

//extern uint8_t SI_writeHwKey(void);
extern uint8_t SI_writeI2Cbyte(uint8_t whichReg, uint8_t data);
extern uint8_t SI_readI2Cbyte(uint8_t whichReg);
extern uint16_t SI_readI2Cword(uint8_t whichReg);

extern uint8_t getSI_PartID(void);
extern uint8_t getSI_RevID(void);
extern uint8_t getSI_SeqID(void);

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
ISR(INT2_vect) {
	toggleLED();
}

// Will eventually handle the anemometer magnetic reed switch
ISR(INT3_vect) {
	
}

// Will eventually handle the AS3935
ISR(INT4_vect) {
	
}

// Will eventually handle the si1145 UV Sensor
ISR(INT5_vect) {
	toggleLED();
	SI_writeI2Cbyte(REG_IRQ_STATUS, 0x03);
	
	rawLightData = SI_readI2Cword(REG_ALS_VIS_DATA0);
	uvIndex = SI_readI2Cword(REG_AUXDAT0_UVI0);
}

// Compare ISR for 8 bit Timer 0
ISR(TIMER0_COMPA_vect) {
	
}

// Compare ISR for 16 bit Timer 1
// Will be used as an accurate seconds counter for timestamping and reading sensor data 
ISR(TIMER1_COMPA_vect) {
	
//	toggleLED();
	
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

	
//	count++;
	
//	if ( (count >= 900) && (isPressureSensorPresent) ) { //Every 15 minutes
	if (isPressureSensorPresent) {
	
		temperature = getBMPtemp();
		pressure = getBMPpressure(elevation);
			
//		memset(data, 0, 128);
//		sprintf(data, "Temperature AND Pressure @time: %d:%d:%d	%.1f	%.2f\r\n", datetime.hours, datetime.minutes, datetime.seconds, temperature, pressure);
//		sendUART0data(data, sizeof(data));
	
//		count = 0;
	}	

}

int main (void)
{
	uint8_t bmpID, si_PartID, si_RevID, si_SeqID, si_hwKey, rValue;
	uint16_t wValue;
//	board_init();

// Set up clock
//	CLKPR = 0x80;
//	CLKPR = 0x00;

// Set up Port B	
	DDRB = 0xff;	
	PORTB &= 0x7f; // Turn off LED
	
	cli();
	
//Set up external interrupts

	// INT2 for rain gauge
	// INT3 for anemometer
	EICRA = 0xa0;	// Enable falling edge interrupts on INT3 and INT2	

	// INT4 for the AS3935
	// INT5 for the UV sensor
	EICRB = 0x0a; // Enable falling edge int on INT5 and INT4
	
	// Interrupt mask
	EIMSK = 0x3c;	// Mask the above mentioned interrupts
	
	
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
	
	if ( (bmpID == 0xba) || (bmpID == 0xbb) )  {
		
		memset (data, 0, 128);
		sprintf(data, "Pressure Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isPressureSensorPresent = false;
		
	} else {
		
		memset (data, 0, 128);
		sprintf(data, "BMP ID: %4x \r\n", bmpID);
		sendUART0data(data, sizeof(data));
		isPressureSensorPresent = true;
	}

	
	si_PartID = SI_readI2Cbyte(REG_PART_ID);
	
	if ( (si_PartID == 0xaa) || (si_PartID== 0xab) )  {
		
		memset (data, 0, 128);
		sprintf(data, "Light Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isLightSensorPresent = false;	 
		
	} else {
		
		isLightSensorPresent = true;
		_delay_ms(1);
		si_RevID = SI_readI2Cbyte(REG_REV_ID);
		_delay_ms(1);
		si_SeqID = SI_readI2Cbyte(REG_SEQ_ID);

		memset (data, 0, 128);
		sprintf(data, "SI UV Sensor Part ID: %x	Part Rev: %x	Sequencer Rev: %x \r\n", si_PartID, si_RevID, si_SeqID);
		sendUART0data(data, sizeof(data));	
		
		si_hwKey= SI_writeI2Cbyte(REG_HW_KEY, HW_KEY);
		_delay_ms(10);
		memset (data, 0, 128);
		sprintf(data, "Write and Verify Si1145 HW Key: %x\r\n", si_hwKey);
		sendUART0data(data, sizeof(data));
		
// Set the UCOEF registers
		rValue = SI_writeI2Cbyte(REG_UCOEF0, 0x0);
		_delay_ms(10);
		rValue = SI_writeI2Cbyte(REG_UCOEF1, 0x02);
		_delay_ms(10);
		rValue = SI_writeI2Cbyte(REG_UCOEF2, 0x89);
		_delay_ms(10);
		rValue = SI_writeI2Cbyte(REG_UCOEF3, 0x29);
		_delay_ms(10);

//		Set the measurement rate
		rValue = SI_writeI2Cbyte(REG_MEAS_RATE0, MEAS_RATE0);
		_delay_ms(10);
		rValue = SI_writeI2Cbyte(REG_MEAS_RATE1, MEAS_RATE1);

		_delay_ms(10);
//		 Set up the interrupts
		SI_writeI2Cbyte(REG_INT_CFG, INT_OE);
		_delay_ms(10);
		SI_writeI2Cbyte(REG_IRQ_ENABLE, ALS_IE);
		_delay_ms(10);
//		Set the CHLIST parameter
		rValue = SI_writeI2Cbyte(REG_PARAM_WR, (EN_ALS_VIS | EN_UV));		

		_delay_ms(10);
		rValue = SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | CHLIST));		
				
		_delay_ms(10);
//		Start autonomous ALS loop
		rValue = SI_writeI2Cbyte(REG_COMMAND, ALS_AUTO);

	}
	
	if (isPressureSensorPresent) {
		getBMPcoefficients();
	
		temperature = getBMPtemp();	
		pressure = getBMPpressure(elevation);
		
		memset(data, 0, 128);
		sprintf(data, "Initial Temperature AND Pressure: %.1f	%.2f\r\n", temperature, pressure);
		sendUART0data(data, sizeof(data));
	}

	while(1) {
		
		_delay_ms(1000);
		
		if (isPressureSensorPresent) {
			memset(data, 0, 128);
			sprintf(data, "Temperature AND Pressure @time: %d:%d:%d	%.1f	%.2f\r\n", datetime.hours, datetime.minutes, datetime.seconds, temperature, pressure);
			sendUART0data(data, sizeof(data));
		}
		
		if (isLightSensorPresent) {
			memset(data, 0, 128);
			sprintf(data, "Ambient Light Level and Sensor Vdd @time: %d:%d:%d	%4x		%4x \r\n", datetime.hours, datetime.minutes, datetime.seconds, rawLightData, uvIndex);
			sendUART0data(data, sizeof(data));
		}
		
		
		 		
	}
}
