/* Teensyduino Core Library
 * http://www.pjrc.com/teensy/
 * Copyright (c) 2017 PJRC.COM, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * 1. The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * 2. If the Software is incorporated into a build system that allows
 * selection among a list of target devices, then similar target
 * devices manufactured by PJRC.COM must be included in the list of
 * target devices and selectable in the same manner.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <math.h>

#include <Arduino.h>
#include "teensy_audio/imxrt_hw.h"

const int AUDIO_SAMPLE_RATE_EXACT = 48000;  // 48 kHz

void config_i2s(void) {
  CCM_CCGR5 |= CCM_CCGR5_SAI1(CCM_CCGR_ON);

  // if either transmitter or receiver is enabled, do nothing
  if (I2S1_TCSR & I2S_TCSR_TE) return;
  if (I2S1_RCSR & I2S_RCSR_RE) return;
//PLL:
  int fs = AUDIO_SAMPLE_RATE_EXACT;
  // PLL between 27*24 = 648MHz und 54*24=1296MHz
  int n1 = 4; //SAI prescaler 4 => (n1*n2) = multiple of 4
  int n2 = 1 + (24000000 * 27) / (fs * 256 * n1);

  double C = ((double)fs * 256 * n1 * n2) / 24000000;
  int c0 = C;
  int c2 = 10000;
  int c1 = C * c2 - (c0 * c2);
  set_audioClock(c0, c1, c2);

  // clear SAI1_CLK register locations
  CCM_CSCMR1 = (CCM_CSCMR1 & ~(CCM_CSCMR1_SAI1_CLK_SEL_MASK))
       | CCM_CSCMR1_SAI1_CLK_SEL(2); // &0x03 // (0,1,2): PLL3PFD0, PLL5, PLL4
  CCM_CS1CDR = (CCM_CS1CDR & ~(CCM_CS1CDR_SAI1_CLK_PRED_MASK | CCM_CS1CDR_SAI1_CLK_PODF_MASK))
       | CCM_CS1CDR_SAI1_CLK_PRED(n1-1) // &0x07
       | CCM_CS1CDR_SAI1_CLK_PODF(n2-1); // &0x3f

  // Select MCLK
  IOMUXC_GPR_GPR1 = (IOMUXC_GPR_GPR1
    & ~(IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL_MASK))
    | (IOMUXC_GPR_GPR1_SAI1_MCLK_DIR | IOMUXC_GPR_GPR1_SAI1_MCLK1_SEL(0));

  CORE_PIN23_CONFIG = 3;  //1:MCLK
  CORE_PIN21_CONFIG = 3;  //1:RX_BCLK
  CORE_PIN20_CONFIG = 3;  //1:RX_SYNC

  int rsync = 0;
  int tsync = 1;

  I2S1_TMR = 0;
  I2S1_TCR1 = I2S_TCR1_RFW(30);
  I2S1_TCR2 = I2S_TCR2_SYNC(tsync) | I2S_TCR2_BCP
        | (I2S_TCR2_BCD | I2S_TCR2_DIV((1)) | I2S_TCR2_MSEL(1));
  I2S1_TCR3 = I2S_TCR3_TCE;
  I2S1_TCR4 = I2S_TCR4_FRSZ((2-1)) | I2S_TCR4_SYWD((32-1)) | I2S_TCR4_MF
        | I2S_TCR4_FSD | I2S_TCR4_FSE | I2S_TCR4_FSP;
  I2S1_TCR5 = I2S_TCR5_WNW((32-1)) | I2S_TCR5_W0W((32-1)) | I2S_TCR5_FBT((32-1));

  I2S1_RMR = 0;
  I2S1_RCR1 = I2S_RCR1_RFW(1);  // FIFO request flag if > 1 entries in FIFO
  I2S1_RCR2 = I2S_RCR2_SYNC(rsync) | I2S_RCR2_BCP | (I2S_RCR2_BCD | I2S_RCR2_DIV((1)) | I2S_RCR2_MSEL(1)); // | I2S_RCR2_BCP
  I2S1_RCR3 = I2S_RCR3_RCE;
  I2S1_RCR4 = I2S_RCR4_FRSZ((2-1)) | I2S_RCR4_SYWD((32-1)) | I2S_RCR4_MF
        | I2S_RCR4_FSE | I2S_RCR4_FSP | I2S_RCR4_FSD;
  I2S1_RCR5 = I2S_RCR5_WNW((32-1)) | I2S_RCR5_W0W((32-1)) | I2S_RCR5_FBT((32-1));

  CORE_PIN7_CONFIG  = 3;  //1:TX_DATA0
  CORE_PIN8_CONFIG  = 3;  //1:RX_DATA0
  IOMUXC_SAI1_RX_DATA0_SELECT_INPUT = 2;
}

void i2s_start() {
  I2S1_RCSR |= I2S_RCSR_RE | I2S_RCSR_BCE | I2S_RCSR_FR; // Enable receiver
  I2S1_TCSR = I2S_TCSR_TE | I2S_TCSR_BCE;  // Enable transmitter
}

bool i2s_write(int32_t value_left, int32_t value_right) {
  if (I2S1_TCSR & I2S_TCSR_FRF) {
    I2S1_TDR0 = value_left;
    I2S1_TDR0 = value_right;
    return true;
  } else {
    return false;
  }
}

bool i2s_read(int32_t* value_left, int32_t* value_right) {
  if (I2S1_RCSR & I2S_RCSR_FRF) {
    *value_left = I2S1_RDR0;
    *value_right = I2S1_RDR0;
    return true;
  } else {
    return false;
  }
}

/** @brief Gets the next 24-bit I2S value that must be written for creating a
 *    440Hz sine wave.
 *  @param[in] write_counter The current message count (can be modulo AUDIO_SAMPLE_RATE_EXACT).
 *  @returns The next value to be written (24-bit).
 */
int32_t get_next_audio_value(int write_counter) {
  float t = (float) write_counter / AUDIO_SAMPLE_RATE_EXACT;
  float sinval = sin(440 * 2 * 3.1415 * t);
  int32_t value = sinval * 4294967296/2;
  return value;
}

extern "C" int main(void)
{
  bool input_to_output = true;
  int write_counter = 0;

  Serial.begin(115200);
  Serial1.begin(31250);

  config_i2s();

  int i = 0;
  while(i2s_write(0, 0)){i++;}
  i2s_start();

  int32_t read_value_left = 0;
  int32_t read_value_right = 0;
  while(1) {
    // Check input on MIDI in and write to MIDI out
    if (Serial1.available() > 0) {
      uint8_t received_byte = Serial1.read();
      Serial.print("rcv: ");
      Serial.println(received_byte, DEC);
      Serial1.write(received_byte);
    }
    // Check input on USB and write 0x900000 to MIDI out
    if (Serial.available() > 0) {
      uint8_t received_byte = Serial.read();
      Serial.print("rcv2: ");
      Serial.println(received_byte, DEC);
      Serial1.write(0x90);
      Serial1.write(0x00);
      Serial1.write(0x00);
    }
    if (input_to_output) {
      if (i2s_read(&read_value_left, &read_value_right)) {

      }
      i2s_write(read_value_left, read_value_right);
    } else {
      // I2S read to avoid errors due to buffer overflow
      int32_t not_used;
      i2s_read(&not_used, &not_used);

      int32_t sine_value = get_next_audio_value(write_counter);
      if (i2s_write(sine_value, sine_value)) {
        write_counter += 1;
        if (write_counter >= 48000) {
          write_counter = 0;
        }
      }
    }
  }
}
