#define F_CPU 1000000UL

#include "io.c"
#include <stdint.h>
#include <avr/interrupt.h>
#include "utils.h"
#include "libs/twi.h"
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
    lcd_init();
    sei();

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