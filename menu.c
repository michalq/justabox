
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
