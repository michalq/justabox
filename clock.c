/**
 * Clock structure.
 */
typedef struct Clock {
    uint32_t tim = 0;

    uint8_t day = 1;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;

    uint8_t state = 0;

    uint32_t blinkDiff;
    uint32_t blinkTimStatus;

    uint32_t progressiveTim;
    uint32_t progressiveState = 0;
    uint32_t progressiveTriggerTim;
} Clock;

/**
 * Manages clock calculations.
 */
inline void clock()
{
    if (millis() - clockTim > 1000) {
        clockSec++;
        clockTim = millis();
    }

    if (clockSec > 59) {
        clockSec = 0;
        clockMin++;
    }

    if (clockMin > 59) {
        clockMin = 0;
        clockHour++;
        refreshTemperatureLimit();
    }

    if (clockHour > 23) {
        clockHour = 0;
        clockDay++;
    }

    if (clockDay > 7) {
        clockDay = 0;
    }
}