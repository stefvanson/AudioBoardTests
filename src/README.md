This project tests the AudioBoard.

In the teensy4 directory, delete main.cpp (or comment out it's contents)

It has an I2S output:
- I2S format (FMT = Low)
- And it's a slave.

It has an I2S input:
- FMT0=1 and FMT1=0, meaning I2S 24-bit format
- JP2 (JP_MODE0) and JP3 (JP_MODE1):
    MODE1 MODE0 INTERFACE MODE
    0     0     Slave mode (256 fS, 384 fS, 512 fS, 768 fS (=32kHz))
    0     1     Master mode (512 fS (=48kHz))
    1     0     Master mode (384 fS (=64kHz))
    1     1     Master mode (256 fS (=96kHz))
   Initially try MODE0 = 0 and MODE1 = 0 (slave mode) and provide data at 512 fS (48kHz)
- JP4 (JP_BYPAS): Leave unoccupied --> Normal mode (DC reject)

Since the I2S input is 24-bit I'm using 24 bit I2S for both the input and the
output.
