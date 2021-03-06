/**
* main.c: Main file for the WX project
* 
* REVISION HISTORY:
*
* pp.10		9/24/2015		NCI
*
*
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
bool isRainGauge = false;
static bool isPressureSensorPresent = false;
static bool isLightSensorPresent = false;
static bool isTHSensorPresent = false;
static volatile uint8_t rxPacket[16], byteDelayCount;
static volatile uint8_t rxByteCount;
static volatile double rainFall;

static char data[128] = "";
static char lcdStr[16] = "";
static char verSering[] = "pp.10";
static char pressTrend[] = "---";


static double temperature, THtemperature, tempF;
static double RH, dewPoint, dewPointF;
static double pressure, lastPressure, inHg;
static uint16_t elevation = 155.5; // Location elevation in meters
static uint16_t rawLightData, uvIndex;
static int uvIndex2;
static uint16_t count, longCount;
static uint8_t lcdCount, lcdLineCount;
uint8_t seconds;

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

extern uint8_t crc8(volatile uint8_t data[], uint8_t length);
extern void init_crc8(void);

extern uint8_t SI_writeI2Cbyte(uint8_t whichReg, uint8_t data);
extern uint8_t SI_readI2Cbyte(uint8_t whichReg);
extern uint16_t SI_readI2Cword(uint8_t whichReg);

extern uint8_t HTU_readI2Cbyte(uint8_t whichReg);
extern uint8_t HTU_writeI2Cbyte(uint8_t whichReg, uint8_t data);
extern double HTU_getData(uint8_t command);

void writeLCD(char strData[]);

void init_USART0(uint8_t);
void sendUART0data(char strData[], uint8_t size);
void toggleLED(void);
void flashLED2(uint8_t);
void getTimePacket(uint8_t);
void getElevationPacket(uint8_t);
void getADCRangeSetPacket(uint8_t);
double calcDewPoint(double, double);
void calcPressureTendancy(double);
void printData(void);
void printLCD(void);




void writeLCD(char strData[]) {
	
	lcd_clrscr();
	lcd_puts(strData);	
	
}

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

//	while( (byteDelayCount < 2) && (rxByteCount < byteCount-1) );
//	while(rxByteCount < byteCount);
	
//	rxByteCount = 0;
				
	if (byteDelayCount >= 2) {
		sprintf(data, "Timeout!\r\n");
		sendUART0data(data, 10);
	} else {	
		crc = crc8(rxPacket, byteCount-2);
		
		if (crc != rxPacket[4]) {
//			sprintf(data, "Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[4], crc);
//			sendUART0data(data, 32);
		} else {
//			sprintf(data, "Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[4], crc);
//			sendUART0data(data, 32);
			
			datetime.hours = rxPacket[1];
			datetime.minutes = rxPacket[2];
			datetime.seconds = rxPacket[3];
			
		}
	}	
}

void getElevationPacket(uint8_t byteCount) {
	
	uint8_t crc;
	
	byteDelayCount = 0;
	
//	while( (byteDelayCount < 2) && (rxByteCount < byteCount-1) );
//	while(rxByteCount < byteCount);
	
//	rxByteCount = 0;
	
	if (byteDelayCount >= 2) {
		sprintf(data, "Timeout!\r\n");
		sendUART0data(data, 10);
	} else {
		crc = crc8(rxPacket, byteCount-2);
		
		if (crc != rxPacket[3]) {
//			sprintf(data, "Elevation Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[3], crc);
//			sendUART0data(data, 64);
		} else {
//			sprintf(data, "Elevation Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[3], crc);
//			sendUART0data(data, 64);
			
			elevation = (rxPacket[1] << 8) + rxPacket[2];
			
		}
	}	
}

void getADCRangeSetPacket(uint8_t byteCount) {
	
	uint8_t crc;
	
	byteDelayCount = 0;
	
//	while( (byteDelayCount < 2) && (rxByteCount < byteCount-1) );
//	while(rxByteCount < byteCount);
	
//	rxByteCount = 0;
	
	if (byteDelayCount >= 2) {
		sprintf(data, "Timeout!\r\n");
		sendUART0data(data, 10);
		} else {
		crc = crc8(rxPacket, byteCount-2);
		
		if (crc != rxPacket[2]) {
//			sprintf(data, "ADCRange Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[2], crc);
//			sendUART0data(data, 64);
		} else {
//			sprintf(data, "ADCrange Packet CRC: %x	Calc CRC: %x\r\n", rxPacket[2], crc);
//			sendUART0data(data, 64);
			
			SI_writeI2Cbyte(REG_COMMAND, ALS_PAUSE);
			_delay_ms(10);
			
			if (rxPacket[1] == 1)
				SI_writeI2Cbyte(REG_PARAM_WR, VIS_RANGE_HI);
			else
				SI_writeI2Cbyte(REG_PARAM_WR, VIS_RANGE_NORMAL);
				
			_delay_ms(10);
			SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | ALS_VIS_ADC_MISC));
			
			if (rxPacket[2] == 1)
			SI_writeI2Cbyte(REG_PARAM_WR, IR_RANGE_HI);
			else
			SI_writeI2Cbyte(REG_PARAM_WR, IR_RANGE_NORMAL);
			
			_delay_ms(10);
			SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | ALS_IR_ADC_MISC));
				
			_delay_ms(10);
			//		Restart autonomous ALS loop
			SI_writeI2Cbyte(REG_COMMAND, ALS_AUTO);		
		}
	}	
}

// Taken from the HTU21D datasheet
double calcDewPoint(double temp, double RH) {
	
	const double A = 8.1332;
	const double B = 1762.39;
	const double C = 235.66;
	
	double dp, pp, temp1, temp2;
	
	temp1 = A - (B / (temp + C));
	
	pp = pow(10, temp1);
	
	temp2 = log10(RH * (pp/100)) - A;
	
	dp = (B / temp2) + C;
	dp = -dp;
	return dp;
}

// Values are in mb and are taken from http://www.islandnet.com/~see/weather/eyes/barometer3.htm
void calcPressureTendancy(double currentPressure) {
	
	sprintf(data, "\r\nLast Pressure: %.1f Current Pressure: %.1f\r\n", lastPressure, currentPressure);
	sendUART0data(data, 64);
	
	if ( ( (currentPressure - lastPressure) <= 0.1) &&  ( (currentPressure - lastPressure) >= -0.1 ) ) 
		sprintf(pressTrend, "STE");
		
	else if ( ( (currentPressure - lastPressure) > 0.1)  && ( (currentPressure - lastPressure) < 1.5) ) 
		sprintf(pressTrend, "RSL");
		
	else if ( ( (currentPressure - lastPressure) > 1.6)  && ( (currentPressure - lastPressure) < 3.5) ) 
		sprintf(pressTrend, "RIS");
		
	else if ( ( (currentPressure - lastPressure) > 3.6)  && ( (currentPressure - lastPressure) < 6.0) ) 
		sprintf(pressTrend, "RFA");

	else if ( ( (currentPressure - lastPressure) > 6.0) )
		sprintf(pressTrend, "RVF");
		
	else if ( ( (lastPressure - currentPressure) > 0.1)  && ( (lastPressure - currentPressure) < 1.5 ) )
		sprintf(pressTrend, "FSL");
	
	else if ( ( (lastPressure - currentPressure) >  1.6)  && ( (lastPressure - currentPressure) < 3.5 ) )
		sprintf(pressTrend, "FAL");
	
	else if ( ( (lastPressure - currentPressure) >  + 3.6)  && ( (lastPressure - currentPressure)  < 6.0) )
		sprintf(pressTrend, "FFA");

	else if ( (lastPressure - currentPressure) > 6.0)
		sprintf(pressTrend, "FVF");	
}

void printData(void) {
	
	memset(data, 0, 128);
//	sprintf(data, "\r\n%s	%d:%d:%d	%.1f	%.2f	%s	%u	%d	%.1f	%.1f	%.1f\r\n", verSering, datetime.hours, datetime.minutes, datetime.seconds, temperature, inHg, pressTrend, rawLightData, uvIndex2, THtemperature, RH, dewPoint);
	sprintf(data, "\r\n%s	%d:%d:%d	%.1f	%.1f	%s	%u	%d	%.1f	%.1f	%.1f	%.1f\r\n", verSering, datetime.hours, datetime.minutes, datetime.seconds, temperature, pressure, pressTrend, rawLightData, uvIndex2, THtemperature, RH, dewPoint, rainFall);
	sendUART0data(data, sizeof(data));	
}

void printLCD(void){
	
	if (isPressureSensorPresent) {
			
		if ( (lcdLineCount >= 0) && (lcdLineCount <= 4) ) {
//			sprintf(lcdStr, "Temp Pressure\n%.1f C  %.2f in", temperature, inHg);
			sprintf(lcdStr, "Temperature\n%.1f C  %.1f F", temperature, tempF);
			writeLCD(lcdStr);
		}
		
		if ( (lcdLineCount > 4) && (lcdLineCount <= 9) ) {
			sprintf(lcdStr, "Pressure  %s\n%.2f in %.1f mb", pressTrend, inHg, pressure);
			writeLCD(lcdStr);			
			
		}
	}
		
	if (isLightSensorPresent) {
			
		if ( (lcdLineCount > 9) && (lcdLineCount <= 14) ) {
			sprintf(lcdStr, "Amb. Light\n%u lux", rawLightData);
			writeLCD(lcdStr);
		}
		
		if ( (lcdLineCount > 14) && (lcdLineCount <= 19) ) {
			sprintf(lcdStr, "UV Index\n%d", uvIndex2);
			writeLCD(lcdStr);
		}
	}
		
	if (isTHSensorPresent) {
			
		if ( (lcdLineCount > 19) && (lcdLineCount <= 24) ) {
			sprintf(lcdStr, "Rel. Humidity\n%.1f %%", RH);
			writeLCD(lcdStr);
		}
		
		if ( (lcdLineCount > 24) && (lcdLineCount <= 29) ) {
			sprintf(lcdStr, "Dew Point\n%.1f C  %.1f F", dewPoint, dewPointF);
			writeLCD(lcdStr);
		}
	}
	
	if ( (isPressureSensorPresent) && (!isLightSensorPresent) && (!isTHSensorPresent) ) {
		if (lcdLineCount++ > 9)
		lcdLineCount = 0;	
	}
	
	else if ( (!isPressureSensorPresent) && (isLightSensorPresent) && (!isTHSensorPresent) ) {
		if (lcdLineCount++ > 19)
		lcdLineCount = 10;		
	}	
	
	else if ( (!isPressureSensorPresent) && (!isLightSensorPresent) && (isTHSensorPresent) ) {
		if (lcdLineCount++ > 29)
		lcdLineCount = 20;		
	}
	
	else if ( (isPressureSensorPresent) && (isLightSensorPresent) && (isTHSensorPresent) ) {
		if (lcdLineCount++ > 29)
		lcdLineCount = 0;		
	}
	
	else {
		if (lcdLineCount++ > 29)
		lcdLineCount = 0;
	}
}


// Will eventually handle the rain gauge... Pin 9 on the Arduino XMEGA2560 board
ISR(INT2_vect) {

	isRainGauge = true;
	seconds = 0;	
}

// Will eventually handle the anemometer magnetic reed switch
ISR(INT3_vect) {
	
}

// Will eventually handle the AS3935
ISR(INT4_vect) {
	
}

// Handles the si1145 UV Sensor... Pin 3 on the Arduino XMEGA2560 board
ISR(INT5_vect) {
	uint8_t rValue;
	toggleLED();
	
	rValue = SI_readI2Cbyte(REG_IRQ_STATUS);
	SI_writeI2Cbyte(REG_IRQ_STATUS, (rValue & 0x03));
	
	rawLightData = SI_readI2Cword(REG_ALS_VIS_DATA0);
	uvIndex = SI_readI2Cword(REG_AUXDAT0_UVI0);
	
	uvIndex2 = round((double)uvIndex / 100);

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
	seconds++;
	
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
	
//	byteDelayCount ++;
	
	if (isPressureSensorPresent) {
		temperature = getBMPtemp();
		pressure = getBMPpressure(elevation); // pressure in hPA = mb
		inHg = pressure * 0.02953; // 1 inHg = 0.02953 hPa
		tempF = (temperature*1.8) + 32;
		
		if (longCount == 0)
			lastPressure = pressure;
	}
		
	if (isTHSensorPresent) {
			
		THtemperature = HTU_getData(0xe3);
		RH = HTU_getData(0xe5);
		dewPoint = calcDewPoint(THtemperature, RH);
		dewPointF = (dewPoint * 1.8) + 32;
	}
	
	count++;
	longCount++;
	lcdCount++;
}


ISR(USART0_RX_vect) {
	
	rxPacket[rxByteCount] = UDR0;	
	rxByteCount++;
}



int main (void)
{
	uint8_t bmpID, si_PartID, si_RevID, si_SeqID, HTU_UserReg;
	uint8_t rValue;
//	board_init();

// Set up clock
//	CLKPR = 0x80;
//	CLKPR = 0x00;

// Set up Port A (LCD)
	DDRA = 0xff;
	PORTA = 0x00;

// Set up Port B	
	DDRB = 0xff;	
	PORTB &= 0x7f; // Turn off LED
	
	cli();
	
// Build the CRC table
	init_crc8();
	
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
	
// Set up and test the LCD 
	lcd_init(LCD_DISP_ON_CURSOR_BLINK);
	writeLCD("HELLO");
//	lcd_putc('C');

	
	sei(); // May want to move this down later on

// Check for the pressure sensor
	bmpID = getBMP_ID();
	
	if ( (bmpID == 0xba) || (bmpID == 0xbb) )  {
		
		memset (data, 0, 128);
		sprintf(data, "\r\nPressure Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isPressureSensorPresent = false;
		
	} else {
		
		memset (data, 0, 128);
		sprintf(data, "\r\nBMP ID: %4x \r\n", bmpID);
		sendUART0data(data, sizeof(data));
		getBMPcoefficients();
		isPressureSensorPresent = true;
	}

// check for the UV sensor and configure if it's present	
	si_PartID = SI_readI2Cbyte(REG_PART_ID);
	
	if ( (si_PartID == 0xaa) || (si_PartID== 0xab) )  {
		
		memset (data, 0, 128);
		sprintf(data, "\r\nLight Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isLightSensorPresent = false;	 
		
	} else {
		
		isLightSensorPresent = true;
		si_RevID = SI_readI2Cbyte(REG_REV_ID);
		si_SeqID = SI_readI2Cbyte(REG_SEQ_ID);

		memset (data, 0, 128);
		sprintf(data, "\r\nSI UV Sensor Part ID: %x	Part Rev: %x	Sequencer Rev: %x \r\n", si_PartID, si_RevID, si_SeqID);
		sendUART0data(data, sizeof(data));	

// Write the hardware key to unlock the sensor		
		SI_writeI2Cbyte(REG_HW_KEY, HW_KEY);
		
// Set the UCOEF registers
		// Calibration values from datasheet
//		SI_writeI2Cbyte(REG_UCOEF0, 0x0);	
//		SI_writeI2Cbyte(REG_UCOEF1, 0x02);	
//		SI_writeI2Cbyte(REG_UCOEF2, 0x89);	
//		SI_writeI2Cbyte(REG_UCOEF3, 0x29);	
		
		SI_writeI2Cbyte(REG_UCOEF0, 0x29);
		SI_writeI2Cbyte(REG_UCOEF1, 0x89);
		SI_writeI2Cbyte(REG_UCOEF2, 0x02);
		SI_writeI2Cbyte(REG_UCOEF3, 0x0);
		
		// Calibration values found online
//		SI_writeI2Cbyte(REG_UCOEF0, 0x7b);
//		SI_writeI2Cbyte(REG_UCOEF1, 0x6b);
//		SI_writeI2Cbyte(REG_UCOEF2, 0x01);
//		SI_writeI2Cbyte(REG_UCOEF3, 0x00);

//		Set the measurement rate
		SI_writeI2Cbyte(REG_MEAS_RATE0, MEAS_RATE0);	
		SI_writeI2Cbyte(REG_MEAS_RATE1, MEAS_RATE1);

//		 Set up the interrupts
		SI_writeI2Cbyte(REG_INT_CFG, INT_OE);
		SI_writeI2Cbyte(REG_IRQ_ENABLE, ALS_IE);

//		Set the CHLIST parameter
		SI_writeI2Cbyte(REG_PARAM_WR, (EN_ALS_IR | EN_ALS_VIS | EN_UV));
//		SI_writeI2Cbyte(REG_PARAM_WR, EN_UV);		
		SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | CHLIST));	
		
// Verify the last set parameter
		SI_writeI2Cbyte(REG_COMMAND, (PARAM_QUERY | CHLIST));
		_delay_ms(10);
		rValue = SI_readI2Cbyte(REG_PARAM_RD);
		sprintf(data, "rValue: %x \r\n", rValue);
		sendUART0data(data, sizeof(data));
		

		
//		For ALS measurements... set for high sensitivity or high signal range
		SI_writeI2Cbyte(REG_PARAM_WR, VIS_RANGE_HI);	
		SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | ALS_VIS_ADC_MISC));		
		SI_writeI2Cbyte(REG_PARAM_WR, IR_RANGE_HI);
		SI_writeI2Cbyte(REG_COMMAND, (PARAM_SET | ALS_IR_ADC_MISC));	
			
//		Start autonomous ALS loop
		SI_writeI2Cbyte(REG_COMMAND, ALS_AUTO);

	}

// Check for the TH sensor
	HTU_UserReg = HTU_readI2Cbyte(0xE7);
	
	if ( (HTU_UserReg == 0xaa) || (HTU_UserReg == 0xab) )  {
		
		memset (data, 0, 128);
		sprintf(data, "\r\nTH Sensor not responding\r\n");
		sendUART0data(data, sizeof(data));
		isTHSensorPresent = false;
		
	} else {
		sprintf(data, "\r\nHTU21D User Register: %x \r\n", HTU_UserReg);
		sendUART0data(data, sizeof(data));
		isTHSensorPresent = true;
		
		// Set the user register byte here
//		HTU_UserReg = HTU_writeI2Cbyte(0xe6, 0x02);	
//		sprintf(data, "HTU21D User Register Set To: %x \r\n", HTU_UserReg);
//		sendUART0data(data, sizeof(data));

	}
	
	if (isPressureSensorPresent) 
		longCount = 0;	
		
			
	rxByteCount = 0;
	rainFall = 0;	
		
	while(1) {
		
		if (count >= 5) {
			count = 0;
			printData();
		}
		
		if (lcdCount >= 1) {
			lcdCount = 0;
			printLCD();
		}
		
		if (longCount >= 10800) {
			longCount = 0;
			calcPressureTendancy(pressure);	
		}
		
		if ( (isRainGauge) && (seconds == 4) ) {
			rainFall += 0.1;
			isRainGauge = false;
		}
			
				
		if (rxByteCount > 0) {
			
			if ( (rxPacket[0] == 0xa1) && (rxByteCount == 6) ) {
				rxByteCount = 0;
				getTimePacket(6);
			}
			else if ( (rxPacket[0] == 0xa0) && (rxByteCount == 5) ) {
				rxByteCount = 0;
				getElevationPacket(5);
			}
			else if ( (rxPacket[0] == 0xa2) && (rxByteCount == 4) ) {
				rxByteCount = 0;
				getADCRangeSetPacket(5);
			}
/*
	
			switch(rxPacket[0]) {
				
				case 0xa0:		// Elevation packet
					getElevationPacket(5);
					break;
				
				case 0xa1:		// Time packet
					getTimePacket(6);
					break;
				
				case 0xa2:		// ALS ADC Range Set Packet
					getADCRangeSetPacket(4);
					break;
				
				default:
					rxByteCount = 0;
					break;
			} 	*/

		}
			 		
	}
}
