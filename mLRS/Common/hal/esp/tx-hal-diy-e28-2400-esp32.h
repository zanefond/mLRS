//*******************************************************
// Copyright (c) MLRS project
// GPL3
// https://www.gnu.org/licenses/gpl-3.0.de.html
//*******************************************************
// hal
//*******************************************************

/*
  ESP32 + EBYTE E28-2G4M20S DIY 2.4 GHz Tx module

  This is a DIY Tx module built from a plain ESP32 module and an EBYTE
  E28-2G4M20S (SX1280 + PA/LNA, 20 dBm = 100 mW) module. The pin mapping is the
  one of the ExpressLRS "DIY 2400 E28" target, but with the CRSF serial on GPIO1/3
  (as ExpressLRS internal module targets, e.g. Jumper T-20 or RM Boxer, do it) and
  with the second UART on GPIO5/18 for a backpack.

  Pin mapping:

    E28 (SX1280)          ESP32
    -----------------------------
    NSS                   GPIO13
    SCK                   GPIO25
    MISO                  GPIO19
    MOSI                  GPIO23
    RESET                 GPIO14
    BUSY                  GPIO21
    DIO1                  GPIO4
    TX_EN                 GPIO26
    RX_EN                 GPIO27

    CRSF to/from radio    GPIO1 (Tx) / GPIO3 (Rx), full duplex, 400k baud
    Fan                   GPIO17 (on/off)
    Backpack UART         GPIO5 (Tx) / GPIO18 (Rx)  - see at the bottom of this file

  Notes:
  - CRSF to/from the radio is a FULL DUPLEX UART (two wires) here, since the board
    definition gives separate Rx and Tx pins. The radio must offer CRSF at 400k;
    mLRS auto-bauds (400k/921600/1.87M inverted, 416k non-inverted), i.e. the
    normal "CRSF 400k" setting of EdgeTX does it.
    If your board is instead wired to the single bidirectional JR bay pin (CRSF
    half duplex on ONE wire, as e.g. BetaFPV/Ranger modules are), then
      * set UART_USE_TX_IO and UART_USE_RX_IO to that same pin, e.g. IO_P13
      * delete the "JR_PIN5_FULL_DUPLEX" define below
  - The board has no separate COM/USB-serial port (GPIO1/3 are used for CRSF), so
    there is no CLI. Do all settings with the mLRS Lua script on the radio. See the
    "ESP8285 backpack" block at the end of this file if you want the wireless
    bridge (WiFi MAVLink) and/or CLI access.
  - No LED is defined (the hardware json has none). If your board has one, see the
    LED block below.
*/

#define DEVICE_HAS_JRPIN5
#define DEVICE_HAS_NO_SERIAL
#define DEVICE_HAS_NO_COM
#define DEVICE_HAS_NO_LED
#define DEVICE_HAS_FAN_ONOFF
#define DEVICE_HAS_NO_DEBUG


//-- UARTS
// UARTB = serial port
// UARTC (or USB) = com (CLI) port
// UARTD = serial2 port or wireless bridge port
// UART  = JR bay pin5, here: full duplex CRSF connection to radio
// UARTE = in port, SBus or whatever
// UARTF or SWUART = debug port

#define UART_USE_SERIAL1 // full duplex CRSF to the radio
#define UART_BAUD                 400000
#define UART_USE_TX_IO            IO_P1
#define UART_USE_RX_IO            IO_P3
#define UART_TXBUFSIZE            0 // TX FIFO = 128
#define UART_RXBUFSIZE            0 // RX FIFO = 128 + 1

#define JR_PIN5_FULL_DUPLEX


//-- SX1: SX12xx & SPI

#define SPI_CS_IO                 IO_P13
#define SPI_MISO                  IO_P19
#define SPI_MOSI                  IO_P23
#define SPI_SCK                   IO_P25
#define SPI_FREQUENCY             18000000L
#define SX_RESET                  IO_P14
#define SX_DIO                    IO_P4
#define SX_BUSY                   IO_P21
#define SX_TX_EN                  IO_P26
#define SX_RX_EN                  IO_P27

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

void sx_dio_enable_exti_isr(void) { attachInterrupt(SX_DIO, SX_DIO_EXTI_IRQHandler, RISING); }
void sx_dio_init_exti_isroff(void) { detachInterrupt(SX_DIO); }
IRAM_ATTR void sx_dio_exti_isr_clearflag(void) {}


//-- Button
// has none, bind via power up cycling or via the Lua script

void button_init(void) {}
IRAM_ATTR bool button_pressed(void) { return false; }


//-- LEDs
// has none (the hardware json does not define one)
//
// If your board has a WS2812 RGB LED - e.g. on GPIO15, as on the ELRS DIY 2400 E28
// target - then remove DEVICE_HAS_NO_LED above and put this here instead:
//
//#define DEVICE_HAS_SINGLE_LED_RGB
//#define LED_RGB                   IO_P15
//#define LED_RGB_PIXEL_NUM         1
//#include "esp-hal-led-rgb.h"
//
// If your board has a plain (single color) LED on some GPIO, use this instead:
//
//#define DEVICE_HAS_SINGLE_LED
//#define LED_RED                   IO_Pxx
//
//void leds_init(void)
//{
//    gpio_init(LED_RED, IO_MODE_OUTPUT_PP_HIGH);
//}
//
//IRAM_ATTR void led_red_off(void) { gpio_low(LED_RED); }
//IRAM_ATTR void led_red_on(void) { gpio_high(LED_RED); }
//IRAM_ATTR void led_red_toggle(void) { gpio_toggle(LED_RED); }


//-- Cooling Fan

#define FAN_IO                    IO_P17
#define FAN_POWER_ON_THRESH       POWER_17_DBM // fan is switched on from this power level on

void fan_init(void)
{
    gpio_init(FAN_IO, IO_MODE_OUTPUT_PP_LOW);
    gpio_low(FAN_IO);
}

IRAM_ATTR void fan_set_power(int8_t power_dbm)
{
    if (power_dbm >= FAN_POWER_ON_THRESH) {
        gpio_high(FAN_IO);
    } else {
        gpio_low(FAN_IO);
    }
}


//-- POWER

#define POWER_PA_E28_2G4M20S
#include "../hal-power-pa.h"


//-------------------------------------------------------
// Optional: ESP8285 backpack as mLRS wireless bridge
//-------------------------------------------------------
/*
  The hardware json has a "backpack" UART on GPIO5 (Tx) / GPIO18 (Rx). With the
  stock ELRS backpack firmware it is of no use for mLRS. Instead, flash the mLRS
  wireless bridge firmware (esp/mlrs-wireless-bridge) onto that ESP8285 and you
  get the mLRS WiFi bridge: MAVLink over WiFi, plus flashing/passthrough.

  For that mLRS also needs to be able to reset the ESP8285 and to pull its GPIO0
  low, so the backpack's RST/EN and GPIO0/BOOT pins must be wired to two free
  ESP32 GPIOs. Adapt the three IO_Pxx values below (GPIO32/33 are just examples,
  they are free on this board) and change "#if 0" to "#if 1".
*/
#if 0

#define DEVICE_HAS_ESP_WIFI_BRIDGE_ESP8266
#define DEVICE_HAS_ESP_WIFI_BRIDGE_CONFIGURE

#define UARTD_USE_SERIAL2 // bridge port
#define UARTD_BAUD                115200
#define UARTD_USE_TX_IO           IO_P5
#define UARTD_USE_RX_IO           IO_P18
#define UARTD_TXBUFSIZE           TX_SERIAL_TXBUFSIZE
#define UARTD_RXBUFSIZE           TX_SERIAL_RXBUFSIZE

#define ESP_RESET                 IO_P32 // backpack EN/RST
#define ESP_GPIO0                 IO_P33 // backpack GPIO0/BOOT, is inverted in the code

void esp_init(void)
{
    gpio_init(ESP_GPIO0, IO_MODE_OUTPUT_PP_LOW); // high -> esp will start in bootloader mode
    gpio_init(ESP_RESET, IO_MODE_OUTPUT_PP_LOW); // low -> esp is in reset
}

IRAM_ATTR void esp_reset_high(void) { gpio_high(ESP_RESET); }
IRAM_ATTR void esp_reset_low(void) { gpio_low(ESP_RESET); }

IRAM_ATTR void esp_gpio0_high(void) { gpio_low(ESP_GPIO0); }
IRAM_ATTR void esp_gpio0_low(void) { gpio_high(ESP_GPIO0); }

#endif
