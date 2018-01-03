#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "stdarg.h"
#include <OneWire.h>

typedef struct fifo_t {
    uint8_t* buf;
    uint8_t head;
    uint8_t tail;
    uint8_t size;
} fifo_t;
namespace {
    void fifo_init(fifo_t *f, uint8_t *buf, uint8_t size) {
        f->head = 0;
        f->tail = 0;
        f->buf = buf;
        f->size = size;
    }

    uint8_t fifo_push(fifo_t *f, uint8_t val) {
        if (f->head + 1 == f->tail || (f->head + 1 == f->size && 0 == f->tail)) {
            return 0;
        }

        f->buf[f->head] = val;
        f->head++;
        if (f->head == f->size) {
            f->head = 0;
        }

        return 1;
    }

    uint8_t fifo_pop(fifo_t *f, uint8_t *val) {
        if (f->tail == f->head) {
            return 0;
        }

        uint8_t tmp = f->buf[f->tail];
        f->tail++;
        if (f->tail == f->size) {
            f->tail = 0;
        }

        *val = tmp;

        return 1;
    }
}

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

/// Sensors
#define SENSOR_READ_INTERVAL 3000 // couldn't be less than 2s
static float temperature;
static uint32_t sensorReadTim = -9999;

#define TEMP_SENSOR_PIN 10
OneWire ds(TEMP_SENSOR_PIN);
byte oneWireData[12];
byte oneWireAddr[8];
uint8_t tempSensorState = 0;
uint32_t tempSensorTim;
/// LCD
#define LCD_LIGHT_MAX_TIM 10000
static uint8_t backlighState = 0;
static uint8_t lcdState = 0;
static uint8_t lcdLight = 0;
static uint32_t lcdLightTim = LCD_LIGHT_MAX_TIM;
/// Control
#define PIN_BTN1 7
#define PIN_BTN2 8
static uint8_t btn1;
static uint8_t pbtn1;
static uint8_t btn2;
static uint8_t pbtn2;
/// Bluetooth
static uint8_t zn;
static uint32_t index;
#define FUNC_LOAD_PROGRAM 1

#define BT_COMMAND_LISTEN 255

#define BT_STATE_MAIN         0
#define BT_STATE_HEAT_PROGRAM 1
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

/// Clock
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

uint8_t btBuff[16];
fifo_t btFifo;
uint8_t currTempLimit = NULL;

/// Validates input.
class ProgramValidator {
public:
    static uint8_t Name(uint8_t tmpChar, uint8_t length)
    {
        if (length > 16) {
            return 0;
        }

        if ((tmpChar >= 65 && tmpChar <= 90) || (tmpChar >= 97 && tmpChar <= 122) || 32 == tmpChar) {
            return 1;
        }

        return 0;
    }
    static uint8_t Day(uint8_t day)
    {
        if (day >= 0 && day <= 6) {
            return 1;
        }

        return 0;
    }
    static uint8_t Hour(uint8_t hour)
    {
        if (hour >= 0 && hour <= 23) {
            return 1;
        }

        return 0;
    }
    static uint8_t Temperature(uint8_t temperature)
    {
        if (temperature >= 0 && temperature <= 100) {
            return 1;
        }

        return 0;
    }
    static uint8_t Id(uint8_t id)
    {
        if (id >= 0 && id <= 4) {
            return 1;
        }

        return 0;
    }
};
/// Heating program entity.
typedef struct Program {
    /// Assumptions: per day max 5 settings,
    /// max size of one program will be 5 (max settings) * 7 (days) * 4 (bytes per line) = 140 (bytes)
    uint8_t payload[140];
    /// Lengths indicates bytes amount
    uint8_t length = 0;
    /// Max 8 chars per name
    /// allowed chars a-zA-Z and space, in ascii 69-90, 97-122, 32
    char name[16];
    uint8_t nameLength = 0;
    uint8_t id = 0;
} Program;

/// States of loading heating program.
#define HEATING_PROGRAM_STATE_READ_ID      0
#define HEATING_PROGRAM_STATE_READ_NAME    1
#define HEATING_PROGRAM_STATE_READ_PROGRAM 2
#define HEATING_PROGRAM_STATE_READ_CRC     3
#define HEATING_PROGRAM_STATE_READ_END     4
/// Error codes of program reader.
#define ERR_INVALID_PROGRAM 1
#define ERR_INVALID_ID      2
#define ERR_INVALID_NAME    3
#define ERR_INVALID_DAY     4
#define ERR_INVALID_HOUR    5
#define ERR_INVALID_TEMP    6
#define ERR_INVALID_CRC     7

#define RETURN_TO_MAIN_IF_NO_BACKLIGHT if (2 == backlighState) lcdState = MENU_STATE_MAIN;

/// Heating state
#define HEATING_STATE_OFF 0
#define HEATING_STATE_ON 1
#define HEATING_WIRE_PIN 6
uint8_t heatingState = HEATING_STATE_OFF;
#if 0
/// Heating program
static uint8_t heatingProgram = 1;
// Loaded programs.
static Program* heatingPrograms[1];
#endif

/// Manages programming heating programs.
class ServiceProgram {
    uint8_t error;
    Program** programsContainer;
public:
    /// Currently programmed program.
    Program* program;
    /// State of writer.
    uint8_t state;
    /// Constructor.
    ServiceProgram()
    {
        this->program = new Program;
        this->reset();
    }

    uint8_t checkTemperature(uint8_t currDay, uint8_t currHour, uint8_t currMin)
    {
        if (0 == this->program->length) {
            return NULL;
        }

        uint8_t day, hourFrom, hourTo, temp = NULL, iter;

        iter = this->program->length;
        for (uint8_t i = 0; i < iter; i += 4) {
            day = this->program->payload[i];
            if (day != currDay) continue;
            hourFrom = this->program->payload[i + 1];
            if (currHour < hourFrom) continue;
            hourTo = this->program->payload[i + 2];
            if (currHour > hourTo) continue;
            temp = this->program->payload[i + 3];
            break;
        }

        return temp;
    }

    uint8_t calculateCrc()
    {
        return 1;
    }

    /// Resets to initial state.
    void reset()
    {
        this->error = NULL;
        this->state = HEATING_PROGRAM_STATE_READ_ID;
        this->program->nameLength = 0;
        this->program->name[0] = '\0';
        this->program->length = 0;
        this->program->id = 0;
    }

    uint8_t getError()
    {
        return this->error;
    }

    uint8_t isSuccess()
    {
        if (HEATING_PROGRAM_STATE_READ_END != this->state || NULL != this->error) {
            return 0;
        }

        return 1;
    }
    /// Reads another byte of data.
    uint8_t read(uint8_t payload)
    {
        if (NULL == this->program && this->state != HEATING_PROGRAM_STATE_READ_ID) {
            this->error = ERR_INVALID_PROGRAM;
            return 0;
        }

        switch (this->state) {
            case HEATING_PROGRAM_STATE_READ_ID:
                /// Rading ID
                if (!ProgramValidator::Id(payload)) {
                    this->error = ERR_INVALID_ID;
                    return 0;
                }

                this->state = HEATING_PROGRAM_STATE_READ_NAME;
                this->program->id = payload;
                break;
            case HEATING_PROGRAM_STATE_READ_NAME:
                /// Reading Name
                if (200 == payload) {
                    this->state = HEATING_PROGRAM_STATE_READ_PROGRAM;
                    return 1;
                }

                if (!ProgramValidator::Name(payload, this->program->nameLength)) {
                    this->error = ERR_INVALID_NAME;
                    return 0;
                }

                this->program->name[this->program->nameLength++] = payload;
                this->program->name[this->program->nameLength] = '\0';

                break;
            case HEATING_PROGRAM_STATE_READ_PROGRAM:
                /// Reading program
                if (200 == payload) {
                    this->state = HEATING_PROGRAM_STATE_READ_CRC;
                    return 1;
                }

                switch (this->program->length % 4) {
                    case 0:
                        /// Reading day
                        if (!ProgramValidator::Day(payload)) {
                            this->error = ERR_INVALID_DAY;
                            return 0;
                        }

                        this->program->payload[this->program->length++] = payload;
                        break;
                    case 1:
                        /// Reading hour from
                    case 2:
                        /// Reading hour to
                        if (!ProgramValidator::Hour(payload)) {
                            this->error = ERR_INVALID_HOUR;
                            return 0;
                        }

                        this->program->payload[this->program->length++] = payload;
                        break;
                    case 3:
                        /// Reading temperature
                        if (!ProgramValidator::Temperature(payload)) {
                            this->error = ERR_INVALID_TEMP;
                            return 0;
                        }

                        this->program->payload[this->program->length++] = payload;
                        break;
                }

                break;
            case HEATING_PROGRAM_STATE_READ_CRC:
                this->state = HEATING_PROGRAM_STATE_READ_END;
                // TODO check if crc is correct.
                break;
        }

        return 1;
    }
};

ServiceProgram serviceProg;

void setup()
{
    Serial.begin(9600);

    pinMode(PIN_BTN1, INPUT);
    pinMode(PIN_BTN2, INPUT);
    pinMode(HEATING_WIRE_PIN, OUTPUT);

    lcd.begin(16, 2);
    lcd.backlight();

    fifo_init(&btFifo, btBuff, 16);
}

void menuAction()
{
    if (btn1 || btn2) {
        lcdLightTim = millis();
    }

    if (millis() - lcdLightTim < LCD_LIGHT_MAX_TIM) {
        if (1 != backlighState) {
            lcd.backlight();
            backlighState = 1;
            return;
        }
    } else {
        if (2 != backlighState) {
            lcd.noBacklight();
            backlighState = 2;
        }
    }

    switch (lcdState) {
        default:
        case MENU_STATE_MAIN:
            menuBlinkTimDiff = millis() - menuBlinkTimStatus;

            lcd.setCursor(0, 0);
            lcd.print("Day ");
            if (clockDay < 10) lcd.print(" ");
            lcd.print(clockDay);
            lcd.print("     ");
            if (clockHour < 10) lcd.print("0");
            lcd.print(clockHour);
            lcd.print(":");
            if (clockMin < 10) lcd.print("0");
            lcd.print(clockMin);

            lcd.setCursor(0, 1);
            lcd.print("  ");
            lcd.print(temperature);
            lcd.print((char) 223);
            lcd.print("C      ");
            if (HEATING_STATE_ON == heatingState) {
                if (menuBlinkTimDiff > MENU_BLINK_TIM) {
                    lcd.print((char) 176); // dec 176-178
                } else {
                    lcd.print(" ");
                }

                if (menuBlinkTimDiff > 2 * MENU_BLINK_TIM) {
                    menuBlinkTimStatus = millis();
                }
            } else {
                lcd.print(" ");
            }

            /// Actions
            if (btn1 && !pbtn1) lcdState = MENU_STATE_PROGRAM;
            if (btn2 && !pbtn2) lcdState = MENU_STATE_SET_CLOCK;

            break;
        case MENU_STATE_SET_CLOCK:
            clockBlinkDiff = millis() - clockBlinkTimStatus;

            lcd.setCursor(0, 0);
            if (0 == clockProgressiveState && 0 == clockState) {
                if (clockBlinkDiff > MENU_BLINK_TIM) {
                    lcd.print("Day ");
                    if (clockDay < 10) lcd.print(" ");
                    lcd.print(clockDay);
                } else {
                    lcd.print("      ");
                }
            } else {
                lcd.print("Day ");
                if (clockDay < 10) lcd.print(" ");
                lcd.print(clockDay);
            }

            lcd.print("     ");

            if (0 == clockProgressiveState && 1 == clockState) {
                if (clockBlinkDiff > MENU_BLINK_TIM) {
                    if (clockHour < 10) lcd.print("0");
                    lcd.print(clockHour);
                } else {
                    lcd.print("  ");
                }
            } else {
                if (clockHour < 10) lcd.print("0");
                lcd.print(clockHour);
            }

            lcd.print(":");

            if (0 == clockProgressiveState && 2 == clockState) {
                if (clockBlinkDiff > MENU_BLINK_TIM) {
                    if (clockMin < 10) lcd.print("0");
                    lcd.print(clockMin);
                } else {
                    lcd.print("  ");
                }
            } else {
                if (clockMin < 10) lcd.print("0");
                lcd.print(clockMin);
            }

            lcd.setCursor(0, 1);
            lcd.print("                ");

            if (clockBlinkDiff > 2 * MENU_BLINK_TIM) {
                clockBlinkTimStatus = millis();
            }

            switch (clockProgressiveState) {
                case 0:
                    if (btn1 && !pbtn1) {
                        clockProgressiveState = 1;
                        clockProgressiveTim = millis();
                    }
                    break;
                case 1:
                    if (!btn1) clockProgressiveState = 0;
                    if (millis() - clockProgressiveTim > CLOCK_PROGRESSIVE_THRESHOLD) {
                        clockProgressiveState = 2;
                        clockProgressiveTim = millis();
                    }

                    // Break whole function.
                    return;
                case 2:
                    if (!btn1) clockProgressiveState = 0;
                    if (millis() - clockProgressiveTim > CLOCK_PROGRESSIVE_THRESHOLD_SECOND) clockProgressiveState = 3;
                    if (millis() - clockProgressiveTriggerTim > 300) {
                        clockProgressiveTriggerTim = millis();
                    } else {
                        // Break whole function.
                        return;
                    }

                    break;
                case 3:
                    if (!btn1) clockProgressiveState = 0;
                    if (millis() - clockProgressiveTriggerTim > 150) {
                        clockProgressiveTriggerTim = millis();
                    } else {
                        // Break whole function.
                        return;
                    }

                    break;
            }

            switch (clockState) {
                case 0:
                    if (btn2 && !pbtn2) clockState = 1;
                    if (btn1) clockDay++;
                    if (clockDay > 7) clockDay = 1;

                    break;
                case 1:
                    if (btn2 && !pbtn2) clockState = 2;
                    if (btn1) clockHour++;
                    if (clockHour > 23) clockHour = 0;

                    break;
                case 2:
                    if (btn2 && !pbtn2) {
                        clockState = 0;
                        lcdState = MENU_STATE_MAIN;
                        // Refresh after updating hour.
                        refreshTemperatureLimit();
                    }
                    if (btn1) clockMin++;
                    if (clockMin > 59) clockMin = 0;

                    break;
            }

            break;
        case MENU_STATE_PROGRAM:
            lcd.setCursor(0, 0);
            lcd.print("Program         ");
            lcd.setCursor(0, 1);
            if (NULL == serviceProg.program) {
                lcd.print(" UNDEFINED      ");
            } else {
                lcd.print(serviceProg.program->name);
                lcd.print("                ");
            }

            /// Actions
            if (btn1 && !pbtn1) lcdState = MENU_STATE_MAIN;
            if (btn2 && !pbtn2) {
                lcdState = MENU_STATE_LIMITS;
                // Refresh after checking for boundaries.
                refreshTemperatureLimit();
            }

            RETURN_TO_MAIN_IF_NO_BACKLIGHT
#if 0 // Disabled.
            if (btn2 && !pbtn2) {
                settingProgramCounter = millis();
                lcdState = MENU_STATE_SET_PROGRAM_ENTER;
            }
#endif
            break;
        case MENU_STATE_LIMITS:
            lcd.setCursor(0, 0);
            lcd.print("Day ");
            if (clockDay < 10) lcd.print(" ");
            lcd.print(clockDay);
            lcd.print("     ");
            if (clockHour < 10) lcd.print("0");
            lcd.print(clockHour);
            lcd.print(":");
            if (clockMin < 10) lcd.print("0");
            lcd.print(clockMin);

            lcd.setCursor(0, 1);
            if (NULL == currTempLimit) {
                lcd.print("Min. temp.  --");
            } else {
                lcd.print("Min. temp.  ");
                if (currTempLimit < 10) lcd.print("0");
                lcd.print(currTempLimit);
            }

            lcd.print((char) 223);
            lcd.print("C");

            /// Actions
            if (!btn2) lcdState = MENU_STATE_PROGRAM;
            RETURN_TO_MAIN_IF_NO_BACKLIGHT

            break;
#if 0 // Disabled
        case MENU_STATE_SET_PROGRAM_ENTER:
            /// This state will be omitted for now.
            /// Actions
            if (!btn2) lcdState = MENU_STATE_PROGRAM;
            RETURN_TO_MAIN_IF_NO_BACKLIGHT
            if (millis() - settingProgramCounter > MENU_CHANGE_PROGRAM_TIM_THRESHOLD) lcdState = MENU_STATE_SET_PROGRAM;

            break;
        case MENU_STATE_SET_PROGRAM:
            /// This state will be omitted for now.
            if (btn2 && !pbtn2) {
                heatingProgram++;
                if (heatingProgram > 5) {
                    heatingProgram = 1;
                }
            }

            menuBlinkTimDiff = millis() - menuBlinkTimStatus;
            if (menuBlinkTimDiff > 2 * MENU_BLINK_TIM) {
                menuBlinkTimStatus = millis();
            }

            if (menuBlinkTimDiff > MENU_BLINK_TIM) {
                lcd.setCursor(0, 0);
                lcd.print("Program         ");
                lcd.setCursor(0, 1);
                lcd.print(" ");
                lcd.print(heatingProgram);
                lcd.print(" | Test       ");
            } else {
                lcd.setCursor(0, 0);
                lcd.print("                ");
                lcd.setCursor(0, 1);
                lcd.print("                ");
            }

            /// Actions
            if (btn1 && !pbtn1) lcdState = MENU_STATE_PROGRAM;
            RETURN_TO_MAIN_IF_NO_BACKLIGHT

            break;
#endif
    }
}

static uint8_t btState = 0;
void bluetoothReadAction()
{
    // TODO listening for conection just here? Is it possible?
    if (!Serial.available()) {
        return;
    }

    while (Serial.available()) {
        fifo_push(&btFifo, Serial.read());
    }

    while (fifo_pop(&btFifo, &zn)) {
        switch (btState) {
            default:
            case BT_STATE_MAIN:
                switch (zn) {
                    case BT_STATE_HEAT_PROGRAM:
                        Serial.write("gt:hp\n");
                        serviceProg.reset();
                        btState = BT_STATE_HEAT_PROGRAM;
                        break;
                }

                break;
            case BT_STATE_HEAT_PROGRAM:
                if (BT_COMMAND_LISTEN == zn) {
                    Serial.write("return\n");
                    btState = BT_STATE_MAIN;
                    break;
                }

                if (serviceProg.isSuccess()) {
                    Serial.write("success\n");
                    btState = BT_STATE_MAIN;
                    serviceProg.state = HEATING_PROGRAM_STATE_READ_ID;
                } else if (serviceProg.read(zn)) {
                    Serial.write("ok:char:");
                    Serial.write(zn + 48);
                    Serial.write(":");
                    Serial.write(zn);
                    Serial.write(":state:");
                    Serial.write(serviceProg.state + 48);
                    Serial.write("\n");
                } else {
                    Serial.write("err:");
                    Serial.write(serviceProg.getError() + 48);
                    Serial.write(":");
                    Serial.write(zn);
                    Serial.write("\n");
                    btState = BT_STATE_MAIN;
                }

                break;
        }
    }

    delay(500);
    zn = NULL;
}

/**
 * Reads sensors.
 */
void readSensors()
{
    if ((millis() - sensorReadTim) < SENSOR_READ_INTERVAL) {
        return;
    }

    sensorReadTim = millis();

    switch (tempSensorState) {
        case 0:
            if (!ds.search(oneWireAddr)) {
                Serial.println("onewire:err:no_more_addr");
                ds.reset_search();
                delay(250);
                return;
            }

            if (OneWire::crc8(oneWireAddr, 7) != oneWireAddr[7]) {
                Serial.println("onewire:err:crc");
                return;
            }

            if (oneWireAddr[0] != 0x28) {
                Serial.println("onewire:err:no_device");
                return;
            }

            tempSensorState = 1;

            break;
        case 1:
            ds.reset();
            ds.select(oneWireAddr);
            ds.write(0x44, 1);

            tempSensorState = 2;
            tempSensorTim = millis();
            break;
        case 2:
            if (millis() - tempSensorTim > 1000) {
                tempSensorState = 3;
            }
            break;
        case 3:
            ds.reset();
            ds.select(oneWireAddr);
            ds.write(0xBE);

            uint8_t i;
            for (i = 0; i < 9; i++) {
                oneWireData[i] = ds.read();
            }

            int16_t raw = (oneWireData[1] << 8) | oneWireData[0];

            byte cfg = (oneWireData[4] & 0x60);
            // at lower res, the low bits are undefined, so let's zero them
            if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
            else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
            else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms

            temperature = (float) raw / 16.0;
            tempSensorState = 1;
            break;
    }
}

/**
 * Manages clock calculations.
 */
inline void clock()
{
    if (millis() - clockTim > 1000) { clockSec++; clockTim = millis(); }
    if (clockSec > 59) { clockSec = 0; clockMin++; }
    if (clockMin > 59) {
        clockMin = 0;
        clockHour++;
        refreshTemperatureLimit();
    }
    if (clockHour > 23) { clockHour = 0; clockDay++; }
    if (clockDay > 7) { clockDay = 0; }
}

/**
 * According to time, checks what is the temperature setted.
 */
inline void refreshTemperatureLimit()
{
    currTempLimit = serviceProg.checkTemperature(clockDay, clockHour, clockMin);
}

/**
 *
 */
inline void refreshPeripheralsState()
{
    switch (heatingState) {
        case HEATING_STATE_ON:
            if (NULL == currTempLimit || currTempLimit <= temperature) {
                heatingState = HEATING_STATE_OFF;
                digitalWrite(HEATING_WIRE_PIN, 0);
            }

            break;
        case HEATING_STATE_OFF:
            if (NULL != currTempLimit && currTempLimit > temperature) {
                heatingState = HEATING_STATE_ON;
                digitalWrite(HEATING_WIRE_PIN, 1);
            }

            break;
    }
}

void loop()
{
    clock();

    btn1 = digitalRead(PIN_BTN1);
    btn2 = digitalRead(PIN_BTN2);

    menuAction();
    readSensors();
    refreshPeripheralsState();
    bluetoothReadAction();

    pbtn1 = btn1;
    pbtn2 = btn2;
}