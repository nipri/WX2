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


//#define FOSC	1843200UL
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
#include "lcd.h"

static bool isLED = false;
static bool isPressureSensorPresent = false;
static bool isLightSensorPresent = false;
static bool isTHSensorPresent = false;
uint8_t rxByteCount, rxPacket[16], byteDelayCount;

static char data[128] = "";

static double temperature, THtemperature;
static double RH, dewPoint;
static double pressure;
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

extern uint8_t crc8(uint8_t data[], uint8_t length);

extern uint8_t SI_writeI2Cbyte(uint8_t whichReg, uint8_t data);
extern uint8_t SI_readI2Cbyte(uint8_t whichReg);
extern uint16_t SI_readI2Cword(uint8_t whichReg);

extern uint8_t HTU_readI2Cbyte(uint8_t whichReg);
extern uint8_t HTU_writeI2Cbyte(uint8_t whichReg, uint8_t data);
extern double HTU_getData(uint8_t command);

void init_USART0(uint8_t);
void sendUART0data(char strData[], uint8_t size);
void toggleLED(void);
void flashLED2(uint8_t);
void getTimePacket(uint8_t);
void getElevationPacket(uint8_t);
void getADCRangeSetPacket(uint8_t);

double calcDewPoint(double, double);




inline void init_USART0(uint8_t ubrr){
	
	UBRR0H = (ubrr >> 8);
	UBRR0L = ubrr;
	
	//enable RX, the RX Complete Interrupt, and TX
	UCSR0B = 0x98; // 0x98 to enable RX int
	
	//set frame parameters
	UCSR0C = 0x06;
	
}


inline void sendUART0data(char strData[], uint8_t size) {	
	int i;
	
	for (i=0; i<size; i++) {
	
		while( !(UCSR0A & 0x20) ); // Wait for TX buffer to be ready
		UDR0 = strData[i];
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

inline void flashLED2(uint8_t data) {
	
	int i;
	
	for (i=0; i<data; i++){
		
		PORTB &= 0xbf;
		_delay_ms(200);
		PORTB |= 0x40;
		_delay_ms(200);
	}
	
	PORTB &= 0x40;
}


void getTimePacket(uint8_t byteCount) {
	uint8_t crc;
	
	byteDelayCount = 0;
	
	while( (byteDelayCount < 2) && (rxByteCount < byteCount-1) );
//	while(rxByteCount < byteCount-1);
	
	rxByteCount = 0;
				
	if (byteDelayCount >= 2) {
		sprintf(data, "Timeout!\r\n");
		sendUART0data(data, 10);
	} else {	
		crc = crc8(rxPacket, byteCount-1);
		
		if (crc != rxPacket[4]) {
			sprintf(data, "Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[4], crc);
			sendUART0data(data, 32);
		} else {
			sprintf(data, "Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[4], crc);
			sendUART0data(data, 32);
			
			datetime.hours = rxPacket[1];
			datetime.minutes = rxPacket[2];
			datetime.seconds = rxPacket[3];
			
		}
	}		
}

void getElevationPacket(uint8_t byteCount) {
	
	uint8_t crc;
	
	byteDelayCount = 0;
	
	while( (byteDelayCount < 2) && (rxByteCount < byteCount-1) );
	//	while(rxByteCount < byteCount-1);
	
	rxByteCount = 0;
	
	if (byteDelayCount >= 2) {
		sprintf(data, "Timeout!\r\n");
		sendUART0data(data, 10);
		} else {
		crc = crc8(rxPacket, byteCount-1);
		
		if (crc != rxPacket[3]) {
			sprintf(data, "Elevation Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[3], crc);
			sendUART0data(data, 32);
			} else {
			sprintf(data, "Elevation Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[3], crc);
			sendUART0data(data, 32);
			
			elevation = (rxPacket[1] << 8) + rxPacket[2];
			
		}
	}	
}

void getADCRangeSetPacket(uint8_t byteCount) {
	
	uint8_t crc;
	
	byteDelayCount = 0;
	
	while( (byteDelayCount < 2) && (rxByteCount < byteCount-1) );
	//	while(rxByteCount < byteCount-1);
	
	rxByteCount = 0;
	
	if (byteDelayCount >= 2) {
		sprintf(data, "Timeout!\r\n");
		sendUART0data(data, 10);
		} else {
		crc = crc8(rxPacket, byteCount-1);
		
		if (crc != rxPacket[2]) {
			sprintf(data, "ADCRange Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[2], crc);
			sendUART0data(data, 32);
			} else {
			sprintf(data, "ADCrange Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[2], crc);
			sendUART0data(data, 32);
			
			SI_writeI2Cbyte(REG_COMMAND, ALS_PAUSE);
			_delay_ms(10);
			
			if (rxPacket[1] == 1)
				SI_writeI2Cbyte(REG_PARAM_WR, VIS_RANGE_HI);
			else
				SI_writeI2Cbyte(REG_PARAM_WR, VIS_RANGE_NORMAL);
				
			_delay_ms(10);
			SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | ALS_VIS_ADC_MISC));
				
			_delay_ms(10);
			//		Restart autonomous ALS loop
			SI_writeI2Cbyte(REG_COMMAND, ALS_AUTO);		
		}
	}	
}

// Taken from the HTU21D datasheet
double calcDewPoint(double temp, double RH) {
	
	const A = 8.1332;
	const B = 1762.39;
	const C = 235.66;
	
	double dp, pp, temp1, temp2;
	
	temp1 = A - (B / (temp + C));
	
	pp = pow(10, temp1);
	
	temp2 = log10(RH * (pp/100)) - A;
	
	dp = (B / temp2) + C;
	dp = -dp;
	return dp;
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

// Handles the si1145 UV Sensor... Pin 3 on the Arduino board
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
	
	uint16_t result;
	
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
	
	byteDelayCount ++;
//	count++;
	
//	if (count >= 900) { //Every 15 minutes
//	if (count >= 10) { //Every 10 seconds
		memset(data, 0, 128);
		sprintf(data, "\r\nTime: %d:%d:%d\r\n", datetime.hours, datetime.minutes, datetime.seconds);
		sendUART0data(data, sizeof(data));
	
		if (isPressureSensorPresent) {
			temperature = getBMPtemp();
			pressure = getBMPpressure(elevation);
			memset(data, 0, 128);
			sprintf(data, "\r\nTemperature and Pressure: %.1f	%.2f\r\n", temperature, pressure);
			sendUART0data(data, sizeof(data));
		}
		
		if (isLightSensorPresent) {
			memset(data, 0, 128);
			sprintf(data, "\r\nAmbient Light Level (lux) and UV Index: %4x		%4x \r\n", rawLightData, uvIndex);
			sendUART0data(data, sizeof(data));
		}
		
		if (isTHSensorPresent) {
			
			THtemperature = HTU_getData(0xe3);
			RH = HTU_getData(0xe5);
			memset(data, 0, 128);
			sprintf(data, "\r\nTH Temperature and %%RH: %.1f	%.1f\r\n", THtemperature, RH);
			sendUART0data(data, sizeof(data));	
			
			dewPoint = calcDewPoint(THtemperature, RH);
			memset(data, 0, 128);
			sprintf(data, "Dew Point:%.1f\r\n", dewPoint);
			sendUART0data(data, sizeof(data));			
		}
	
//		count = 0;
//	}	

}


ISR(USART0_RX_vect) {
	
	rxPacket[rxByteCount] = UDR0;	
//	sprintf(data, "RXpacket %d:	%x\r\n", rxByteCount, rxPacket[rxByteCount]);
//	sprintf(data, "UCSR0A: %x\r\n", UCSR0A);
//	sendUART0data(data, 20);
	rxByteCount++;
}



int main (void)
{
	uint8_t bmpID, si_PartID, si_RevID, si_SeqID, HTU_UserReg;
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
	TWSR = (TWPS1 << 0) | (TWPS0 << 0); // TWPS Prescaler Bits = b00
//	TWBR = 0x0b;
	TWBR = 0x01;

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
	
// Set up tke LCD 
lcd_init(LCD_DISP_ON_CURSOR_BLINK);

	
	sei(); // May want to move this down later on

// Check for the pressure sensor
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

// check for the UV sensor and configure if it's present	
	si_PartID = SI_readI2Cbyte(REG_PART_ID);
	
	if ( (si_PartID == 0xaa) || (si_PartID== 0xab) )  {
		
		memset (data, 0, 128);
		sprintf(data, "Light Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isLightSensorPresent = false;	 
		
	} else {
		
		isLightSensorPresent = true;
		si_RevID = SI_readI2Cbyte(REG_REV_ID);
		si_SeqID = SI_readI2Cbyte(REG_SEQ_ID);

		memset (data, 0, 128);
		sprintf(data, "SI UV Sensor Part ID: %x	Part Rev: %x	Sequencer Rev: %x \r\n", si_PartID, si_RevID, si_SeqID);
		sendUART0data(data, sizeof(data));	

// Write the hardware key to unlock the sensor		
		SI_writeI2Cbyte(REG_HW_KEY, HW_KEY);
		
// Set the UCOEF registers
		SI_writeI2Cbyte(REG_UCOEF0, 0x0);	
		SI_writeI2Cbyte(REG_UCOEF1, 0x02);	
		SI_writeI2Cbyte(REG_UCOEF2, 0x89);	
		SI_writeI2Cbyte(REG_UCOEF3, 0x29);	

//		Set the measurement rate
		SI_writeI2Cbyte(REG_MEAS_RATE0, MEAS_RATE0);	
		SI_writeI2Cbyte(REG_MEAS_RATE1, MEAS_RATE1);

//		 Set up the interrupts
		SI_writeI2Cbyte(REG_INT_CFG, INT_OE);
		SI_writeI2Cbyte(REG_IRQ_ENABLE, ALS_IE);

//		Set the CHLIST parameter
//		SI_writeI2Cbyte(REG_PARAM_WR, (EN_ALS_VIS | EN_UV));
		SI_writeI2Cbyte(REG_PARAM_WR, EN_ALS_VIS);		
		SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | CHLIST));		
		
//		For ALS_VIS measurements... set for high sensitivity or high signal range
		SI_writeI2Cbyte(REG_PARAM_WR, VIS_RANGE_NORMAL);	
		SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | ALS_VIS_ADC_MISC));	
		
		
			
//		Start autonomous ALS loop
		SI_writeI2Cbyte(REG_COMMAND, ALS_AUTO);

	}

// Check for the TH sensor
	HTU_UserReg = HTU_readI2Cbyte(0xE7);
	
	if ( (HTU_UserReg == 0xaa) || (HTU_UserReg == 0xab) )  {
		
		memset (data, 0, 128);
		sprintf(data, "TH Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isTHSensorPresent = false;
		
	} else {
		sprintf(data, "HTU21D User Register: %x \r\n", HTU_UserReg);
		sendUART0data(data, sizeof(data));
		isTHSensorPresent = true;
		
		// Set the user register byte here
		HTU_UserReg = HTU_writeI2Cbyte(0xe6, 0x02);	
		sprintf(data, "HTU21D User Register Set To: %x \r\n", HTU_UserReg);
		sendUART0data(data, sizeof(data));

	}
			
	
	if (isPressureSensorPresent) {
		getBMPcoefficients();
	
		temperature = getBMPtemp();	
		pressure = getBMPpressure(elevation);
		
		memset(data, 0, 128);
		sprintf(data, "Initial Temperature AND Pressure: %.1f	%.2f\r\n", temperature, pressure);
		sendUART0data(data, sizeof(data));
	}
	
	rxByteCount = 0;
		
		
	while(1) {
					
		if (rxByteCount > 0) {
			
			_delay_ms(10);
	
			switch(rxPacket[0]) {
				
				case 0xa0:		// Elevation packet
					getElevationPacket(4);
					break;
				
				case 0xa1:		// Time packet
					getTimePacket(5);
					break;
				
				case 0xa2:		// ALS ADC Range Set Packet
					getADCRangeSetPacket(3);
					break;
				
				default:
					rxByteCount = 0;
					break;
			} 	
		}			 		
	}
}
