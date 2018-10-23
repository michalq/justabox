#define F_CPU 1000000UL

#include <stdint.h>
#include "utils.h"
#include "libs/i2c_master.h"
#include "libs/lcd.h"
#include "io.c"

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

    i2c_init();

}

inline static void loop()
{
    uint8_t data[1];
    if (btnPrimary) {
        IO_SetOutputPrimary(true);
        data[0] = LCD_NOBACKLIGHT;
        i2c_transmit(0x27, data, 1);
    }

    if (btnSecondary) {
        IO_SetOutputPrimary(false);
        data[0] = LCD_BACKLIGHT;
        i2c_transmit(0x27, data, 1);
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