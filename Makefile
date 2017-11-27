TARG = main
CC = avr-gcc
OBJCOPY = avr-objcopy
SRCS = main.c i2cmaster.c
OBJS = $(SRCS:.c=.o)
MCU = atmega328p

# Flags for compiler
CFLAGS = -mmcu=$(MCU) -Wall -g -Os -lm  -mcall-prologues -std=c99
LDFLAGS = -mmcu=$(MCU) -Wall -g -Os

all: $(TARG)

$(TARG): $(OBJS)
	$(CC) $(LDFLAGS) -o $@.elf  $(OBJS) -lm
	$(OBJCOPY) -O ihex -R .eeprom -R .nwram  $@.elf $@.hex

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(SRCS:.c=.elf) $(OBJS)

load:
	avrdude -c usbasp -p $(MCU) -U flash:w:$(TARG).hex