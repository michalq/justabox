#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "stdarg.h"

#define DHTPIN 9
#define DHTTYPE DHT11

typedef struct fifo_t {
    uint8_t* buf;
    uint8_t head;
    uint8_t tail;
    uint8_t size;
} fifo_t;
void fifo_init(fifo_t* f, uint8_t* buf, uint8_t size){
    f->head = 0;
    f->tail = 0;
    f->size = size;
    f->buf = buf;
}
uint8_t fifo_push(fifo* f, uint8_t val) {
    if (f->head + 1 == f->tail || (f->head +1 == f->size && 0 == f->tail)) {
        return 0;
    }

    f->buff[t->head] = val;
    f->head++;
    if (f->head == f->size) {
        f->head = 0;
    }

    return 1;
}
uint8_t fifo_pop(fifo_t* f, uint8_t* val) {
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

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Sensors
#define SENSOR_READ_INTERVAL 3000 // couldn't be less than 2s
static uint8_t temperature;
static uint8_t humidity;
static uint32_t sensorReadTim = -9999;
// LCD
#define LCD_LIGHT_MAX_TIM 5000
static uint8_t backlighState = 0;
static uint8_t lcdState = 0;
static uint8_t lcdLight = 0;
static uint32_t lcdLightTim = LCD_LIGHT_MAX_TIM;
// Control
#define PIN_BTN1 7
#define PIN_BTN2 8
static uint8_t btn1;
static uint8_t pbtn1;
static uint8_t btn2;
static uint8_t pbtn2;
// Bluetooth
static char zn;
static char txt[150];
static uint32_t index;
#define FUNC_LOAD_PROGRAM 1

#define BT_COMMAND_LISTEN 255

#define BT_STATE_MAIN         0
#define BT_STATE_HEAT_PROGRAM 1
// Menu
#define MENU_BLINK_TIM 500
#define MENU_CHANGE_PROGRAM_TIM_THRESHOLD 2000

#define MENU_STATE_MAIN                 0
#define MENU_STATE_LIMITS               1
#define MENU_STATE_PROGRAM              2
#define MENU_STATE_SET_PROGRAM_ENTER    3
#define MENU_STATE_SET_PROGRAM          4
#define MENU_STATE_BLUETOOTH_CONNECTION 5

static uint32_t menuBlinkTimStatus;
static uint32_t menuBlinkTimDiff;
static uint32_t settingProgramCounter = 0;

// Heating program
static uint8_t heatingProgram = 1;
uint8_t btBuff[16];
fifo_t btFifo;

/// Validates input.
class ProgramValidator {
public:
    static uint8_t Name(uint8_t tmpChar, uint8_t length)
    {
        if (length > 8) {
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
    char name[8];
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

/// Loaded programs.
static Program* heatingPrograms[1];

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
        this->reset();
    }

    void setProgramsContainer(Program* progContainer[1])
    {
        this->programsContainer = progContainer;
    }

    uint8_t calculateCrc()
    {
        return 1;
    }

    /// Resets to initial state.
    void reset()
    {
        this->error = NULL;
        this->program = NULL;
        this->state = HEATING_PROGRAM_STATE_READ_ID;
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
                if (!ProgramValidator::Id(payload)) {
                    this->error = ERR_INVALID_ID;
                    return 0;
                }

                this->state = HEATING_PROGRAM_STATE_READ_NAME;
                this->programsContainer[0] = new Program;
                this->program = this->programsContainer[payload];
                this->program->id = payload;
                break;
            case HEATING_PROGRAM_STATE_READ_NAME:
                if (200 == payload) {
                    this->state = HEATING_PROGRAM_STATE_READ_PROGRAM;
                    return 1;
                }

                if (!ProgramValidator::Name(payload, this->program->length)) {
                    this->error = ERR_INVALID_NAME;
                    return 0;
                }

                this->program->name[this->program->length++] = payload;
                break;
            case HEATING_PROGRAM_STATE_READ_PROGRAM:
                if (200 == payload) {
                    this->state = HEATING_PROGRAM_STATE_READ_CRC;
                    return 1;
                }

                switch (this->program->length % 3) {
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
                if (0) {
                    this->error = ERR_INVALID_CRC;
                    return 0;
                }
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

    dht.begin();
    lcd.begin(16, 2);
    lcd.backlight();

    fifo_init(btFifo, btBuff, 16);
    serviceProg.setProgramsContainer(heatingPrograms);
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
            lcdState = MENU_STATE_MAIN;
        }

        // No action when menu is off.
        return;
    }

    switch (lcdState) {
        default:
        case MENU_STATE_MAIN:
            menuBlinkTimDiff = millis() - menuBlinkTimStatus;

            lcd.setCursor(0, 0);
            lcd.print("Temp      ");
            lcd.print(temperature);
            lcd.print((char) 223);
            lcd.print("C ");
            lcd.print(menuBlinkTimDiff > MENU_BLINK_TIM ? "_" : " ");
            lcd.setCursor(0, 1);
            lcd.print("Humid     ");
            lcd.print(humidity);
            lcd.print("%  ");
            lcd.print(menuBlinkTimDiff > MENU_BLINK_TIM ? "_" : " ");

            if (menuBlinkTimDiff > 2 * MENU_BLINK_TIM) {
                menuBlinkTimStatus = millis();
            }

            if (btn1 && !pbtn1) {
                lcdState = MENU_STATE_PROGRAM;
            }

            if (btn2) {
                lcdState = MENU_STATE_LIMITS;
            }

            break;
        case MENU_STATE_LIMITS:
            lcd.setCursor(0, 0);
            lcd.print("Min temp   ");
            lcd.print("22");
            lcd.print((char) 223);
            lcd.print("C     ");
            lcd.setCursor(0, 1);
            lcd.print("Min humid  ");
            lcd.print("65%      ");

            if (!btn2) {
                lcdState = MENU_STATE_MAIN;
            }

            break;
        case MENU_STATE_PROGRAM:
            lcd.setCursor(0, 0);
            lcd.print("Program         ");
            lcd.setCursor(0, 1);
            lcd.print(" ");
            lcd.print(heatingProgram);
            lcd.print(" | Test       ");

            if (btn1 && !pbtn1) {
                lcdState = MENU_STATE_BLUETOOTH_CONNECTION;
            }

            // Hold btn2 to enter change program mode.
//            if (btn2 && !pbtn2) {
//                settingProgramCounter = millis();
//                lcdState = MENU_STATE_SET_PROGRAM_ENTER;
//            }

            break;
        case MENU_STATE_SET_PROGRAM_ENTER:
            /// This state will be omitted for now.
            if (!btn2) {
                lcdState = MENU_STATE_PROGRAM;
            }

            if (millis() - settingProgramCounter > MENU_CHANGE_PROGRAM_TIM_THRESHOLD) {
                lcdState = MENU_STATE_SET_PROGRAM;
            }

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

            if (btn1 && !pbtn1) {
                lcdState = MENU_STATE_PROGRAM;
            }

            break;
        case MENU_STATE_BLUETOOTH_CONNECTION:
            if (btn1 && !pbtn1) {
                lcdState = MENU_STATE_MAIN;
            }

            lcd.setCursor(0, 0);
            lcd.print("Reading         ");
            lcd.setCursor(0, 1);
            lcd.print("Bluetooth Input ");

            bluetoothReadAction();
            break;
    }
}

static uint8_t btState = 0;
void bluetoothReadAction()
{
    while (Serial.available()) {
        zn = Serial.read();
        fifo_push(btFifo, zn);
    }

    if (NULL == zn) {
        return;
    }

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

    delay(500);
    zn = NULL;
}

void readSensorsAction()
{
    if ((millis() - sensorReadTim) >= SENSOR_READ_INTERVAL) {
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();
        sensorReadTim = millis();
    }
}

void loop()
{
    btn1 = digitalRead(PIN_BTN1);
    btn2 = digitalRead(PIN_BTN2);

    menuAction();
    readSensorsAction();

    pbtn1 = btn1;
    pbtn2 = btn2;
}