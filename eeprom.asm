; define microcontroller
	list p=12f635
; set custom megacode in EEPROM
; megacode is 3 bytes long
; end with 0x00 to indicate the end
	org 2100h ; EEPROM memory address
	DE 0x55, 0x14, 0xbc, 0x00 ; megacode
	end
