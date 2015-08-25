// HW_KEY value: need to write this to REG_HW_KEY after initialization to enable sensor operaton
#define HW_KEY					0x17		


// I2C Registers
#define REG_PART_ID				0x00
#define REG_REV_ID				0x01
#define REG_SEQ_ID				0x02
#define REG_INT_CFG				0x03
#define REG_IRQ_ENABLE			0x04
#define REG_HW_KEY				0x07
#define REG_MEAS_RATE0			0x08
#define REG_MEAS_RATE1			0x09
#define REG_PS_RATE				0x0a
#define REG_PS_LED21			0x0f
#define REG_PS_LED3				0x10
#define REG_UCOEF0				0x13
#define REG_UCOEF1				0x14	
#define REG_UCOEF2				0x15
#define REG_UCOEF3				0x16
#define REG_PARAM_WR			0x17
#define REG_COMMAND				0x18
#define REG_RESPONSE			0x20
#define REG_IRQ_STATUS			0x21
#define REG_ALS_VIS_DATA0		0x22
#define REG_ALS_VIS_DATA1		0x23
#define REG_ALS_IR_DATA0		0x24
#define REG_ALS_IR_DATA1		0x25
#define REG_PS1_DATA0			0x26
#define REG_PS1_DATA1			0x27
#define REG_PS2_DATA0			0x28
#define REG_PS2_DATA1			0x29
#define REG_PS3_DATA0			0x2a
#define REG_PS3_DATA1			0x2b
#define REG_AUXDAT0_UVI0		0x2c
#define REG_AUXDAT1_UVI1		0x2d
#define REG_PARAM_RD			0x2e
#define REG_CHIP_STAT			0x30
#define REG_ANAIN_KEY			0x3b	// ANA_IN_KEY is 4 bytes from 0x3b to 0x3e

//Commands
#define PARAM_QUERY				0x80	// Bitwise OR this with a parameter value and write to REG_COMMAND; read result from REG_PARAM_RD
#define PARAM_SET				0xc0    // Bitwise OR this with a parameter value and write to REG_COMMAND; 
#define NOP						0x00
#define RESET					0x01			
#define BUSADDR					0x02
#define PS_FORCE				0x05
#define GET_CAL					0x12
#define ALS_FORCE				0x06
#define PSALS_FORCE				0x07
#define PS_PAUSE				0x09
#define ALS_PAUSE				0x0a
#define PSALS_PAUSE				0x0b
#define PS_AUTO					0x0d		
#define ALS_AUTO				0x0e
#define PSALS_AUTO				0x0f

//Responses: errors
#define RESP_INVALID_SETTING	0x80
#define RESP_PS1_ADC_OFLOW		0x88
#define RESP_PS2_ADC_OFLOW		0x89
#define RESP_PS3_ADC_OFLOW		0x8a
#define RESP_ALS_VIS_ADC_OFLOW	0x8c
#define RESP_ALS_IR_ADC_OFLOW	0x8d
#define RESP_AUX_ADC_OFLOW		0x8e



