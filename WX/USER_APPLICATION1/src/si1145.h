// I2C Registers

#define PART_ID_ADDR		0x0
#define REV_ID_ADDR			0x01
#define SEQ_ID_ADDR			0x02
#define INT_CFG_ADDR		0x03
#define IRQ_ENABLE_ADDR		0x04
#define HW_KEY_ADDR			0x07
#define MEAS_RATE0_ADDR		0x08
#define MEAS_RATE1_ADDR		0x09
#define PS_RATE_ADDR		0x0a
#define PS_LED21_ADDR		0x0f
#define PS_LED3_ADDR		0x10

#define UCOEF0_ADDR			0x13
#define UCOEF1_ADDR			0x14
#define UCOEF2_ADDR			0x15
#define UCOEF3_ADDR			0x16	
#define PARAM_WR_ADDR		0x17
#define COMMAND_ADDR		0x18
#define RESPONSE_ADDR		0x20
#define IRQ_STATUS_ADDR		0x21
#define ALS_VIS_DATA0_ADDR	0x22
#define ALS_VIS_DATA1_ADDR	0x23
#define ALS_IR_DATA0_ADDR	0x24
#define ALS_IR_DATA1_ADDR	0x25
#define PS1_DATA0_ADDR		0x26
#define PS1_DATA1_ADDR		0x27
#define PS2_DATA0_ADDR		0x28
#define PS2_DATA1_ADDR		0x29
#define PS3_DATA0_ADDR		0x2a
#define PS3_DATA1_ADDR		0x2b
#define AUX_D0_UVI0_ADDR	0x2c
#define AUX_D1_UVI1_ADDR	0x2d
#define PARAM_RD_ADDR		0x2e
#define CHIP_STAT_ADDR		0x30
#define ANA_IN_KEY_ADDR0	0x3b	// Need to do a burst R/W from 0x3b to 0x3e... this is a uint32_t


