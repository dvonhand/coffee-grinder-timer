OBJECTS=main.o

AVRDUDE=avrdude
AVRDUDEFLAGS=-c arduino -p ATMEGA328P -P $(DEVICE) -b 115200 -U flash:w:$<
CC=avr-gcc
CFLAGS=-mmcu=atmega328p -Os
CPPFLAGS=-DF_CPU=16000000UL
DEPFLAGS=-MT $@ -MD -MP
DEVICE=/dev/ttyACM0
LDFLAGS=
OBJCOPY=avr-objcopy
RM=rm

.SECONDARY:

.PHONY: all
all: main.hex

.PHONY: clean
clean:
	$(RM) -f *.d *.E *.elf *.hex *.o *.s

.PHONY: install
install: main.hex
	$(AVRDUDE) $(AVRDUDEFLAGS)

%.E: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -E -o $@ $<

%.s: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -S -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) $(DEPFLAGS) -c -o $@ $<

%.elf: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.hex: %.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

DEPFILES := $(OBJECTS:%.o=%.d)
$(DEPFILES):
include $(wildcard $(DEPFILES))
