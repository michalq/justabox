#define F_CPU 1600000L

#include <avr/io.h>
#include <util/delay.h>

#include "i2cmaster.h"

#define LED1 PB0
#define LED2 PB1

#define LCD_ADDR 0x2F

int main(void)
{
    DDRB  |= (1<<LED1) | (1<<LED2);
    PORTB |= (1<<LED1);

    unsigned char ret;
    i2c_init();
    i2c_start_wait(LCD_ADDR + I2C_WRITE);
//    i2c_write(0x05);
//    i2c_write(0x75);
    i2c_stop();
//
//    // read previously written value back from EEPROM address 5
//    i2c_start_wait(LCD_ADDR + I2C_WRITE);     // set device address and write mode
//    i2c_write(0x05);
//    i2c_rep_start(LCD_ADDR + I2C_READ);       // set device address and read mode
//    ret = i2c_readNak();                    // read one byte from EEPROM
//    i2c_stop();

    while (1) {
        PORTB ^=(1<<LED1);
        PORTB ^=(1<<LED2);
        _delay_ms(5000);
    }
}