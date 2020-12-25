To use this template:

1. Make sure you have installed Teensyduino
   (execute the commands in the "Command Line Install" section on
   https://www.pjrc.com/teensy/td_download.html).
2. Adapt line 12 of the Makefile to your installation directory.
2. Run `git submodule update --init` to checkout the teensy cores repo in lib.
3. Run `make` to build.
4. Have fun putting your code in `src/`


# To do list

For MIDI IN: The pin routing is correct!

For MIDI OUT: 5V should be 3.3V and the pins (4 & 5) are swapped!
R11 --> 33 Ohm, 5% 0.5W
R10 --> 10 Ohm, 5% 0.25W
1kOhm@100MHz ferrite beads in series optional for both for EMC.

Fix Crystal
