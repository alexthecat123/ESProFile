// Pin definitions for the LisaFPGA onboard ESProFile

// SD card pins
#define SD_CLK  40
#define SD_MISO 12
#define SD_MOSI 39
#define SD_CS   15

// ProFile bus pins
#define dataBusStart 4
#define CMD_Pin 17
#define BSY_Pin 18
#define RW_Pin 21
#define STRB_Pin 33
#define PRES_Pin 35
#define PARITY_Pin 34

// Pins for the red and green LEDs
#define red_led 2
#define green_led 1
// Duty cycles for the red and green LEDs' PWM (out of 255) for brightness control
#define red_pwm_duty 10
#define green_pwm_duty 10

// Pin for the switch that selects between diagnostic and emulation modes
#define switch_pin 16

// Watchdog timer write enable register and value
// These only change between different ESP32 families (ESP32, ESP32-C3, ESP32-S3, and so on)
// The values below are for the ESP32-S3; no need to change them in your pin definition file unless you're using a different ESP32 family
#define TIMG1_WDT_WE 0x050D83AA1
#define TIMG1_WDT_WE_REG 0x60020064

// Watchdog timer configuration register and enable bit
// Same goes for these; they're for the ESP32-S3 but will need to be changed if you're using a different ESP32 family
#define TIMG1_WDT_CONF_REG 0x60020048
#define TIMG1_WDT_EN 1 << 31

// Given that different versions of the board might put these pins in either the low or high GPIO banks, we need to figure out which one they're in
// If you're making a custom version of ESProFile with different pin assignments, just update the stuff above
// No need to touch anything below this line
#if dataBusStart < 32
    #define busOffset dataBusStart
    #define BUS_W1TS_REG GPIO_OUT_W1TS_REG
    #define BUS_W1TC_REG GPIO_OUT_W1TC_REG
    #define BUS_REG GPIO_OUT_REG
    #define BUS_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define BUS_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define BUS_IN_REG GPIO_IN_REG
#else
    #define busOffset (dataBusStart - 32)
    #define BUS_W1TS_REG GPIO_OUT1_W1TS_REG
    #define BUS_W1TC_REG GPIO_OUT1_W1TC_REG
    #define BUS_REG GPIO_OUT1_REG
    #define BUS_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define BUS_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define BUS_IN_REG GPIO_IN1_REG
#endif


#if CMD_Pin < 32
    #define CMDPin CMD_Pin
    #define CMD_W1TS_REG GPIO_OUT_W1TS_REG
    #define CMD_W1TC_REG GPIO_OUT_W1TC_REG
    #define CMD_OUT_REG GPIO_OUT_REG
    #define CMD_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define CMD_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define CMD_IN_REG GPIO_IN_REG
#else
    #define CMDPin (CMD_Pin - 32)
    #define CMD_W1TS_REG GPIO_OUT1_W1TS_REG
    #define CMD_W1TC_REG GPIO_OUT1_W1TC_REG
    #define CMD_OUT_REG GPIO_OUT1_REG
    #define CMD_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define CMD_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define CMD_IN_REG GPIO_IN1_REG
#endif

#if BSY_Pin < 32
    #define BSYPin BSY_Pin
    #define BSY_W1TS_REG GPIO_OUT_W1TS_REG
    #define BSY_W1TC_REG GPIO_OUT_W1TC_REG
    #define BSY_OUT_REG GPIO_OUT_REG
    #define BSY_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define BSY_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define BSY_IN_REG GPIO_IN_REG
#else
    #define BSYPin (BSY_Pin - 32)
    #define BSY_W1TS_REG GPIO_OUT1_W1TS_REG
    #define BSY_W1TC_REG GPIO_OUT1_W1TC_REG
    #define BSY_OUT_REG GPIO_OUT1_REG
    #define BSY_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define BSY_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define BSY_IN_REG GPIO_IN1_REG
#endif

#if RW_Pin < 32
    #define RWPin RW_Pin
    #define RW_W1TS_REG GPIO_OUT_W1TS_REG
    #define RW_W1TC_REG GPIO_OUT_W1TC_REG
    #define RW_OUT_REG GPIO_OUT_REG
    #define RW_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define RW_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define RW_IN_REG GPIO_IN_REG
#else
    #define RWPin (RW_Pin - 32)
    #define RW_W1TS_REG GPIO_OUT1_W1TS_REG
    #define RW_W1TC_REG GPIO_OUT1_W1TC_REG
    #define RW_OUT_REG GPIO_OUT1_REG
    #define RW_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define RW_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define RW_IN_REG GPIO_IN1_REG
#endif

#if STRB_Pin < 32
    #define STRBPin STRB_Pin
    #define STRB_W1TS_REG GPIO_OUT_W1TS_REG
    #define STRB_W1TC_REG GPIO_OUT_W1TC_REG
    #define STRB_OUT_REG GPIO_OUT_REG
    #define STRB_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define STRB_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define STRB_IN_REG GPIO_IN_REG
#else
    #define STRBPin (STRB_Pin - 32)
    #define STRB_W1TS_REG GPIO_OUT1_W1TS_REG
    #define STRB_W1TC_REG GPIO_OUT1_W1TC_REG
    #define STRB_OUT_REG GPIO_OUT1_REG
    #define STRB_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define STRB_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define STRB_IN_REG GPIO_IN1_REG
#endif

#if PRES_Pin < 32
    #define PRESPin PRES_Pin
    #define PRES_W1TS_REG GPIO_OUT_W1TS_REG
    #define PRES_W1TC_REG GPIO_OUT_W1TC_REG
    #define PRES_OUT_REG GPIO_OUT_REG
    #define PRES_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define PRES_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define PRES_IN_REG GPIO_IN_REG
#else
    #define PRESPin (PRES_Pin - 32)
    #define PRES_W1TS_REG GPIO_OUT1_W1TS_REG
    #define PRES_W1TC_REG GPIO_OUT1_W1TC_REG
    #define PRES_OUT_REG GPIO_OUT1_REG
    #define PRES_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define PRES_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define PRES_IN_REG GPIO_IN1_REG
#endif

#if PARITY_Pin < 32
    #define PARITYPin PARITY_Pin
    #define PARITY_W1TS_REG GPIO_OUT_W1TS_REG
    #define PARITY_W1TC_REG GPIO_OUT_W1TC_REG
    #define PARITY_OUT_REG GPIO_OUT_REG
    #define PARITY_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define PARITY_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define PARITY_IN_REG GPIO_IN_REG
#else
    #define PARITYPin (PARITY_Pin - 32)
    #define PARITY_W1TS_REG GPIO_OUT1_W1TS_REG
    #define PARITY_W1TC_REG GPIO_OUT1_W1TC_REG
    #define PARITY_OUT_REG GPIO_OUT1_REG
    #define PARITY_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define PARITY_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define PARITY_IN_REG GPIO_IN1_REG
#endif

#if red_led < 32
    #define red red_led
    #define RED_W1TS_REG GPIO_OUT_W1TS_REG
    #define RED_W1TC_REG GPIO_OUT_W1TC_REG
    #define RED_OUT_REG GPIO_OUT_REG
    #define RED_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define RED_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define RED_IN_REG GPIO_IN_REG
#else
    #define red (red_led - 32)
    #define RED_W1TS_REG GPIO_OUT1_W1TS_REG
    #define RED_W1TC_REG GPIO_OUT1_W1TC_REG
    #define RED_OUT_REG GPIO_OUT1_REG
    #define RED_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define RED_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define RED_IN_REG GPIO_IN1_REG
#endif

#if green_led < 32
    #define green green_led
    #define GREEN_W1TS_REG GPIO_OUT_W1TS_REG
    #define GREEN_W1TC_REG GPIO_OUT_W1TC_REG
    #define GREEN_OUT_REG GPIO_OUT_REG
    #define GREEN_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define GREEN_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define GREEN_IN_REG GPIO_IN_REG
#else
    #define green (green_led - 32)
    #define GREEN_W1TS_REG GPIO_OUT1_W1TS_REG
    #define GREEN_W1TC_REG GPIO_OUT1_W1TC_REG
    #define GREEN_OUT_REG GPIO_OUT1_REG
    #define GREEN_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define GREEN_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define GREEN_IN_REG GPIO_IN1_REG
#endif

#if switch_pin < 32
    #define switchPin switch_pin
    #define SWITCH_W1TS_REG GPIO_OUT_W1TS_REG
    #define SWITCH_W1TC_REG GPIO_OUT_W1TC_REG
    #define SWITCH_OUT_REG GPIO_OUT_REG
    #define SWITCH_ENABLE_W1TS_REG GPIO_ENABLE_W1TS_REG
    #define SWITCH_ENABLE_W1TC_REG GPIO_ENABLE_W1TC_REG
    #define SWITCH_IN_REG GPIO_IN_REG
#else
    #define switchPin (switch_pin - 32)
    #define SWITCH_W1TS_REG GPIO_OUT1_W1TS_REG
    #define SWITCH_W1TC_REG GPIO_OUT1_W1TC_REG
    #define SWITCH_OUT_REG GPIO_OUT1_REG
    #define SWITCH_ENABLE_W1TS_REG GPIO_ENABLE1_W1TS_REG
    #define SWITCH_ENABLE_W1TC_REG GPIO_ENABLE1_W1TC_REG
    #define SWITCH_IN_REG GPIO_IN1_REG
#endif