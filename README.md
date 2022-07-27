# Relay Bypass for DIY Guitar Pedals

Visit <https://mas-effects.com/relay-bypass> to purchase pre-made PCBs and pre-programmed AVR ICs.

Build documents here: <https://mas-effects.com/relay.pdf>

This repository contains all you need to build your own relay bypass PCB,
or hack at the one I provide.

This is a true-bypass module that provides:

* Soft touch, momentary switches can toggle your DIY guitar pedal, using a relay to provide true bypass.
* Quick tap to switch between engaged and bypass, or hold the footswitch down to temporarily toggle until you release it
* Fits easily in a 1590A
* Optional optocoupler muting can be enabled with a switch or jumper to mute any popping your circuit might exhibit while switching
* Hold when powering on to toggle auto-on/auto-off
* VERY cost effective. [Shop now](https://shop.mas-effects.com/collections/diy/products/relay-bypass).
* Totally free and open source

## TODO

This version doesn't save your current state to the EEPROM. Instead it lets you set auto-on or auto-off. That's fine for many people, but probably not ideal.

I have a version in another codebase ([for The Expanse](https://mas-effects.com/expanse/)) that does this, and utilizes wear leveling to give you a MUCH longer practical lifetime (as opposed to just storing the state in the same memory location repeatedly).  I will adapt it for the next version, but in the meantime if you want it let me know (mark@mas-effects.com) and I will send you some code snippets.

## Build Instructions

This is a pretty quick and easy build. All of the surface mount (SMD) components come pre-soldered. You simply need to add the relay, optocoupler (optional), IC, voltage regulator, and capacitor.

Mount a normally-open, momentary SPST switch on the PCB next to the relay using double-sided foam tape.

## Optocoupler Muting

If you connect the 2 pads labeled J1 with a jumper or a switch, this will tell the IC to momentarily mute (35ms) the signal while switching. This can be useful if you have a circuit that pops when engaged.

## Multiple LEDs

By default you can connect the LED pad to the anode of an LED (in series with a current-limiting resistor) to indicate status.

If you want to use multiple LEDs or need more current than the voltage regulator can provide, then simply use the 5V signal (with an appropriately sized resistor) to drive a transistor. The size of the resistor should limit the base current so the collector current is about 15x greater.

## AVR Microcontroller 

An AVR microcontroller is used at the heart of this module.  You can use either an ATtiny13 or ATtiny85.  The code for this is in the [relay-bypass folder](./relay-bypass/relay-bypass.ino).  You'll find there are a handful of `#define` statements to allow you to easily change its behavior.

## Versions:

1.4 is the latest version in the root of this repository. It uses a latching relay for slightly reduced power consumption.

1.3 also uses a latching relay, but is not as DIY-friendly if you scratch-build (from a kit, however, it's exactly the same as V4). It requires you to write a fuse to disable the RESET pin and turn it into a GPIO pin. After doing this you can't re-program the ATtiny without special equipment. i.e. if you're not careful you may waste some microcontrollers.

1.2 has all the same features as later versions, but uses a non-latching relay instead.

