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
#define HEATING_ANIMATION_TIM 20
static uint8_t timHeatingAnimation = 0;
static uint8_t heatingProgram = 1;
// DHT11
static uint8_t temperature;
static uint8_t humidity;
static uint32_t timer;
// LCD
#define LCD_LIGHT_MAX_TIM 200
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

void setup ()
{
    timer = -9999;
    pinMode(PIN_BTN1, INPUT);
    pinMode(PIN_BTN2, INPUT);

    dht.begin();
    lcd.begin(16,2);
    lcd.backlight();
}

void menu()
{
    switch (lcdState) {
        default:
        case 0:
            lcd.setCursor(0, 0);
            lcd.print("Temp      ");
            lcd.print(temperature);
            lcd.print("*C ");
            lcd.print(timHeatingAnimation > HEATING_ANIMATION_TIM ? "_" : " ");
            lcd.setCursor(0, 1);
            lcd.print("Humid     ");
            lcd.print(humidity);
            lcd.print("%  ");
            lcd.print(timHeatingAnimation > HEATING_ANIMATION_TIM ? "_" : " ");

            timHeatingAnimation++;
            if (timHeatingAnimation > 2 * HEATING_ANIMATION_TIM) {
                timHeatingAnimation = 0;
            }

            if (lcdLightTim < LCD_LIGHT_MAX_TIM && btn1 && !pbtn1) {
                lcdState = 2;
            }

            if (lcdLightTim < LCD_LIGHT_MAX_TIM && btn2) {
                lcdState = 1;
            }
            break;
        case 1:
            lcd.setCursor(0, 0);
            lcd.print("Min temp     ");
            lcd.print("22*C     ");
            lcd.setCursor(0, 1);
            lcd.print("Min humid    ");
            lcd.print("65%      ");

            if (lcdLightTim < LCD_LIGHT_MAX_TIM && !btn2) {
                lcdState = 0;
            }
            break;
        case 2:
            lcd.setCursor(0, 0);
            lcd.print("Program         ");
            lcd.setCursor(0, 1);
            lcd.print("       ");
            lcd.print(heatingProgram);
            lcd.print("        ");

            if (lcdLightTim < LCD_LIGHT_MAX_TIM && btn1 && !pbtn1) {
                lcdState = 0;
            }

            if (lcdLightTim < LCD_LIGHT_MAX_TIM && btn2 && !pbtn2) {
                heatingProgram++;
                if (heatingProgram > 5) {
                    heatingProgram = 1;
                }
            }

            break;
    }
}

void lcdLightControl()
{
    if (lcdLightTim < LCD_LIGHT_MAX_TIM) {
        lcdLightTim++;
        lcd.backlight();
    } else {
        lcd.noBacklight();
    }

    if (btn1 || btn2) {
        lcdLightTim = 0;
    }
}

void loop()
{
    btn1 = digitalRead(PIN_BTN1);
    btn2 = digitalRead(PIN_BTN2);
    menu();
    lcdLightControl();

    if ((millis() - timer) >= 3000) {
        humidity = dht.readHumidity();
        temperature = dht.readTemperature();
        timer = millis();
    }
    pbtn1 = btn1;
    pbtn2 = btn2;
}