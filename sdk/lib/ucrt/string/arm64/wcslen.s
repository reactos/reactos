;
; wcslen.asm
;
;      Copyright (c) Microsoft Corporation.  All rights reserved.
;
; Optimized wcslen and wcsnlen implementations for ARM64.
;
#include "ksarm64.h"

        ; size_t wcslen(const wchar_t *str);
        ; size_t wcsnlen(const wchar_t *str, size_t numberOfElements);

        ; This file could also define wcsnlen_s. wcsnlen_s is currently defined in the headers (string.h & wchar.h) in C
        ; using a check for null and a call to wcsnlen. This avoids making the call in the case where the string is null,
        ; which should be infrequent. However it makes code larger by inlining that check everywhere wcsnlen_s is called.
        ; An alternative would be to modify the standard headers and define wcsnlen_s here. It would be just one instruction:
        ;
        ; LEAF_ENTRY wcsnlen_s
        ; cbz     x0, AnyRet         ; AnyRet would be a label in front of any ret instruction. Return value in x0 is already 0.
        ;                            ; fallthrough into wcsnlen code
        ; ALTERNATE_ENTRY wcsnlen    ; change LEAF_ENTRY for wcsnlen to ALTERNATE_ENTRY
        ; ...

        ; Note: this code assumes that the input parameter is always aligned to an even byte boundary.

#if !defined(_M_ARM64EC)

        EXPORT A64NAME(wcslen)    [FUNC]
        EXPORT A64NAME(wcsnlen)   [FUNC]

#endif

        SET_COMDAT_ALIGNMENT 6

        ; With wcslen we will usually read some chars past the end of the string. To avoid getting an AV
        ; when a char-by-char implementation would not, we have to ensure that we never cross a page boundary with a
        ; vector load, so we must align the vector loads to 16-byte-aligned boundaries.
        ;
        ; For wcsnlen we know the buffer length and so we won't read any chars beyond the end of the buffer. This means
        ; we have a choice whether to arrange our vector loads to be 16-byte aligned. (Note that on arm64 a vector load
        ; only produces an alignment fault when the vector *elements* are misaligned, so an "8H" vector load will never
        ; give an alignment fault on any even address). Aligning the vector loads on 16-byte boundaries saves one cycle
        ; per vector load instruction. The cost of forcing 16-byte aligned loads is the 10 instructions preceding the
        ; 'NoNeedToAlign' label below. On Cortex-A57, the execution latency of those 10 instructions is 26 cycles,
        ; (one less than the strnlen case because uminv is 1 cycle faster for halfwords than it is for bytes),
        ; assuming no branch mispredict on the 'beq'. To account for the cost of an occasional mispredict we guess a
        ; mispredict rate of 2% and a mispredict cost of 50 cycles, or 1 cycle per call amortized, 27 total. 27 * 8 = 216.
        ; In this analysis we are ignoring the chance of extra cache misses due to loads crossing cache lines when
        ; they are not 16-byte aligned. When the vector loads span cache line boundaries each cache line is referenced
        ; one more time than it is when the loads are aligned. But we assume that the cache line stays loaded for the
        ; short time we need to do all the references to it, and so one extra reference won't matter.
        ; It is expected that the number of cycles (27) will stay about the same for future processor models. If it
        ; changes radically, it will be worth converting the EQU to a global, using ldr to load it instead of a
        ; mov-immediate, and dynamically setting the global during CRT startup based on processor model.

__wcsnlen_forceAlignThreshold  EQU 216                      ; code logic below assumes >= 16

        ARM64EC_ENTRY_THUNK A64NAME(wcslen),1,0
        LEAF_ENTRY_COMDAT A64NAME(wcslen)

        ; check for empty string to avoid huge perf degradation in this case.
        ldrh    w2, [x0], #0
        cbz     w2, EmptyStr

        mov     x5, x0                                      ; keep original x0 value for the final 'sub'
        tbnz    x0, #0, WCharAtATime                        ; check for misaligned characters. Must go char-by-char when
                                                            ; misaligned so that if there's an access violation it gets
                                                            ; generated on the correct address (one byte into a new page
                                                            ; instead of up to 15 bytes in if we had loaded a vector
                                                            ; from the last byte of the previous page)

        ; calculate number of bytes until first 16-byte alignment point

        ands    x1, x5, #15                                 ; x1 = (addr mod 16)
        beq     WcslenMainLoop                              ; no need to force alignment if already aligned

        ; we need to align, check whether we are within 16 bytes of the end of the page.
        ; branch if ((address mod PAGESIZE - 1) > (PAGESIZE - 16))

        and     x2, x5, #4095                               ; x2 = address mod (PAGESIZE - 1)
        cmp     x2, #4080                                   ; compare x2 to (PAGESIZE - 16)
        bgt     AlignSlowly                                 ; too close to end of page, must align one wchar at a time

        ; AlignFast: safe to do one 2-byte aligned vector load to force alignment to a 16-byte boundary

        ld1     v0.8h, [x5]                                 ; don't post-increment x5
        uminv   h1, v0.8h
        fmov    w2, s1                                      ; fmov is sometimes 1 cycle faster than "umov w2, v1.h[0]"
        cbz     w2, FindWideNullInVector                    ; jump when string < 15 bytes (<= 7 wchar_t's) long & not near end of page
        add     x5, x5, #16                                 ; move x5 forward only to aligned address.  (Assumes even address)
        and     x5, x5, 0xFFFFFFFFFFFFFFF0                  ; first iter of StrlenMainLoop will retest some bytes we already tested

        ; The code at WcslenMainLoop should be 64-byte aligned for best performance.
        ; Due to VSO#1106651, automatic padding with NOPs when in code areas is
        ; broken. The workaround is to use -721215457.
        ; MSFT:21876224 tracks removal of this workaround.
        ALIGN 64,0,-721215457,4

WcslenMainLoop                                              ; test 8 wchar_t's at a time until we find it
        ld1     v0.8h, [x5], #16
        uminv   h1, v0.8h                                   ; use unsigned min to look for a zero wchar_t; too bad it doesn't set CC
        fmov    w2, s1                                      ; need to move min wchar_t into gpr to test it
        cbnz    w2, WcslenMainLoop                          ; fall through when any one of the wchar_ts in v0 is zero

        sub     x5, x5, #16                                 ; undo the last #16 post-increment of x5

FindWideNullInVector
        ldr     q1, ReverseBytePos                          ; load the position indicator mask

        cmeq    v0.8h, v0.8h, #0                            ; +----
        and     v0.16b, v0.16b, v1.16b                      ; |
        umaxv   h0, v0.8h                                   ; | see big comment below
        fmov    w2, s0                                      ; |
        eor     w2, w2, #7                                  ; +----

        sub     x0, x5, x0                                  ; subtract ptr to null char from ptr to first char to get the string length in bytes
        add     x0, x2, x0, ASR #1                          ; divide x0 by 2 to get the number of wide chars and then add in the final vector char pos
        ret

AlignSlowly
        sub     x1, x1, #16                                 ; x1 = (addr mod 16) - 16
        sub     x1, xzr, x1, ASR #1                         ; x1 = -(((addr mod 16) - 16) / 2) = (16 - (addr mod 16)) / 2 = num wchar_ts

AlignLoop                                                   ; test one wchar_t at a time until we are 16-byte aligned
        ldrh    w2, [x5], #2
        cbz     w2, OneByOneFoundIt                         ; branch if found the null
        subs    x1, x1, #1
        bgt     AlignLoop                                   ; fall through when not found and reached 16-byte alignment
        b       WcslenMainLoop

WCharAtATime
        ldrh    w2, [x5], #2
        cbnz    w2, WCharAtATime                            ; when found use same exit sequence as when found during slow alignment

OneByOneFoundIt
        sub     x5, x5, #2                                  ; Undo the final post-increment that happened on the load of the null wchar_t.
        sub     x0, x5, x0                                  ; With x5 pointing at the null char, x5-x0 is the length in bytes
        asr     x0, x0, #1                                  ; divide by 2 to get length in wchar_ts
        ret

EmptyStr
        mov     x0, 0
        ret

        ; The challenge is to find a way to efficiently determine which of the 8 wchar_t's we loaded is the end of the string.
        ; The trick is to load a position indicator mask and generate the position of the rightmost null from that.
        ; Little-endian order means when we load the mask below v1.8h[0] has 7, and v0.8h[0] is the wchar_t of the string
        ; that comes first of the 8 we loaded. We do a cmeq, mapping all the wchar_t's we loaded to either 0xFFFF (for nulls)
        ; or 0x0000 for non-nulls. Then we and with the mask below. SIMD lanes corresponding to a non-null wchar_t will be 0x0000,
        ; and SIMD lanes corresponding to a null wchar_t will have a halfword from the mask. We take the max across the halfwords
        ; of the vector to find the highest position that corresponds to a null wchar_t. The numbering order means we find the
        ; rightmost null in the vector, which is the null that occurred first in memory due to little endian loading.
        ; Exclusive oring the position indicator byte with 7 inverts the order, which gives us the character position of the null
        ; counting from the first wchar_t we loaded into the v0 SIMD reg.

ReverseBytePos \
        dcw      7, 6, 5, 4, 3, 2, 1, 0                     ; vector of halfwords

        LEAF_END


        ARM64EC_ENTRY_THUNK A64NAME(wcsnlen),1,0
        LEAF_ENTRY_COMDAT A64NAME(wcsnlen)

        mov     x5, x0                                      ; keep original x0 value for the final 'sub'

        tbnz    x0, #0, ShortWcsnlen                        ; check for misaligned characters; must go char-by-char if misaligned

        cmp     x1, #8                                      ; x1 has length. When x1 < 8 we have to go char-by-char
        blo     ShortWcsnlen                                ; only do char-by-char for 0 to 7 characters

        ands    x3, x5, #15                                 ; x3 = start address mod 16
        beq     NoNeedToAlign                               ; branch on x3 == 0 because it's already aligned

        ; we need to align, check whether we are within 16 bytes of the end of the page.
        ; branch if ((address mod PAGESIZE - 1) > (PAGESIZE - 16))

        and     x2, x5, #4095                               ; x2 = address mod (PAGESIZE - 1)
        cmp     x2, #4080                                   ; compare x2 to (PAGESIZE - 16)
        bgt     AlignSlowly_Wcsnlen                         ; too close to end of page, must align one wchar at a time

        ; force vector loads in the main loop to be 16-byte aligned
        sub     x3, x3, #16                                 ; x3 = (start address mod 16) - 16
        neg     x3, x3                                      ; x3 = 16 - (start address mod 16) = number of *bytes* to advance to get aligned
        ld1     v0.8h, [x5]                                 ; don't post-increment x5
        uminv   h1, v0.8h
        fmov    w2, s1                                      ; fmov is sometimes 1 cycle faster than "umov w2, v1.h[0]"
        cbz     w2, FindWideNullInVector_Wcsnlen            ; jump when found null within first 8 wchar_t's
        sub     x1, x1, x3, ASR #1                          ; reduce elements remaining by number of wchar_t's needed to get aligned (bytes/2)
        add     x5, x5, x3                                  ; move x5 forward by x3 bytes, so x5 is now a 16-byte aligned address
ResumeAfterAlignSlowly
        cmp     x1, #8                                      ; check for size < 8 after alignment adjustment
        blo     ShortWcsnlen
NoNeedToAlign
        asr     x3, x1, #3                                  ; set up interations remaining after alignment point reached (8 wchar_t's per iteration)
                                                            ; no need to check here for x3 == 0 because:
                                                            ; - if we didn't align it, it is at least 16 bytes long
                                                            ; - if we did align it, we checked for <16 before coming here
WcsNlenMainLoop                                             ; test 8 wchar_t's at a time until we find it
        ld1     v0.8h, [x5], #16
        uminv   h1, v0.8h                                   ; use unsigned min to look for a zero wchar_t
        fmov    w2, s1                                      ; need to move min wchar_t into gpr to test it
        cbz     w2, UndoPI_FindNullInVector                 ; jump out and over into wcslen function when any one of the wchar_t's in v0 is zero
        subs    x3, x3, #1
        bne     WcsNlenMainLoop

        ands    x1, x1, #7                                  ; check for remainder
        beq     WcsNLenOverrun                              ; orig buffer size was multiple of 8 wchar_t's so no remainder; goto overrun case

        ; We're less than 8 wchar_t's from the end of the buffer and haven't found a '\0\0' yet. We know we were originally longer than
        ; 16 bytes so we can do a 2-byte aligned vector compare of the last 8 wchar_t's of the buffer, overlapping with some wchar_t's
        ; we already know are non-zero, without fear of underrunning the original front of the buffer. This avoids a more costly
        ; char-by-char comparison for the remainder (which would average 32 instructions executed and two branch mispredicts).
        ; At this point:
        ; x5 points at one of the last 7 wchar_t's of the buffer
        ; x1 has the number of wchar_t's remaining in the buffer. 1 <= x1 <= 7
        ; 8 - x1 is the number of wchar_t's we have to 'back up', LSL that by 1 to get bytes
FastRemainderHandling
        sub     x1, x1, #8
        neg     x1, x1                                      ; x1 = (8 - number of chars remaining); the number of wchar_t's to back up.
        sub     x5, x5, x1, LSL #1                          ; x5 = x5 - (2*x1); back up number of bytes equivalent to x1 wchar_t's
        ld1     v0.8h, [x5], #16                            ; load all of remainder and some already-checked wchar_t's.
        uminv   h1, v0.8h
        fmov    w2, s1                                      ; fmov is sometimes 1 cycle faster than "umov w2, v1.h[0]"
        cbz     w2, UndoPI_FindNullInVector                 ; found a '\0\0' within the last 8 elements of the buffer
        b       WcsNLenOverrun                              ; else x5 points one past end of buffer, and we're all set for the overrun exit.

ShortWcsnlen
        cbz     x1, WcsNLenOverrun                          ; if original number of elements was zero, we must return 0 without touching the buffer

ShortWcsNLenLoop
        ldrh    w2, [x5], #2
        cbz     w2, OneByOneFoundIt_Wcsnlen                 ; jump into other function to avoid code duplication of exit sequence
        subs    x1, x1, #1
        bhi     ShortWcsNLenLoop

WcsNLenOverrun
        sub     x0, x5, x0                                  ; x5 points one past the end of the buffer, x5-x0 is original buffer size in bytes
        asr     x0, x0, #1                                  ; adjust return value from bytes to wchar_t elements
                                                            ; as an alternative to the above two instructions, we could save the original x1 value
                                                            ; and just move that to x0 here, but that would add an instruction to all paths in order
                                                            ; to save one here that's only on the overrun path. So we reconstruct the value instead.
        ret

AlignSlowly_Wcsnlen
        sub     x3, x3, #16                                 ; x3 = (addr mod 16) - 16
        sub     x3, xzr, x3, ASR #1                         ; x3 = -(((addr mod 16) - 16) / 2) = (16 - (addr mod 16)) / 2 = num wchar_ts

AlignLoop_Wcsnlen                                           ; test one wchar_t at a time until we are 16-byte aligned
        ldrh    w2, [x5], #2
        cbz     w2, OneByOneFoundIt_Wcsnlen                 ; branch if found the null
        subs    x1, x1, #1
        beq     OneByOneReachedMax_Wcsnlen                  ; branch if byte-at-a-time testing reached end of buffer count
        subs    x3, x3, #1
        bgt     AlignLoop_Wcsnlen                           ; fall through when not found and reached 16-byte alignment
        b       ResumeAfterAlignSlowly

OneByOneFoundIt_Wcsnlen
        sub     x5, x5, #2                                  ; Undo the final post-increment that happened on the load of the null wchar_t.
OneByOneReachedMax_Wcsnlen
        sub     x0, x5, x0                                  ; With x5 pointing at the null char, x5-x0 is the length in bytes
        asr     x0, x0, #1                                  ; divide by 2 to get length in wchar_ts
        ret

UndoPI_FindNullInVector
        sub     x5, x5, #16                                 ; undo the last #16 post-increment of x5

FindWideNullInVector_Wcsnlen
        ldr     q1, ReverseBytePos_Wcsnlen                  ; load the position indicator mask

        cmeq    v0.8h, v0.8h, #0                            ; +----
        and     v0.16b, v0.16b, v1.16b                      ; |
        umaxv   h0, v0.8h                                   ; | see big comment below
        fmov    w2, s0                                      ; |
        eor     w2, w2, #7                                  ; +----

        sub     x0, x5, x0                                  ; subtract ptr to null char from ptr to first char to get the string length in bytes
        add     x0, x2, x0, ASR #1                          ; divide x0 by 2 to get the number of wide chars and then add in the final vector char pos
        ret

        ; The challenge is to find a way to efficiently determine which of the 8 wchar_t's we loaded is the end of the string.
        ; The trick is to load a position indicator mask and generate the position of the rightmost null from that.
        ; Little-endian order means when we load the mask below v1.8h[0] has 7, and v0.8h[0] is the wchar_t of the string
        ; that comes first of the 8 we loaded. We do a cmeq, mapping all the wchar_t's we loaded to either 0xFFFF (for nulls)
        ; or 0x0000 for non-nulls. Then we and with the mask below. SIMD lanes corresponding to a non-null wchar_t will be 0x0000,
        ; and SIMD lanes corresponding to a null wchar_t will have a halfword from the mask. We take the max across the halfwords
        ; of the vector to find the highest position that corresponds to a null wchar_t. The numbering order means we find the
        ; rightmost null in the vector, which is the null that occurred first in memory due to little endian loading.
        ; Exclusive oring the position indicator byte with 7 inverts the order, which gives us the character position of the null
        ; counting from the first wchar_t we loaded into the v0 SIMD reg.

ReverseBytePos_Wcsnlen \
        dcw      7, 6, 5, 4, 3, 2, 1, 0                     ; vector of halfwords

        LEAF_END 

        END
