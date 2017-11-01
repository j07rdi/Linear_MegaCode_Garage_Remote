/* micro-controller firmware fot the Monarch 318LIPW1K(-L) remote
   Copyright (C) 2014 Kévin Redon <kingkevin@cuvoodoo.info>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 */
/* libraries */
#define __12f629
#include <pic12f629.h>
#include <stdint.h>

/* the peripherals connected to the pins */
#define LED_ANODE   _GP0
#define TX          _GP1
#define SWITCH      _GP4
#define LED_CATHODE _GP5

/* simple functions */
// note: LED and TX can not be enabled at the same time
#define led_on() GPIO |= LED_ANODE;
#define led_off() GPIO &= ~LED_ANODE;
#define tx_on() GPIO |= TX;
#define tx_off() GPIO &= ~TX;
#define sleep() __asm sleep __endasm

/* variables */
//volatile uint32_t code = 0; // the code to transmit (read from EEPROM)
volatile int8_t transmit = 0; // transmitting (0: do not transmit, 1: transmit, -1: finish transmiting)
volatile uint8_t phase = 0; // the 4 phases of the bit (2ms pause, 1ms pulse, 2ms pause, 1ms pulse. only transmit during one of the two pulse periode depending on the bit value)

/* configuration bits 
 * all set (to 1) per default
 * clear (to 0) relevant bits
 */
uint16_t __at(_CONFIG) __CONFIG = _CPD_OFF & // no data code protect
                                  _CP_OFF & // no code proect
                                  _BODEN_OFF & // no brown out detect
                                  _MCLRE_OFF & // disable master clear reset
                                  _PWRTE_ON & // enable power-up timeout
                                  _WDTE_OFF & // no watchdog
                                  _INTRC_OSC_NOCLKOUT; // use internal oscilator and both I/O pins

/* default code to transmit in EEPROM
   a megacode is 3 bytes long (MSB of byte 1 is 1)
   append 0x00 to indicate end
*/
// 0x2100 comes from the PIC12F629 Memory Programming document
__code uint8_t __at(0x2100) EEPROM[] = {0xc9, 0x17, 0xc2, 0x00};

void timer_1ms() {
  TMR1ON = 0; // disable timer 1 (to write value safely)
  TMR1L = 0x6e; // set time (tuned per hand)
  TMR1H = 0xff; // set time
  TMR1ON = 1; // start timer 1
}

void timer_2ms() {
  TMR1ON = 0; // disable timer 1 (to write value safely)
  TMR1L = 0x22; // set time (tunef per hand)
  TMR1H = 0xff; // set time
  TMR1ON = 1; // start timer 1
}

/* read EEPROM data */
uint8_t read_eeprom(uint8_t address) {
  EEADR = address;
  RD = 1;
  return EEDATA;
}

/* transmit the megacode */
void megacode (void) {
  static uint8_t byte;
  uint8_t bit = phase/4;
  if (transmit != 0) {
    if (bit%8==0) { // read byte to transmit
      byte = read_eeprom(bit/8);
    }
    if (bit<24) { // transmit bit
      if (phase%2) {
        uint8_t pulse = (byte>>((23-bit)%8))&0x01;
        if ((phase%4==1 && !pulse) || (phase%4==3 && pulse)) {
          led_off();
          tx_on();
        }
        timer_1ms();
      } else {
        tx_off();
        led_on();
        timer_2ms();
      }
    } else if (bit==24) { // 25th bit is a blank
      led_on();
      tx_off();
      timer_2ms();
    } else { // restart after 25th bit
      phase = 0xff; // phase will be 0 after incrementing
      if (transmit == -1) { // stop transmiting if requested
        led_off();
        transmit = 0; // stop transmiting
      }
      timer_1ms();
    }
    phase++; // go to next phase
  }
}

/* initialize micro-conroller */
void init (void) {
  TRISIO |= SWITCH; // switch is input
  WPU |= SWITCH; // enable pull-up on switch
  NOT_GPPU = 0; // enable global weak pull-up for inputs
  TRISIO &= ~(LED_ANODE|LED_CATHODE|TX); // all other are outputs
  GPIO &= ~(LED_ANODE|LED_CATHODE|TX); // clear output
}

// funcion called on interrupts
// interrupt 0 is only one on PIC12
static void interrupt(void) __interrupt 0
{
  if (GPIF) { // pin state changed
    if (GPIO&SWITCH) { // switch is released stop transmission
      if (transmit == 1) {
        transmit = -1; // stop transmitting after last transmition
      }
    } else { // switch is pressed, start transmission
      if (transmit == 0) { // only transmit if code is loaded
        transmit = 1;
        phase = 0;
        megacode(); // start sending megacode
      }
    }
    GPIF = 0; // clear interrupt
  }
  if (TMR1IF) { // timer 1 overflow
    megacode(); // continue sending megacode
    TMR1IF = 0; // clean interrupt
  }

}

void main (void)
{
  init(); // configure micro-controller

  IOC |= SWITCH; // enable interrupt for the switch
  GPIE = 1; // enable interrupt on GPIO

  T1CON = _T1CKPS1 | _T1CKPS0; // set prescaler to 8
  // 0 is per default
  //TMR1ON = 0; // stop timer 1
  //TMR1GE = 0; // enable timer 1
  //TMR1CS = 0; // use internal clock / 4 (=1MHz)
  //TMR1IF = 0; // clear interrupt
  PIE1 = 1; // enable timer 1 interrupt
  PEIE = 1; // enable timer interrupt

  if (read_eeprom(0)&0x80) { // verify if the code is programm (the MSB bit is always 1 for megacode codes)
    GIE = 1; // globally enable interrupts
  } else { // show error  and got to sleep forever
    led_on();
  }

  while (1) {
    if (transmit == 0) {
      sleep(); // sleep to save power (this will shut down timer 1)
    }
  }
}
