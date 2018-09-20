
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
