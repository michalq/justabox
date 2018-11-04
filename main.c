#define F_CPU 1000000UL

#include "io.c"
#include <stdint.h>
#include <avr/interrupt.h>
#include "utils.h"
#include "libs/i2c_master.h"
#include "libs/lcd.h"

inline static void beforeLoop() __attribute__((always_inline));
inline static void loop() __attribute__((always_inline));
inline static void afterLoop() __attribute__((always_inline));

static bool btnPrimary;
static bool pBtnPrimary;
static bool btnSecondary;
static bool pBtnSecondary;

int main(void)
{
    IO_Setup();
    i2c_init();

    IO_SetOutputPrimary(false);
    i2c_start_wait(0x27 + I2C_WRITE);
    IO_SetOutputPrimary(true);
    i2c_write(1);
    // i2c_stop();

    // lcd_init();
    while (true) {
        beforeLoop();
        loop();
        afterLoop();
    }

    return 0;
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