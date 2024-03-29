# THE PROJECT
An arduino-based, features-loaded Sim Racing button box interface.

# FEATURES
- multi HID emulation: keyboard, joystick and mouse in the same device
- up to 24 digital inputs, plus three auxiliary buttons
- up to 2 analog joysticks (4 analog axis)
- up to 24 LEDs (user definable voltage)
- up to 3 rotary encoders
- built-in LEDs driver circuits
- stand-alone: no additional software needed (keeps the CPU happy!)
- multi platform (works on any OS)
- open-firmware

# PCB LAYOUT & CIRCUITS
## Pin Assignment
I want my button box to host at least one analog joystick for mouse emulation and at least two rotary encoders. All the other elements are momentary or latching switches/buttons/levers.

To increase the interface flexibility, two analog joysticks are actually supported which means four analog axis in total. Arduino's analog GPIOs used are A0, A1 and A2, A3.

Rotary encoders call for two digital GPIOs each. In my experience rotary encoders work best with interrupt routines, so arduino pro-micro's pins 0, 1, 2, 3 and 7 are the candidates. Three rotaries are here supported, with interrupt routines assigned to pins 2, 3 and 7.

To receive-data-from and send-data-to shift registers I have candidated the use of Serial Peripheral Interface (SPI) protocol. This reserves pins 16 (MOSI), 14 (MISO) and 15 (SCLK), PLUS two latch pins, one for inputs, the other for outputs.
## Integrated Circuits
This PCB is intended to be feature-loaded but "generic", meaning "also usable in button box configurations different from mine". This is why I extended the number of inputs to 24 even if I used only a few in my box. The standard detection limit of joystick buttons in most softwares is 32, so 8 inputs are left for rotary encoders (one for every direction, so two each) and auxiliary switches. Rotaries (buffered, see next) and auxiliaries are directly handled by the microcontroller through dedicated GPIOs, which spares a shift register to the total count.
Rotative encoders signals are buffered via a 74HC14N Hex Inverting Schmitt trigger. This cleans the signal and makes it easier for the microcontroller to detect state changes. In my experience (mostly in the arcade world) this solution makes a world of difference.
The maximum number of outputs is 24. Even if LEDs absorb a limited current, shift registers are not intended to drive them directly. This means that a driver circuit is in the need. Three ULN2803 high-voltage, high-current Darlington arrays each containing eight open collector Darlington pairs with common emitters are here used to drive LEDs.
As said, if the number of LEDs is limited, one could juice them with USB power, but what if the number of LEDs increse? Again, what if some of those LEDs are actually +12V LEDs and not +5V?I was open to no compromises here (as much as possible), then I ended laying out a tunable driver circuit for LEDs. In other words, the user can select to power any of the Darlinghton arrays from USB or an external source (+12V MAX!), independently. The power source of any Darlinghton array is independend from the other two, leaving the user the choice. In other words, one could power two Darlington's with USB and one with +12V from an external source, or all with external +5V, as a function of the nature and number of buttons in use.
A dedicated LED shows the presence of external power.
## Layout
This PCB is intended to be placed somewhere on box "bottom" and wired to elements through cables. I preferred this approach to the "elements on the PCB" one because:
(a) leaves more elements choice (you are not forced to use those very specific elements matching the PCB footprint)
(b) limits PCB dimensions
Inputs and outputs have dedicated pads for direct soldering in exclusive PCB's areas with due labelling for easier identification:
B01-B24 are generic inputs. Buttons are obvious here, but any switch-based device would work such as a shifter, pedal, hand brake and so on. These are handled through the input shift registers
J1X, J1Y and J2X, J2Y are analog inputs. Any of these correspond to a single analog axis, then two joysticks in total. These are directly handled by the microcontroller board.
R1A, R1B, R2A, R2B and R3A, R3B are the three couples of inputs for incremental rotary encoders. These are buffered first, then enter dedicated microcontroller GPIO's. R1A, R2A and R3A are connected to interrupt GPIOs.
AUX1, 2 and 3 are inputs directly connected to microcontroller GPIOs. These are intended for special function such as shift, page selection etc etc.
L01-L24 are LEDs outputs. As said, LEDs are driven by three independent ULN2803, each powered by a user definable source. This source could be 5V coming from USB, or an external power source, the selection being dependend on the total current your LEDs will sink and working tension. The external power source can be, in example, 12V, but also 5V if you are worried that the current needed could exceed the 500 mA a common USB port can source.
Please, notice that LED pads goes to LEDs negative leg (sinking configuration). The positive leg must be connected to the relative power source (LA+, LB+ or LC+).

# FIRMWARE
I wrote a generic firmware in order to give you maximum flexibility in layout design. The firmware is written to perform simple tasks (nothing too fancy) in order to be a good starting point for any project.
Arduino makes things easy because of it's open libraries. Here I have used Mattew Heironimus joystick library and some offical library (Mouse, Keyboard and SPI). Official arduino libraries are installed by default on the IDE; Mattew's lib must be installed instead.
Switches functions are triggered at state change only. This keeps the MPU cool, but also makes the use of momentary switches and latching switches interchangeable, without the need to treat them differently at coding level or adopting special hardware configurations. Box buttons can be freely assigned to joystick buttons (up to #24), keyboard keys (ASCII format) or mouse buttons. I assigned keyboard keys "ESC" and "ENTER" to two buttons. I also assigned left mouse click to another. All the other are joystick buttons.
In normal operation mode all LEDs are on.
When the mode toggling button is pressed, chat box mode is activated. In this mode the press of some button sends whole phrases to the chat. In chat box mode LEDs blink in order to give user a visual feedback. Phrases can be freely defined by the user and assigned to any button. To confirm and send to the outher world a phrase press "ENTER", to delete it press "ESC" (this at least if how it works in ACC).
The firmware emulates a mouse controlled by the analog joystick. The farther from the center the joystick lever, the faster the mouse movement.
Rotary (optical) encoders are read through a 1X counting routine. To avoid rotation losses even at high speeds, one of the two encoder optics is attached to MPU's interrupts pins, then hardware filtered (see previous step).
Please notice that even if rotary encoders are supported, I made a stupid mistake in my prototipe of the board such that I could not test the code (not with the PCB, at least). The shared PCB has been corrected, but I have not a copy of it to test the code and the fixed circuit. 
Don't like the overall behaviour or have in mind some groundbreaking features? Perfect! This project is open-firmware which means that you can modify it's core at your will, even with limited coding skills (it's arduino magic people!).

# ACKNOWLEDGMENTS
Many thanks to those nice girls and guys at JLCPCB for sponsoring the manufacturing of this project's FR-4 PCB and SMD assembly
JLCPCB is a high-tech manufacturer specialized in the production of high-reliable and cost-effective PCBs. They offer a flexible PCB assembly service with a huge library of more than 350.000 components in stock.
3D printing is part of their portfolio of services so one could create a full finished product, all in one place!
By registering at JLCPCB site via [THIS LINK](https://jlcpcb.com/IAT) (affiliated link) you will receive a series of coupons for your orders. Registering costs nothing, so it could be the right opportunity to give their service a due try ;)
All Gerber files, sketches and utilities I realized for this project are stored in this Github repo. I always upload the most recent file's versions, so it could be the case that some PCB looks a little different from those pictured in my instructables.
My projects are free and for everybody. You are anyway welcome if you want to donate some change to help me cover components costs and push the development of new projects.
My paypal donation page is [HERE](https://paypal.me/GuidolinMarco?country.x=IT&locale.x=it_IT), just in case ;)
