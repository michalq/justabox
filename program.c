#if 0
// Heating program
static uint8_t heatingProgram = 1;
// Loaded programs.
static Program* heatingPrograms[1];
#endif

/// Error codes of program reader.
#define ERR_INVALID_PROGRAM 1
#define ERR_INVALID_ID      2
#define ERR_INVALID_NAME    3
#define ERR_INVALID_DAY     4
#define ERR_INVALID_HOUR    5
#define ERR_INVALID_TEMP    6
#define ERR_INVALID_CRC     7

/// States of loading heating program.
#define HEATING_PROGRAM_STATE_READ_ID      0
#define HEATING_PROGRAM_STATE_READ_NAME    1
#define HEATING_PROGRAM_STATE_READ_PROGRAM 2
#define HEATING_PROGRAM_STATE_READ_CRC     3
#define HEATING_PROGRAM_STATE_READ_END     4

/**
 * Heating program entity.
 */
typedef struct Program {
    // Assumptions: per day max 5 settings,
    // max size of one program will be 5 (max settings) * 7 (days) * 4 (bytes per line) = 140 (bytes)
    uint8_t payload[140];
    // Lengths indicates bytes amount
    uint8_t length = 0;
    // Max 8 chars per name
    // allowed chars a-zA-Z and space, in ascii 69-90, 97-122, 32
    char name[16];
    uint8_t nameLength = 0;
    uint8_t id = 0;
} Program;

/**
 * Validates user input.
 */
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

/// Manages programming heating programs.
class ServiceProgram {
    uint8_t error;
    Program** programsContainer;
public:
    /** Currently programmed program. */
    Program* program;
    /** State of writer. */
    uint8_t state;

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
            if (day != currDay) {
                continue;
            }
            hourFrom = this->program->payload[i + 1];
            if (currHour < hourFrom) {
                continue;
            }
            hourTo = this->program->payload[i + 2];
            if (currHour > hourTo) {
                continue;
            }
            temp = this->program->payload[i + 3];
            break;
        }

        return temp;
    }

    uint8_t calculateCrc()
    {
        return 1;
    }

    /**
     * Resets to initial state.
     */
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
    /**
     * Reads another byte of data.
     */
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
