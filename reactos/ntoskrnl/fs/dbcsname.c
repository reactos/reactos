/* $Id$
 *
 * reactos/ntoskrnl/fs/dbcsname.c
 *
 */

#include <ntoskrnl.h>

/* DATA ********************************************************************/

static UCHAR LegalAnsiCharacterArray[] =
{
  0,				/* CTRL+@, 0x00 */
  0,				/* CTRL+A, 0x01 */
  0,				/* CTRL+B, 0x02 */
  0,				/* CTRL+C, 0x03 */
  0,				/* CTRL+D, 0x04 */
  0,				/* CTRL+E, 0x05 */
  0,				/* CTRL+F, 0x06 */
  0,				/* CTRL+G, 0x07 */
  0,				/* CTRL+H, 0x08 */
  0,				/* CTRL+I, 0x09 */
  0,				/* CTRL+J, 0x0a */
  0,				/* CTRL+K, 0x0b */
  0,				/* CTRL+L, 0x0c */
  0,				/* CTRL+M, 0x0d */
  0,				/* CTRL+N, 0x0e */
  0,				/* CTRL+O, 0x0f */
  0,				/* CTRL+P, 0x10 */
  0,				/* CTRL+Q, 0x11 */
  0,				/* CTRL+R, 0x12 */
  0,				/* CTRL+S, 0x13 */
  0,				/* CTRL+T, 0x14 */
  0,				/* CTRL+U, 0x15 */
  0,				/* CTRL+V, 0x16 */
  0,				/* CTRL+W, 0x17 */
  0,				/* CTRL+X, 0x18 */
  0,				/* CTRL+Y, 0x19 */
  0,				/* CTRL+Z, 0x1a */
  0,				/* CTRL+[, 0x1b */
  0,				/* CTRL+\, 0x1c */
  0,				/* CTRL+], 0x1d */
  0,				/* CTRL+^, 0x1e */
  0,				/* CTRL+_, 0x1f */
  0,				/* ` ', 0x20 */
  0,				/* `!', 0x21 */
  FSRTL_WILD_CHARACTER,		/* `"', 0x22 */
  0,				/* `#', 0x23 */
  0,				/* `$', 0x24 */
  0,				/* `%', 0x25 */
  0,				/* `&', 0x26 */
  0,				/* `'', 0x27 */
  0,				/* `(', 0x28 */
  0,				/* `)', 0x29 */
  FSRTL_WILD_CHARACTER,		/* `*', 0x2a */
  0,				/* `+', 0x2b */
  0,				/* `,', 0x2c */
  0,				/* `-', 0x2d */
  0,				/* `.', 0x2e */
  0,				/* `/', 0x2f */
  0,				/* `0', 0x30 */
  0,				/* `1', 0x31 */
  0,				/* `2', 0x32 */
  0,				/* `3', 0x33 */
  0,				/* `4', 0x34 */
  0,				/* `5', 0x35 */
  0,				/* `6', 0x36 */
  0,				/* `7', 0x37 */
  0,				/* `8', 0x38 */
  0,				/* `9', 0x39 */
  0,				/* `:', 0x3a */
  0,				/* `;', 0x3b */
  FSRTL_WILD_CHARACTER,		/* `<', 0x3c */
  0,				/* `=', 0x3d */
  FSRTL_WILD_CHARACTER,		/* `>', 0x3e */
  FSRTL_WILD_CHARACTER,		/* `?', 0x3f */
  FSRTL_WILD_CHARACTER,		/* `@', 0x40 */
  0,				/* `A', 0x41 */
  0,				/* `B', 0x42 */
  0,				/* `C', 0x43 */
  0,				/* `D', 0x44 */
  0,				/* `E', 0x45 */
  0,				/* `F', 0x46 */
  0,				/* `G', 0x47 */
  0,				/* `H', 0x48 */
  0,				/* `I', 0x49 */
  0,				/* `J', 0x4a */
  0,				/* `K', 0x4b */
  0,				/* `L', 0x4c */
  0,				/* `M', 0x4d */
  0,				/* `N', 0x4e */
  0,				/* `O', 0x4f */
  0,				/* `P', 0x50 */
  0,				/* `Q', 0x51 */
  0,				/* `R', 0x52 */
  0,				/* `S', 0x53 */
  0,				/* `T', 0x54 */
  0,				/* `U', 0x55 */
  0,				/* `V', 0x56 */
  0,				/* `W', 0x57 */
  0,				/* `X', 0x58 */
  0,				/* `Y', 0x59 */
  0,				/* `Z', 0x5a */
  0,				/* `[', 0x5b */
  0,				/* `\', 0x5c */
  0,				/* `]', 0x5d */
  0,				/* `^', 0x5e */
  0,				/* `_', 0x5f */
  0,				/* ``', 0x60 */
  0,				/* `a', 0x61 */
  0,				/* `b', 0x62 */
  0,				/* `c', 0x63 */
  0,				/* `d', 0x64 */
  0,				/* `e', 0x65 */
  0,				/* `f', 0x66 */
  0,				/* `g', 0x67 */
  0,				/* `h', 0x68 */
  0,				/* `i', 0x69 */
  0,				/* `j', 0x6a */
  0,				/* `k', 0x6b */
  0,				/* `l', 0x6c */
  0,				/* `m', 0x6d */
  0,				/* `n', 0x6e */
  0,				/* `o', 0x6f */
  0,				/* `p', 0x70 */
  0,				/* `q', 0x71 */
  0,				/* `r', 0x72 */
  0,				/* `s', 0x73 */
  0,				/* `t', 0x74 */
  0,				/* `u', 0x75 */
  0,				/* `v', 0x76 */
  0,				/* `w', 0x77 */
  0,				/* `x', 0x78 */
  0,				/* `y', 0x79 */
  0,				/* `z', 0x7a */
  0,				/* `{', 0x7b */
  0,				/* `|', 0x7c */
  0,				/* `}', 0x7d */
  0,				/* `~', 0x7e */
  0				/* 0x7f */
};

PUCHAR EXPORTED FsRtlLegalAnsiCharacterArray = LegalAnsiCharacterArray;


/* FUNCTIONS ***************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDissectDbcs@16
 *
 *	Dissects a given path name into first and remaining part.
 *
 * ARGUMENTS
 *	Name
 *		ANSI string to dissect.
 *
 *	FirstPart
 *		Pointer to user supplied ANSI_STRING, that will
 *		later point to the first part of the original name.
 *
 *	RemainingPart
 *		Pointer to user supplied ANSI_STRING, that will
 *		later point to the remaining part of the original name.
 *
 * RETURN VALUE
 *	None
 *
 * EXAMPLE
 *	Name:		\test1\test2\test3
 *	FirstPart:	test1
 *	RemainingPart:	test2\test3
 *
 * @implemented
 */
VOID STDCALL
FsRtlDissectDbcs(IN ANSI_STRING Name,
		 OUT PANSI_STRING FirstPart,
		 OUT PANSI_STRING RemainingPart)
{
    ULONG i;
    ULONG FirstLoop;
    
    /* Initialize the Outputs */
    RtlZeroMemory(&FirstPart, sizeof(ANSI_STRING));
    RtlZeroMemory(&RemainingPart, sizeof(ANSI_STRING));
    
    /* Bail out if empty */
    if (!Name.Length) return;
    
    /* Ignore backslash */
    if (Name.Buffer[0] == '\\') {
        i = 1;
    } else {
        i = 0;
    }
    
    /* Loop until we find a backslash */
    for (FirstLoop = i;i < Name.Length;i++) {
        if (Name.Buffer[i] != '\\') break;
        if (FsRtlIsLeadDbcsCharacter(Name.Buffer[i])) i++;
    }
    
    /* Now we have the First Part */
    FirstPart->Length = (i-FirstLoop);
    FirstPart->MaximumLength = FirstPart->Length; /* +2?? */
    FirstPart->Buffer = &Name.Buffer[FirstLoop];
    
    /* Make the second part if something is still left */
    if (i<Name.Length) {
        RemainingPart->Length = (Name.Length - (i+1));
        RemainingPart->MaximumLength = RemainingPart->Length; /* +2?? */
        RemainingPart->Buffer = &Name.Buffer[i+1];
    }
    
    return;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlDoesDbcsContainWildCards@4
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @implemented
 */
BOOLEAN STDCALL
FsRtlDoesDbcsContainWildCards(IN PANSI_STRING Name)
{
    ULONG i;
    
    /* Check every character */
    for (i=0;i < Name->Length;i++) {
        
        /* First make sure it's not the Lead DBCS */
        if (FsRtlIsLeadDbcsCharacter(Name->Buffer[i])) {
            i++;
        } else if (FsRtlIsAnsiCharacterWild(Name->Buffer[i])) {
            /* Now return if it has a Wilcard */
            return TRUE;
        }
    }
    
    /* We didn't return above...so none found */
    return FALSE;        
}

/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsDbcsInExpression@8
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsDbcsInExpression (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsFatDbcsLegal@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsFatDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	return FALSE;
}


/**********************************************************************
 * NAME							EXPORTED
 *	FsRtlIsHpfsDbcsLegal@20
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * @unimplemented
 */
BOOLEAN
STDCALL
FsRtlIsHpfsDbcsLegal (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4
	)
{
	return FALSE;
}

/* EOF */
