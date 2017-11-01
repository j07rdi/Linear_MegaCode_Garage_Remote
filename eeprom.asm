; define microcontroller
	list p=12f629
; set custom megacode in EEPROM
; megacode is 3 bytes long
; end with 0x00 to indicate the end
	org 2100h ; EEPROM memory address
	DE 0xc9, 0x17, 0xc2, 0x00 ; megacode
	end
