# variables
TARGET = 318LPW1K-L
EEPROM = eeprom
PIC = 12f629
# software verion used:
# pk2cmd: 1.21
# sdcc: 3.4.0
# gputils: 1.3.0

all: off flash on

# flash program
flash: $(TARGET).hex
	pk2cmd -PPIC$(PIC) -F$< -M

# flash custom megacode in EEPROM
eeprom: $(EEPROM).hex
	pk2cmd -PPIC$(PIC) -F$< -ME

# power on remote
on:
	pk2cmd -PPIC$(PIC) -T

# power off remote
off:
	pk2cmd -PPIC$(PIC) -V0

# compile program and EEPROm
compile: $(TARGET).hex $(EEPROM).hex

# compile program C source code (includes a default code)
$(TARGET).hex: $(TARGET).c
	sdcc --std-c99 --opt-code-size --use-non-free -mpic14 -p$(PIC) $<

# compile eeprom for custom megacode
$(EEPROM).hex: $(EEPROM).asm
	gpasm -o $(EEPROM).o -c $<
	gplink -w -r -o $(EEPROM) $(EEPROM).o

# remove temporary files
clean:
	rm -f $(TARGET).hex $(TARGET).lst $(TARGET).asm $(TARGET).adb $(TARGET).o $(TARGET).cod $(EEPROM).hex $(EEPROM).cod $(EEPROM).lst $(EEPROM).o
