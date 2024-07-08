#ifndef __INKPLATE6MOTIONPINS_H__
#define __INKPLATE6MOTIONPINS_H__

// Header guard for the Arduino include
#ifdef BOARD_INKPLATE6_MOTION

// Include SPI Arduino Library, needed for SdFat Library.
#include <SPI.h>

// I2C address for Internal GPIO expander.
#define IO_EXPANDER_INTERNAL_I2C_ADDR 0x20

#define INKPLATE_WSLED_FRONT            0
#define INKPLATE_WSLED_ROTARY_ENCODER   1

// Define EPD Pins. Using direct port manipulation for higher speed.
// EPD Latch pin <-> PE6
#define LE                  GPIO_PIN_6
#define LE_SET              GPIOE->BSRR = LE;
#define LE_CLEAR            GPIOE->BSRR = (LE << 16);
#define EPD_LE_GPIO         PE6

// EPD Start Pulse Vertical pin <-> PB1
#define CKV                 GPIO_PIN_1
#define CKV_SET             GPIOB->BSRR = CKV;
#define CKV_CLEAR           GPIOB->BSRR = (CKV << 16);
#define EPD_CKV_GPIO        PB1

// EPD Start Pulse Horizontal pin <-> PD6
#define SPH                 GPIO_PIN_6
#define SPH_SET             GPIOD->BSRR = SPH;
#define SPH_CLEAR           GPIOD->BSRR = (SPH << 16);
#define EPD_SPH_GPIO        PD6

// EPD GMODE Pin <-> PG7
#define GMODE               GPIO_PIN_7
#define GMODE_SET           GPIOG->BSRR = GMODE;
#define GMODE_CLEAR         GPIOG->BSRR = (GMODE << 16);
#define EPD_GMODE_GPIO      PG7 

// EPD Output Enable Pin <-> PB7
#define OE                  GPIO_PIN_7
#define OE_SET              GPIOB->BSRR = OE;
#define OE_CLEAR            GPIOB->BSRR = (OE << 16);
#define EPD_OE_GPIO         PB7

// EPD Start Pulse Horizontal Pin <-> PB7
#define SPV                 GPIO_PIN_12
#define SPV_SET             GPIOG->BSRR = SPV;
#define SPV_CLEAR           GPIOG->BSRR = (SPV << 16);
#define EPD_SPV_GPIO        PG12

#define EPD_BUF_EN          GPIO_PIN_2
#define EPD_BUF_SET         GPIOB->BSRR = EPD_BUF_EN
#define EPD_BUF_CLEAR       GPIOB->BSRR = (EPD_BUF_EN <<16)
#define EPD_BUFF_PIN        PB2

#define TPS_WAKE_PIN        3
#define TPS_PWRUP_PIN       4
#define TPS_VCOM_CTRL_PIN   5

// GPIO pins for TPS65185/TPS65186 EPD PMIC.
#define TPS_SDA_GPIO        PB9
#define TPS_SCL_GPIO        PB8

// External Static RAM power Supply (to save power in deep sleep).
#define RAM_EN_GPIO         PB0

// Battery measurement MOSFET enable GPIO pin
#define BATTERY_MEASUREMENT_EN  PE2

// Analog battery measurement GPIO pin
#define ANALOG_BATTERY_MEASUREMENT   PF10

// Magnetic position nencoder power enable pin
#define PERIPHERAL_POSITIONENC_ENABLE_PIN IO_PIN_B0

// Pins for controlling the onboard WSLEDs
#define PERIPHERAL_WSLED_DATA_PIN PG14
#define PERIPHERAL_WSLED_ENABLE_PIN IO_PIN_B2

// Pins for controlling the onboard WSLEDs
#define INKPLATE_MICROSD_SPI_MISO   PF8
#define INKPLATE_MICROSD_SPI_MOSI   PF9
#define INKPLATE_MICROSD_SPI_SCK    PF7
#define INKPLATE_MICROSD_SPI_CS     PB3
#define PERIPHERAL_SD_ENABLE_PIN    IO_PIN_B1

#endif

#endif