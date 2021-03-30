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

## V1.1 vs V1.2 vs V1.3

V3 (1.3) is the latest version in the root of this repository. It uses a latching relay. It's not as DIY-friendly as V1 and V2 since it requires you to write a fuse to disable the RESET pin and turn it into a GPIO pin. After doing this you can't re-program the ATtiny without special equipment.

V1 and V2 have all the same features, but use a non-latching relay instead.

V1 (green PCBs) are just as capable as the V2 PCBs (black), but they aren't as convenient. As such I'm selling them at a deep discount. It's a great way to stock up on some budget relay boards.

Here are the specific differences:

* V2 moved the SW1 and SW2 pads directly under the solder lugs for most SPDT momentary switches
* V1 doesn't fit as neatly between the screw holes of a 1590A. It can fit a 1590A but needs to be offset from the switch
* V1 has no LED solder pad, but you can get that functionality by soldering a wire to Pin 3 of U2 or its socket.
* V1 has 2 extra components (R3, C4) that should be omitted. These are already removed from the PCB if you ordered from me.
