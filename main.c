#define F_CPU 1000000UL

#include "utils.h"
#include <stdint.h>
#include "io.c"

// #include <Wire.h>
// #include <LiquidCrystal_I2C.h>
// #include "stdarg.h"
// #include <OneWire.h>

// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/**
 * Configuration.
 */
// Heating
// #define HEATING_WIRE_PIN 6
// #define HEATING_STATE_OFF 0
// #define HEATING_STATE_ON 1
// // Sensors
// #define TEMP_SENSOR_PIN 10
// #define SENSOR_READ_INTERVAL 3000 // couldn't be less than 2s

// uint8_t heatingState = HEATING_STATE_OFF;
// static float temperature;
// static uint32_t sensorReadTim = -9999;
// // OneWire ds(TEMP_SENSOR_PIN);
// byte oneWireData[12];
// byte oneWireAddr[8];
// uint8_t tempSensorState = 0;
// uint32_t tempSensorTim;

// // Bluetooth
// uint8_t btBuff[16];
// fifo_t btFifo;
// uint8_t currTempLimit = NULL;

// #define RETURN_TO_MAIN_IF_NO_BACKLIGHT if (2 == backlighState) lcdState = MENU_STATE_MAIN;

// ServiceProgram serviceProg;

/**
 * According to time, checks what is the temperature setted.
 */
// inline void refreshTemperatureLimit()
// {
//     currTempLimit = serviceProg.checkTemperature(clockDay, clockHour, clockMin);
// }

// inline void refreshPeripheralsState()
// {
//     switch (heatingState) {
//         case HEATING_STATE_ON:
//             if (NULL == currTempLimit || currTempLimit <= temperature) {
//                 heatingState = HEATING_STATE_OFF;
//                 digitalWrite(HEATING_WIRE_PIN, 0);
//             }

//             break;
//         case HEATING_STATE_OFF:
//             if (NULL != currTempLimit && currTempLimit > temperature) {
//                 heatingState = HEATING_STATE_ON;
//                 digitalWrite(HEATING_WIRE_PIN, 1);
//             }

//             break;
//     }
// }

// void setup()
// {
//     Serial.begin(9600);

//     pinMode(HEATING_WIRE_PIN, OUTPUT);

//     lcd.begin(16, 2);
//     lcd.backlight();

//     fifo_init(&btFifo, btBuff, 16);
// }

static bool btnPrimary;
static bool pBtnPrimary;
static bool btnSecondary;
static bool pBtnSecondary;

inline static void beforeLoop() __attribute__((always_inline));
inline static void loop() __attribute__((always_inline));
inline static void afterLoop() __attribute__((always_inline));

int main()
{
    IO_Setup();
    while (true) {
        beforeLoop();
        loop();
        afterLoop();
    }
}

inline static void beforeLoop()
{
    btnPrimary = IO_GetInputPrimary();
    btnSecondary = IO_GetInputSecondary();
}

inline static void loop()
{
    if (btnPrimary) {
        IO_SetOutputPrimary(true);
    }

    if (btnSecondary) {
        IO_SetOutputPrimary(false);
    }
    // clock();
    // menuAction();
    // readSensors();
    // refreshPeripheralsState();
    // bluetoothReadAction();
}

inline static void afterLoop()
{
    pBtnPrimary = btnPrimary;
    pBtnSecondary = btnSecondary;
}