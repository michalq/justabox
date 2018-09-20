
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
