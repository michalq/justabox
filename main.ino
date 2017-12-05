#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "string.h"
#include "stdarg.h"

#define DHTPIN 9
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);

// Heating program
static uint8_t heatingProgram = 1;
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

#define MENU_STATE_MAIN              0
#define MENU_STATE_LIMITS            1
#define MENU_STATE_PROGRAM           2
#define MENU_STATE_SET_PROGRAM_ENTER 3
#define MENU_STATE_SET_PROGRAM       4

static uint32_t menuBlinkTimStatus;
static uint32_t menuBlinkTimDiff;
static uint32_t settingProgramCounter = 0;

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

            if (1 == index) {
                txt[index++] = zn;
                txt[index] = '\0';
            }

            break;
    }

    if (strcmp(txt, "t1") == 0) {
        //
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