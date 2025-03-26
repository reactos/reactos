;
; strlen.asm
;
;      Copyright (c) Microsoft Corporation.  All rights reserved.
;
; Optimized strlen and strnlen implementations for ARM64.
;
#include "ksarm64.h"

        ; size_t strlen(const char *str);                                // AV's when str == NULL
        ; size_t strnlen(const char *str, size_t numberOfElements);      // AV's when str == NULL

        ; This file could also define strnlen_s. strnlen_s is currently defined in the header string.h in C
        ; using a check for null and a call to strnlen. This avoids making the call in the case where the string is null,
        ; which should be infrequent. However it makes code larger by inlining that check everywhere strnlen_s is called.
        ; A better alternative would be to modify the standard headers and define strnlen_s here. It would be just one
        ; instruction because the required return value in x0 is already 0 when null is passed for the first parameter.
        ;
        ; EXPORT strnlen_s [FUNC]
        ; LEAF_ENTRY strnlen_s
        ; cbz     x0, AnyRet         ; add the label AnyRet in front of any ret instruction you want
        ;                            ; fallthrough into strnlen code
        ; ALTERNATE_ENTRY strnlen    ; change LEAF_ENTRY for strnlen to ALTERNATE_ENTRY
        ; ...                        ; current body of strnlen
        ; LEAF_END                   ; change strnlen leaf_end to strnlen_s.

#if !defined(_M_ARM64EC)

        EXPORT A64NAME(strlen)    [FUNC]
        EXPORT A64NAME(strnlen)   [FUNC]

#endif

        SET_COMDAT_ALIGNMENT 5

        ; With strlen we will usually read some bytes past the end of the string. To avoid getting an AV
        ; when a byte-by-byte implementation would not, we must ensure that we never cross a page boundary with a
        ; vector load, so we must align the vector loads to 16-byte-aligned boundaries.
        ;
        ; For strnlen we know the buffer length and so we won't read any bytes beyond the end of the buffer. This means
        ; we have a choice whether to arrange our vector loads to be 16-byte aligned. (Note that on arm64 a vector load
        ; only produces an alignment fault when the vector *elements* are misaligned, so a "16B" vector load will never
        ; give an alignment fault for user memory). Aligning the vector loads on 16-byte boundaries saves one cycle
        ; per vector load instruction. The cost of forcing 16-byte aligned loads is the 10 instructions preceding the
        ; 'NoNeedToAlign' label below. On Cortex-A57, the execution latency of those 10 instructions is 27 cycles,
        ; assuming no branch mispredict on the 'beq'. To account for the cost of an occasional mispredict we guess a
        ; mispredict rate of 2% and a mispredict cost of 50 cycles, or 1 cycle per call amortized, 28 total. 28 * 16 = 448.
        ; In this analysis we are ignoring the chance of extra cache misses due to loads crossing cache lines when
        ; they are not 16-byte aligned. When the vector loads span cache line boundaries each cache line is referenced
        ; one more time than it is when the loads are aligned. But we assume that the cache line stays loaded for the
        ; short time we need to do all the references to it, and so one extra reference won't matter.
        ; It is expected that the number of cycles (28) will stay about the same for future processor models. If it
        ; changes radically, it will be worth converting the EQU to a global, using ldr to load it instead of a
        ; mov-immediate, and dynamically setting the global during CRT startup based on processor model.

__strnlen_forceAlignThreshold  EQU 448                     ; code below assumes must be >= 32

        ; If a strlen is performed on an unterminated buffer, strlen may try to access an invalid address
        ; and generate an access violation. The prime imperative is for us not to generate an AV by loading
        ; characters beyond the end of a valid string. But also, in the case of an invalid string that should
        ; generate an AV, we need to have the AV report the proper bad address. If we only perform 16-byte aligned
        ; vector loads then the first time we touch an invalid page we will be loading from offset 0 in that
        ; page, which is the correct address for the AV to report. Even though we will usually load some bytes
        ; from beyond the end of the string, we won't load bytes beyond the end of the page unless the string
        ; extends into the next page. This is the primary purpose for forcing the vector loads to be
        ; 16-byte aligned.

        ; There is a slight performance boost for using aligned vector loads vs. unaligned ones but
        ; it is only worth the cost of aligning for longer strings (at least 512 chars).

        ; For strings less than 16 characters long the byte-by-byte loop will be about as fast as the
        ; compiler could produce, so we're not losing any performance vs. compiled C in any case.

        ARM64EC_ENTRY_THUNK A64NAME(strlen),1,0
        LEAF_ENTRY_COMDAT A64NAME(strlen)

        ; check for empty string to avoid huge perf degradation in this case.
        ldrb    w2, [x0], #0
        cbz     w2, EmptyStr

        mov     x5, x0                                      ; keep original x0 value for the final 'sub'
        ; calculate number of bytes until first 16-byte alignment point

        ands    x1, x5, #15                                 ; x1 = (addr mod 16)
        beq     StrlenMainLoop                              ; no need to force alignment if already aligned

        ; we need to align, check whether we are within 16 bytes of the end of the page

        ands    x2, x5, #4095
        cmp     x2, #4080
        bgt     AlignByteByByte                             ; too close to end of page, must align byte-by-byte

        ; safe to do one unaligned 16-byte vector load to force alignment

        ld1     v0.16b, [x5]                                ; don't post-increment x5
        uminv   b1, v0.16b
        fmov    w2, s1                                      ; fmov is sometimes 1 cycle faster than 'umov w2, v1.b[0]'
        cbz     w2, FindNullInVector                        ; jump when string <= 15 bytes long & not near end of page
        add     x5, x5, #16                                 ; move x5 forward only to aligned address
        and     x5, x5, 0xFFFFFFFFFFFFFFF0                  ; first iter of StrlenMainLoop will retest some bytes we already tested

StrlenMainLoop                                              ; test 16 bytes at a time until we find it
        ld1     v0.16b, [x5], #16
        uminv   b1, v0.16b                                  ; use unsigned min to look for a zero byte; too bad it doesn't set CC
        fmov    w2, s1                                      ; need to move min byte into gpr to test it
        cbnz    w2, StrlenMainLoop                          ; fall through when any one of the bytes in v0 is zero

        sub     x5, x5, #16                                 ; undo the last #16 post-increment of x5

FindNullInVector                                            ; this label is also the target of a jump from strnlen
        ldr     q1, ReverseBytePos                          ; load the position indicator mask

        cmeq    v0.16b, v0.16b, #0                          ; +----
        and     v0.16b, v0.16b, v1.16b                      ; |
        umaxv   b0, v0.16b                                  ; | see big comment below
        fmov    w2, s0                                      ; |
        eor     w2, w2, #15                                 ; +----

        add     x5, x5, x2                                  ; which is the offset we need to add to x5 to point at the null byte
        sub     x0, x5, x0                                  ; subtract ptr to null char from ptr to first char to get the strlen
        ret

ByteByByteFoundIt                                           ; this label is also the target of a jump from strnlen
        sub     x5, x5, #1                                  ; Undo the final post-increment that happened on the load of the null char.
        sub     x0, x5, x0                                  ; With x5 pointing at the null char, x5-x0 is the strlen
        ret

AlignByteByByte
        sub     x1, x1, #16                                 ; x1 = (addr mod 16) - 16
        neg     x1, x1                                      ; x1 = 16 - (addr mod 16) = count for byte-by-byte loop
ByteByByteLoop                                              ; test one byte at a time until we are 16-byte aligned
        ldrb    w2, [x5], #1
        cbz     w2, ByteByByteFoundIt                       ; branch if byte-at-a-time testing finds the null
        subs    x1, x1, #1
        bgt     ByteByByteLoop                              ; fall through when not found and 16-byte aligned
        b       StrlenMainLoop

EmptyStr
        mov     x0, 0
        ret

        ; The challenge is to find a way to efficiently determine which of the 16 bytes we loaded is the end of the string.
        ; The trick is to load a position indicator mask and generate the position of the rightmost null from that.
        ; Little-endian order means when we load the mask below v1.16b[0] has 0x0F, and v0.16b[0] is the byte of the string
        ; that comes first of the 16 we loaded. We do a cmeq, mapping all the characters we loaded to either 0xFF (for nulls)
        ; or 0x00 for non-nulls. Then we and with the mask below. SIMD lanes corresponding to a non-null character will be 0,
        ; and SIMD lanes corresponding to null bytes will have a byte from the mask. We take the max across the bytes of the
        ; vector to find the highest position that corresponds to a null character. The numbering order means we find the
        ; rightmost null in the vector, which is the null that occurred first in memory due to little endian loading.
        ; Exclusive oring the position indicator byte with 15 inverts the order, which gives us the offset of the null
        ; counting from the first character we loaded into the v0 SIMD reg.

ReverseBytePos \
        dcb     15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

        LEAF_END


        ARM64EC_ENTRY_THUNK A64NAME(strnlen),1,0
        LEAF_ENTRY_COMDAT A64NAME(strnlen)                  ; start here for strnlen

        mov     x5, x0
        cmp     x1, #16                                     ; x1 has length. When < 16 we have to go byte-by-byte
        blo     ShortStrNLen                                ; only do byte-by-byte for 0 to 15 bytes.

        ; Do an aligned strnlen

        ands    x3, x5, #15                                 ; x3 = start address mod 16
        beq     NoNeedToAlign                               ; branch on x3 == 0 because its already aligned

        ands    x2, x5, #4095                               ; x2 = start address mod (PAGE_SIZE - 1)
        cmp     x2, #4080
        bgt     AlignByteByByte_Strnlen                     ; too close to end of page, must align byte-by-byte

        sub     x3, x3, #16                                 ; x3 = (start address mod 16) - 16
        neg     x3, x3                                      ; x3 = 16 - (start address mod 16) = number of bytes to advance to get aligned
        ld1     v0.16b, [x5]                                ; don't post-increment
        uminv   b1, v0.16b
        fmov    w2, s1                                      ; fmov is sometimes 1 cycle faster than 'umov w2, v1.b[0]'
        cbz     w2, FindNullInVector_Strnlen                ; jump out when null found in first 16 bytes
        sub     x1, x1, x3                                  ; reduce length remaining by number of bytes needed to get aligned
        add     x5, x5, x3                                  ; move x5 forward only to aligned address
ResumeAfterAlignByteByByte
        cmp     x1, #16                                     ; check for size < 16 after alignment adjustment
        blo     ShortStrNLen
NoNeedToAlign
        asr     x3, x1, #4                                  ; set up number of interations remaining after alignment point reached
                                                            ; no need to check here for x3 == 0 because:
                                                            ; - if we didn't align it, it is at least 16 bytes long
                                                            ; - if we did align it, we checked for <16 before coming here
StrNlenMainLoop                                             ; test 16 bytes at a time until we find it
        ld1     v0.16b, [x5], #16
        uminv   b1, v0.16b                                  ; use unsigned min to look for a zero byte; too bad it doesn't set CC
        fmov    w2, s1                                      ; need to move min byte into gpr to test it
        cbz     w2, UndoPI_FindNullInVector                 ; jump out to when any one of the bytes in v0 is zero
        subs    x3, x3, #1
        bne     StrNlenMainLoop

        ands    x1, x1, #15                                 ; check for remainder
        beq     StrNLenOverrun                              ; orig buffer size was multiple of 16 bytes so no remainder; goto overrun case

        ; We're within 16 bytes of the end of the buffer and haven't found a '\0' yet. We know we were originally longer than
        ; 16 bytes so we can do an unaligned vector compare of the last 16 bytes of the buffer, overlapping with some bytes
        ; we already know are non-zero, without fear of underrunning the original front of the buffer. This avoids a more costly
        ; byte-by-byte comparison for the remainder (which would average 32 instructions executed and two branch mispredicts).
        ; At this point:
        ; x5 points at one of the last 15 chars of the buffer
        ; x1 has the number of chars remaining in the buffer. 1 <= x1 <= 15
        ; 16 - x1 is the number of characters we have to 'back up'
FastRemainderHandling
        sub     x1, x1, #16
        neg     x1, x1
        sub     x5, x5, x1
        ld1     v0.16b, [x5], #16
        uminv   b1, v0.16b
        fmov    w2, s1                                      ; fmov is sometimes 1 cycle faster than 'umov w2, v1.b[0]'
        cbz     w2, UndoPI_FindNullInVector                 ; found a '\0'
        b       StrNLenOverrun                              ; x5 points one past end of buffer, we're all set for the overrun exit.

ShortStrNLen
        cbz     x1, StrNLenOverrun                          ; if original length was zero, we must return 0 without touching the buffer

ShortStrNLenLoop
        ldrb    w2, [x5], #1
        cbz     w2, ByteByByteFoundIt_Strnlen               ; jump into other function to avoid code duplication
        subs    x1, x1, #1
        bhi     ShortStrNLenLoop

StrNLenOverrun
        sub     x0, x5, x0                                  ; x5 points one past the end of the buffer, x5-x0 is original numberOfElements
        ret

AlignByteByByte_Strnlen
        sub     x3, x3, #16                                 ; x3 = (addr mod 16) - 16
        neg     x3, x3                                      ; x3 = 16 - (addr mod 16) = count for byte-by-byte loop
ByteByByteLoop_Strnlen                                      ; test one byte at a time until we are 16-byte aligned
        ldrb    w2, [x5], #1
        cbz     w2, ByteByByteFoundIt_Strnlen               ; branch if byte-at-a-time testing finds the null
        subs    x1, x1, #1                                  ; check remaining length = 0
        beq     ByteByByteReachedMax_Strnlen                ; branch if byte-at-a-time testing reached end of buffer count
        subs    x3, x3, #1
        bgt     ByteByByteLoop_Strnlen                      ; fall through when not found and 16-byte aligned
        b       ResumeAfterAlignByteByByte

UndoPI_FindNullInVector                                     ; this label is the target of a jump from strnlen
        sub     x5, x5, #16                                 ; undo the last #16 post-increment of x5

FindNullInVector_Strnlen                                    ; this label is also the target of a jump from strnlen
        ldr     q1, ReverseBytePos_Strnlen                  ; load the position indicator mask

        cmeq    v0.16b, v0.16b, #0                          ; +----
        and     v0.16b, v0.16b, v1.16b                      ; |
        umaxv   b0, v0.16b                                  ; | see big comment below
        fmov    w2, s0                                      ; |
        eor     w2, w2, #15                                 ; +----

        add     x5, x5, x2                                  ; which is the offset we need to add to x5 to point at the null byte
        sub     x0, x5, x0                                  ; subtract ptr to null char from ptr to first char to get the strlen
        ret

ByteByByteFoundIt_Strnlen                                   ; this label is also the target of a jump from strnlen
        sub     x5, x5, #1                                  ; Undo the final post-increment that happened on the load of the null char.
ByteByByteReachedMax_Strnlen
        sub     x0, x5, x0                                  ; With x5 pointing at the null char, x5-x0 is the strlen
        ret

ReverseBytePos_Strnlen \
        dcb     15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

        LEAF_END

        END
