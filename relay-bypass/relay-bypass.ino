#include <avr/io.h>
#include <util/delay.h>
#include <avr/eeprom.h>

// By default we assume normally open unless this is defined
//#define NORMALLY_CLOSED

// If you define SIMPLIFIED_DEBOUNCE it will trust the
// first instance of a button press, then ignore subsequent
// presses for the SIMPLIFIED_DEBOUNCE_DELAY.
// Otherwise if this is not defined it will do a more traditional
// debounce that looks for a stabilized change of state.
#define SIMPLIFIED_DEBOUNCE

#ifdef SIMPLIFIED_DEBOUNCE
# define DEBOUNCE_DELAY 250
#else
# define DEBOUNCE_DELAY 14
#endif

// Define DISABLE_TEMPORARY_SWITCH to turn off the ability to hold
// the switch to temporarily engage/bypass.
//#define DISABLE_TEMPORARY_SWITCH

#define PIN_J1     PB2
#define PIN_SW     PB3
#define PIN_LED    PB4
#define PIN_BYPASS PB0
#define PIN_MUTE   PB1

// If you hold down the switch for at least this number of ms, then
// we assume you are holding it down to temporarily toggle the state,
// and want it to toggle back on release.
// i.e. it's not a quick press and release.
#define TEMPORARY_SWITCH_TIME 350

/* If J1 is shorted, pulling PIN_J1 to ground, we will do muting:
 * 1. When switch is pressed, start muting for MUTE_LENGTH ms.
 * 2. Wait MUTED_RELAY_DELAY before actually toggling the relay though.
 */
#define MUTE_LENGTH 35
#define MUTED_RELAY_DELAY 20


/* TBD: If space requires we can consolidate these state vars into a single byte */
uint8_t is_bypassed  = 0;
uint8_t use_mute     = 0;
uint8_t sw_state     = HIGH; // its debounced state. i.e. what we assume is intended by player
uint8_t sw_last_loop = HIGH; // state last loop

unsigned long sw_stable_since = 0;
unsigned long sw_pressed_at   = 0;


void setup() {
  DDRB &= ~(1 << PIN_J1); // input
  PORTB |= (1 << PIN_J1); // activate pull-up resistor
  DDRB &= ~(1 << PIN_SW); // input
  PORTB |= (1 << PIN_SW); // activate pull-up resistor
  DDRB |= (1 << PIN_BYPASS); // output
  DDRB |= (1 << PIN_LED); // output
  DDRB |= (1 << PIN_MUTE); // output


  // wait for the pullup to do its job,
  // otherwise PIN_SW can sometimes get false LOW readings
  _delay_ms(5);
  sw_last_loop = read_switch();
  sw_state = sw_last_loop;

  uint8_t auto_on = 0x0;

  bool signal_eeprom_written = false;

  if (eeprom_is_ready()) {
    auto_on = eeprom_read_byte((uint8_t *)0x0);

    if (auto_on != 0x0 && auto_on != 0x1) { // never written, probably 0xff
      auto_on = 0x0;
    }

    // If switch is held when powered on, toggle the auto-on feature
    if (sw_last_loop == LOW) {
      is_bypassed = ! is_bypassed;
      auto_on ^= 0x1;
      eeprom_update_byte((uint8_t *)0x0, auto_on);
      signal_eeprom_written = true;
    }

    is_bypassed = auto_on;
  }

  uint8_t j1 = PINB & (1 << PIN_J1);
  if (j1 == LOW) {
    use_mute = 1;
  }

  write_bypass(); // get the relay setup before blinking the LED for EEPROM
  _delay_ms(1);

  if (signal_eeprom_written) {
    // Blink LED. Ordinarily this should be done asynchronously,
    // but on initial boot it probably doesn't matter
    for (int i = 0; i < 3; i++) {
      PORTB |= (1 << PIN_LED); //high
      _delay_ms(200);
      PORTB &= ~(1 << PIN_LED); //low
      _delay_ms(200);
    }
    write_bypass(); // set the LED again
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

    if (sw_state == LOW) {// sw pressed
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
}

void write_bypass() {
  if (is_bypassed) {
    PORTB &= ~(1 << PIN_LED); // low
    PORTB &= ~(1 << PIN_BYPASS); // low
  } else {
    PORTB |= (1 << PIN_LED); // high
    PORTB |= (1 << PIN_BYPASS); // high
  }
}

uint8_t read_switch() {
  uint8_t rv = PINB & (1 << PIN_SW);
#ifdef NORMALLY_CLOSED
  rv = ! rv;
#endif
  return rv;
}

