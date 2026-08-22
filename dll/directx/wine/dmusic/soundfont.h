/*
 * Copyright 2023 Rémi Bernon for CodeWeavers
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "stdarg.h"
#include "stddef.h"

#include "windef.h"
#include "winbase.h"

#include "wine/debug.h"

#include <pshpack1.h>

/* SoundFont 2.04 data structures, from http://www.synthfont.com/sfspec24.pdf */

struct sf_range
{
    BYTE low;
    BYTE high;
};

union sf_amount
{
    struct sf_range range;
    WORD value;
};

C_ASSERT(sizeof(union sf_amount) == 2);

enum
{
    SF_SAMPLE_MONO = 1,
    SF_SAMPLE_RIGHT = 2,
    SF_SAMPLE_LEFT = 4,
    SF_SAMPLE_LINKED = 8,
    SF_SAMPLE_ROM_MONO = 0x8001,
    SF_SAMPLE_ROM_RIGHT = 0x8002,
    SF_SAMPLE_ROM_LEFT = 0x8004,
    SF_SAMPLE_ROM_LINKED = 0x8008,
};
typedef WORD sf_sample_type;

enum
{
    SF_GEN_START_ADDRS_OFFSET = 0,
    SF_GEN_END_ADDRS_OFFSET = 1,
    SF_GEN_STARTLOOP_ADDRS_OFFSET = 2,
    SF_GEN_ENDLOOP_ADDRS_OFFSET = 3,
    SF_GEN_START_ADDRS_COARSE_OFFSET = 4,
    SF_GEN_MOD_LFO_TO_PITCH = 5,
    SF_GEN_VIB_LFO_TO_PITCH = 6,
    SF_GEN_MOD_ENV_TO_PITCH = 7,
    SF_GEN_INITIAL_FILTER_FC = 8,
    SF_GEN_INITIAL_FILTER_Q = 9,
    SF_GEN_MOD_LFO_TO_FILTER_FC = 10,
    SF_GEN_MOD_ENV_TO_FILTER_FC = 11,
    SF_GEN_END_ADDRS_COARSE_OFFSET = 12,
    SF_GEN_MOD_LFO_TO_VOLUME = 13,

    SF_GEN_CHORUS_EFFECTS_SEND = 15,
    SF_GEN_REVERB_EFFECTS_SEND = 16,
    SF_GEN_PAN = 17,

    SF_GEN_DELAY_MOD_LFO = 21,
    SF_GEN_FREQ_MOD_LFO = 22,
    SF_GEN_DELAY_VIB_LFO = 23,
    SF_GEN_FREQ_VIB_LFO = 24,
    SF_GEN_DELAY_MOD_ENV = 25,
    SF_GEN_ATTACK_MOD_ENV = 26,
    SF_GEN_HOLD_MOD_ENV = 27,
    SF_GEN_DECAY_MOD_ENV = 28,
    SF_GEN_SUSTAIN_MOD_ENV = 29,
    SF_GEN_RELEASE_MOD_ENV = 30,
    SF_GEN_KEYNUM_TO_MOD_ENV_HOLD = 31,
    SF_GEN_KEYNUM_TO_MOD_ENV_DECAY = 32,
    SF_GEN_DELAY_VOL_ENV = 33,
    SF_GEN_ATTACK_VOL_ENV = 34,
    SF_GEN_HOLD_VOL_ENV = 35,
    SF_GEN_DECAY_VOL_ENV = 36,
    SF_GEN_SUSTAIN_VOL_ENV = 37,
    SF_GEN_RELEASE_VOL_ENV = 38,
    SF_GEN_KEYNUM_TO_VOL_ENV_HOLD = 39,
    SF_GEN_KEYNUM_TO_VOL_ENV_DECAY = 40,
    SF_GEN_INSTRUMENT = 41,

    SF_GEN_KEY_RANGE = 43,
    SF_GEN_VEL_RANGE = 44,
    SF_GEN_STARTLOOP_ADDRS_COARSE_OFFSET = 45,
    SF_GEN_KEYNUM = 46,
    SF_GEN_VELOCITY = 47,
    SF_GEN_INITIAL_ATTENUATION = 48,

    SF_GEN_ENDLOOP_ADDRS_COARSE_OFFSET = 50,
    SF_GEN_COARSE_TUNE = 51,
    SF_GEN_FINE_TUNE = 52,
    SF_GEN_SAMPLE_ID = 53,
    SF_GEN_SAMPLE_MODES = 54,

    SF_GEN_SCALE_TUNING = 56,
    SF_GEN_EXCLUSIVE_CLASS = 57,
    SF_GEN_OVERRIDING_ROOT_KEY = 58,

    SF_GEN_END_OPER = 60,
};
typedef WORD sf_generator;

static inline const char *debugstr_sf_generator(sf_generator oper)
{
    switch (oper)
    {
    case SF_GEN_START_ADDRS_OFFSET: return "start_addrs_offset";
    case SF_GEN_END_ADDRS_OFFSET: return "end_addrs_offset";
    case SF_GEN_STARTLOOP_ADDRS_OFFSET: return "startloop_addrs_offset";
    case SF_GEN_ENDLOOP_ADDRS_OFFSET: return "endloop_addrs_offset";
    case SF_GEN_START_ADDRS_COARSE_OFFSET: return "start_addrs_coarse_offset";
    case SF_GEN_MOD_LFO_TO_PITCH: return "mod_lfo_to_pitch";
    case SF_GEN_VIB_LFO_TO_PITCH: return "vib_lfo_to_pitch";
    case SF_GEN_MOD_ENV_TO_PITCH: return "mod_env_to_pitch";
    case SF_GEN_INITIAL_FILTER_FC: return "initial_filter_fc";
    case SF_GEN_INITIAL_FILTER_Q: return "initial_filter_q";
    case SF_GEN_MOD_LFO_TO_FILTER_FC: return "mod_lfo_to_filter_fc";
    case SF_GEN_MOD_ENV_TO_FILTER_FC: return "mod_env_to_filter_fc";
    case SF_GEN_END_ADDRS_COARSE_OFFSET: return "end_addrs_coarse_offset";
    case SF_GEN_MOD_LFO_TO_VOLUME: return "mod_lfo_to_volume";
    case SF_GEN_CHORUS_EFFECTS_SEND: return "chorus_effects_send";
    case SF_GEN_REVERB_EFFECTS_SEND: return "reverb_effects_send";
    case SF_GEN_PAN: return "pan";
    case SF_GEN_DELAY_MOD_LFO: return "delay_mod_lfo";
    case SF_GEN_FREQ_MOD_LFO: return "freq_mod_lfo";
    case SF_GEN_DELAY_VIB_LFO: return "delay_vib_lfo";
    case SF_GEN_FREQ_VIB_LFO: return "freq_vib_lfo";
    case SF_GEN_DELAY_MOD_ENV: return "delay_mod_env";
    case SF_GEN_ATTACK_MOD_ENV: return "attack_mod_env";
    case SF_GEN_HOLD_MOD_ENV: return "hold_mod_env";
    case SF_GEN_DECAY_MOD_ENV: return "decay_mod_env";
    case SF_GEN_SUSTAIN_MOD_ENV: return "sustain_mod_env";
    case SF_GEN_RELEASE_MOD_ENV: return "release_mod_env";
    case SF_GEN_KEYNUM_TO_MOD_ENV_HOLD: return "keynum_to_mod_env_hold";
    case SF_GEN_KEYNUM_TO_MOD_ENV_DECAY: return "keynum_to_mod_env_decay";
    case SF_GEN_DELAY_VOL_ENV: return "delay_vol_env";
    case SF_GEN_ATTACK_VOL_ENV: return "attack_vol_env";
    case SF_GEN_HOLD_VOL_ENV: return "hold_vol_env";
    case SF_GEN_DECAY_VOL_ENV: return "decay_vol_env";
    case SF_GEN_SUSTAIN_VOL_ENV: return "sustain_vol_env";
    case SF_GEN_RELEASE_VOL_ENV: return "release_vol_env";
    case SF_GEN_KEYNUM_TO_VOL_ENV_HOLD: return "keynum_to_vol_env_hold";
    case SF_GEN_KEYNUM_TO_VOL_ENV_DECAY: return "keynum_to_vol_env_decay";
    case SF_GEN_INSTRUMENT: return "instrument";
    case SF_GEN_KEY_RANGE: return "key_range";
    case SF_GEN_VEL_RANGE: return "vel_range";
    case SF_GEN_STARTLOOP_ADDRS_COARSE_OFFSET: return "startloop_addrs_coarse_offset";
    case SF_GEN_KEYNUM: return "keynum";
    case SF_GEN_VELOCITY: return "velocity";
    case SF_GEN_INITIAL_ATTENUATION: return "initial_attenuation";
    case SF_GEN_ENDLOOP_ADDRS_COARSE_OFFSET: return "endloop_addrs_coarse_offset";
    case SF_GEN_COARSE_TUNE: return "coarse_tune";
    case SF_GEN_FINE_TUNE: return "fine_tune";
    case SF_GEN_SAMPLE_ID: return "sample_id";
    case SF_GEN_SAMPLE_MODES: return "sample_modes";
    case SF_GEN_SCALE_TUNING: return "scale_tuning";
    case SF_GEN_EXCLUSIVE_CLASS: return "exclusive_class";
    case SF_GEN_OVERRIDING_ROOT_KEY: return "overriding_root_key";
    case SF_GEN_END_OPER: return "end_oper";
    }

    return wine_dbg_sprintf("%u", oper);
}

enum
{
    /* sf_modulator is a set of flags ored together */

    SF_MOD_CTRL_GEN_NONE = 0,
    SF_MOD_CTRL_GEN_VELOCITY = 0x2,
    SF_MOD_CTRL_GEN_KEY = 0x3,
    SF_MOD_CTRL_GEN_POLY_PRESSURE = 0xa,
    SF_MOD_CTRL_GEN_CHAN_PRESSURE = 0xd,
    SF_MOD_CTRL_GEN_PITCH_WHEEL = 0xe,
    SF_MOD_CTRL_GEN_PITCH_WHEEL_SENSITIVITY = 0x10,
    SF_MOD_CTRL_GEN_LINK = 0x7f,

    SF_MOD_CTRL_GEN = 0 << 7,
    SF_MOD_CTRL_MIDI = 1 << 7, /* with LSB: MIDI CC */

    SF_MOD_DIR_INCREASING = 0 << 8,
    SF_MOD_DIR_DECREASING = 1 << 8,

    SF_MOD_POL_UNIPOLAR = 0 << 9,
    SF_MOD_POL_BIPOLAR = 1 << 9,

    SF_MOD_SRC_LINEAR = 0 << 10,
    SF_MOD_SRC_CONCAVE = 1 << 10,
    SF_MOD_SRC_CONVEX = 2 << 10,
    SF_MOD_SRC_SWITCH = 3 << 10,
};
typedef WORD sf_modulator;

enum
{
    SF_TRAN_LINEAR = 0,
    SF_TRAN_ABSOLUTE = 2,
};
typedef WORD sf_transform;

struct sf_preset /* <phdr-rec> */
{
    char name[20];
    WORD preset;
    WORD bank;
    WORD bag_ndx;
    DWORD library;
    DWORD genre;
    DWORD morphology;
};

C_ASSERT(sizeof(struct sf_preset) == 38);

struct sf_bag /* <pbag-rec> / <ibag-rec> */
{
    WORD gen_ndx;
    WORD mod_ndx;
};

C_ASSERT(sizeof(struct sf_bag) == 4);

struct sf_mod /* <pmod-rec> / <imod-rec> */
{
    sf_modulator src_mod;
    sf_generator dest_gen;
    SHORT amount;
    sf_modulator amount_src_mod;
    sf_transform transform;
};

C_ASSERT(sizeof(struct sf_mod) == 10);

static inline const char *debugstr_sf_mod(struct sf_mod *mod)
{
    const char *dest_name = debugstr_sf_generator(mod->dest_gen);
    return wine_dbg_sprintf("%#x x %#x -> %s: %d (%#x)", mod->src_mod, mod->amount_src_mod, dest_name, mod->amount, mod->transform);
}

struct sf_gen /* <pgen-rec> / <igen-rec> */
{
    sf_generator oper;
    union sf_amount amount;
};

C_ASSERT(sizeof(struct sf_gen) == 4);

static inline const char *debugstr_sf_gen(struct sf_gen *gen)
{
    const char *name = debugstr_sf_generator(gen->oper);

    switch (gen->oper)
    {
    case SF_GEN_KEY_RANGE:
    case SF_GEN_VEL_RANGE:
        return wine_dbg_sprintf("%s: %u-%u", name, gen->amount.range.low, gen->amount.range.high);
    default:
        return wine_dbg_sprintf("%s: %u", name, gen->amount.value);
    }
}

struct sf_instrument /* <inst-rec> */
{
    char name[20];
    WORD bag_ndx;
};

C_ASSERT(sizeof(struct sf_instrument) == 22);

struct sf_sample /* <shdr-rec> */
{
    char name[20];
    DWORD start;
    DWORD end;
    DWORD start_loop;
    DWORD end_loop;
    DWORD sample_rate;
    BYTE original_key;
    char correction;
    WORD sample_link;
    sf_sample_type sample_type;
};

C_ASSERT(sizeof(struct sf_sample) == 46);

#include <poppack.h>

struct soundfont
{
    UINT preset_count;
    struct sf_preset *phdr;
    struct sf_bag *pbag;
    struct sf_mod *pmod;
    struct sf_gen *pgen;

    UINT instrument_count;
    struct sf_instrument *inst;
    struct sf_bag *ibag;
    struct sf_mod *imod;
    struct sf_gen *igen;

    UINT sample_count;
    struct sf_sample *shdr;
    BYTE *sdta;
};
