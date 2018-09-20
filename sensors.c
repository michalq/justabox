
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

