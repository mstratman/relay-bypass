// This is v1.49 - for use with the v1.4 PCB.
// It adds memory for the most recent engage/bypass setting.
// It also disables the muting since we're sold out of optocouplers
// and they've been discontinued.

#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

//------------------------------------------------
// BASIC BEHAVIOR SETTINGS:

// which uC
//#define ATTINY85
#define ATTINY13

// By default we assume normally open unless this is defined
//#define NORMALLY_CLOSED

// If you define SIMPLIFIED_DEBOUNCE it will trust the
// first instance of a button press, then ignore subsequent
// presses for the SIMPLIFIED_DEBOUNCE_DELAY.
// Otherwise if this is not defined it will do a more traditional
// debounce that looks for a stabilized change of state.
#define SIMPLIFIED_DEBOUNCE

// Define DISABLE_TEMPORARY_SWITCH to turn off the ability to hold
// the switch to temporarily engage/bypass.
//#define DISABLE_TEMPORARY_SWITCH

#define USE_MUTE 0
//------------------------------------------------



#ifdef ATTINY85
# define EEPROM_SIZE 512
#else
# define EEPROM_SIZE 64
#endif

#define EEPROM_CHUNK_SIZE 1 // we're choosing to use 1 byte
#define EEPROM_ADDR_FLAG_MASK 0b10000000  // chosen to indicate current addr
#define ADDR_BOOT_SETTING   0x0
#define ADDR_BYPASS_SETTING 0x1 // starting location to look
uint16_t eeprom_addr = ADDR_BYPASS_SETTING;

// These will also indicate the number of LED blinks
#define ON_BOOT_ENGAGE   0x3
#define ON_BOOT_BYPASS   0x4
#define ON_BOOT_REMEMBER 0x5

#ifdef SIMPLIFIED_DEBOUNCE
# define DEBOUNCE_DELAY 250
#else
# define DEBOUNCE_DELAY 14
#endif

#define PIN_SW     PB2
#define PIN_LED    PB4
#define PIN_MUTE   PB1
#define PIN_BYPASS PB0
#define PIN_ENGAGE PB3

// If you hold down the switch for at least this number of ms, then
// we assume you are holding it down to temporarily toggle the state,
// and want it to toggle back on release.
// i.e. it's not a quick press and release.
#define TEMPORARY_SWITCH_TIME 500

/* 1. When switch is pressed, start muting for MUTE_LENGTH ms.
 * 2. Wait MUTED_RELAY_DELAY before actually toggling the relay though.
 * IMPORTANT NOTE: To disable the physical muting, leave the optocoupler diode floating.
 *                 i.e. don't connect the MUTE jumpers on the PCB
 */
#define MUTE_LENGTH 35
#define MUTED_RELAY_DELAY 20

#define RELAY_SWITCH_TIME 40



/* TBD: If space requires we can consolidate these state vars into a single byte */
uint8_t is_bypassed  = 0;
uint8_t use_mute     = USE_MUTE;
uint8_t sw_state;     // its debounced state. i.e. what we assume is intended by player
uint8_t sw_last_loop; // state last loop

unsigned long sw_stable_since = 0;
unsigned long sw_pressed_at   = 0;


uint8_t use_eeprom = 0;

void setup() {
  DDRB &= ~(1 << PIN_SW); // input
  PORTB |= (1 << PIN_SW); // activate pull-up resistor
  DDRB |= (1 << PIN_BYPASS); // output
  DDRB |= (1 << PIN_ENGAGE); // output
  DDRB |= (1 << PIN_LED); // output
  DDRB |= (1 << PIN_MUTE); // output


  // wait for the pullup to do its job,
  // otherwise PIN_SW can sometimes get false LOW readings
  _delay_ms(5);
  sw_last_loop = read_switch();
  sw_state = sw_last_loop;

  uint8_t blink_n_times = 0;

  if (eeprom_is_ready()) {
    uint8_t on_boot_setting = eeprom_read_byte((const uint8_t *)ADDR_BOOT_SETTING);

    if (on_boot_setting == ON_BOOT_ENGAGE) {
      is_bypassed = 0;

    } else if (on_boot_setting == ON_BOOT_BYPASS) {
      is_bypassed = 1;

    // If this hasn't been set (probably 0xff), we'll always assume it's ON_BOOT_REMEMBER
    } else {
      on_boot_setting = ON_BOOT_REMEMBER;
      is_bypassed = eeprom_read_is_bypassed();
    }

    // If switch is held when powered on, toggle the on_boot setting
    if (sw_last_loop == 0) {
      if (on_boot_setting == ON_BOOT_REMEMBER) {
        on_boot_setting = ON_BOOT_ENGAGE;
      } else if (on_boot_setting == ON_BOOT_ENGAGE) {
        on_boot_setting = ON_BOOT_BYPASS;
      } else {
        on_boot_setting = ON_BOOT_REMEMBER;
        // More important than getting the is_bypassed setting is
        // setting the eeprom_addr, which this call does.
        is_bypassed = eeprom_read_is_bypassed();
      }
      eeprom_update_byte((uint8_t *)ADDR_BOOT_SETTING, on_boot_setting);
      blink_n_times = on_boot_setting;
    }

    if (on_boot_setting == ON_BOOT_REMEMBER) {
      use_eeprom = 1;
    }
  }

  write_bypass(); // get the relay setup before blinking the LED for EEPROM
  _delay_ms(1);

  if (blink_n_times > 0) {
    // Blink LED. Ordinarily this should be done asynchronously,
    // but on initial boot it probably doesn't matter
    for (int i = 1; i < blink_n_times; i++) {
      PORTB |= (1 << PIN_LED); //high
      _delay_ms(200);
      PORTB &= ~(1 << PIN_LED); //low
      _delay_ms(200);
    }
    _delay_ms(1000);
    set_led(); // set the LED again
  }
}

void loop() {
  uint8_t sw_this_loop = read_switch();

  unsigned long now = millis();

#ifndef SIMPLIFIED_DEBOUNCE
  // If still unstable, reset the clock
  if (sw_this_loop != sw_last_loop) {
    sw_stable_since = now;
  }
#endif

  // has sw state has changed?
  // Is switch state stable?
  if ((sw_this_loop != sw_state) 
      && (now - sw_stable_since) > DEBOUNCE_DELAY)
  {
#ifdef SIMPLIFIED_DEBOUNCE
    sw_stable_since = now;
#endif
    sw_state = sw_this_loop;

    if (sw_state == 0) {// sw pressed
      sw_pressed_at = now;
      toggle_bypass_state();

    // Switch has just been released. Let's decide if it was a long press for temporary engagement/disengagement,
    // or a quick press to toggle its state.
    } else {
#ifndef DISABLE_TEMPORARY_SWITCH
      if ((now - sw_pressed_at) > TEMPORARY_SWITCH_TIME) {
        toggle_bypass_state();
      }
#endif
    }
  }

  sw_last_loop = sw_this_loop;
}

// returns is_bypassed
uint8_t eeprom_read_is_bypassed() {
  // This may not find the flag, so these two defaults are important.
  eeprom_addr = ADDR_BYPASS_SETTING;
  uint8_t rv = 0;

  for (uint16_t i = 0; i < (EEPROM_SIZE / EEPROM_CHUNK_SIZE); i++) {
    uint16_t addr = i * EEPROM_CHUNK_SIZE;

    uint8_t b = eeprom_read_byte((const uint8_t *)addr);
    if (b & EEPROM_ADDR_FLAG_MASK) {
      eeprom_addr = addr;

      rv = b & 0x1; // remove the EEPROM_ADDR_FLAG_MASK bit
      break;
    }
  }
  return rv;
}

// writes is_bypassed
void eeprom_write_is_bypassed(uint8_t setting) {
  // don't unnecessarily wear out the eeprom.
  if (use_eeprom == 0) {
    return;
  }

  eeprom_update_byte((uint8_t *)eeprom_addr, 0x0); // Clear flag on previous location.
                                        // This will also clean up any default data in
                                        // the eeprom that coincidentally had the flag bit set.

  if (eeprom_addr < (EEPROM_SIZE - EEPROM_CHUNK_SIZE)) {
    eeprom_addr += EEPROM_CHUNK_SIZE;
  } else {
    eeprom_addr = ADDR_BYPASS_SETTING;
  }
  eeprom_update_byte((uint8_t *)eeprom_addr, EEPROM_ADDR_FLAG_MASK | setting);
}

// This may block. See comments
void toggle_bypass_state() {
  if (use_mute) {
    // Is there any reason to do this asynchronously?
    // Until it seems there is, let's keep it MUCH simpler.
    PORTB |= (1 << PIN_MUTE); // HIGH - engage mute
    // wait a while for mute to fully engage before toggling relay
    _delay_ms(MUTED_RELAY_DELAY);
  }

  is_bypassed = ! is_bypassed;
  write_bypass();

  if (use_mute) {
    // wait for the rest of the MUTE_LENGTH period
    _delay_ms(MUTE_LENGTH - MUTED_RELAY_DELAY);
    // Unmute by setting PIN_MUTE to low
    PORTB &= ~(1 << PIN_MUTE);

    // since we've been blocking, the switch may have been released.
    sw_state = read_switch();
  }

  eeprom_write_is_bypassed(is_bypassed);
}

void write_bypass() {
  set_led();
  PORTB &= ~(1 << PIN_BYPASS); // low
  PORTB &= ~(1 << PIN_ENGAGE); // low
  // There's a non-zero chance the blocking we do here could
  // interfere with the switch debouncing.
  // But with the simplified debounce it's a non-issue.
  if (is_bypassed) {
    PORTB |= (1 << PIN_BYPASS); // high
    _delay_ms(RELAY_SWITCH_TIME);
    PORTB &= ~(1 << PIN_BYPASS); // low
  } else {
    PORTB |= (1 << PIN_ENGAGE); // high
    _delay_ms(RELAY_SWITCH_TIME);
    PORTB &= ~(1 << PIN_ENGAGE); // low
  }
}

void set_led() {
  if (is_bypassed) {
    PORTB &= ~(1 << PIN_LED); // low
  } else {
    PORTB |= (1 << PIN_LED); // high
  }
}

uint8_t read_switch() {
  uint8_t rv = PINB & (1 << PIN_SW);
#ifdef NORMALLY_CLOSED
  rv = ! rv;
#endif
  return rv;
}
