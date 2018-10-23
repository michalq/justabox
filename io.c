#include <avr/io.h>
#include "utils.h"

inline void IO_Setup() __attribute__((always_inline));
inline bool IO_GetInputPrimary() __attribute__((always_inline));
inline bool IO_GetInputSecondary() __attribute__((always_inline));
inline void IO_SetOutputPrimary(bool status) __attribute__((always_inline));

inline void IO_Setup()
{
    DDRB &= ~(1 << DDB0);
    PORTB |= (1 << PB0);
    DDRD &= ~(1 << DDD7);
    PORTD |= (1 << PD7);
}

inline bool IO_GetInputPrimary()
{
    return PINB & (1 << PD0);
}

inline bool IO_GetInputSecondary()
{
    return PIND & (1 << PD7);
}

inline void IO_SetOutputPrimary(bool status)
{
    if (status) {
        PORTD |= (1 << PD6);
    }
    else {
        PORTD &= ~(1 << PD6);
    }
}