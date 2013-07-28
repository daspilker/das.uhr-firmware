#
#   Copyright 2012 Daniel A. Spilker
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

DEVICE    = atmega88pa
CLOCK     = 8000000
VERSION   = \"1.0\"
PORT      = USB
FUSES     = DFA2
EXT_FUSES = F9

SOURCES   = src/main.c src/dcf77.c src/gamma.c src/matrix.c src/rtc.c src/uart.c
OBJECTS   = $(SOURCES:.c=.o)

CFLAGS    = -Wall -O2 -mmcu=$(DEVICE) -std=c99
CPPFLAGS  = -DF_CPU=$(CLOCK) -DVERSION=$(VERSION)
LDFLAGS   = -Wl,-u,vfscanf -lscanf_min -lm
CC        = avr-gcc

ifeq ($(OS), Windows_NT)
	SHELL = C:/Windows/System32/cmd.exe
endif

.PHONY: all
all: main.hex

-include $(SOURCES:.c=.d)

.PHONY: flash
flash: main.hex
	stk500 -d$(DEVICE) -c$(PORT) -e -ifmain.hex -pf -vf

.PHONY: fuse
fuse:
	stk500 -d$(DEVICE) -c$(PORT) -f$(FUSES) -F$(FUSES) -E$(EXT_FUSES) -G$(EXT_FUSES)

.PHONY: clean
clean:
	rm -f main.hex main.elf $(OBJECTS) $(SOURCES:.c=.d)

.PHONY: size
size: main.elf
	avr-size --format=avr --mcu=$(DEVICE) main.elf

main.elf: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o main.elf $(OBJECTS)

main.hex: main.elf
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex

%.d: %.c
	@set -e; $(CC) -MM $(CPPFLAGS) $< -o $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' $@.$$$$ > $@; \
	rm -f $@.$$$$
