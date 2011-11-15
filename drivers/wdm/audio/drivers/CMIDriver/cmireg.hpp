/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _CMIREG_HPP_
#define _CMIREG_HPP_

// registers (from CMI8738_6ch_series_spec_v18f_registers.pdf)

#define REG_FUNCTRL0  0x00       // Function Control Register 0 (32bit)
#define ADC_CH0       0x00000001 // enable ADC on channel 0 for recording
#define ADC_CH1       0x00000002 // enable ADC on channel 1 for recording
#define PAUSE_CH0     0x00000004 // pause channel 0
#define PAUSE_CH1     0x00000008 // pause channel 1
#define EN_CH0        0x00010000 // enable channel 0
#define EN_CH1        0x00020000 // enable channel 1
#define RST_CH0       0x00040000 // reset channel 0
#define RST_CH1       0x00080000 // reset channel 1

#define REG_FUNCTRL1  0x04       // Function Control Register 1 (32bit)
#define EN_ZVPORT     0x00000001 // enable ZVPort
#define EN_GAMEPORT   0x00000002 // enable legacy gameport
#define EN_UART       0x00000004 // enable UART (MIDI interface)
#define BREQ          0x00000010 // enable bus master request
#define INTRM         0x00000020 // enable master control block interrupt
#define SPDO2DAC      0x00000040 // send S/PDIF out through DAC
#define LOOP_SPDF     0x00000080 // loop S/PDIF in to S/PDIF out
#define SPDF_0        0x00000100 // enable S/PDIF out at channel 0
#define SPDF_1        0x00000200 // enable S/PDIF in/out at channel 1
#define SFC_CH0_MASK  0x00001C00 // mask for the sample rate bits of channel 0
#define SFC_44K_CH0   0x00000C00 // 44.1kHz channel 0
#define SFC_48K_CH0   0x00001C00 // 48kHz channel 0
#define SFC_CH1_MASK  0x0000E000 // mask for the sample rate bits of channel 1
#define SFC_44K_CH1   0x00006000 // 44.1kHz channel 1
#define SFC_48K_CH1   0x0000E000 // 48kHz channel 1

#define REG_CHFORMAT  0x08       // Channel Format Register (32bit)
#define FORMAT_CH0    0x00000003 // enable 16bit stereo format for channel 0
#define FORMAT_CH1    0x0000000C // enable 16bit stereo format for channel 1
#define SPDLOCKED     0x00000010 //
#define POLVALID      0x00000020 // invert S/PDIF in valid bit
#define DBLSPDS       0x00000040 // double S/PDIF sampling rate (44.1 => 88.2, 48 => 96)
#define INV_SPDIFI2   0x00000080 // invert S/PDIF in signal (whatever that means, version >37)
#define SPD88_CH0     0x00000100 // double sample rate from 44.1 to 88.2 kHz on channel 0
#define SPD88_CH1     0x00000200 // double sample rate from 44.1 to 88.2 kHz on channel 1
#define SPD96_CH0     0x00000400 // double sample rate from 48 to 96 kHz on channel 0
#define SPD96_CH1     0x00000800 // double sample rate from 48 to 96 kHz on channel 1
#define SEL_SPDIFI1   0x00080000 // select secondary S/PDIF in (only for versions <=37)
#define EN_SPDO_AC3_3 0x00020000 // undocumented: enable AC3 mode on S/PDIF out (requires hardware support)
#define EN_SPDO_AC3_1 0x00100000 // undocumented: enable AC3 mode on S/PDIF out
#define SPD24SEL      0x00200000 // enable 24bit S/PDIF out
#define VERSION_37    0x01000000 // undocumented: hardware revision 37
#define EN_4CH_CH1    0x20000000 // enable 4 channel mode on channel 1
#define EN_5CH_CH1    0x80000000 // enable 5 channel mode on channel 1

#define REG_INTHLDCLR 0x0C       // Interrupt Hold/Clear Register (32bit)
#define VERSION_MASK  0xFF000000 // mask for the version number, bits [31:24], highest byte
#define VERSION_68    0x20000000 // undocumented: hardware revision 68 (8768)
#define VERSION_55    0x08000000 // undocumented: hardware revision 55
#define VERSION_39    0x04000000 // undocumented: hardware revision 39
#define VERSION_39_6  0x01000000 // undocumented: 6 channel version of revision 39
#define INT_CLEAR     0x00000001 // clear interrupt
#define INT_HOLD      0x00000002 // hold interrupt
#define EN_CH0_INT    0x00010000 // enable interrupt on channel 0
#define EN_CH1_INT    0x00020000 // enable interrupt on channel 1

#define REG_INT_STAT  0x10       // Interrupt Register (32bit)
#define INT_CH0       0x00000001 // interrupt on channel 0
#define INT_CH1       0x00000002 // interrupt on channel 1
#define BUSY_CH0      0x00000004 // channel 0 busy
#define BUSY_CH1      0x00000008 // channel 1 busy
#define INT_UART      0x00010000 // interrupt on UART interface
#define INT_PENDING   0x80000000 // interrupt pending

#define REG_LEGACY    0x14       // Legacy Control Register (32bit)
#define CENTER2LINE   0x00002000 // route center channel to line-in jack
#define BASS2LINE     0x00004000 // route bass channel to line-in jack
#define EN_6CH_CH1    0x00008000 // enable 6 channel mode on channel 1
#define DAC2SPDO      0x00200000 // enable PCM+FM to S/PDIF out
#define EN_SPDCOPYRHT 0x00400000 // enable S/PDIF out copyright bit
#define EN_SPDIF_OUT  0x00800000 // enable S/PDIF out
#define UART_330      0x00000000 // i/o addresses for UART
#define UART_320      0x20000000
#define UART_310      0x40000000
#define UART_300      0x60000000
#define DWORD_MAPPING 0x80000000 // enable DWORD-based position in base register

#define REG_MISCCTRL  0x18       // Miscellaneous Control Register (32bit)
#define EN_CENTER     0x00000080 // enable center channel
#define SEL_SPDIFI2   0x00000100 // select secondary S/PDIF in
#define EN_SPDIF_48_1 0x00008000 // enable 48kHz sampling rate on S/PDIF out
#define EN_SPDO_AC3_2 0x00040000 // enable AC3 mode on S/PDIF out
#define LOOP_SPDF_I   0x00100000 // loop internal S/PDIF out to internal S/PDIF in
#define SPD32SEL      0x00200000 // enable 32bit S/PDIF out
#define XCHG_DAC      0x00400000 // exchange DACs
#define EN_DBLDAC     0x00800000 // enable double DAC mode
#define EN_SPDIF_48_2 0x01000000 // enable 48kHz sampling rate on S/PDIF out
#define EN_SPDO5V     0x02000000 // enable 5V levels on S/PDIF out
#define RST_CHIP      0x40000000 // reset bus master / DSP engine
#define PWD_CHIP      0x80000000 // enable power down mode (standby mode etc.)
#define EN_SPDIF_48   (EN_SPDIF_48_1 | EN_SPDIF_48_2)

#define REG_SBDATA    0x22       // SoundBlaster compatible mixer data register (8bit)
#define REG_SBINDEX   0x23       // SoundBlaster compatible mixer index register (8bit)

#define REG_MIXER1    0x24       // Mixer Register 1 (8bit)
#define EN_SPDI2DAC   0x01       // enable S/PDIF in conversion
#define EN_3DSOUND    0x02       // enable 3D sound
#define EN_WAVEIN_L   0x04       // enable left wave in recording channel
#define EN_WAVEIN_R   0x08       // enable right wave in recording channel
#define REAR2FRONT    0x10       // exchange rear/front jacks
#define REAR2LINE     0x20       // enable rear out on line-in jack
#define MUTE_WAVE     0x40       // disable analog conversion of the wave stream
#define MUTE_FM       0x80       // mute FM

#define REG_MIXER2    0x25       // Mixer Register 2 (8bit)
#define DIS_MICGAIN   0x01       // disable microphone gain
#define MUTE_AUX_L    0x10       // mute left aux playback channel
#define MUTE_AUX_R    0x20       // mute right aux playback channel
#define MUTE_RAUX_L   0x40       // mute left aux recording channel
#define MUTE_RAUX_R   0x80       // mute right aux recording channel

#define REG_MIXER3    0x26       // Mixer Register 3 (8bit)

#define REG_MIXER4    0x27       // Mixer Register 4 (8bit)
#define INV_SPDIFI1   0x04       // invert S/PDIF in signal (version <=37)
#define CENTER2MIC    0x04       // route center to mic-in jack (version >37)

#define REG_CH0_FRAME1	0x80     // Channel 0 Frame Register 1 (32bit)
#define REG_CH0_FRAME2	0x84     // Channel 0 Frame Register 2 (32bit)
#define REG_CH1_FRAME1	0x88     // Channel 1 Frame Register 1 (32bit)
#define REG_CH1_FRAME2	0x8C     // Channel 1 Frame Register 2 (32bit)

#define REG_MISCCTRL2 0x92       // Miscellaneous Control Register 2 (16bit)
#define EN_8CH_CH1    0x0020     // enable 8 channel mode on channel 1

#define SBREG_OUTPUTCTRL  0x3C   // Soundblaster register for output controls (8bit)
#define EN_MIC            0x01   // enable microphone output
#define EN_CD_L           0x02   // enable left channel of CD input
#define EN_CD_R           0x04   // enable right channel of CD input
#define EN_LINEIN_L       0x08   // enable left channel of line-in
#define EN_LINEIN_R       0x10   // enable right channel of line-in

#define SBREG_IN_CTRL_L   0x3D   // Soundblaster register for left channel recording controls (8bit)
#define SBREG_IN_CTRL_R   0x3E   // Soundblaster register for right channel recording controls (8bit)

#define SBREG_EXTENSION   0xF0   // Soundblaster Extension Register (8bit)
#define EN_MICBOOST       0x01   // enable microphone boost for recording

#endif
