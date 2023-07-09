/*	File: \wacker\emu\emudec.hh (Created: 29-Jan-1998)
 *
 *	Copyright 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 2 $
 *	$Date: 8/03/99 2:10p $
 */

// The maximum number of user defined keys.  Used by the VT220 and VT320.
//
#define MAX_UDK_KEYS    15
#define MAX_KEY_SPACE   256

// These constants are state table states used when processing user
// defined keys for the VT220 and VT320.
//
#define KEY_NUMBER_NEXT     0
#define KEY_DIGIT2_NEXT     1
#define SLASH_NEXT          2
#define CHAR_DIGIT1_NEXT    3
#define CHAR_DIGIT2_NEXT    4
#define ESC_SEEN            5

// The following key data structures are used only for user-defined
// keys in HTPE. But in shared code, they completely replace all 
// key data structures. In shared code, these definitios are in 
// \shared\emulator\emu.hh

// Key table structure definitions.  These are conditionally defined
// so the debug version of the program can supply additional information
// for testing.  In the debug version, when the user
// presses the F1 key, we can output the name of the key "HVK_F1" and
// the sequence that is assigned to that key.  The KEYDEF macro defined
// below is used in the initialization of the emulator key tables, found
// in each emulator's initialize function.  See John Masters for more
// details.
//
#if defined(_DEBUG)

typedef struct
    {
    KEYDEF Key;
    TCHAR * pSequence;
    unsigned int iSequenceLen;
    TCHAR * pszKeyName;
    } STEMUKEYDATA;

    #define EMUKEY(K, V, C, A, S, E, SEQ, L) \
        { K | (V ? VIRTUAL_KEY : 0) | (C ? CTRL_KEY : 0) | \
        (A ? ALT_KEY : 0) | (S ? SHIFT_KEY : 0) | (E ? EXTENDED_KEY : 0), \
        { TEXT(SEQ) }, {L}, {#K} }
//{ {K, V, C, A, S, E}, { TEXT(SEQ) }, {L}, {#K} }

#else

typedef struct
    {
    KEYDEF Key;
    TCHAR * pSequence;
    unsigned int iSequenceLen;
    } STEMUKEYDATA;

    #define EMUKEY(K, V, C, A, S, E, SEQ, L) \
        { K | (V ? VIRTUAL_KEY : 0) | (C ? CTRL_KEY : 0) | \
        (A ? ALT_KEY : 0) | (S ? SHIFT_KEY : 0) | (E ? EXTENDED_KEY : 0), \
        { TEXT(SEQ) }, {L} }
//{ {K, V, C, A, S, E}, { TEXT(SEQ) }, {L} }

#endif

typedef STEMUKEYDATA const * PSTCEMUKEYDATA;
typedef STEMUKEYDATA * PSTEMUKEYDATA;

// Private emulator data for DEC Terminals.
//
typedef struct stPrivateDEC
	{
	int sv_row,
		sv_col,
		sv_state,
		sv_AWM,
		sv_DECOM,
		sv_protectmode,
		fAttrsSaved,
		len_s,
		len_t,
		nState,
		gn,
		old_gl,
		gl,
		gr,
		sv_gr,
		sv_gl,
		fDecColHold,
		*aiLineAttr;

	ECHAR	storage[40],
			vt_charset[4],
			vt_sv_charset[4],
			terminate[4],
			*pntr;

	STATTR sv_attr;

#if defined(INCL_VT220)
    PSTCEMUKEYDATA  pstcEmuKeyTbl1,
                    pstcEmuKeyTbl2,
                    pstcEmuKeyTbl3,
                    pstcEmuKeyTbl4,
                    pstcEmuKeyTbl5,
                    pstcEmuKeyTbl6;

    int             iKeyTable1Entries,
                    iKeyTable2Entries,
                    iKeyTable3Entries,
                    iKeyTable4Entries,
                    iKeyTable5Entries,
                    iKeyTable6Entries;

	// A pointer to a table of user defined keys,
	//
	PSTEMUKEYDATA  pstUDK;

	int     iUDKTableEntries;

	// This variable is a state variable used in the processing of
	// user defined keys in the VT220 and VT320.
	//
	int     iUDKState,
			iUDKTableIndex,
			iUDKSequenceLen;

	// A flag that is used identify the locked or unlocked status
	// of the UDK's, after they are defined.  See emuDecClearUDK.
	//
	int     fUnlockedUDK;

	// A temporary buffer to collect the user defined key sequence as
	// it is being processed.
	//
	TCHAR   acUDKSequence[MAX_KEY_SPACE];

	TCHAR   chUDKAssignment;

	TCHAR   const *pacUDKSelectors;
#endif
	} DECPRIVATE;

typedef DECPRIVATE *PSTDECPRIVATE;

// From vt220ini.c
void vt220_init(const HHEMU hhEmu);

// From vt220.c
void    vt220_DA(const HHEMU hhEmu);
void    vt220_hostreset(const HHEMU hhEmu);
void    vt100_printcmnds(const HHEMU hhEmu);
void    emuDecClearUDK(const HHEMU hhEmu);
void    vt220_softreset(const HHEMU hhEmu);
void    vt220_2ndDA(const HHEMU hhEmu);
void    vt220_definekey(const HHEMU hhEmu);
void    vt220_level(const HHEMU hhEmu);
void    vt220_protmode(const HHEMU hhEmu);
int     vt220_reset(const HHEMU hhEmu, const int host_request);
void    vt220mode_reset(const HHEMU hhEmu);
void    vt220_savekeys(const HHEMU hhEmu, const int iSave);
void    emuDecSendKeyString(const HHEMU hhEmu,
                        const int iIndex,
                        PSTCEMUKEYDATA pstcKeyTbl,
                        const int iMaxEntries);

void    emuDecDefineUDK(const HHEMU hhEmu);
int     emuDecStoreUDK(const HHEMU hhEmu);
int		emuDecKeyboardIn(const HHEMU hhEmu, int Key, 
				const int fTest);
int		emuDecKbdKeyLookup(const HHEMU hhEmu,
                const KEYDEF Key, PSTCEMUKEYDATA pstKeyTbl,
                const int iMaxEntries);
void	emuDecSendKeyString(const HHEMU hhEmu, const int iIndex,
                PSTCEMUKEYDATA pstcKeyTbl, const int iMaxEntries);
void	emuVT220SendKeyString(const HHEMU hhEmu,
                const int iIndex, PSTCEMUKEYDATA pstcKeyTbl,
                const int iMaxEntries);
void	emuDecEL(const HHEMU hhEmu);
void	emuDecClearLine(const HHEMU hhEmu, const int iClearSelect);
void	emuVT220ED(const HHEMU hhEmu);
void	emuDecEraseScreen(const HHEMU hhEmu, const int iClearSelect);
void	emuDecClearImageRowSelective(const HHEMU hhEmu, const int iImageRow);
void	emuDecUnload(const HHEMU hhEmu);

