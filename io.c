#include <avr/io.h>

inline void IO_Setup()
{
    DDRB &= ~(1 << DDB0);
    PORTB |= (1 << PB0);
    DDRD &= ~(1 << DDD7);
    PORTD |= (1 << PD7);
}

inline uint8_t IO_Read_1()
{
    return PINB & (1 << PD0);
}

inline uint8_t IO_Read_2()
{
    return PIND & (1 << PD7);
}

inline uint8_t IO_Write_1()
{

}