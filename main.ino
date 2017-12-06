#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "string.h"
#include "stdarg.h"

#define DHTPIN 9
#define DHTTYPE DHT11

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

#define BT_COMMAND_LISTEN    255

#define BT_STATE_MAIN         0
#define BT_STATE_HEAT_PROGRAM 1
// Menu
#define MENU_BLINK_TIM 500
#define MENU_CHANGE_PROGRAM_TIM_THRESHOLD 2000

#define MENU_STATE_MAIN              0
#define MENU_STATE_LIMITS            1
#define MENU_STATE_PROGRAM           2
#define MENU_STATE_SET_PROGRAM_ENTER 3
#define MENU_STATE_SET_PROGRAM       4

static uint32_t menuBlinkTimStatus;
static uint32_t menuBlinkTimDiff;
static uint32_t settingProgramCounter = 0;

// Heating program
static uint8_t heatingProgram = 1;
static uint8_t programReadingState = 0;
static struct HeatingProgram heatingPrograms[5];

#define HEATING_PROGRAM_STATE_READ_ID 0
#define HEATING_PROGRAM_STATE_READ_NAME 1
#define HEATING_PROGRAM_STATE_READ_PROGRAM 2
#define HEATING_PROGRAM_STATE_READ_END 3

typedef struct HeatingProgram {
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
} HeatingProgram;

uint8_t HeatingProgram_ReadProgram(HeatingProgram* program, uint8_t payload)
{
    switch (programReadingState) {
        case HEATING_PROGRAM_STATE_READ_NAME:
            // Zero means stop recording.
            if (0 == payload) {
                programReadingState = HEATING_PROGRAM_STATE_READ_PROGRAM;
                return 1;
            }

            if (!HeatingProgramValidator_Name(payload, program->length)) {
                return 0;
            }

            program->name[program->length++] = payload;
            break;
        case HEATING_PROGRAM_STATE_READ_PROGRAM:
            // Zero means stop recording.
            if (0 == payload) {
                programReadingState = HEATING_PROGRAM_STATE_READ_END;
                return 1;
            }

            switch (program->length % 3) {
                case 0:
                    /// Reading day
                    if (!HeatingProgramValidator_Day(payload)) {
                        // @TODO send info about error.
                        return 0;
                    }

                    program->payload[program->length++] = payload;
                    break;
                case 1:
                    /// Reading hour from
                case 2:
                    /// Reading hour to
                    if (!HeatingProgramValidator_Hour(payload)) {
                        // @TODO send info about error.
                        return 0;
                    }

                    program->payload[program->length++] = payload;
                    break;
                case 3:
                    /// Reading temperature
                    if (!HeatingProgramValidator_Temperature(payload)) {
                        // @TODO send info about error.
                        return 0;
                    }

                    program->payload[program->length++] = payload;
                    break;
            }

            break;
    }

    return 1;
}
uint8_t HeatingProgramValidator_Name(uint8_t tmpChar, uint8_t length)
{
    if (length > 8) {
        return 0;
    }

    if ((tmpChar >= 69 && tmpChar <= 90) || (tmpChar >= 97 && tmpChar <= 122) || 32 == tmpChar) {
        return 1;
    }

    return 0;
}
uint8_t HeatingProgramValidator_Day(uint8_t day)
{
    if (day >= 0 && day <= 6) {
        return 1;
    }

    return 0;
}
uint8_t HeatingProgramValidator_Hour(uint8_t hour)
{
    if (hour >= 0 && hour <= 23) {
        return 1;
    }

    return 0;
}
uint8_t HeatingProgramValidator_Temperature(uint8_t temperature)
{
    if (temperature >= 0 && temperature <= 100) {
        return 1;
    }

    return 0;
}
uint8_t HeatingProgramValidator_Id(uint8_t id)
{
    if (id >= 0 && id <= 4) {
        return 1;
    }

    return 0;
}

void setup ()
{
    Serial.begin(9600);

    pinMode(PIN_BTN1, INPUT);
    pinMode(PIN_BTN2, INPUT);

    dht.begin();
    lcd.begin(16, 2);
    lcd.backlight();
}

void menu()
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
                lcdState = MENU_STATE_MAIN;
            }

            // Hold btn2 to enter change program mode.
            if (btn2 && !pbtn2) {
                settingProgramCounter = millis();
                lcdState = MENU_STATE_SET_PROGRAM_ENTER;
            }

            break;
        case MENU_STATE_SET_PROGRAM_ENTER:
            if (!btn2) {
                lcdState = MENU_STATE_PROGRAM;
            }

            if (millis() - settingProgramCounter > MENU_CHANGE_PROGRAM_TIM_THRESHOLD) {
                lcdState = MENU_STATE_SET_PROGRAM;
            }

            break;
        case MENU_STATE_SET_PROGRAM:
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
            lcd.setCursor(0, 0);
            lcd.print("Bluetooth       ");
            lcd.setCursor(0, 1);
            lcd.print("ON              ");
            break;
    }
}

static uint8_t btState = 0;
void bluetoothRead()
{
    if (!Serial.available()) {
        return;
    }

    zn = Serial.read();
    switch (btState) {
        default:
        case BT_STATE_MAIN:
            switch (zn) {
                case BT_STATE_HEAT_PROGRAM:
                    btState = BT_STATE_HEAT_PROGRAM;
                return;
            }

            break;
        case BT_STATE_HEAT_PROGRAM:
            if (zn == BT_COMMAND_LISTEN) {
                btState = BT_STATE_MAIN;
                return;
            }

            if (HEATING_PROGRAM_STATE_READ_ID == programReadingState) {
                if (!HeatingProgramValidator_Id(zn)) {
                    // @TODO send serial with error.
                    btState = BT_STATE_MAIN;
                }

                currentProgrammedProgram = zn;
                heatingPrograms[currentProgrammedProgram]->id = currentProgrammedProgram;
                programReadingState = HEATING_PROGRAM_STATE_READ_NAME;
            } else if (HEATING_PROGRAM_STATE_READ_END == programReadingState) {
                // @TODO send info about success.
                btState = BT_STATE_MAIN;
                programReadingState = HEATING_PROGRAM_STATE_READ_ID;
            } else {
                HeatingProgram_ReadProgram(heatingPrograms[currentProgrammedProgram], zn);
            }

            break;
    }
}

void readSensors()
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

    menu();
    readSensors();

    pbtn1 = btn1;
    pbtn2 = btn2;
}