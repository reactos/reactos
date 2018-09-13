/****************************************************************************
 *
 *   mmreg.h  - Registered Multimedia Information Public Header File
 *
 *   Copyright (c) 1991,1992,1993 Microsoft Corporation.  All Rights Reserved.
 *
 * Multimedia Registration
 *
 * Place this system include file in your INCLUDE path with the Windows SDK
 * include files.
 *
 * Obtain the Multimedia Developer Registration Kit from:
 *
 *  Heidi Breslauer
 *  Microsoft Corporation
 *  Multimedia Technology Group
 *  One Microsoft Way
 *  Redmond, WA 98052-6399
 *
 * Developer Services:
 * 800-227-4679 x11771
 *
 * Last Update:  10/04/93
 *
 ***************************************************************************/

// Define the following to skip definitions
//
// NOMMIDS      Multimedia IDs are not defined
// NONEWWAVE    No new waveform types are defined except WAVEFORMATEX
// NONEWRIFF    No new RIFF forms are defined
// NOJPEGDIB    No JPEG DIB definitions
// NONEWIC      No new Image Compressor types are defined

#ifndef _INC_MMREG
/* use version number to verify compatibility */
#define _INC_MMREG     142      // version * 100 + revision

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifndef NOMMIDS
    
/* manufacturer IDs */
#ifndef MM_MICROSOFT
#define MM_MICROSOFT            1   /* Microsoft Corporation */
#endif
#define MM_CREATIVE             2   /* Creative Labs Inc. */
#define MM_MEDIAVISION          3   /* Media Vision Inc. */
#define MM_FUJITSU              4
#define MM_ARTISOFT             20  /* Artisoft Inc. */
#define MM_TURTLE_BEACH         21      
#define MM_IBM                  22  /* International Bussiness Machines Corp. */
#define MM_VOCALTEC             23  /* Vocaltec LTD. */
#define MM_ROLAND               24
#define MM_DIGISPEECH           25  /* Digispeech, Inc. */
#define MM_NEC                  26  /* NEC */
#define MM_ATI                  27  /* ATI */
#define MM_WANGLABS             28  /* Wang Laboratories, Inc. */
#define MM_TANDY                29  /* Tandy Corporation */
#define MM_VOYETRA              30  /* Voyetra */
#define MM_ANTEX                31  /* Antex */
#define MM_ICL_PS               32
#define MM_INTEL                33
#define MM_GRAVIS               34
#define MM_VAL                  35  /* Video Associates Labs */
#define MM_INTERACTIVE          36  /* InterActive, Inc. */
#define MM_YAMAHA               37  /* Yamaha Corp. of America */
#define MM_EVEREX               38  /* Everex Systems, Inc. */
#define MM_ECHO                 39  /* Echo Speech Corporation */
#define MM_SIERRA               40  /* Sierra Semiconductor */
#define MM_CAT                  41  /* Computer Aided Technologies */
#define MM_APPS                 42  /* APPS Software International */
#define MM_DSP_GROUP            43  /* DSP Group, Inc. */
#define MM_MELABS               44  /* microEngineering Labs */
#define MM_COMPUTER_FRIENDS     45  /* Computer Friends, Inc */
#define MM_ESS                  46  /* ESS Technology */
#define MM_AUDIOFILE            47  /* Audio, Inc. */
#define MM_MOTOROLA             48  /* Motorola, Inc. */
#define MM_CANOPUS              49  /* Canopus Co., Ltd. */
#define MM_EPSON                50  /* Seiko Epson Corp. */
#define MM_TRUEVISION           51  /* Truevision */
#define MM_AZTECH               52  /* Aztech Labs, Inc.*/
#define MM_VIDEOLOGIC           53  /* VideoLogic */
#define MM_SCALACS              54  /* SCALACS */
#define MM_KORG                 55  /* Toshihiko Okuhara, Korg Inc.*/
#define MM_APT                  56  /* Audio Processing Technology */
#define MM_ICS                  57  /* Integrated Circuit Systems */
#define MM_ITERATEDSYS          58  /* Iterated Systems Inc.*/
#define MM_METHEUS              59  /* Metheus Corp*/
#define MM_LOGITECH             60  /* Logitech Inc, Fremont, CA*/
#define MM_WINNOV               61  /* Winnov, Inc., Sunnyvale, CA*/

/* MM_MICROSOFT product IDs */
#ifndef MM_MIDI_MAPPER

#define MM_MIDI_MAPPER          1   /* MIDI Mapper */
#define MM_WAVE_MAPPER          2   /* Wave Mapper */
#define MM_SNDBLST_MIDIOUT      3   /* Sound Blaster MIDI output port */
#define MM_SNDBLST_MIDIIN       4   /* Sound Blaster MIDI input port */
#define MM_SNDBLST_SYNTH        5   /* Sound Blaster internal synthesizer */
#define MM_SNDBLST_WAVEOUT      6   /* Sound Blaster waveform output */
#define MM_SNDBLST_WAVEIN       7   /* Sound Blaster waveform input */
#define MM_ADLIB                9   /* Ad Lib-compatible synthesizer */
#define MM_MPU401_MIDIOUT      10   /* MPU401-compatible MIDI output port */
#define MM_MPU401_MIDIIN       11   /* MPU401-compatible MIDI input port */
#define MM_PC_JOYSTICK         12   /* Joystick adapter */

#endif

#define MM_PCSPEAKER_WAVEOUT            13  /* PC Speaker waveform output */

#define MM_MSFT_WSS_WAVEIN              14  /* MS Audio Board waveform input */
#define MM_MSFT_WSS_WAVEOUT             15  /* MS Audio Board waveform output */
#define MM_MSFT_WSS_FMSYNTH_STEREO      16  /* MS Audio Board Stereo FM synthesizer */
#define MM_MSFT_WSS_MIXER               17  /* MS Audio Board Mixer Driver */
#define MM_MSFT_WSS_OEM_WAVEIN          18  /* MS OEM Audio Board waveform input */
#define MM_MSFT_WSS_OEM_WAVEOUT         19  /* MS OEM Audio Board waveform Output */
#define MM_MSFT_WSS_OEM_FMSYNTH_STEREO  20  /* MS OEM Audio Board Stereo FM synthesizer */
#define MM_MSFT_WSS_AUX                 21  /* MS Audio Board Auxiliary Port */
#define MM_MSFT_WSS_OEM_AUX             22  /* MS OEM Audio Auxiliary Port */

#define MM_MSFT_GENERIC_WAVEIN          23  /* MS vanilla driver waveform input */
#define MM_MSFT_GENERIC_WAVEOUT         24  /* MS vanilla driver waveform output */
#define MM_MSFT_GENERIC_MIDIIN          25  /* MS vanilla driver MIDI input */
#define MM_MSFT_GENERIC_MIDIOUT         26  /* MS vanilla driver external MIDI output */
#define MM_MSFT_GENERIC_MIDISYNTH       27  /* MS vanilla driver MIDI synthesizer */
#define MM_MSFT_GENERIC_AUX_LINE        28  /* MS vanilla driver aux (line in) */
#define MM_MSFT_GENERIC_AUX_MIC         29  /* MS vanilla driver aux (mic) */
#define MM_MSFT_GENERIC_AUX_CD          30  /* MS vanilla driver aux (CD) */

#define MM_MSFT_WSS_OEM_MIXER           31  /* MS OEM Audio Board Mixer Driver */

#define MM_MSFT_MSACM                   32  /* MS Audio Compression Manager*/
#define MM_MSFT_ACM_MSADPCM             33  /* MS ADPCM codec*/
#define MM_MSFT_ACM_IMAADPCM            34  /* IMA ADPCM codec*/
#define MM_MSFT_ACM_MSFILTER            35  /* MS Filter*/
#define MM_MSFT_ACM_GSM610              36  /* GSM 610 codec*/
#define MM_MSFT_ACM_G711                37  /* G.711 codec*/
#define MM_MSFT_ACM_PCM                 38  /* PCM converter */

#define MM_MSFT_SB16_WAVEIN             39  /* Sound Blaster 16 waveform input */
#define MM_MSFT_SB16_WAVEOUT            40  /* Sound Blaster 16 waveform output */
#define MM_MSFT_SB16_MIDIIN             41  /* Sound Blaster 16 midi-in */
#define MM_MSFT_SB16_MIDIOUT            42  /* Sound Blaster 16 midi-out */
#define MM_MSFT_SB16_SYNTH              43  /* Sound Blaster 16 FM synthesis */
#define MM_MSFT_SB16_AUX_LINE           44  /* Sound Blaster 16 aux (line in) */
#define MM_MSFT_SB16_AUX_CD             45  /* Sound Blaster 16 aux (CD) */
#define MM_MSFT_SB16_MIXER              46  /* Sound Blaster 16 mixer device */

#define MM_MSFT_SBPRO_WAVEIN            47  /* Sound Blaster Pro waveform input */
#define MM_MSFT_SBPRO_WAVEOUT           48  /* Sound Blaster Pro waveform output */
#define MM_MSFT_SBPRO_MIDIIN            49  /* Sound Blaster Pro midi-in */
#define MM_MSFT_SBPRO_MIDIOUT           50  /* Sound Blaster Pro midi-out */
#define MM_MSFT_SBPRO_SYNTH             51  /* Sound Blaster Pro FM synthesis */
#define MM_MSFT_SBPRO_AUX_LINE          52  /* Sound Blaster Pro aux (line in) */
#define MM_MSFT_SBPRO_AUX_CD            53  /* Sound Blaster Pro aux (CD) */
#define MM_MSFT_SBPRO_MIXER             54  /* Sound Blaster Pro mixer */

/* MM_CREATIVE product IDs */
#define MM_CREATIVE_SB15_WAVEIN         1   /* SB (r) 1.5 waveform input */
#define MM_CREATIVE_SB20_WAVEIN         2   /* SB (r) 2.0 waveform input */
#define MM_CREATIVE_SBPRO_WAVEIN        3   /* SB Pro (r) waveform input */
#define MM_CREATIVE_SBP16_WAVEIN        4   /* SBP16 (r) waveform input */
#define MM_CREATIVE_SB15_WAVEOUT      101   /* SB (r) 1.5 waveform output */
#define MM_CREATIVE_SB20_WAVEOUT      102   /* SB (r) 2.0 waveform output */
#define MM_CREATIVE_SBPRO_WAVEOUT     103   /* SB Pro (r) waveform output */
#define MM_CREATIVE_SBP16_WAVEOUT     104   /* SBP16 (r) waveform output */
#define MM_CREATIVE_MIDIOUT           201   /* SB (r) MIDI output port */
#define MM_CREATIVE_MIDIIN            202   /* SB (r) MIDI input port */
#define MM_CREATIVE_FMSYNTH_MONO      301   /* SB (r) FM synthesizer */
#define MM_CREATIVE_FMSYNTH_STEREO    302   /* SB Pro (r) stereo FM synthesizer */
#define MM_CREATIVE_AUX_CD            401   /* SB Pro (r) aux (CD) */
#define MM_CREATIVE_AUX_LINE          402   /* SB Pro (r) aux (line in) */
#define MM_CREATIVE_AUX_MIC           403   /* SB Pro (r) aux (mic) */


/* MM_ARTISOFT product IDs */
#define MM_ARTISOFT_SBWAVEIN    1   /* Artisoft Sounding Board waveform input */
#define MM_ARTISOFT_SBWAVEOUT   2   /* Artisoft Sounding Board waveform output */

/* MM_IBM Product IDs */
#define MM_MMOTION_WAVEAUX      1       /* IBM M-Motion Auxiliary Device */
#define MM_MMOTION_WAVEOUT      2       /* IBM M-Motion Waveform Output */
#define MM_MMOTION_WAVEIN       3       /* IBM M-Motion Waveform Input */

/* MM_MEDIAVISION Product IDs */
// Original Pro AudioSpectrum
#define MM_MEDIAVISION_PROAUDIO 0x10
#define MM_PROAUD_MIDIOUT               (MM_MEDIAVISION_PROAUDIO+1)
#define MM_PROAUD_MIDIIN                (MM_MEDIAVISION_PROAUDIO+2)
#define MM_PROAUD_SYNTH                 (MM_MEDIAVISION_PROAUDIO+3)
#define MM_PROAUD_WAVEOUT               (MM_MEDIAVISION_PROAUDIO+4)
#define MM_PROAUD_WAVEIN                (MM_MEDIAVISION_PROAUDIO+5)
#define MM_PROAUD_MIXER                 (MM_MEDIAVISION_PROAUDIO+6)
#define MM_PROAUD_AUX                   (MM_MEDIAVISION_PROAUDIO+7)

// Thunder board
#define MM_MEDIAVISION_THUNDER          0x20
#define MM_THUNDER_SYNTH                (MM_MEDIAVISION_THUNDER+3)
#define MM_THUNDER_WAVEOUT              (MM_MEDIAVISION_THUNDER+4)
#define MM_THUNDER_WAVEIN               (MM_MEDIAVISION_THUNDER+5)
#define MM_THUNDER_AUX                  (MM_MEDIAVISION_THUNDER+7)

// Audio Port
#define MM_MEDIAVISION_TPORT            0x40
#define MM_TPORT_WAVEOUT                (MM_MEDIAVISION_TPORT+1)
#define MM_TPORT_WAVEIN                 (MM_MEDIAVISION_TPORT+2)
#define MM_TPORT_SYNTH                  (MM_MEDIAVISION_TPORT+3)

// Pro AudioSpectrum Plus
#define MM_MEDIAVISION_PROAUDIO_PLUS    0x50
#define MM_PROAUD_PLUS_MIDIOUT          (MM_MEDIAVISION_PROAUDIO_PLUS+1)
#define MM_PROAUD_PLUS_MIDIIN           (MM_MEDIAVISION_PROAUDIO_PLUS+2)
#define MM_PROAUD_PLUS_SYNTH            (MM_MEDIAVISION_PROAUDIO_PLUS+3)
#define MM_PROAUD_PLUS_WAVEOUT          (MM_MEDIAVISION_PROAUDIO_PLUS+4)
#define MM_PROAUD_PLUS_WAVEIN           (MM_MEDIAVISION_PROAUDIO_PLUS+5)
#define MM_PROAUD_PLUS_MIXER            (MM_MEDIAVISION_PROAUDIO_PLUS+6)
#define MM_PROAUD_PLUS_AUX              (MM_MEDIAVISION_PROAUDIO_PLUS+7)


// Pro AudioSpectrum 16
#define MM_MEDIAVISION_PROAUDIO_16      0x60
#define MM_PROAUD_16_MIDIOUT            (MM_MEDIAVISION_PROAUDIO_16+1)
#define MM_PROAUD_16_MIDIIN             (MM_MEDIAVISION_PROAUDIO_16+2)
#define MM_PROAUD_16_SYNTH              (MM_MEDIAVISION_PROAUDIO_16+3)
#define MM_PROAUD_16_WAVEOUT            (MM_MEDIAVISION_PROAUDIO_16+4)
#define MM_PROAUD_16_WAVEIN             (MM_MEDIAVISION_PROAUDIO_16+5)
#define MM_PROAUD_16_MIXER              (MM_MEDIAVISION_PROAUDIO_16+6)
#define MM_PROAUD_16_AUX                (MM_MEDIAVISION_PROAUDIO_16+7)


// CDPC
#define MM_MEDIAVISION_CDPC             0x70
#define MM_CDPC_MIDIOUT                 (MM_MEDIAVISION_CDPC+1)
#define MM_CDPC_MIDIIN                  (MM_MEDIAVISION_CDPC+2)
#define MM_CDPC_SYNTH                   (MM_MEDIAVISION_CDPC+3)
#define MM_CDPC_WAVEOUT                 (MM_MEDIAVISION_CDPC+4)
#define MM_CDPC_WAVEIN                  (MM_MEDIAVISION_CDPC+5)
#define MM_CDPC_MIXER                   (MM_MEDIAVISION_CDPC+6)
#define MM_CDPC_AUX                     (MM_MEDIAVISION_CDPC+7)

//
// Opus MV1208 Chipset
//
#define MM_MEDIAVISION_OPUS1208         0x80
#define MM_OPUS401_MIDIOUT              (MM_MEDIAVISION_OPUS1208+1)
#define MM_OPUS401_MIDIIN               (MM_MEDIAVISION_OPUS1208+2)
#define MM_OPUS1208_SYNTH               (MM_MEDIAVISION_OPUS1208+3)
#define MM_OPUS1208_WAVEOUT             (MM_MEDIAVISION_OPUS1208+4)
#define MM_OPUS1208_WAVEIN              (MM_MEDIAVISION_OPUS1208+5)
#define MM_OPUS1208_MIXER               (MM_MEDIAVISION_OPUS1208+6)
#define MM_OPUS1208_AUX                 (MM_MEDIAVISION_OPUS1208+7)


//
// Opus MV1216 Chipset
//
#define MM_MEDIAVISION_OPUS1216 0x90
#define MM_OPUS1216_MIDIOUT             (MM_MEDIAVISION_OPUS1216+1)
#define MM_OPUS1216_MIDIIN              (MM_MEDIAVISION_OPUS1216+2)
#define MM_OPUS1216_SYNTH               (MM_MEDIAVISION_OPUS1216+3)
#define MM_OPUS1216_WAVEOUT             (MM_MEDIAVISION_OPUS1216+4)
#define MM_OPUS1216_WAVEIN              (MM_MEDIAVISION_OPUS1216+5)
#define MM_OPUS1216_MIXER               (MM_MEDIAVISION_OPUS1216+6)
#define MM_OPUS1216_AUX                 (MM_MEDIAVISION_OPUS1216+7)


// Pro Audio Studio 16
#define MM_MEDIAVISION_PROSTUDIO_16     0x60
#define MM_STUDIO_16_MIDIOUT            (MM_MEDIAVISION_PROSTUDIO_16+1)
#define MM_STUDIO_16_MIDIIN             (MM_MEDIAVISION_PROSTUDIO_16+2)
#define MM_STUDIO_16_SYNTH              (MM_MEDIAVISION_PROSTUDIO_16+3)
#define MM_STUDIO_16_WAVEOUT            (MM_MEDIAVISION_PROSTUDIO_16+4)
#define MM_STUDIO_16_WAVEIN             (MM_MEDIAVISION_PROSTUDIO_16+5)
#define MM_STUDIO_16_MIXER              (MM_MEDIAVISION_PROSTUDIO_16+6)
#define MM_STUDIO_16_AUX                (MM_MEDIAVISION_PROSTUDIO_16+7)

/* MM_VOCALTEC Product IDs */
#define MM_VOCALTEC_WAVEOUT     1       /* Vocaltec Waveform output port */
#define MM_VOCALTEC_WAVEIN      2       /* Vocaltec Waveform input port */
			
/* MM_ROLAND Product IDs */
#define MM_ROLAND_MPU401_MIDIOUT    15
#define MM_ROLAND_MPU401_MIDIIN     16
#define MM_ROLAND_SMPU_MIDIOUTA     17
#define MM_ROLAND_SMPU_MIDIOUTB     18
#define MM_ROLAND_SMPU_MIDIINA      19
#define MM_ROLAND_SMPU_MIDIINB      20
#define MM_ROLAND_SC7_MIDIOUT       21
#define MM_ROLAND_SC7_MIDIIN        22
			

/* MM_DIGISPEECH Product IDs */
#define MM_DIGISP_WAVEOUT       1       /* Digispeech Waveform output port */
#define MM_DIGISP_WAVEIN        2       /* Digispeech Waveform input port */
			
/* MM_NEC Product IDs */
			
/* MM_ATI Product IDs */

/* MM_WANGLABS Product IDs */

#define MM_WANGLABS_WAVEIN1             1
/* Input audio wave device present on the CPU board of the following Wang models: Exec 4010, 4030 and 3450; PC 251/25C, PC 461/25S and PC 461/33C */
#define MM_WANGLABS_WAVEOUT1            2
/* Output audio wave device present on the CPU board of the Wang models listed above. */

/* MM_TANDY Product IDs                  */
#define MM_TANDY_VISWAVEIN              1
#define MM_TANDY_VISWAVEOUT             2
#define MM_TANDY_VISBIOSSYNTH           3
#define MM_TANDY_SENS_MMAWAVEIN         4
#define MM_TANDY_SENS_MMAWAVEOUT        5
#define MM_TANDY_SENS_MMAMIDIIN         6
#define MM_TANDY_SENS_MMAMIDIOUT        7
#define MM_TANDY_SENS_VISWAVEOUT        8
#define MM_TANDY_PSSJWAVEIN             9
#define MM_TANDY_PSSJWAVEOUT            10



/* MM_VOYETRA Product IDs */

/* MM_ANTEX Product IDs */

/* MM_ICL_PS Product IDs */

/* MM_INTEL Product IDs */

#define MM_INTELOPD_WAVEIN      1       // HID2 WaveAudio Input driver
#define MM_INTELOPD_WAVEOUT     101     // HID2 WaveAudio Output driver
#define MM_INTELOPD_AUX         401     // HID2 Auxiliary driver (required for mixing functions)

/* MM_GRAVIS Product IDs */

/* MM_VAL Product IDs */

// values not defined by Manufacturer

// #define MM_VAL_MICROKEY_AP_WAVEIN    ???     // Microkey/AudioPort Waveform Input
// #define MM_VAL_MICROKEY_AP_WAVEOUT   ???     // Microkey/AudioPort Waveform Output

/* MM_INTERACTIVE Product IDs */

#define MM_INTERACTIVE_WAVEIN   0x45    // no comment provided by Manufacturer
#define MM_INTERACTIVE_WAVEOUT  0x45    // no comment provided by Manufacturer

/* MM_YAMAHA Product IDs */

#define MM_YAMAHA_GSS_SYNTH     0x01    // Yamaha Gold Sound Standard FM sythesis driver
#define MM_YAMAHA_GSS_WAVEOUT   0x02    // Yamaha Gold Sound Standard wave output driver
#define MM_YAMAHA_GSS_WAVEIN    0x03    // Yamaha Gold Sound Standard wave input driver
#define MM_YAMAHA_GSS_MIDIOUT   0x04    // Yamaha Gold Sound Standard midi output driver
#define MM_YAMAHA_GSS_MIDIIN    0x05    // Yamaha Gold Sound Standard midi input driver
#define MM_YAMAHA_GSS_AUX       0x06    // Yamaha Gold Sound Standard auxillary driver for mixer functions

/* MM_EVEREX Product IDs */

#define MM_EVEREX_CARRIER       0x01    // Everex Carrier SL/25 Notebook

/* MM_ECHO Product IDs */

#define MM_ECHO_SYNTH   0x01    // Echo EuSythesis driver
#define MM_ECHO_WAVEOUT 0x02    // Wave output driver
#define MM_ECHO_WAVEIN  0x03    // Wave input driver
#define MM_ECHO_MIDIOUT 0x04    // MIDI output driver
#define MM_ECHO_MIDIIN  0x05    // MIDI input driver
#define MM_ECHO_AUX     0x06    // auxillary driver for mixer functions


/* MM_SIERRA Product IDs */

#define MM_SIERRA_ARIA_MIDIOUT  0x14    // Sierra Aria MIDI output
#define MM_SIERRA_ARIA_MIDIIN   0x15    // Sierra Aria MIDI input
#define MM_SIERRA_ARIA_SYNTH    0x16    // Sierra Aria Synthesizer
#define MM_SIERRA_ARIA_WAVEOUT  0x17    // Sierra Aria Waveform output
#define MM_SIERRA_ARIA_WAVEIN   0x18    // Sierra Aria Waveform input
#define MM_SIERRA_ARIA_AUX      0x19    // Siarra Aria Auxiliary device

/* MM_CAT Product IDs */

/* MM_APPS Product IDs */

/* MM_DSP_GROUP Product IDs */

#define MM_DSP_GROUP_TRUESPEECH 0x01    // High quality 9.54:1 Speech Compression Vocoder

/* MM_MELABS Product IDs */

#define MM_MELABS_MIDI2GO       0x01    // parellel port MIDI interface

/* MM_COMPUTER_FRIENDS Product IDs */

/* MM_ESS Product IDs */

#define MM_ESS_AMWAVEOUT        0x01    // ESS Audio Magician Waveform Output Port
#define MM_ESS_AMWAVEIN         0x02    // ESS Audio Magician Waveform Input Port
#define MM_ESS_AMAUX            0x03    // ESS Audio Magician Auxiliary Port
#define MM_ESS_AMSYNTH          0x04    // ESS Audio Magician Internal Music Synthesizer Port
#define MM_ESS_AMMIDIOOUT       0x05    // ESS Audio Magician MIDI Output Port
#define MM_ESS_AMMIDIIN         0x06    // ESS Audio Magician MIDI Input Port

/* MM_TRUEVISION Product IDs */
#define MM_TRUEVISION_WAVEIN1   1
#define MM_TRUEVISION_WAVEOUT1  2

/* MM_AZTECH Product ID's */
#define MM_AZETCH_MIDIOUT       3
#define MM_AZETCH_MIDIIN        4
#define MM_AZETCH_WAVEIN        17
#define MM_AZETCH_WAVEOUT       18
#define MM_AZETCH_FMSYNTH       20
#define MM_AZETCH_PRO16_WAVEIN  33
#define MM_AZETCH_PRO16_WAVEOUT 34
#define MM_AZETCH_PRO16_FMSYNTH 38
#define MM_AZETCH_DSP16_WAVEIN  65
#define MM_AZETCH_DSP16_WAVEOUT 66
#define MM_AZETCH_DSP16_FMSYNTH 68
#define MM_AZETCH_DSP16_WAVESYNTH       70
#define MM_AZETCH_AUX_CD        401
#define MM_AZETCH_AUX_LINE      402
#define MM_AZETCH_AUX_MIC       403

/* MM_VIDEOLOGIC Product IDs */
#define MM_VIDEOLOGIC_MSWAVEIN  1
#define MM_VIDEOLOGIC_MSWAVEOUT 2

/* MM_APT Product ID's */
#define MM_APT_ACE100CD         1

/*MM_ICS Product ID's  */
#define MM_ICS_BIZAUDIO_WAVEOUT 1

/* MM_KORG Product ID's */
#define MM_KORG_PCIF_MIDIOUT    1       /* Korg PC I/F Driver */
#define MM_KORG_PCIF_MIDIIN     2       /* Korg PC I/F Driver */

/* MM_METHEUS product ID's */
#define MM_METHEUS_ZIPPER       1

/* MM_WINNOV Products IDs  */
#define MM_WINNOV_CAVIAR_WAVEIN         1
#define MM_WINNOV_CAVIAR_WAVEOUT        2
#define MM_WINNOV_CAVIAR_VIDC           3
#define MM_WINNOV_CAVIAR_CHAMPAGNE      4 /* FOURCC is CHAM */
#define MM_WINNOV_CAVIAR_YUV8           5 /* FOURCC is YUV8 */


#endif

/*////////////////////////////////////////////////////////////////////////// */

/*              INFO LIST CHUNKS (from the Multimedia Programmer's Reference
					plus new ones)
*/
#define RIFFINFO_IARL      mmioFOURCC ('I', 'A', 'R', 'L')     /*Archival location  */
#define RIFFINFO_IART      mmioFOURCC ('I', 'A', 'R', 'T')     /*Artist  */
#define RIFFINFO_ICMS      mmioFOURCC ('I', 'C', 'M', 'S')     /*Commissioned  */
#define RIFFINFO_ICMT      mmioFOURCC ('I', 'C', 'M', 'T')     /*Comments  */
#define RIFFINFO_ICOP      mmioFOURCC ('I', 'C', 'O', 'P')     /*Copyright  */
#define RIFFINFO_ICRD      mmioFOURCC ('I', 'C', 'R', 'D')     /*Creation date of subject  */
#define RIFFINFO_ICRP      mmioFOURCC ('I', 'C', 'R', 'P')     /*Cropped  */
#define RIFFINFO_IDIM      mmioFOURCC ('I', 'D', 'I', 'M')     /*Dimensions  */
#define RIFFINFO_IDPI      mmioFOURCC ('I', 'D', 'P', 'I')     /*Dots per inch  */
#define RIFFINFO_IENG      mmioFOURCC ('I', 'E', 'N', 'G')     /*Engineer  */
#define RIFFINFO_IGNR      mmioFOURCC ('I', 'G', 'N', 'R')     /*Genre  */
#define RIFFINFO_IKEY      mmioFOURCC ('I', 'K', 'E', 'Y')     /*Keywords  */
#define RIFFINFO_ILGT      mmioFOURCC ('I', 'L', 'G', 'T')     /*Lightness settings  */
#define RIFFINFO_IMED      mmioFOURCC ('I', 'M', 'E', 'D')     /*Medium  */
#define RIFFINFO_INAM      mmioFOURCC ('I', 'N', 'A', 'M')     /*Name of subject  */
#define RIFFINFO_IPLT      mmioFOURCC ('I', 'P', 'L', 'T')     /*Palette Settings. No. of colors requested.   */
#define RIFFINFO_IPRD      mmioFOURCC ('I', 'P', 'R', 'D')     /*Product  */
#define RIFFINFO_ISBJ      mmioFOURCC ('I', 'S', 'B', 'J')     /*Subject description  */
#define RIFFINFO_ISFT      mmioFOURCC ('I', 'S', 'F', 'T')     /*Software. Name of package used to create file.  */
#define RIFFINFO_ISHP      mmioFOURCC ('I', 'S', 'H', 'P')     /*Sharpness.  */
#define RIFFINFO_ISRC      mmioFOURCC ('I', 'S', 'R', 'C')     /*Source.   */
#define RIFFINFO_ISRF      mmioFOURCC ('I', 'S', 'R', 'F')     /*Source Form. ie slide, paper  */
#define RIFFINFO_ITCH      mmioFOURCC ('I', 'T', 'C', 'H')     /*Technician who digitized the subject.  */

/* New INFO Chunks as of August 30, 1993: */
#define RIFFINFO_ISMP      mmioFOURCC ('I', 'S', 'M', 'P')     /*SMPTE time code  */
/* ISMP: SMPTE time code of digitization start point expressed as a NULL terminated
		text string "HH:MM:SS:FF". If performing MCI capture in AVICAP, this
		chunk will be automatically set based on the MCI start time.
*/
#define RIFFINFO_IDIT      mmioFOURCC ('I', 'D', 'I', 'T')     /*Digitization Time  */
/* IDIT: "Digitization Time" Specifies the time and date that the digitization commenced.
		The digitization time is contained in an ASCII string which 
		contains exactly 26 characters and is in the format 
		"Wed Jan 02 02:03:55 1990\n\0".
		The ctime(), asctime(), functions can be used to create strings
		in this format. This chunk is automatically added to the capture 
		file based on the current system time at the moment capture is initiated.
*/

/*Template line for new additions*/
/*#define RIFFINFO_I      mmioFOURCC ('I', '', '', '')        */


/*/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/

#ifndef NONEWWAVE

/* WAVE form wFormatTag IDs */
#define WAVE_FORMAT_UNKNOWN             (0x0000)
#define WAVE_FORMAT_ADPCM               (0x0002)
#define WAVE_FORMAT_IBM_CVSD            (0x0005)
#define WAVE_FORMAT_ALAW                (0x0006)
#define WAVE_FORMAT_MULAW               (0x0007)
#define WAVE_FORMAT_OKI_ADPCM           (0x0010)
#define WAVE_FORMAT_DVI_ADPCM           (0x0011)
#define WAVE_FORMAT_IMA_ADPCM           (WAVE_FORMAT_DVI_ADPCM)
#define WAVE_FORMAT_DIGISTD             (0x0015)
#define WAVE_FORMAT_DIGIFIX             (0x0016)
#define WAVE_FORMAT_YAMAHA_ADPCM        (0x0020)
#define WAVE_FORMAT_SONARC              (0x0021)
#define WAVE_FORMAT_DSPGROUP_TRUESPEECH (0x0022)
#define WAVE_FORMAT_ECHOSC1             (0x0023)
#define WAVE_FORMAT_AUDIOFILE_AF36      (0x0024)
#define WAVE_FORMAT_CREATIVE_ADPCM      (0x0200)
#define WAVE_FORMAT_APTX                (0x0025)
#define WAVE_FORMAT_AUDIOFILE_AF10      (0X0026)
#define WAVE_FORMAT_DOLBY_AC2           (0X0030)
#define WAVE_FORMAT_MEDIASPACE_ADPCM    (0x0012)
#define WAVE_FORMAT_SIERRA_ADPCM        (0x0013)
#define WAVE_FORMAT_G723_ADPCM          (0x0014)
#define WAVE_FORMAT_GSM610              (0x0031)
#define WAVE_FORMAT_G721_ADPCM          (0x0040)




//
//  the WAVE_FORMAT_DEVELOPMENT format tag can be used during the
//  development phase of a new wave format.  Before shipping, you MUST
//  acquire an official format tag from Microsoft.
//
#define WAVE_FORMAT_DEVELOPMENT         (0xFFFF)

#endif /* NONEWWAVE */


#ifndef WAVE_FORMAT_PCM

/* general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} WAVEFORMAT;
typedef WAVEFORMAT       *PWAVEFORMAT;
typedef WAVEFORMAT NEAR *NPWAVEFORMAT;
typedef WAVEFORMAT FAR  *LPWAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
} PCMWAVEFORMAT;
typedef PCMWAVEFORMAT       *PPCMWAVEFORMAT;
typedef PCMWAVEFORMAT NEAR *NPPCMWAVEFORMAT;
typedef PCMWAVEFORMAT FAR  *LPPCMWAVEFORMAT;


#endif /* WAVE_FORMAT_PCM */



/* general extended waveform format structure 
   Use this for all NON PCM formats 
   (information common to all formats)
*/
#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_
typedef struct tWAVEFORMATEX
{
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo...) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
    WORD    wBitsPerSample;    /* Number of bits per sample of mono data */
    WORD    cbSize;            /* The count in bytes of the size of
				    extra information (after cbSize) */

} WAVEFORMATEX;
typedef WAVEFORMATEX       *PWAVEFORMATEX;
typedef WAVEFORMATEX NEAR *NPWAVEFORMATEX;
typedef WAVEFORMATEX FAR  *LPWAVEFORMATEX;
#endif /* _WAVEFORMATEX_ */


#ifndef NONEWWAVE

/* Define data for MS ADPCM */

typedef struct adpcmcoef_tag {
	short   iCoef1;
	short   iCoef2;
} ADPCMCOEFSET;
typedef ADPCMCOEFSET       *PADPCMCOEFSET;
typedef ADPCMCOEFSET NEAR *NPADPCMCOEFSET;
typedef ADPCMCOEFSET FAR  *LPADPCMCOEFSET;


/*
 *  this pragma disables the warning issued by the Microsoft C compiler
 *  when using a zero size array as place holder when compiling for
 *  C++ or with -W4.
 *
 */
#ifdef _MSC_VER
#pragma warning(disable:4200)
#endif

typedef struct adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
	WORD            wNumCoef;
	ADPCMCOEFSET    aCoef[];
} ADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT       *PADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT NEAR *NPADPCMWAVEFORMAT;
typedef ADPCMWAVEFORMAT FAR  *LPADPCMWAVEFORMAT;

#ifdef _MSC_VER
#pragma warning(default:4200)
#endif

//
//  Intel's DVI ADPCM structure definitions
//
//      for WAVE_FORMAT_DVI_ADPCM   (0x0011)
//
//

typedef struct dvi_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} DVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT       *PDVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT NEAR *NPDVIADPCMWAVEFORMAT;
typedef DVIADPCMWAVEFORMAT FAR  *LPDVIADPCMWAVEFORMAT;


//
//  IMA endorsed ADPCM structure definitions--note that this is exactly
//  the same format as Intel's DVI ADPCM.
//
//      for WAVE_FORMAT_IMA_ADPCM   (0x0011)
//
//

typedef struct ima_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wSamplesPerBlock;
} IMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT       *PIMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT NEAR *NPIMAADPCMWAVEFORMAT;
typedef IMAADPCMWAVEFORMAT FAR  *LPIMAADPCMWAVEFORMAT;


//
//  Speech Compression's Sonarc structure definitions
//
//      for WAVE_FORMAT_SONARC   (0x0021)
//
//

typedef struct sonarcwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wCompType;
} SONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT       *PSONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT NEAR *NPSONARCWAVEFORMAT;
typedef SONARCWAVEFORMAT FAR  *LPSONARCWAVEFORMAT;

//
//  DSP Groups's TRUESPEECH structure definitions
//
//      for WAVE_FORMAT_DSPGROUP_TRUESPEECH   (0x0022)
//
//

typedef struct truespeechwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
	WORD            nSamplesPerBlock;
	BYTE            abReserved[28];
} TRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT       *PTRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT NEAR *NPTRUESPEECHWAVEFORMAT;
typedef TRUESPEECHWAVEFORMAT FAR  *LPTRUESPEECHWAVEFORMAT;



//
//  Creative's ADPCM structure definitions
//
//      for WAVE_FORMAT_CREATIVE_ADPCM   (0x0200)
//
//

typedef struct creative_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
} CREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT       *PCREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT NEAR *NPCREATIVEADPCMWAVEFORMAT;
typedef CREATIVEADPCMWAVEFORMAT FAR  *LPCREATIVEADPCMWAVEFORMAT;

/*
//VideoLogic's Media Space ADPCM Structure definitions
// for  WAVE_FORMAT_MEDIASPACE_ADPCM    (0x0012)
//
//
*/
typedef struct mediaspace_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD    wRevision;
} MEDIASPACEADPCMWAVEFORMAT;
typedef MEDIASPACEADPCMWAVEFORMAT           *PMEDIASPACEADPCMWAVEFORMAT;
typedef MEDIASPACEADPCMWAVEFORMAT NEAR     *NPMEDIASPACEADPCMWAVEFORMAT;
typedef MEDIASPACEADPCMWAVEFORMAT FAR      *LPMEDIASPACEADPCMWAVEFORMAT;

/*
// Sierra Semiconductor ADPCM 
*/
typedef struct sierra_adpcmwaveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            wRevision;
} SIERRAADPCMWAVEFORMAT;
typedef SIERRAADPCMWAVEFORMAT           *PSIERRAADPCMWAVEFORMAT;
typedef SIERRAADPCMWAVEFORMAT NEAR     *NPSIERRAADPCMWAVEFORMAT;
typedef SIERRAADPCMWAVEFORMAT FAR      *LPSIERRAADPCMWAVEFORMAT;

/* Dolby's AC-2 wave format structure definition */
typedef struct dolbyac2waveformat_tag {
	WAVEFORMATEX    wfx;
	WORD            nAuxBitsCode;
} DOLBYAC2WAVEFORMAT;







//==========================================================================;
//
//  ACM Wave Filters
//
//
//==========================================================================;

#ifndef _ACM_WAVEFILTER
#define _ACM_WAVEFILTER
    
#define WAVE_FILTER_UNKNOWN	    0x0000
#define WAVE_FILTER_DEVELOPMENT	   (0xFFFF)
            
typedef struct wavefilter_tag {
    DWORD   cbStruct;           /* Size of the filter in bytes */
    DWORD   dwFilterTag;        /* fitler type */
    DWORD   fdwFilter;          /* Flags for the filter (Universal Dfns) */
    DWORD   dwReserved[5];      /* Reserved for system use */
} WAVEFILTER;
typedef WAVEFILTER       *PWAVEFILTER;
typedef WAVEFILTER NEAR *NPWAVEFILTER;
typedef WAVEFILTER FAR  *LPWAVEFILTER;

#endif  /* _ACM_WAVEFILTER */


#ifndef WAVE_FILTER_VOLUME
#define WAVE_FILTER_VOLUME      0x0001


typedef struct wavefilter_volume_tag {
        WAVEFILTER      wfltr;
        DWORD           dwVolume;
} VOLUMEWAVEFILTER;
typedef VOLUMEWAVEFILTER       *PVOLUMEWAVEFILTER;
typedef VOLUMEWAVEFILTER NEAR *NPVOLUMEWAVEFILTER;
typedef VOLUMEWAVEFILTER FAR  *LPVOLUMEWAVEFILTER;

#endif  /* WAVE_FILTER_VOLUME */

#ifndef WAVE_FILTER_ECHO
#define WAVE_FILTER_ECHO        0x0002


typedef struct wavefilter_echo_tag {
        WAVEFILTER      wfltr;
        DWORD           dwVolume;
        DWORD           dwDelay;
} ECHOWAVEFILTER;
typedef ECHOWAVEFILTER       *PECHOWAVEFILTER;
typedef ECHOWAVEFILTER NEAR *NPECHOWAVEFILTER;
typedef ECHOWAVEFILTER FAR  *LPECHOWAVEFILTER;

#endif  /* WAVEFILTER_ECHO */
    



/*//////////////////////////////////////////////////////////////////////////
//
// New RIFF WAVE Chunks
//
*/

#define RIFFWAVE_inst   mmioFOURCC('i','n','s','t')

struct tag_s_RIFFWAVE_inst {
    BYTE    bUnshiftedNote;
    char    chFineTune;
    char    chGain;
    BYTE    bLowNote;
    BYTE    bHighNote;
    BYTE    bLowVelocity;
    BYTE    bHighVelocity;
};

typedef struct tag_s_RIFFWAVE_INST s_RIFFWAVE_inst;

#endif

/*//////////////////////////////////////////////////////////////////////////
//
// New RIFF Forms
//
*/

#ifndef NONEWRIFF

/* RIFF AVI */

//
// AVI file format is specified in a seperate file (AVIFMT.H),
// which is available from the sources listed in MSFTMM
//

/* RIFF CPPO */

#define RIFFCPPO        mmioFOURCC('C','P','P','O')

#define RIFFCPPO_objr   mmioFOURCC('o','b','j','r')
#define RIFFCPPO_obji   mmioFOURCC('o','b','j','i')

#define RIFFCPPO_clsr   mmioFOURCC('c','l','s','r')
#define RIFFCPPO_clsi   mmioFOURCC('c','l','s','i')

#define RIFFCPPO_mbr    mmioFOURCC('m','b','r',' ')

#define RIFFCPPO_char   mmioFOURCC('c','h','a','r')


#define RIFFCPPO_byte   mmioFOURCC('b','y','t','e')
#define RIFFCPPO_int    mmioFOURCC('i','n','t',' ')
#define RIFFCPPO_word   mmioFOURCC('w','o','r','d')
#define RIFFCPPO_long   mmioFOURCC('l','o','n','g')
#define RIFFCPPO_dwrd   mmioFOURCC('d','w','r','d')
#define RIFFCPPO_flt    mmioFOURCC('f','l','t',' ')
#define RIFFCPPO_dbl    mmioFOURCC('d','b','l',' ')
#define RIFFCPPO_str    mmioFOURCC('s','t','r',' ')


#endif

/*
//////////////////////////////////////////////////////////////////////////
//
// DIB Compression Defines
//
*/

#ifndef BI_BITFIELDS
#define BI_BITFIELDS    3
#endif

#ifndef QUERYDIBSUPPORT

#define QUERYDIBSUPPORT 3073
#define QDI_SETDIBITS   0x0001
#define QDI_GETDIBITS   0x0002
#define QDI_DIBTOSCREEN 0x0004
#define QDI_STRETCHDIB  0x0008


#endif

#ifndef NOBITMAP
/* Structure definitions */

typedef struct tagEXBMINFOHEADER {
	BITMAPINFOHEADER    bmi;
	/* extended BITMAPINFOHEADER fields */
	DWORD   biExtDataOffset;
	
	/* Other stuff will go here */

	/* ... */

	/* Format-specific information */
	/* biExtDataOffset points here */
	
} EXBMINFOHEADER;

#endif	//NOBITMAP

/* New DIB Compression Defines */

#define BICOMP_IBMULTIMOTION    mmioFOURCC('U', 'L', 'T', 'I')
#define BICOMP_IBMPHOTOMOTION   mmioFOURCC('P', 'H', 'M', 'O')
#define BICOMP_CREATIVEYUV      mmioFOURCC('c', 'y', 'u', 'v')

#ifndef NOJPEGDIB

/* New DIB Compression Defines */
#define JPEG_DIB        mmioFOURCC('J','P','E','G')    /* Still image JPEG DIB biCompression */
#define MJPG_DIB        mmioFOURCC('M','J','P','G')    /* Motion JPEG DIB biCompression     */

/* JPEGProcess Definitions */
#define JPEG_PROCESS_BASELINE           0       /* Baseline DCT */

/* AVI File format extensions */
#define AVIIF_CONTROLFRAME              0x00000200L     /* This is a control frame */

    /* JIF Marker byte pairs in JPEG Interchange Format sequence */
#define JIFMK_SOF0    0xFFC0   /* SOF Huff  - Baseline DCT*/
#define JIFMK_SOF1    0xFFC1   /* SOF Huff  - Extended sequential DCT*/
#define JIFMK_SOF2    0xFFC2   /* SOF Huff  - Progressive DCT*/
#define JIFMK_SOF3    0xFFC3   /* SOF Huff  - Spatial (sequential) lossless*/
#define JIFMK_SOF5    0xFFC5   /* SOF Huff  - Differential sequential DCT*/
#define JIFMK_SOF6    0xFFC6   /* SOF Huff  - Differential progressive DCT*/
#define JIFMK_SOF7    0xFFC7   /* SOF Huff  - Differential spatial*/
#define JIFMK_JPG     0xFFC8   /* SOF Arith - Reserved for JPEG extensions*/
#define JIFMK_SOF9    0xFFC9   /* SOF Arith - Extended sequential DCT*/
#define JIFMK_SOF10   0xFFCA   /* SOF Arith - Progressive DCT*/
#define JIFMK_SOF11   0xFFCB   /* SOF Arith - Spatial (sequential) lossless*/
#define JIFMK_SOF13   0xFFCD   /* SOF Arith - Differential sequential DCT*/
#define JIFMK_SOF14   0xFFCE   /* SOF Arith - Differential progressive DCT*/
#define JIFMK_SOF15   0xFFCF   /* SOF Arith - Differential spatial*/
#define JIFMK_DHT     0xFFC4   /* Define Huffman Table(s) */
#define JIFMK_DAC     0xFFCC   /* Define Arithmetic coding conditioning(s) */
#define JIFMK_RST0    0xFFD0   /* Restart with modulo 8 count 0 */
#define JIFMK_RST1    0xFFD1   /* Restart with modulo 8 count 1 */
#define JIFMK_RST2    0xFFD2   /* Restart with modulo 8 count 2 */
#define JIFMK_RST3    0xFFD3   /* Restart with modulo 8 count 3 */
#define JIFMK_RST4    0xFFD4   /* Restart with modulo 8 count 4 */
#define JIFMK_RST5    0xFFD5   /* Restart with modulo 8 count 5 */
#define JIFMK_RST6    0xFFD6   /* Restart with modulo 8 count 6 */
#define JIFMK_RST7    0xFFD7   /* Restart with modulo 8 count 7 */
#define JIFMK_SOI     0xFFD8   /* Start of Image */
#define JIFMK_EOI     0xFFD9   /* End of Image */
#define JIFMK_SOS     0xFFDA   /* Start of Scan */
#define JIFMK_DQT     0xFFDB   /* Define quantization Table(s) */
#define JIFMK_DNL     0xFFDC   /* Define Number of Lines */
#define JIFMK_DRI     0xFFDD   /* Define Restart Interval */
#define JIFMK_DHP     0xFFDE   /* Define Hierarchical progression */
#define JIFMK_EXP     0xFFDF   /* Expand Reference Component(s) */
#define JIFMK_APP0    0xFFE0   /* Application Field 0*/
#define JIFMK_APP1    0xFFE1   /* Application Field 1*/
#define JIFMK_APP2    0xFFE2   /* Application Field 2*/
#define JIFMK_APP3    0xFFE3   /* Application Field 3*/
#define JIFMK_APP4    0xFFE4   /* Application Field 4*/
#define JIFMK_APP5    0xFFE5   /* Application Field 5*/
#define JIFMK_APP6    0xFFE6   /* Application Field 6*/
#define JIFMK_APP7    0xFFE7   /* Application Field 7*/
#define JIFMK_JPG0    0xFFF0   /* Reserved for JPEG extensions */
#define JIFMK_JPG1    0xFFF1   /* Reserved for JPEG extensions */
#define JIFMK_JPG2    0xFFF2   /* Reserved for JPEG extensions */
#define JIFMK_JPG3    0xFFF3   /* Reserved for JPEG extensions */
#define JIFMK_JPG4    0xFFF4   /* Reserved for JPEG extensions */
#define JIFMK_JPG5    0xFFF5   /* Reserved for JPEG extensions */
#define JIFMK_JPG6    0xFFF6   /* Reserved for JPEG extensions */
#define JIFMK_JPG7    0xFFF7   /* Reserved for JPEG extensions */
#define JIFMK_JPG8    0xFFF8   /* Reserved for JPEG extensions */
#define JIFMK_JPG9    0xFFF9   /* Reserved for JPEG extensions */
#define JIFMK_JPG10   0xFFFA   /* Reserved for JPEG extensions */
#define JIFMK_JPG11   0xFFFB   /* Reserved for JPEG extensions */
#define JIFMK_JPG12   0xFFFC   /* Reserved for JPEG extensions */
#define JIFMK_JPG13   0xFFFD   /* Reserved for JPEG extensions */
#define JIFMK_COM     0xFFFE   /* Comment */
#define JIFMK_TEM     0xFF01   /* for temp private use arith code */
#define JIFMK_RES     0xFF02   /* Reserved */
#define JIFMK_00      0xFF00   /* Zero stuffed byte - entropy data */
#define JIFMK_FF      0xFFFF   /* Fill byte */

 
/* JPEGColorSpaceID Definitions */
#define JPEG_Y          1       /* Y only component of YCbCr */
#define JPEG_YCbCr      2       /* YCbCr as define by CCIR 601 */
#define JPEG_RGB        3       /* 3 component RGB */

/* Structure definitions */

typedef struct tagJPEGINFOHEADER {
    /* compression-specific fields */
    /* these fields are defined for 'JPEG' and 'MJPG' */
    DWORD       JPEGSize;
    DWORD       JPEGProcess;

    /* Process specific fields */
    DWORD       JPEGColorSpaceID;
    DWORD       JPEGBitsPerSample;
    DWORD       JPEGHSubSampling;
    DWORD       JPEGVSubSampling;
} JPEGINFOHEADER;


#ifdef MJPGDHTSEG_STORAGE

/* Default DHT Segment */

MJPGHDTSEG_STORAGE BYTE MJPGDHTSeg[0x1A0] = {
 /* JPEG DHT Segment for YCrCb omitted from MJPG data */
0xFF,0xC4,0xA2,0x01,
0x00,0x00,0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, 
0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x10,0x00,0x03,0x01,0x01,0x01,0x01, 
0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07, 
0x08,0x09,0x0A,0x0B,0x01,0x00,0x02,0x01,0x03,0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00, 
0x00,0x01,0x7D,0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,0x13,0x51,0x61, 
0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24, 
0x33,0x62,0x72,0x82,0x09,0x0A,0x16,0x17,0x18,0x19,0x1A,0x25,0x26,0x27,0x28,0x29,0x2A,0x34, 
0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56, 
0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78, 
0x79,0x7A,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99, 
0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9, 
0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9, 
0xDA,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7, 
0xF8,0xF9,0xFA,0x11,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x04,0x00,0x01, 
0x02,0x77,0x00,0x01,0x02,0x03,0x11,0x04,0x05,0x21,0x31,0x06,0x12,0x41,0x51,0x07,0x61,0x71, 
0x13,0x22,0x32,0x81,0x08,0x14,0x42,0x91,0xA1,0xB1,0xC1,0x09,0x23,0x33,0x52,0xF0,0x15,0x62, 
0x72,0xD1,0x0A,0x16,0x24,0x34,0xE1,0x25,0xF1,0x17,0x18,0x19,0x1A,0x26,0x27,0x28,0x29,0x2A, 
0x35,0x36,0x37,0x38,0x39,0x3A,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x53,0x54,0x55,0x56, 
0x57,0x58,0x59,0x5A,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x73,0x74,0x75,0x76,0x77,0x78, 
0x79,0x7A,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x92,0x93,0x94,0x95,0x96,0x97,0x98, 
0x99,0x9A,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8, 
0xB9,0xBA,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8, 
0xD9,0xDA,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8, 
0xF9,0xFA
};

/* End DHT default */
#endif

/* End JPEG */
#endif

/*//////////////////////////////////////////////////////////////////////////
//
// Defined IC types
*/

#ifndef NONEWIC

#ifndef ICTYPE_VIDEO
#define ICTYPE_VIDEO    mmioFOURCC('v', 'i', 'd', 'c')
#define ICTYPE_AUDIO    mmioFOURCC('a', 'u', 'd', 'c')
#endif

#endif
/*
//   Misc. FOURCC registration
*/

/* Sierra Semiconductor: RDSP- Confidential RIFF file format  
//       for the storage and downloading of DSP
//       code for Audio and communications devices. 
*/
#define FOURCC_RDSP mmioFOURCC('R', 'D', 'S', 'P')



#ifndef RC_INVOKED
#pragma pack()          /* Revert to default packing */
#endif  /* RC_INVOKED */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#endif  /* _INC_MMREG */
