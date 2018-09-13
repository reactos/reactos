/***
*dbgstk.c - debug check stack routine
*
*       Copyright (c) 1986-1995, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This module contains a debug impelmentation of the standard _chkstk
*   for i386.  It will do the standard stack probe (code copied from
*   VC5 CRT) and then call a debug routine which will have the oportunity
*   top spew the stack before it gets initialized (or not).
*
*******************************************************************************/
#include "headers.hxx"

#if defined(USE_STACK_SPEW) && defined(_X86_)

#pragma check_stack(off)

static BOOL    g_fStackSpewEnabled = FALSE;
static DWORD   g_dwSpew = 0x0;

extern "C" void __declspec(naked) __cdecl _chkstk()
{
    _asm
    {
        ;  First probe the stack.  We do this because
        ;  we don't want to write past the stack guard page
        ;  Note that this code came from the original
        ;  c run time source.

        push    ecx                     ; save ecx
        push    eax                     ; save eax (size of stack needed)
        cmp     eax,1000h               ; more than one page requested?
        lea     ecx,[esp] + 12          ;   compute new stack pointer in ecx
                                        ;   correct for return address and
                                        ;   saved ecx, eax
        jb      short lastpage          ; no

probepages:
        sub     ecx,1000h               ; yes, move down a page
        sub     eax,1000h               ; adjust request and...

        test    dword ptr [ecx],eax     ; ...probe it

        cmp     eax,1000h               ; more than one page requested?
        jae     short probepages        ; no

lastpage:
        sub     ecx,eax                 ; move stack down by eax
        mov     eax,esp                 ; save current tos and do a...

        test    dword ptr [ecx],eax     ; ...probe in case a page was crossed

        ;  Now set up and write our data into the area of the stack
        ;  that was opened up

        lea     esp,[ecx] - 12          ; set the stack pointer to the bottom
                                        ; leave room 12 in padding so we don't 
                                        ; clobber ourselves

        mov     ecx,dword ptr [eax+8]   ; recover return address
        push    ecx                        

        cmp     g_fStackSpewEnabled,0   ; see if we are enabled
        
        mov     ecx,dword ptr [eax+4]   ; recover original ecx

        je      done                    ; not enabled

        push    ecx                     ; save original ecx

        pushfd                          ; save flags
        std                             ; set DI: decr edi after stosd

        mov     ecx,dword ptr [eax]     ; recover original eax (stack size)

        push    edi                     ; save edi on stack also
        lea     edi,[eax]+8             ; load up iterator start address

        shr     ecx,2                   ; get count of dwords

        mov     eax,g_dwSpew            ; load up value

        rep stosd                       ; let 'er rip

        pop     edi                     ; pop saved edi
        popfd                           ; pop flags
        pop     ecx                     ; pop saved ecx

done:        
        ret     12                      ; return, popping off 12 padding
    }
}

// NOTE: _alloca_probe is impelemented exactly the same as _chkstk
// I'd like to find some way to merge these two pieces of code but I
// don't know how with inline assembly...
extern "C" void __declspec(naked) __cdecl _alloca_probe()
{
    _asm
    {
        ;  First probe the stack.  We do this because
        ;  we don't want to write past the stack guard page
        ;  Note that this code came from the original
        ;  c run time source.

        push    ecx                     ; save ecx
        push    eax                     ; save eax (size of stack needed)
        cmp     eax,1000h               ; more than one page requested?
        lea     ecx,[esp] + 12          ;   compute new stack pointer in ecx
                                        ;   correct for return address and
                                        ;   saved ecx, eax
        jb      short lastpage          ; no

probepages:
        sub     ecx,1000h               ; yes, move down a page
        sub     eax,1000h               ; adjust request and...

        test    dword ptr [ecx],eax     ; ...probe it

        cmp     eax,1000h               ; more than one page requested?
        jae     short probepages        ; no

lastpage:
        sub     ecx,eax                 ; move stack down by eax
        mov     eax,esp                 ; save current tos and do a...

        test    dword ptr [ecx],eax     ; ...probe in case a page was crossed

        ;  Now set up and write our data into the area of the stack
        ;  that was opened up

        lea     esp,[ecx] - 12          ; set the stack pointer to the bottom
                                        ; leave room 12 in padding so we don't 
                                        ; clobber ourselves

        mov     ecx,dword ptr [eax+8]   ; recover return address
        push    ecx                        

        cmp     g_fStackSpewEnabled,0   ; see if we are enabled
        
        mov     ecx,dword ptr [eax+4]   ; recover original ecx

        je      done                    ; not enabled

        push    ecx                     ; save original ecx

        pushfd                          ; save flags
        std                             ; set DI: decr edi after stosd

        mov     ecx,dword ptr [eax]     ; recover original eax (stack size)

        push    edi                     ; save edi on stack also
        lea     edi,[eax]+8             ; load up iterator start address

        shr     ecx,2                   ; get count of dwords

        mov     eax,g_dwSpew            ; load up value

        rep stosd                       ; let 'er rip

        pop     edi                     ; pop saved edi
        popfd                           ; pop flags
        pop     ecx                     ; pop saved ecx

done:        
        ret     12                      ; return, popping off 12 padding
    }
}

//
// Initialize the debug stack system
//

extern "C" void
InitChkStk(BOOL dwFill)
{
    g_dwSpew = dwFill;
    g_fStackSpewEnabled = TRUE;
}

#endif

