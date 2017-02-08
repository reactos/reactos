/*
 * COPYRIGHT:   See COPYING.LIB in the top level directory
 * PROJECT:     ReactOS system libraries
 * PURPOSE:     Resets FPU state to the default
 * PROGRAMER:   Thomas Faber <thomas.faber@reactos.org>
 */

#include <precomp.h>

/*********************************************************************
 *		_fpreset (MSVCRT.@)
 */
void CDECL _fpreset(void)
{
    const unsigned int x86_cw = 0x27f;
#ifdef _MSC_VER
    __asm { fninit }
    __asm { fldcw [x86_cw] }
#else
    __asm__ __volatile__( "fninit; fldcw %0" : : "m" (x86_cw) );
#endif
    if (IsProcessorFeaturePresent(PF_XMMI64_INSTRUCTIONS_AVAILABLE))
    {
          const unsigned long sse2_cw = 0x1f80;
#ifdef _MSC_VER
        __asm { ldmxcsr [sse2_cw] }
#else
        __asm__ __volatile__( "ldmxcsr %0" : : "m" (sse2_cw) );
#endif
    }
}
