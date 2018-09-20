
/// Menu
#define MENU_BLINK_TIM 500
#define MENU_CHANGE_PROGRAM_TIM_THRESHOLD 2000

#define MENU_STATE_MAIN                 0
#define MENU_STATE_LIMITS               1
#define MENU_STATE_PROGRAM              2
#define MENU_STATE_SET_PROGRAM_ENTER    3
#define MENU_STATE_SET_PROGRAM          4
#define MENU_STATE_SET_CLOCK            6

static uint32_t menuBlinkTimStatus;
static uint32_t menuBlinkTimDiff;
static uint32_t settingProgramCounter = 0;
