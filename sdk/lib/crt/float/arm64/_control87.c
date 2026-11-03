/*
 * PROJECT:     ReactOS CRT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     ARM64 implementation of _control87
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "fpcr.h"

/* Control word bits for the exceptions, in the order of the FPCR enable bits */
static const unsigned int Arm64ExceptionMasks[] =
{
    _EM_INVALID, _EM_ZERODIVIDE, _EM_OVERFLOW, _EM_UNDERFLOW, _EM_INEXACT, 0, 0, _EM_DENORMAL
};

unsigned int __cdecl _control87(unsigned int newval, unsigned int mask)
{
    unsigned int Fpcr, Old, Word, i;
    int Changed;

    Fpcr = Arm64ReadFpcr();

    /* A clear trap enable means the exception is masked */
    Old = 0;
    for (i = 0; i < sizeof(Arm64ExceptionMasks) / sizeof(Arm64ExceptionMasks[0]); i++)
    {
        if (!(Fpcr & (1 << (ARM64_FPCR_ENABLE_SHIFT + i))))
            Old |= Arm64ExceptionMasks[i];
    }

    switch (Fpcr & ARM64_FPCR_RMODE_MASK)
    {
        case ARM64_FPCR_RMODE_UP: Old |= _RC_UP; break;
        case ARM64_FPCR_RMODE_DOWN: Old |= _RC_DOWN; break;
        case ARM64_FPCR_RMODE_CHOP: Old |= _RC_CHOP; break;
    }

    if (Fpcr & ARM64_FPCR_FZ)
        Old |= _DN_FLUSH;

    mask &= ARM64_CW_MASK;
    Word = (Old & ~mask) | (newval & mask);

    /* FPCR is only rebuilt from the control word when the request changes it */
    Changed = (Word != Old);

    /* Every flush mode maps onto FZ, it reads back as _DN_FLUSH */
    if (Word & _MCW_DN)
        Word = (Word & ~_MCW_DN) | _DN_FLUSH;

    if (!Changed)
        return Word;

    Fpcr = 0;
    for (i = 0; i < sizeof(Arm64ExceptionMasks) / sizeof(Arm64ExceptionMasks[0]); i++)
    {
        if (Arm64ExceptionMasks[i] && !(Word & Arm64ExceptionMasks[i]))
            Fpcr |= 1 << (ARM64_FPCR_ENABLE_SHIFT + i);
    }

    switch (Word & _MCW_RC)
    {
        case _RC_UP: Fpcr |= ARM64_FPCR_RMODE_UP; break;
        case _RC_DOWN: Fpcr |= ARM64_FPCR_RMODE_DOWN; break;
        case _RC_CHOP: Fpcr |= ARM64_FPCR_RMODE_CHOP; break;
    }

    if (Word & _DN_FLUSH)
        Fpcr |= ARM64_FPCR_FZ;

    Arm64WriteFpcr(Fpcr);
    return Word;
}
