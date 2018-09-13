#include "precomp.h"
#pragma hdrstop


BOOL
FIsVCallThunk(
    BYTE *          rgbBuffer,
    DWORD           cbBuff,
    HTHDX           hthd,
    UOFFSET         uoffEIP,
    UOFFSET *       lpuoffThunkDest,
    LPDWORD         lpdwThunkSize
    )
{
    static BYTE rgbVCall[] = {
        0x8B, 0x44, 0x24, 0x04,     // MOV  EAX, [ESP+4]
        0x8B, 0x00,                 // MOV  EAX, [EAX]
    };

    BOOL    fRet = FALSE;
    UOFF32  uoffEAX;
    BYTE *  pbNextInstr = NULL;
    UOFF32  uoffTemp;

    assert( sizeof( rgbVCall ) <= CB_THUNK_MAX );
    assert( sizeof( UOFF32 ) == sizeof( long ) );

    // this_ptr in register
    if ( *(short UNALIGNED *)&rgbBuffer[ 0 ] == 0x018B ) {  // MOV  EAX, [ECX]
        // MOV  EAX, [ECX]
        fRet = DbgReadMemory(
                hthd->hprc,
                hthd->context.Ecx,
                (LPVOID)&uoffEAX,
                sizeof( uoffEAX ),
                NULL
                );

        pbNextInstr = &rgbBuffer[ 2 ];
    }
    else if ( cbBuff >= sizeof( rgbVCall ) &&
        !memcmp( rgbBuffer, rgbVCall, sizeof( rgbVCall ) )
    ) {
            // MOV  EAX, [ESP+4]
        fRet = DbgReadMemory(
                hthd->hprc,
                ( hthd->context.Esp + 4 ),
                (LPVOID)&uoffEAX,
                sizeof( uoffEAX ),
                NULL
            ) &&

            // MOV  EAX, [EAX]
            DbgReadMemory(
                hthd->hprc,
                uoffEAX,
                (LPVOID)&uoffEAX,
                sizeof( uoffEAX ),
                NULL
            );

        pbNextInstr = &rgbBuffer[ sizeof( rgbVCall ) ];
    }

    // Everything is fine so far, now get the last instruction
    // and return address
    if ( fRet ) {
        UOFF32 uoffDisp;

        // JMP [EAX+disp] (get displacement)
        if ( *(short UNALIGNED *)pbNextInstr == 0x60FF) {
            uoffDisp = (UOFF32)*( pbNextInstr + 2 );
            pbNextInstr += 3;
        }
        // JMP[EAX+disp] (disp is 4 btyes)
        else if (*(short UNALIGNED *)pbNextInstr == 0xA0FF) {
            uoffDisp = *(UOFF32 UNALIGNED *)(pbNextInstr + 2);
            pbNextInstr += 6;
        }
        // JMP [EAX]
        else if ( *(short UNALIGNED *)pbNextInstr == 0x20FF ) {
            uoffDisp = 0;
            pbNextInstr += 2;
        }

        // Don't care, but this will force the compare
        // below to fail
        else {
            cbBuff = (DWORD)0;
        }

        // If we have made it here, then pbNextInstr must
        // have been reset to non-null
        assert( pbNextInstr );
        assert( pbNextInstr != rgbBuffer );

        // If the buffer is smaller than the number of bytes
        // consumed in this thunk or we can read the address
        // fail ([eap+disp])
        if ( cbBuff >= (DWORD)( pbNextInstr - rgbBuffer - 1 ) &&
            DbgReadMemory(
                hthd->hprc,
                ( uoffEAX + uoffDisp ),
                &uoffTemp,
                sizeof( UOFF32 ),
                NULL
            )
        ) {
            *lpuoffThunkDest = uoffTemp;
        } else {
            fRet = FALSE;
        }
    }
    if (fRet) {
        *lpdwThunkSize = (DWORD)(pbNextInstr - rgbBuffer);
    }
    return fRet;
}

BOOL
FIsVTDispAdjustorThunk(
    BYTE *          rgbBuffer,
    DWORD           cbBuff,
    HTHDX           hthd,
    UOFFSET         uoffEIP,
    UOFFSET *       lpuoffThunkDest,
    LPDWORD         lpdwThunkSize
    )
{
    BOOL    fThisOnStack = (BOOL)( *(long UNALIGNED *)&rgbBuffer[ 0 ] == 0x04244C8BL );
    BYTE *  pbNextInstr = &rgbBuffer[ 0 ];

    // If on the stack skip over this instruction
    if ( fThisOnStack ) {
        pbNextInstr += 4;
    }

    // Required SUB ECX, [ECX + dvtordisp]
    if ( *pbNextInstr == 0x2B ) {
        ++pbNextInstr;

        // SUB ECX, [ECX + byte/sign-extend]
        if ( *pbNextInstr == 0x49 ) {
            pbNextInstr += 2;
        }
        // SUB ECX, [ECX + dword]
        else if ( *pbNextInstr == 0x89 ) {
            pbNextInstr += 5;
        }
        else {
            return FALSE;
        }

        // Maybe an adjust (byte/sign extended)
        if ( *(short UNALIGNED *)pbNextInstr == 0xE983 ) {
            pbNextInstr += 3;
        }

        // Maybe an adjust (dword)
        else if ( *(short UNALIGNED *)pbNextInstr == 0xE981 ) {
            pbNextInstr += 6;
        }

        // If this is on the stack there must be a MOV [ESP + 4 ],ECX
        if ( fThisOnStack ) {
            if ( *(long UNALIGNED *)pbNextInstr != 0x04244C89L ) {
                return FALSE;
            }
            pbNextInstr += 4;
        }

        // Now make sure that the number of bytes consumed in this
        // thunk is valid.  By now, we know that we needed everything
        // up to this instruction and including the (hopefully) jmp
        // instruction number of bytes.
        //
        // Now we should have a direct relative jump (JMP  xxxxxxxx )
        if ( cbBuff >= (DWORD)( pbNextInstr - rgbBuffer + 5 ) &&
            *pbNextInstr == 0xE9
        ) {
            *lpuoffThunkDest = uoffEIP + pbNextInstr - rgbBuffer +
                *(DWORD UNALIGNED *)( pbNextInstr + 1 ) + 5;
            *lpdwThunkSize = (DWORD)(pbNextInstr - rgbBuffer + 5);
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
FIsAdjustorThunk(
    BYTE *          rgbBuffer,
    DWORD           cbBuff,
    HTHDX           hthd,
    UOFFSET         uoffEIP,
    UOFFSET *       lpuoffThunkDest,
    LPDWORD         lpdwThunkSize
    )
{
    BYTE *  pbNextInstr = &rgbBuffer[ 0 ];
    BOOL    fRet = FALSE;

    // this adjustor ADD (byte/sign extended)
    if ( *pbNextInstr == 0x83 ) {
        ++pbNextInstr;

        // this in ECX: ECX, byte const
        if ( *pbNextInstr == 0xC1 || *pbNextInstr == 0xE9 ) {
            pbNextInstr += 2;
        }
        // this on stack:  ADD DWORD PTR [ESP+4],byte const
        else {
            DWORD dwInstrCode = (*(DWORD UNALIGNED *)pbNextInstr & 0x00FFFFFF );
            if ( dwInstrCode == 0x0004246CL || dwInstrCode == 0x00042444L ) {
                pbNextInstr += 4;
            }
            else {
                // not an adjustor thunk
                return FALSE;
            }
        }
        fRet = TRUE;
    }

    // this adjustor ADD (dword)
    else if ( *pbNextInstr == 0x81 ) {
        ++pbNextInstr;

        // this in ECX: ECX, dword const
        if ( *pbNextInstr == 0xC1 || *pbNextInstr == 0xE9 ) {
            pbNextInstr += 5;
        }
        // this on stack:  ADD DWORD PTR [ESP+4],byte const
        else {
            DWORD dwInstrCode = (*(DWORD UNALIGNED *)pbNextInstr & 0x00FFFFFF );
            if ( dwInstrCode == 0x0004246CL || dwInstrCode == 0x00042444L ) {
                pbNextInstr += 7;
            }
            else {
                // not an adjustor thunk
                return FALSE;
            }
        }
        fRet = TRUE;
    }

    if ( fRet ) {
        // Now make sure that the number of bytes consumed in this
        // thunk is valid.  By now, we know that we needed everything
        // up to this instruction and including the (hopefully) jmp
        // instruction number of bytes.
        //
        // Now we should have a direct relative jump (JMP  xxxxxxxx )
        if ( cbBuff >= (DWORD)( pbNextInstr - rgbBuffer + 5 ) &&
            *pbNextInstr == 0xE9
        ) {
            *lpuoffThunkDest = uoffEIP + pbNextInstr - rgbBuffer +
                *( DWORD UNALIGNED * )( pbNextInstr + 1 ) + 5;
            *lpdwThunkSize = (DWORD)(pbNextInstr - rgbBuffer + 5);
        }
        else {
            fRet = FALSE;
        }
    }
    return fRet;
}

BOOL
FIsIndirectJump(
    BYTE *          rgbBuffer,
    DWORD           cbBuff,
    HTHDX           hthd,
    UOFFSET         uoffEIP,
    UOFFSET *       lpuoffThunkDest,
    LPDWORD         lpdwThunkSize
    )
{
    UOFF32 uoffTemp;
    // FF 25 xxxxxxxx is indirect jump (jmp [xxxxxxxx])
    // Buffer needs to be opcode + 32-bit offset
    if ( cbBuff >= 6 && *(short *)rgbBuffer == 0x25FF ) {
        if ( DbgReadMemory (
                hthd->hprc,
                *(DWORD UNALIGNED *)&rgbBuffer[ 2 ],
                &uoffTemp,
                sizeof(UOFF32),
                NULL)
        ) {
            *lpuoffThunkDest = uoffTemp;
            *lpdwThunkSize = 6;
            return TRUE;
        }
    }
    return FALSE;
}

BOOL
FIsDirectJump(
    BYTE *          rgbBuffer,
    DWORD           cbBuff,
    HTHDX           hthd,
    UOFFSET         uoffEIP,
    UOFFSET *       lpuoffThunkDest,
    LPDWORD         lpdwThunkSize
    )
{
    // E9 = direct jump.  (jmp xxxxxxxx).
    // Buffer must be large enough for opcode plus 32-bit offset
    if ( cbBuff >= 5 && rgbBuffer[ 0 ] == 0xE9 ) {
        *lpuoffThunkDest = uoffEIP +
            *( DWORD UNALIGNED * )( rgbBuffer + sizeof ( BYTE ) ) + 5;
        *lpdwThunkSize = 5;
        return TRUE;
    }
    return FALSE;
}


/*** GETPMFDEST
 *
 * PURPOSE:
 *      Given the this pointer and a SI PMF figure out the address of the member
 *      function being called.
 *
 * INPUT:
 *      uThis           - value of the this pointer.
 *      uPMF            - the ptr to mbr function.
 *      lpuoffDest      - Where the PMF is going to
 *
 * OUTPUT:
 *
 * EXCEPTIONS:
 *
 * IMPLEMENTATION:
 *
 ****************************************************************************/

 BOOL
 GetPMFDest(
    HTHDX hthd,
    UOFFSET uThis,
    UOFFSET uPMF,
    UOFFSET *lpuOffDest
    )
 {
    static BYTE rgbVCall[] = {
        0x8B, 0x44, 0x24, 0x04,     // MOV  EAX, [ESP+4]
        0x8B, 0x00,                 // MOV  EAX, [EAX]
    };

    UOFFSET uOffCurr;
    UOFFSET uOffNext = uPMF;
    BYTE rgbBuffer[CB_THUNK_MAX];
    DWORD dwLength;
    int cThunks = 8; // Look for a max of 8 to avoid any infinite loop.
    UOFFSET uVtable; // The v-table ptr.
    UOFFSET uOffDisp; // The disp in the vtable where the func ptr is.
    BYTE * pbNextInstr = NULL;
    DWORD dwSize;

    UOFF32 uoffTemp;

    *lpuOffDest = (UOFFSET)0;

    // First skip past any ilink thunks so we get to the
    // vcall thunk.
    do
    {
        uOffCurr = uOffNext;
        dwLength = CB_THUNK_MAX;

        // Read until a read succeeds or there's no room left to read
        if (--cThunks == 0 ||
            !DbgReadMemory (
                 hthd->hprc,
                 uOffCurr,
                 rgbBuffer,
                 dwLength,
                 &dwLength
                 )) {

            // If we couldn't read anything at the location OR
            // we have already looked at 8 thunks, just return
            // indicating  we couldn't find it.
            return FALSE;
        }

    } while ( FIsDirectJump( rgbBuffer, dwLength, hthd, uOffCurr,
                    &uOffNext, &dwSize)
              || FIsIndirectJump( rgbBuffer, dwLength, hthd, uOffCurr,
                    &uOffNext, &dwSize ) );

    // At this point we should still have a valid set of code bytes
    // in rgbBuffer, and uOffCurr will have the address to it.
    assert(dwLength);

    // There is code here which is essentially duplication of the
    // logic in FIsVCallThunk.

    if ((dwLength >= 2) &&
        *(short UNALIGNED *)rgbBuffer == 0x018B ) {       //  MOV EAX, [ECX]
        pbNextInstr = &rgbBuffer[ 2 ];
        dwLength -= 2;
    }
    else if ( dwLength >= sizeof( rgbVCall) &&
        !memcmp(rgbBuffer, rgbVCall, sizeof(rgbVCall) )
    ) {
        pbNextInstr = &rgbBuffer[ sizeof( rgbVCall ) ];
        dwLength -= sizeof( rgbVCall );
    }
    else {
        // This must be a pointer to a non-virtual member func.
        *lpuOffDest = uOffCurr;
        return TRUE;
    }

    assert( pbNextInstr );

    // JMP [EAX+disp] (get displacement)
    if ( dwLength >= 3 && *(short UNALIGNED *)pbNextInstr == 0x60FF ) {
        uOffDisp = *(pbNextInstr + 2);
    }
    // JMP [EAX+disp] (4 byte displacement)
    else if ( dwLength >= 6 && *(short UNALIGNED *)pbNextInstr == 0xA0FF ) {
        uOffDisp = *(UOFF32 UNALIGNED *)(pbNextInstr + 2);
    }
    else if ( dwLength >=2 &&  *(short UNALIGNED *)pbNextInstr == 0x20FF ) {
        uOffDisp = (UOFF32) 0;
    }
    else { // Not one of the jump variants, must not be a thunk.
        assert(FALSE);
        return FALSE;
    }

    // Get the vtable ptr
    if ( !DbgReadMemory(
            hthd->hprc,
            uThis,
            &uoffTemp,
            sizeof(uoffTemp),
            NULL
            )
    ) {
        return FALSE;
    }

    uVtable = uoffTemp;

    // Use the vtable pointer and uOffDisp to get the
    // vtable entry corresponding to this call.

    if ( DbgReadMemory(
            hthd->hprc,
            ( uVtable + uOffDisp ),
            &uoffTemp,
            sizeof ( uoffTemp ),
            NULL
            )
    ) {
        *lpuOffDest = uoffTemp;
        return TRUE;
    }

    return FALSE;
}
