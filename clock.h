
// Clock
#define CLOCK_PROGRESSIVE_THRESHOLD        1000
#define CLOCK_PROGRESSIVE_THRESHOLD_SECOND 1000
uint32_t clockTim = 0;
uint8_t clockDay = 1;
uint8_t clockHour = 0;
uint8_t clockMin = 0;
uint8_t clockSec = 0;
uint8_t clockState = 0;
uint32_t clockBlinkDiff;
uint32_t clockBlinkTimStatus;
uint32_t clockProgressiveTim;
uint32_t clockProgressiveState = 0;
uint32_t clockProgressiveTriggerTim;
