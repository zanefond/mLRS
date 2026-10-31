//*******************************************************
// Copyright (c) MLRS project
// GPL3
// https://www.gnu.org/licenses/gpl-3.0.de.html
//*******************************************************
// hal
//*******************************************************

/*
  ESP8285 + EBYTE E28-2G4M27SX (or E28-2G4M27S) DIY 2.4 GHz Rx module

  This is a DIY Rx module built from an ESP8285 MCU (or modified BetaFPV
  Nano 2.4G RX / generic ELRS 2.4G PA receiver board) paired with an
  EBYTE E28-2G4M27SX (SX1280/SX1281 + PA/LNA, up to 27 dBm = 500 mW).

  Pin mapping (BetaFPV Nano 2.4G RX / Generic 2400 PA layout):

    E28 (SX1280)          ESP8285
    ----------------------------------------
    NSS                   GPIO15 (IO_P15)
    SCK                   GPIO14
    MISO                  GPIO12
    MOSI                  GPIO13
    RESET                 GPIO2  (IO_P2)
    BUSY                  GPIO5  (IO_P5)
    DIO1                  GPIO4  (IO_P4)
    TX_EN                 GPIO10 (IO_P10)
    RX_EN                 GPIO9  (IO_P9)

    Serial RX (to FC TX)  GPIO3  (IO_P3)
    Serial TX (to FC RX)  GPIO1  (IO_P1)
    LED (Red)             GPIO16 (IO_P16)
    Button (Boot)         GPIO0  (IO_P0)

  CRITICAL POWER SUPPLY & HARDWARE NOTES:
  - Output Power & Current Draw:
    The Ebyte E28-2G4M27SX can output up to +27 dBm (500 mW). At full 500 mW
    telemetry transmit bursts, peak current on the 3.3V rail reaches ~580 mA!
  - 3.3V LDO Dimensioning:
    Standard commercial ELRS Nano receivers (such as BetaFPV Nano RX) use a small
    onboard 3.3V LDO typically rated for only 150 mA - 300 mA.
    * If powering directly from a flight controller via a small onboard LDO:
      DO NOT select 500 mW telemetry in the mLRS Lua script! Limit RF power to
      100 mW (20 dBm) or 250 mW (24 dBm) to prevent thermal overload and brownouts.
    * For full 500 mW (27 dBm) operation:
      Use an external high-current 3.3V buck regulator or LDO rated for >= 800 mA - 1 A
      with low-ESR ceramic decoupling capacitors (10 uF + 0.1 uF) close to the E28 VCC pins.
  - SX1280 Drive Level:
    Ebyte E28-2G4M27SX has ~27 dB front-end PA gain. The maximum recommended drive
    level from the SX1280 internal register is 0 dBm. Exceeding 0 dBm saturates
    the internal PA, generating spurious emissions and excess heat.
    This is handled automatically by defining POWER_PA_E28_2G4M27SX.
*/

#define DEVICE_HAS_SINGLE_LED
#define DEVICE_HAS_NO_DEBUG
//#define DEVICE_HAS_NO_SERIAL


//-- UARTS
// UARTB = serial port (MAVLink / CRSF to Flight Controller)
// UARTF = debug port

#define UARTB_USE_SERIAL
#define UARTB_BAUD                RX_SERIAL_BAUDRATE
#define UARTB_TXBUFSIZE           RX_SERIAL_TXBUFSIZE
#define UARTB_RXBUFSIZE           RX_SERIAL_RXBUFSIZE

#define UARTF_USE_SERIAL
#define UARTF_BAUD                115200


//-- SX1: SX12xx & SPI

#define SPI_CS_IO                 IO_P15
#define SPI_FREQUENCY             16000000L
#define SX_RESET                  IO_P2
#define SX_DIO                    IO_P4
#define SX_BUSY                   IO_P5
#define SX_TX_EN                  IO_P10
#define SX_RX_EN                  IO_P9

#define SX_USE_REGULATOR_MODE_DCDC

IRQHANDLER(void SX_DIO_EXTI_IRQHandler(void);)

void sx_init_gpio(void)
{
    gpio_init(SX_RESET, IO_MODE_OUTPUT_PP_HIGH);
    gpio_init(SX_DIO, IO_MODE_INPUT_ANALOG);
    gpio_init(SX_BUSY, IO_MODE_INPUT_PU);
    gpio_init(SX_TX_EN, IO_MODE_OUTPUT_PP_LOW);
    gpio_init(SX_RX_EN, IO_MODE_OUTPUT_PP_LOW);
}

IRAM_ATTR bool sx_busy_read(void) { return (gpio_read_activehigh(SX_BUSY)) ? true : false; }

IRAM_ATTR void sx_amp_transmit(void)
{
    gpio_low(SX_RX_EN);
    gpio_high(SX_TX_EN);
}

IRAM_ATTR void sx_amp_receive(void)
{
    gpio_low(SX_TX_EN);
    gpio_high(SX_RX_EN);
}

void sx_dio_init_exti_isroff(void) {}
void sx_dio_enable_exti_isr(void) { attachInterrupt(SX_DIO, SX_DIO_EXTI_IRQHandler, RISING); }
IRAM_ATTR void sx_dio_exti_isr_clearflag(void) {}


//-- Button

#define BUTTON                    IO_P0

void button_init(void)
{
    gpio_init(BUTTON, IO_MODE_INPUT_PU);
}

IRAM_ATTR bool button_pressed(void)
{
    return gpio_read_activelow(BUTTON) ? true : false;
}


//-- LEDs

#define LED_RED                   IO_P16

void leds_init(void)
{
    gpio_init(LED_RED, IO_MODE_OUTPUT_PP_LOW);
}

IRAM_ATTR void led_red_off(void) { gpio_low(LED_RED); }
IRAM_ATTR void led_red_on(void) { gpio_high(LED_RED); }
IRAM_ATTR void led_red_toggle(void) { gpio_toggle(LED_RED); }


//-- POWER

#define POWER_PA_E28_2G4M27SX
#include "../hal-power-pa.h"
