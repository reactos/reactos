/*****************************************************************8
Speech.H - Header file to use the Microsoft Speech APIs.

Copyright 1994, 1995 by Microsoft corporation.All rights reserved.
*/

#ifndef _SPEECH_
#define _SPEECH_

// Disable the warning for zero-length arrays in structures
#pragma warning(disable:4200)


/************************************************************************
Defines common to all of the speech APIs.
*/

// Application  Speech API   Compiler Defines                   _S_UNICODE
// -----------------------------------------------------------------------------
//   ANSI        ANSI        <none>                             undefined
//   ANSI        Unicode     _S_UNICODE                         defined
//   Unicode     ANSI        (UNICODE || _UNICODE) && _S_ANSI   undefined
//   Unicode     Unicode     (UNICODE || _UNICODE)              defined

#if (defined(UNICODE) || defined(_UNICODE)) && !defined(_S_ANSI)
#ifndef _S_UNICODE
#define _S_UNICODE
#endif
#endif

/************************************************************************
defines */
#define  SVFN_LEN    (262)
#define  LANG_LEN    (64)
#define  EI_TITLESIZE   (128)
#define  EI_DESCSIZE    (512)
#define  EI_FIXSIZE     (512)
#define  SVPI_MFGLEN    (64)
#define  SVPI_PRODLEN   (64)
#define  SVPI_COMPLEN   (64)
#define  SVPI_COPYRIGHTLEN (128)
#define  SVI_MFGLEN     (SVPI_MFGLEN)
#define  SETBIT(x)      ((DWORD)1 << (x))


// Error Macros
#define  FACILITY_SPEECH   (FACILITY_ITF)
#define  SPEECHERROR(x)    MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x200)
#define  AUDERROR(x)       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x300)
#define  SRWARNING(x)      MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_SPEECH, (x)+0x400)
#define  SRERROR(x)        MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x400)
#define  TTSERROR(x)       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x500)
#define  VCMDERROR(x)      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x600)
#define  VTXTERROR(x)      MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x700)
#define  LEXERROR(x)       MAKE_SCODE(SEVERITY_ERROR,   FACILITY_SPEECH, (x)+0x800)

// Audio Errors
#define  AUDERR_NONE                      S_OK                          // 0x00000000
#define  AUDERR_BADDEVICEID               AUDERROR(1)                   // 0x80040301
#define  AUDERR_NEEDWAVEFORMAT            AUDERROR(2)                   // 0x80040302
#define  AUDERR_NOTSUPPORTED              E_NOTIMPL                     // 0x80004001
#define  AUDERR_NOTENOUGHDATA             SPEECHERROR(1)                // 0x80040201
#define  AUDERR_NOTPLAYING                AUDERROR(6)                   // 0x80040306
#define  AUDERR_INVALIDPARAM              E_INVALIDARG                  // 0x80070057
#define  AUDERR_WAVEFORMATNOTSUPPORTED    SPEECHERROR(2)                // 0x80040202
#define  AUDERR_WAVEDEVICEBUSY            SPEECHERROR(3)                // 0x80040203
#define  AUDERR_WAVEDEVNOTSUPPORTED       AUDERROR(18)                  // 0x80040312
#define  AUDERR_NOTRECORDING              AUDERROR(19)                  // 0x80040313
#define  AUDERR_INVALIDFLAG               SPEECHERROR(4)                // 0x80040204
#define  AUDERR_INVALIDHANDLE             E_HANDLE                      // 0x80070006
#define  AUDERR_NODRIVER                  AUDERROR(23)                  // 0x80040317
#define  AUDERR_HANDLEBUSY                AUDERROR(24)                  // 0x80040318
#define  AUDERR_INVALIDNOTIFYSINK         AUDERROR(25)                  // 0x80040319
#define  AUDERR_WAVENOTENABLED            AUDERROR(26)                  // 0x8004031A
#define  AUDERR_ALREADYCLAIMED            AUDERROR(29)                  // 0x8004031D
#define  AUDERR_NOTCLAIMED                AUDERROR(30)                  // 0x8004031E
#define  AUDERR_STILLPLAYING              AUDERROR(31)                  // 0x8004031F
#define  AUDERR_ALREADYSTARTED            AUDERROR(32)                  // 0x80040320
#define  AUDERR_SYNCNOTALLOWED            AUDERROR(33)                  // 0x80040321

// Speech Recognition Warnings
#define  SRWARN_BAD_LIST_PRONUNCIATION    SRWARNING(1)

// Speech Recognition Errors
#define  SRERR_NONE                       S_OK                          // 0x00000000
#define  SRERR_OUTOFDISK                  SPEECHERROR(5)                // 0x80040205
#define  SRERR_NOTSUPPORTED               E_NOTIMPL                     // 0x80004001
#define  SRERR_NOTENOUGHDATA              AUDERR_NOTENOUGHDATA          // 0x80040201
#define  SRERR_VALUEOUTOFRANGE            E_UNEXPECTED                  // 0x8000FFFF
#define  SRERR_GRAMMARTOOCOMPLEX          SRERROR(6)                    // 0x80040406
#define  SRERR_GRAMMARWRONGTYPE           SRERROR(7)                    // 0x80040407
#define  SRERR_INVALIDWINDOW              OLE_E_INVALIDHWND             // 0x8004000F
#define  SRERR_INVALIDPARAM               E_INVALIDARG                  // 0x80070057
#define  SRERR_INVALIDMODE                SPEECHERROR(6)                // 0x80040206
#define  SRERR_TOOMANYGRAMMARS            SRERROR(11)                   // 0x8004040B
#define  SRERR_INVALIDLIST                SPEECHERROR(7)                // 0x80040207
#define  SRERR_WAVEDEVICEBUSY             AUDERR_WAVEDEVICEBUSY         // 0x80040203
#define  SRERR_WAVEFORMATNOTSUPPORTED     AUDERR_WAVEFORMATNOTSUPPORTED // 0x80040202
#define  SRERR_INVALIDCHAR                SPEECHERROR(8)                // 0x80040208
#define  SRERR_GRAMTOOCOMPLEX             SRERR_GRAMMARTOOCOMPLEX       // 0x80040406
#define  SRERR_GRAMTOOLARGE               SRERROR(17)                   // 0x80040411
#define  SRERR_INVALIDINTERFACE           E_NOINTERFACE                 // 0x80004002
#define  SRERR_INVALIDKEY                 SPEECHERROR(9)                // 0x80040209
#define  SRERR_INVALIDFLAG                AUDERR_INVALIDFLAG            // 0x80040204
#define  SRERR_GRAMMARERROR               SRERROR(22)                   // 0x80040416
#define  SRERR_INVALIDRULE                SRERROR(23)                   // 0x80040417
#define  SRERR_RULEALREADYACTIVE          SRERROR(24)                   // 0x80040418
#define  SRERR_RULENOTACTIVE              SRERROR(25)                   // 0x80040419
#define  SRERR_NOUSERSELECTED             SRERROR(26)                   // 0x8004041A
#define  SRERR_BAD_PRONUNCIATION          SRERROR(27)                   // 0x8004041B
#define  SRERR_DATAFILEERROR              SRERROR(28)                   // 0x8004041C
#define  SRERR_GRAMMARALREADYACTIVE       SRERROR(29)                   // 0x8004041D
#define  SRERR_GRAMMARNOTACTIVE           SRERROR(30)                   // 0x8004041E
#define  SRERR_GLOBALGRAMMARALREADYACTIVE SRERROR(31)                   // 0x8004041F
#define  SRERR_LANGUAGEMISMATCH           SRERROR(32)                   // 0x80040420
#define  SRERR_MULTIPLELANG               SRERROR(33)                   // 0x80040421
#define  SRERR_LDGRAMMARNOWORDS           SRERROR(34)                   // 0x80040422
#define  SRERR_NOLEXICON                  SRERROR(35)                   // 0x80040423
#define  SRERR_SPEAKEREXISTS              SRERROR(36)                   // 0x80040424
#define  SRERR_GRAMMARENGINEMISMATCH      SRERROR(37)                   // 0x80040425


// Text to Speech Errors
#define  TTSERR_NONE                      S_OK                          // 0x00000000
#define  TTSERR_INVALIDINTERFACE          E_NOINTERFACE                 // 0x80004002
#define  TTSERR_OUTOFDISK                 SRERR_OUTOFDISK               // 0x80040205
#define  TTSERR_NOTSUPPORTED              E_NOTIMPL                     // 0x80004001
#define  TTSERR_VALUEOUTOFRANGE           E_UNEXPECTED                  // 0x8000FFFF
#define  TTSERR_INVALIDWINDOW             OLE_E_INVALIDHWND             // 0x8004000F
#define  TTSERR_INVALIDPARAM              E_INVALIDARG                  // 0x80070057
#define  TTSERR_INVALIDMODE               SRERR_INVALIDMODE             // 0x80040206
#define  TTSERR_INVALIDKEY                SRERR_INVALIDKEY              // 0x80040209
#define  TTSERR_WAVEFORMATNOTSUPPORTED    AUDERR_WAVEFORMATNOTSUPPORTED // 0x80040202
#define  TTSERR_INVALIDCHAR               SRERR_INVALIDCHAR             // 0x80040208
#define  TTSERR_QUEUEFULL                 SPEECHERROR(10)               // 0x8004020A
#define  TTSERR_WAVEDEVICEBUSY            AUDERR_WAVEDEVICEBUSY         // 0x80040203
#define  TTSERR_NOTPAUSED                 TTSERROR(1)                   // 0x80040501
#define  TTSERR_ALREADYPAUSED             TTSERROR(2)                   // 0x80040502


// Voice Command Errors

/*
 *  Everything worked
 */
#define  VCMDERR_NONE                     S_OK                          // 0x00000000

/*
 *  Voice Commands could not allocate memory
 */
#define  VCMDERR_OUTOFMEM                 E_OUTOFMEMORY                 // 0x8007000E

/*
 *  Voice Commands could not store/retrieve a command set from the database
 */
#define  VCMDERR_OUTOFDISK                SRERR_OUTOFDISK               // 0x80040205

/*
 *  Function not implemented
 */
#define  VCMDERR_NOTSUPPORTED             E_NOTIMPL                     // 0x80004001

/*
 *  A parameter was passed that was out of the ranged of accepted values
 */
#define  VCMDERR_VALUEOUTOFRANGE          E_UNEXPECTED                  // 0x8000FFFF

/*
 *  A menu was too complex to compile a context-free grammar
 */
#define  VCMDERR_MENUTOOCOMPLEX           VCMDERROR(0x06)               //  0x80040606

/*
 *  Language mismatch between the speech recognition mode and menu trying
 *  to create
 */
#define  VCMDERR_MENUWRONGLANGUAGE        VCMDERROR(0x07)               // 0x80040607

/*
 *  An invalid window handle was passed to Voice Commands
 */
#define  VCMDERR_INVALIDWINDOW            OLE_E_INVALIDHWND             // 0x8004000F

/*
 *  Voice Commands detected a bad function parameter
 */
#define  VCMDERR_INVALIDPARAM             E_INVALIDARG                  // 0x80070057

/*
 *  This function cannot be completed right now, usually when trying to do
 *  some operation while no speech recognition site is established
 */
#define  VCMDERR_INVALIDMODE              SRERR_INVALIDMODE             // 0x80040206

/*
 *  There are too many Voice Commands menu
 */                                                                     // 0x8004060B
#define  VCMDERR_TOOMANYMENUS             VCMDERROR(0x0B)

/*
 *  Invalid list passed to ListSet/ListGet
 */
#define  VCMDERR_INVALIDLIST              SRERR_INVALIDLIST             // 0x80040207

/*
 *  Trying to open an existing menu that is not in the Voice Commands database
 */
#define  VCMDERR_MENUDOESNOTEXIST         VCMDERROR(0x0D)               // 0x8004060D

/*
 *  The function could not be completed because the menu is actively 
 *  listening for commands
 */
#define  VCMDERR_MENUACTIVE               VCMDERROR(0x0E)               // 0x8004060E

/*
 *  No speech recognition engine is started
 */
#define  VCMDERR_NOENGINE                 VCMDERROR(0x0F)               // 0x8004060F

/*
 *  Voice Commands could not acquire a Grammar interface from the speech
 *  recognition engine
 */
#define  VCMDERR_NOGRAMMARINTERFACE       VCMDERROR(0x10)               // 0x80040610

/*
 *  Voice Commands could not acquire a Find interface from the speech
 *  recognition engine
 */
#define  VCMDERR_NOFINDINTERFACE          VCMDERROR(0x11)               // 0x80040611

/*
 *  Voice Commands could not create a speech recognition enumerator
 */
#define  VCMDERR_CANTCREATESRENUM         VCMDERROR(0x12)               // 0x80040612

/*
 *  Voice Commands could get the appropriate site information to start a
 *  speech recognition engine
 */
#define  VCMDERR_NOSITEINFO               VCMDERROR(0x13)               // 0x80040613

/*
 *  Voice Commands could not find a speech recognition engine
 */
#define  VCMDERR_SRFINDFAILED             VCMDERROR(0x14)               // 0x80040614

/*
 *  Voice Commands could not create an audio source object
 */
#define  VCMDERR_CANTCREATEAUDIODEVICE    VCMDERROR(0x15)               // 0x80040615

/*
 *  Voice Commands could not set the appropriate device number in the
 *  audio source object
 */
#define  VCMDERR_CANTSETDEVICE            VCMDERROR(0x16)               // 0x80040616

/*
 *  Voice Commands could not select a speech recognition engine. Usually the
 *  error will occur when Voice Commands has enumerated and found an
 *  appropriate speech recognition engine, then it is not able to actually
 *  select/start the engine. There are different reasons that the engine won't
 *  start, but the most common is that there is no wave in device.
 */
#define  VCMDERR_CANTSELECTENGINE         VCMDERROR(0x17)               // 0x80040617

/*
 *  Voice Commands could not create a notfication sink for engine
 *  notifications
 */
#define  VCMDERR_CANTCREATENOTIFY         VCMDERROR(0x18)               // 0x80040618

/*
 *  Voice Commands could not create internal data structures.
 */
#define  VCMDERR_CANTCREATEDATASTRUCTURES VCMDERROR(0x19)               // 0x80040619

/*
 *  Voice Commands could not initialize internal data structures
 */
#define  VCMDERR_CANTINITDATASTRUCTURES   VCMDERROR(0x1A)               // 0x8004061A

/*
 *  The menu does not have an entry in the Voice Commands cache
 */
#define  VCMDERR_NOCACHEDATA              VCMDERROR(0x1B)               // 0x8004061B

/*
 *  The menu does not have commands
 */
#define  VCMDERR_NOCOMMANDS               VCMDERROR(0x1C)               // 0x8004061C

/*
 *  Voice Commands cannot extract unique words needed for the engine grammar
 */
#define  VCMDERR_CANTXTRACTWORDS          VCMDERROR(0x1D)               // 0x8004061D

/*
 *  Voice Commands could not get the command set database name
 */
#define  VCMDERR_CANTGETDBNAME            VCMDERROR(0x1E)               // 0x8004061E

/*
 *  Voice Commands could not create a registry key
 */
#define  VCMDERR_CANTCREATEKEY            VCMDERROR(0x1F)               // 0x8004061F

/*
 *  Voice Commands could not create a new database name
 */
#define  VCMDERR_CANTCREATEDBNAME         VCMDERROR(0x20)               // 0x80040620

/*
 *  Voice Commands could not update the registry
 */
#define  VCMDERR_CANTUPDATEREGISTRY       VCMDERROR(0x21)               // 0x80040621

/*
 *  Voice Commands could not open the registry
 */
#define  VCMDERR_CANTOPENREGISTRY         VCMDERROR(0x22)               // 0x80040622

/*
 *  Voice Commands could not open the command set database
 */
#define  VCMDERR_CANTOPENDATABASE         VCMDERROR(0x23)               // 0x80040623

/*
 *  Voice Commands could not create a database storage object
 */
#define  VCMDERR_CANTCREATESTORAGE        VCMDERROR(0x24)               // 0x80040624

/*
 *  Voice Commands could not do CmdMimic
 */
#define  VCMDERR_CANNOTMIMIC              VCMDERROR(0x25)               // 0x80040625

/*
 *  A menu of this name already exist
 */
#define  VCMDERR_MENUEXIST                VCMDERROR(0x26)               // 0x80040626

/*
 *  A menu of this name is open and cannot be deleted right now
 */
#define  VCMDERR_MENUOPEN                 VCMDERROR(0x27)               // 0x80040627


// Voice Text Errors
#define  VTXTERR_NONE                     S_OK                          // 0x00000000

/*
 *  Voice Text failed to allocate memory it needed
 */
#define  VTXTERR_OUTOFMEM                 E_OUTOFMEMORY                 // 0x8007000E

/*
 *  An empty string ("") was passed to the Speak function
 */
#define  VTXTERR_EMPTYSPEAKSTRING         SPEECHERROR(0x0b)             // 0x8004020B

/*
 *  An invalid parameter was passed to a Voice Text function
 */
#define  VTXTERR_INVALIDPARAM             E_INVALIDARG                  // 0x80070057

/*
 *  The called function cannot be done at this time. This usually occurs
 *  when trying to call a function that needs a site, but no site has been
 *  registered.
 */
#define  VTXTERR_INVALIDMODE              SRERR_INVALIDMODE             // 0x80040206

/*
 *  No text-to-speech engine is started
 */
#define  VTXTERR_NOENGINE                 VTXTERROR(0x0F)               // 0x8004070F

/*
 *  Voice Text could not acquire a Find interface from the text-to-speech
 *  engine
 */
#define  VTXTERR_NOFINDINTERFACE          VTXTERROR(0x11)               // 0x80040711

/*
 *  Voice Text could not create a text-to-speech enumerator
 */
#define  VTXTERR_CANTCREATETTSENUM        VTXTERROR(0x12)               // 0x80040712

/*
 *  Voice Text could get the appropriate site information to start a
 *  text-to-speech engine
 */
#define  VTXTERR_NOSITEINFO               VTXTERROR(0x13)               // 0x80040713

/*
 *  Voice Text could not find a text-to-speech engine
 */
#define  VTXTERR_TTSFINDFAILED            VTXTERROR(0x14)               // 0x80040714

/*
 *  Voice Text could not create an audio destination object
 */
#define  VTXTERR_CANTCREATEAUDIODEVICE    VTXTERROR(0x15)               // 0x80040715

/*
 *  Voice Text could not set the appropriate device number in the
 *  audio destination object
 */
#define  VTXTERR_CANTSETDEVICE            VTXTERROR(0x16)               // 0x80040716

/*
 *  Voice Text could not select a text-to-speech engine. Usually the
 *  error will occur when Voice Text has enumerated and found an
 *  appropriate text-to-speech engine, then it is not able to actually
 *  select/start the engine.
 */
#define  VTXTERR_CANTSELECTENGINE         VTXTERROR(0x17)               // 0x80040717

/*
 *  Voice Text could not create a notfication sink for engine
 *  notifications
 */
#define  VTXTERR_CANTCREATENOTIFY         VTXTERROR(0x18)               // 0x80040718

/*
 *  Voice Text is disabled at this time
 */
#define  VTXTERR_NOTENABLED               VTXTERROR(0x19)               // 0x80040719

#define  VTXTERR_OUTOFDISK                SRERR_OUTOFDISK               // 0x80040205
#define  VTXTERR_NOTSUPPORTED             E_NOTIMPL                     // 0x80004001
#define  VTXTERR_NOTENOUGHDATA            AUDERR_NOTENOUGHDATA          // 0x80040201
#define  VTXTERR_QUEUEFULL                TTSERR_QUEUEFULL              // 0x8004020A
#define  VTXTERR_VALUEOUTOFRANGE          E_UNEXPECTED                  // 0x8000FFFF
#define  VTXTERR_INVALIDWINDOW            OLE_E_INVALIDHWND             // 0x8004000F
#define  VTXTERR_WAVEDEVICEBUSY           AUDERR_WAVEDEVICEBUSY         // 0x80040203
#define  VTXTERR_WAVEFORMATNOTSUPPORTED   AUDERR_WAVEFORMATNOTSUPPORTED // 0x80040202
#define  VTXTERR_INVALIDCHAR              SRERR_INVALIDCHAR             // 0x80040208


// ILexPronounce errors
#define  LEXERR_INVALIDTEXTCHAR           LEXERROR(0x01)                // 0x80040801
#define  LEXERR_INVALIDSENSE              LEXERROR(0x02)                // 0x80040802
#define  LEXERR_NOTINLEX                  LEXERROR(0x03)                // 0x80040803
#define  LEXERR_OUTOFDISK                 LEXERROR(0x04)                // 0x80040804
#define  LEXERR_INVALIDPRONCHAR           LEXERROR(0x05)                // 0x80040805
#define  LEXERR_ALREADYINLEX              LEXERROR(0x06)                // 0x80040806
#define  LEXERR_PRNBUFTOOSMALL            LEXERROR(0x07)                // 0x80040807
#define  LEXERR_ENGBUFTOOSMALL            LEXERROR(0x08)                // 0x80040808



/************************************************************************
typedefs */

typedef LPUNKNOWN FAR * PIUNKNOWN;

typedef struct {
   PVOID    pData;
   DWORD    dwSize;
   } SDATA, * PSDATA;



typedef struct {
   LANGID   LanguageID;
   WCHAR    szDialect[LANG_LEN];
   } LANGUAGEW, FAR * PLANGUAGEW;

typedef struct {
   LANGID   LanguageID;
   CHAR     szDialect[LANG_LEN];
   } LANGUAGEA, FAR * PLANGUAGEA;

#ifdef  _S_UNICODE
#define LANGUAGE    LANGUAGEW
#define PLANGUAGE   PLANGUAGEW
#else
#define LANGUAGE    LANGUAGEA
#define PLANGUAGE   PLANGUAGEA
#endif  // _S_UNICODE



typedef unsigned _int64 QWORD, * PQWORD;

typedef enum {
   CHARSET_TEXT           = 0,
   CHARSET_IPAPHONETIC    = 1,
   CHARSET_ENGINEPHONETIC = 2
   } VOICECHARSET;

typedef enum _VOICEPARTOFSPEECH {
   VPS_UNKNOWN = 0,
   VPS_NOUN = 1,
   VPS_VERB = 2,
   VPS_ADVERB = 3,
   VPS_ADJECTIVE = 4,
   VPS_PROPERNOUN = 5,
   VPS_PRONOUN = 6,
   VPS_CONJUNCTION = 7,
   VPS_CARDINAL = 8,
   VPS_ORDINAL = 9,
   VPS_DETERMINER = 10,
   VPS_QUANTIFIER = 11,
   VPS_PUNCTUATION = 12,
   VPS_CONTRACTION = 13,
   VPS_INTERJECTION = 14,
   VPS_ABBREVIATION = 15,
   VPS_PREPOSITION = 16
   } VOICEPARTOFSPEECH;


typedef struct {
   DWORD   dwNextPhonemeNode;
   DWORD   dwUpAlternatePhonemeNode;
   DWORD   dwDownAlternatePhonemeNode;
   DWORD   dwPreviousPhonemeNode;
   DWORD   dwWordNode;
   QWORD   qwStartTime;
   QWORD   qwEndTime;
   DWORD   dwPhonemeScore;
   WORD    wVolume;
   WORD    wPitch;
   } SRRESPHONEMENODE, *PSRRESPHONEMENODE;


typedef struct {
   DWORD   dwNextWordNode;
   DWORD   dwUpAlternateWordNode;
   DWORD   dwDownAlternateWordNode;
   DWORD   dwPreviousWordNode;
   DWORD   dwPhonemeNode;
   QWORD   qwStartTime;
   QWORD   qwEndTime;
   DWORD   dwWordScore;
   WORD      wVolume;
   WORD      wPitch;
   VOICEPARTOFSPEECH   pos;
   DWORD   dwCFGParse;
   DWORD   dwCue;
   } SRRESWORDNODE, * PSRRESWORDNODE;


/************************************************************************
interfaces */

/*
 * ILexPronounce
 */

#undef   INTERFACE
#define  INTERFACE   ILexPronounceW

DEFINE_GUID(IID_ILexPronounceW, 0x090CD9A2, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ILexPronounceW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // LexPronounceW members
   STDMETHOD (Add)            (THIS_ VOICECHARSET, PCWSTR, PCWSTR, 
			       VOICEPARTOFSPEECH, PVOID, DWORD) PURE;
   STDMETHOD (Get)            (THIS_ VOICECHARSET, PCWSTR, WORD, PWSTR, 
			       DWORD, DWORD *, VOICEPARTOFSPEECH *, PVOID, 
			       DWORD, DWORD *) PURE;
   STDMETHOD (Remove)         (THIS_ PCWSTR, WORD) PURE;
   };

typedef ILexPronounceW FAR * PILEXPRONOUNCEW;


#undef   INTERFACE
#define  INTERFACE   ILexPronounceA

DEFINE_GUID(IID_ILexPronounceA, 0x2F26B9C0L, 0xDB31, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ILexPronounceA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // LexPronounceA members
   STDMETHOD (Add)            (THIS_ VOICECHARSET, PCSTR, PCSTR, 
			       VOICEPARTOFSPEECH, PVOID, DWORD) PURE;
   STDMETHOD (Get)            (THIS_ VOICECHARSET, PCSTR, WORD, PSTR, 
			       DWORD, DWORD *, VOICEPARTOFSPEECH *, PVOID, 
			       DWORD, DWORD *) PURE;
   STDMETHOD (Remove)         (THIS_ PCSTR, WORD) PURE;
   };

typedef ILexPronounceA FAR * PILEXPRONOUNCEA;


#ifdef _S_UNICODE
 #define ILexPronounce        ILexPronounceW
 #define IID_ILexPronounce    IID_ILexPronounceW
 #define PILEXPRONOUNCE       PILEXPRONOUNCEW

#else
 #define ILexPronounce        ILexPronounceA
 #define IID_ILexPronounce    IID_ILexPronounceA
 #define PILEXPRONOUNCE       PILEXPRONOUNCEA

#endif   // _S_UNICODE



/************************************************************************
Audio source/destiantion API
*/

/************************************************************************
defines */

// AudioStop
#define      IANSRSN_NODATA             0
#define      IANSRSN_PRIORITY           1
#define      IANSRSN_INACTIVE           2
#define      IANSRSN_EOF                3

// IAudioSourceInstrumented::StateSet
#define          IASISTATE_PASSTHROUGH      0
#define          IASISTATE_PASSNOTHING      1
#define          IASISTATE_PASSREADFROMWAVE 2
#define          IASISTATE_PASSWRITETOWAVE  3

/************************************************************************
typedefs */

/************************************************************************
Class IDs */
// {CB96B400-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(CLSID_MMAudioDest, 
0xcb96b400, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);
// {D24FE500-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(CLSID_MMAudioSource, 
0xd24fe500, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);
// {D4023720-E4B9-11cf-8D56-00A0C9034A7E}
DEFINE_GUID(CLSID_InstAudioSource, 
0xd4023720, 0xe4b9, 0x11cf, 0x8d, 0x56, 0x0, 0xa0, 0xc9, 0x3, 0x4a, 0x7e);

/************************************************************************
interfaces */

// IAudio
#undef   INTERFACE
#define  INTERFACE   IAudio

// {F546B340-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_IAudio, 
0xf546b340, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (IAudio, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudio members
   STDMETHOD (Flush)          (THIS) PURE;
   STDMETHOD (LevelGet)       (THIS_ DWORD *) PURE;
   STDMETHOD (LevelSet)       (THIS_ DWORD) PURE;
   STDMETHOD (PassNotify)     (THIS_ PVOID, IID) PURE;
   STDMETHOD (PosnGet)        (THIS_ PQWORD) PURE;
   STDMETHOD (Claim)          (THIS) PURE;
   STDMETHOD (UnClaim)        (THIS) PURE;
   STDMETHOD (Start)          (THIS) PURE;
   STDMETHOD (Stop)           (THIS) PURE;
   STDMETHOD (TotalGet)       (THIS_ PQWORD) PURE;
   STDMETHOD (ToFileTime)     (THIS_ PQWORD, FILETIME *) PURE;
   STDMETHOD (WaveFormatGet)  (THIS_ PSDATA) PURE;
   STDMETHOD (WaveFormatSet)  (THIS_ SDATA) PURE;
   };

typedef IAudio FAR * PIAUDIO;

// IAudioDest

#undef   INTERFACE
#define  INTERFACE   IAudioDest

// {2EC34DA0-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_IAudioDest, 
0x2ec34da0, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (IAudioDest, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudioDest members
   STDMETHOD (FreeSpace)      (THIS_ DWORD *, BOOL *) PURE;
   STDMETHOD (DataSet)        (THIS_ PVOID, DWORD) PURE;
   STDMETHOD (BookMark)       (THIS_ DWORD) PURE;
   };

typedef IAudioDest FAR * PIAUDIODEST;



// IAudioDestNotifySink

#undef   INTERFACE
#define  INTERFACE   IAudioDestNotifySink

// {ACB08C00-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_IAudioDestNotifySink, 
0xacb08c00, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (IAudioDestNotifySink, IUnknown) {
   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudioDestNotifySink members
   STDMETHOD (AudioStop)      (THIS_ WORD) PURE;
   STDMETHOD (AudioStart)     (THIS) PURE;
   STDMETHOD (FreeSpace)      (THIS_ DWORD, BOOL) PURE;
   STDMETHOD (BookMark)       (THIS_ DWORD, BOOL) PURE;
   };

typedef IAudioDestNotifySink FAR * PIAUDIODESTNOTIFYSINK;



// IAudioMultiMediaDevice

#undef   INTERFACE
#define  INTERFACE   IAudioMultiMediaDevice

// {B68AD320-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_IAudioMultiMediaDevice, 
0xb68ad320, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (IAudioMultiMediaDevice, IUnknown) {
   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudioMultiMediaDevice members
   STDMETHOD (CustomMessage)  (THIS_ UINT, SDATA) PURE;
   STDMETHOD (DeviceNumGet)   (THIS_ DWORD*) PURE;
   STDMETHOD (DeviceNumSet)   (THIS_ DWORD) PURE;
   };

typedef IAudioMultiMediaDevice FAR * PIAUDIOMULTIMEDIADEVICE;



// IAudioSource
#undef   INTERFACE
#define  INTERFACE   IAudioSource

// {BC06A220-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_IAudioSource, 
0xbc06a220, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (IAudioSource, IUnknown) {
   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudioSource members
   STDMETHOD (DataAvailable)  (THIS_ DWORD *, BOOL *) PURE;
   STDMETHOD (DataGet)        (THIS_ PVOID, DWORD, DWORD *) PURE;
   };

typedef IAudioSource FAR * PIAUDIOSOURCE;



// IAudioSourceInstrumented
#undef   INTERFACE
#define  INTERFACE   IAudioSourceInstrumented

// {D4023721-E4B9-11cf-8D56-00A0C9034A7E}
DEFINE_GUID(IID_IAudioSourceInstrumented, 
0xd4023721, 0xe4b9, 0x11cf, 0x8d, 0x56, 0x0, 0xa0, 0xc9, 0x3, 0x4a, 0x7e);

DECLARE_INTERFACE_ (IAudioSourceInstrumented, IUnknown) {
   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudioSourceInstrumented members
   STDMETHOD (AudioSource)    (THIS_ LPUNKNOWN) PURE;
   STDMETHOD (RegistryGet)    (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (RegistrySet)    (THIS_ PCWSTR) PURE;
   STDMETHOD (StateGet)       (THIS_ DWORD*) PURE;
   STDMETHOD (StateSet)       (THIS_ DWORD) PURE;
   STDMETHOD (WaveFileReadGet)(THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (WaveFileReadSet)(THIS_ PCWSTR) PURE;
   STDMETHOD (WaveFileWriteGet)(THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (WaveFileWriteSet)(THIS_ PCWSTR) PURE;
   };

typedef IAudioSourceInstrumented FAR * PIAUDIOSOURCEINSTRUMENTED;


// IAudioSourceNotifySink
#undef   INTERFACE
#define  INTERFACE   IAudioSourceNotifySink

// {C0BD9A80-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_IAudioSourceNotifySink, 
0xc0bd9a80, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (IAudioSourceNotifySink, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IAudioSourceNotifySink members
   STDMETHOD (AudioStop)      (THIS_ WORD) PURE;
   STDMETHOD (AudioStart)     (THIS) PURE;
   STDMETHOD (DataAvailable)  (THIS_ DWORD, BOOL) PURE;
   STDMETHOD (Overflow)       (THIS_ DWORD) PURE;
   };

typedef IAudioSourceNotifySink FAR * PIAUDIOSOURCENOTIFYSINK;



/************************************************************************
defines */
/* SRINFO */
#define  SRMI_NAMELEN                  SVFN_LEN

#define  SRSEQUENCE_DISCRETE           (0)
#define  SRSEQUENCE_CONTINUOUS          (1)
#define  SRSEQUENCE_WORDSPOT            (2)
#define  SRSEQUENCE_CONTCFGDISCDICT     (3)

#define  SRGRAM_CFG                    SETBIT(0)
#define  SRGRAM_DICTATION              SETBIT(1)
#define  SRGRAM_LIMITEDDOMAIN          SETBIT(2)

#define  SRFEATURE_INDEPSPEAKER        SETBIT(0)
#define  SRFEATURE_INDEPMICROPHONE     SETBIT(1)
#define  SRFEATURE_TRAINWORD           SETBIT(2)
#define  SRFEATURE_TRAINPHONETIC       SETBIT(3)
#define  SRFEATURE_WILDCARD            SETBIT(4)
#define  SRFEATURE_ANYWORD             SETBIT(5)
#define  SRFEATURE_PCOPTIMIZED         SETBIT(6)
#define  SRFEATURE_PHONEOPTIMIZED      SETBIT(7)
#define  SRFEATURE_GRAMLIST            SETBIT(8)
#define  SRFEATURE_GRAMLINK            SETBIT(9)
#define  SRFEATURE_MULTILINGUAL        SETBIT(10)
#define  SRFEATURE_GRAMRECURSIVE       SETBIT(11)
#define  SRFEATURE_IPAUNICODE          SETBIT(12)

#define  SRI_ILEXPRONOUNCE             SETBIT(0)
#define  SRI_ISRATTRIBUTES             SETBIT(1)
#define  SRI_ISRCENTRAL                SETBIT(2)
#define  SRI_ISRDIALOGS                SETBIT(3)
#define  SRI_ISRGRAMCOMMON             SETBIT(4)
#define  SRI_ISRGRAMCFG                SETBIT(5)
#define  SRI_ISRGRAMDICTATION          SETBIT(6)
#define  SRI_ISRGRAMINSERTIONGUI       SETBIT(7)
#define  SRI_ISRESBASIC                SETBIT(8)
#define  SRI_ISRESMERGE                SETBIT(9)
#define  SRI_ISRESAUDIO                SETBIT(10)
#define  SRI_ISRESCORRECTION           SETBIT(11)
#define  SRI_ISRESEVAL                 SETBIT(12)
#define  SRI_ISRESGRAPH                SETBIT(13)
#define  SRI_ISRESMEMORY               SETBIT(14)
#define  SRI_ISRESMODIFYGUI            SETBIT(15)
#define  SRI_ISRESSPEAKER              SETBIT(16)
#define  SRI_ISRSPEAKER                SETBIT(17)
#define  SRI_ISRESSCORES               SETBIT(18)


// ISRGramCommon::TrainQuery
#define   SRGRAMQ_NONE                    0
#define   SRGRAMQ_GENERALTRAIN            1
#define   SRGRAMQ_PHRASE                  2
#define   SRGRAMQ_DIALOG                  3

// ISRGramNotifySink::PhraseFinish
#define   ISRNOTEFIN_RECOGNIZED         SETBIT(0)
#define   ISRNOTEFIN_THISGRAMMAR        SETBIT(1)
#define   ISRNOTEFIN_FROMTHISGRAMMAR    SETBIT(2)

// ISRGramNotifySink::Training
#define   SRGNSTRAIN_GENERAL            SETBIT(0)
#define   SRGNSTRAIN_GRAMMAR            SETBIT(1)
#define   SRGNSTRAIN_MICROPHONE         SETBIT(2)

// ISRNotifySink::AttribChange
#define   ISRNSAC_AUTOGAINENABLE        1
#define   ISRNSAC_THRESHOLD             2
#define   ISRNSAC_ECHO                  3
#define   ISRNSAC_ENERGYFLOOR           4
#define   ISRNSAC_MICROPHONE            5
#define   ISRNSAC_REALTIME              6
#define   ISRNSAC_SPEAKER               7
#define   ISRNSAC_TIMEOUT               8

/* Interference */
#define  SRMSGINT_NOISE                (0x0001)
#define  SRMSGINT_NOSIGNAL             (0x0002)
#define  SRMSGINT_TOOLOUD              (0x0003)
#define  SRMSGINT_TOOQUIET             (0x0004)
#define  SRMSGINT_AUDIODATA_STOPPED    (0x0005)
#define  SRMSGINT_AUDIODATA_STARTED    (0x0006)
#define  SRMSGINT_IAUDIO_STARTED       (0x0007)
#define  SRMSGINT_IAUDIO_STOPPED       (0x0008)

// Gramamr header values
#define   SRHDRTYPE_CFG                  0
#define   SRHDRTYPE_LIMITEDDOMAIN        1
#define   SRHDRTYPE_DICTATION            2

#define   SRHDRFLAG_UNICODE              SETBIT(0)  

/* SRCFGSYMBOL */
#define  SRCFG_STARTOPERATION          (1)
#define  SRCFG_ENDOPERATION            (2)
#define  SRCFG_WORD                    (3)
#define  SRCFG_RULE                    (4)
#define  SRCFG_WILDCARD                (5)
#define  SRCFG_LIST                    (6)

#define  SRCFGO_SEQUENCE               (1)
#define  SRCFGO_ALTERNATIVE            (2)
#define  SRCFGO_REPEAT                 (3)
#define  SRCFGO_OPTIONAL               (4)


// Grammar-chunk IDs
#define   SRCK_LANGUAGE                  1
#define   SRCKCFG_WORDS                  2
#define   SRCKCFG_RULES                  3
#define   SRCKCFG_EXPORTRULES            4
#define   SRCKCFG_IMPORTRULES            5
#define   SRCKCFG_LISTS                  6
#define   SRCKD_TOPIC                    7
#define   SRCKD_COMMON                   8
#define   SRCKD_GROUP                    9
#define   SRCKD_SAMPLE                   10
#define   SRCKLD_WORDS                   11
#define   SRCKLD_GROUP                   12
#define   SRCKLD_SAMPLE                  13 
#define   SRCKD_WORDCOUNT                14

/* TrainQuery */
#define  SRTQEX_REQUIRED               (0x0000)
#define  SRTQEX_RECOMMENDED            (0x0001)

/* ISRResCorrection */
#define  SRCORCONFIDENCE_SOME          (0x0001)
#define  SRCORCONFIDENCE_VERY          (0x0002)

/* ISRResMemory constants */
#define  SRRESMEMKIND_AUDIO            SETBIT(0)
#define  SRRESMEMKIND_CORRECTION       SETBIT(1)
#define  SRRESMEMKIND_EVAL             SETBIT(2)
#define  SRRESMEMKIND_PHONEMEGRAPH     SETBIT(3)
#define  SRRESMEMKIND_WORDGRAPH        SETBIT(4)

// Attribute minimums and maximums
#define  SRATTR_MINAUTOGAIN               0
#define  SRATTR_MAXAUTOGAIN               100
#define  SRATTR_MINENERGYFLOOR            0
#define  SRATTR_MAXENERGYFLOOR            0xffff
#define  SRATTR_MINREALTIME               0
#define  SRATTR_MAXREALTIME               0xffffffff
#define  SRATTR_MINTHRESHOLD              0
#define  SRATTR_MAXTHRESHOLD              100
#define  SRATTR_MINTOINCOMPLETE           0
#define  SRATTR_MAXTOINCOMPLETE           0xffffffff
#define  SRATTR_MINTOCOMPLETE             0
#define  SRATTR_MAXTOCOMPLETE             0xffffffff


/************************************************************************
typedefs */

typedef struct {
   DWORD    dwSize;
   DWORD    dwUniqueID;
   BYTE     abData[0];
   } SRCFGRULE, * PSRCFGRULE;



typedef struct {
   DWORD    dwSize;
   DWORD    dwRuleNum;
   WCHAR    szString[0];
   } SRCFGIMPRULEW, * PSRCFGIMPRULEW;

typedef struct {
   DWORD    dwSize;
   DWORD    dwRuleNum;
   CHAR     szString[0];
   } SRCFGIMPRULEA, * PSRCFGIMPRULEA;

#ifdef  _S_UNICODE
#define  SRCFGIMPRULE      SRCFGIMPRULEW
#define  PSRCFGIMPRULE     PSRCFGIMPRULEW
#else
#define  SRCFGIMPRULE      SRCFGIMPRULEA
#define  PSRCFGIMPRULE     PSRCFGIMPRULEA
#endif  // _S_UNICODE



typedef struct {
   DWORD    dwSize;
   DWORD    dwRuleNum;
   WCHAR    szString[0];
   } SRCFGXRULEW, * PSRCFGXRULEW;

typedef struct {
   DWORD    dwSize;
   DWORD    dwRuleNum;
   CHAR     szString[0];
   } SRCFGXRULEA, * PSRCFGXRULEA;

#ifdef  _S_UNICODE
#define  SRCFGXRULE     SRCFGXRULEW
#define  PSRCFGXRULE    PSRCFGXRULEW
#else
#define  SRCFGXRULE     SRCFGXRULEA
#define  PSRCFGXRULE    PSRCFGXRULEA
#endif  // _S_UNICODE



typedef struct {
   DWORD    dwSize;
   DWORD    dwListNum;
   WCHAR    szString[0];
   } SRCFGLISTW, * PSRCFGLISTW;

typedef struct {
   DWORD    dwSize;
   DWORD    dwListNum;
   CHAR     szString[0];
   } SRCFGLISTA, * PSRCFGLISTA;

#ifdef  _S_UNICODE
#define  SRCFGLIST      SRCFGLISTW
#define  PSRCFGLIST     PSRCFGLISTW
#else
#define  SRCFGLIST      SRCFGLISTA
#define  PSRCFGLIST     PSRCFGLISTA
#endif  // _S_UNICODE



typedef struct {
   WORD     wType;
   WORD     wProbability;
   DWORD    dwValue;
   } SRCFGSYMBOL, * PSRCFGSYMBOL;



typedef struct {
   DWORD    dwSize;
   DWORD    dwWordNum;
   WCHAR    szWord[0];
   } SRWORDW, * PSRWORDW;

typedef struct {
   DWORD    dwSize;
   DWORD    dwWordNum;
   CHAR     szWord[0];
   } SRWORDA, * PSRWORDA;

#ifdef  _S_UNICODE
#define  SRWORD      SRWORDW
#define  PSRWORD     PSRWORDW
#else
#define  SRWORD      SRWORDA
#define  PSRWORD     PSRWORDA
#endif  // _S_UNICODE



typedef struct {
   DWORD    dwSize;
   BYTE     abWords[0];
   } SRPHRASEW, * PSRPHRASEW;

typedef struct {
   DWORD    dwSize;
   BYTE     abWords[0];
   } SRPHRASEA, * PSRPHRASEA;

#ifdef  _S_UNICODE
#define  SRPHRASE    SRPHRASEW
#define  PSRPHRASE   PSRPHRASEW
#else
#define  SRPHRASE    SRPHRASEA
#define  PSRPHRASE   PSRPHRASEA
#endif  // _S_UNICODE



typedef struct {
   DWORD      dwType;
   DWORD      dwFlags;
   } SRHEADER, *PSRHEADER;

typedef struct {
   DWORD      dwChunkID;
   DWORD      dwChunkSize;
   BYTE       avInfo[0];
   } SRCHUNK, *PSRCHUNK;



typedef struct {
   GUID       gEngineID;
   WCHAR      szMfgName[SRMI_NAMELEN];
   WCHAR      szProductName[SRMI_NAMELEN];
   GUID       gModeID;
   WCHAR      szModeName[SRMI_NAMELEN];
   LANGUAGEW  language;
   DWORD      dwSequencing;
   DWORD      dwMaxWordsVocab;
   DWORD      dwMaxWordsState;
   DWORD      dwGrammars;
   DWORD      dwFeatures;
   DWORD      dwInterfaces;
   DWORD      dwEngineFeatures;
   } SRMODEINFOW, * PSRMODEINFOW;

typedef struct {
   GUID       gEngineID;
   CHAR       szMfgName[SRMI_NAMELEN];
   CHAR       szProductName[SRMI_NAMELEN];
   GUID       gModeID;
   CHAR       szModeName[SRMI_NAMELEN];
   LANGUAGEA  language;
   DWORD      dwSequencing;
   DWORD      dwMaxWordsVocab;
   DWORD      dwMaxWordsState;
   DWORD      dwGrammars;
   DWORD      dwFeatures;
   DWORD      dwInterfaces;
   DWORD      dwEngineFeatures;
   } SRMODEINFOA, * PSRMODEINFOA;

#ifdef  _S_UNICODE
#define  SRMODEINFO     SRMODEINFOW
#define  PSRMODEINFO    PSRMODEINFOW
#else
#define  SRMODEINFO     SRMODEINFOA
#define  PSRMODEINFO    PSRMODEINFOA
#endif  // _S_UNICODE



typedef struct {
   DWORD      dwEngineID;
   DWORD      dwMfgName;
   DWORD      dwProductName;
   DWORD      dwModeID;
   DWORD      dwModeName;
   DWORD      dwLanguage;
   DWORD      dwDialect;
   DWORD      dwSequencing;
   DWORD      dwMaxWordsVocab;
   DWORD      dwMaxWordsState;
   DWORD      dwGrammars;
   DWORD      dwFeatures;
   DWORD      dwInterfaces;
   DWORD      dwEngineFeatures;
   } SRMODEINFORANK, * PSRMODEINFORANK;



// speech recognition enumeration sharing object
typedef struct {
   QWORD        qwInstanceID;
   DWORD        dwDeviceID;
   SRMODEINFOW  srModeInfo;
} SRSHAREW, * PSRSHAREW;

typedef struct {
   QWORD        qwInstanceID;
   DWORD        dwDeviceID;
   SRMODEINFOA  srModeInfo;
} SRSHAREA, * PSRSHAREA;

#ifdef  _S_UNICODE
#define  SRSHARE    SRSHAREW
#define  PSRSHARE   PSRSHAREW
#else
#define  SRSHARE    SRSHAREA
#define  PSRSHARE   PSRSHAREA
#endif  // _S_UNICODE




// ISRCentral::GrammarLoad
typedef enum {
   SRGRMFMT_CFG = 0x0000,
   SRGRMFMT_LIMITEDDOMAIN = 0x0001,
   SRGRMFMT_DICTATION = 0x0002,
   SRGRMFMT_CFGNATIVE = 0x8000,
   SRGRMFMT_LIMITEDDOMAINNATIVE = 0x8001,
   SRGRMFMT_DICTATIONNATIVE = 0x8002,
   } SRGRMFMT, * PSRGRMFMT;

/************************************************************************
Class IDs */


// {E02D16C0-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(CLSID_SREnumerator, 
0xe02d16c0, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);


/************************************************************************
interfaces */


/*
 * ISRAttributes
 */

#undef   INTERFACE
#define  INTERFACE   ISRAttributesW

DEFINE_GUID(IID_ISRAttributesW, 0x68A33AA0L, 0x44CD, 0x101B, 0x90, 0xA8, 0x00, 0xAA, 0x00, 0x3E, 0x4B, 0x50);

DECLARE_INTERFACE_ (ISRAttributesW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRAttributesW members
   STDMETHOD (AutoGainEnableGet) (THIS_ DWORD *) PURE;
   STDMETHOD (AutoGainEnableSet) (THIS_ DWORD) PURE;
   STDMETHOD (EchoGet)           (THIS_ BOOL *) PURE;
   STDMETHOD (EchoSet)           (THIS_ BOOL) PURE;
   STDMETHOD (EnergyFloorGet)    (THIS_ WORD *) PURE;
   STDMETHOD (EnergyFloorSet)    (THIS_ WORD) PURE;
   STDMETHOD (MicrophoneGet)     (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (MicrophoneSet)     (THIS_ PCWSTR) PURE;
   STDMETHOD (RealTimeGet)       (THIS_ DWORD *) PURE;
   STDMETHOD (RealTimeSet)       (THIS_ DWORD) PURE;
   STDMETHOD (SpeakerGet)        (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (SpeakerSet)        (THIS_ PCWSTR) PURE;
   STDMETHOD (TimeOutGet)        (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD (TimeOutSet)        (THIS_ DWORD, DWORD) PURE;
   STDMETHOD (ThresholdGet)      (THIS_ DWORD *) PURE;
   STDMETHOD (ThresholdSet)      (THIS_ DWORD) PURE;
   };

typedef ISRAttributesW FAR * PISRATTRIBUTESW;


#undef   INTERFACE
#define  INTERFACE   ISRAttributesA

DEFINE_GUID(IID_ISRAttributesA, 0x2F26B9C1L, 0xDB31, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRAttributesA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRAttributesA members
   STDMETHOD (AutoGainEnableGet) (THIS_ DWORD *) PURE;
   STDMETHOD (AutoGainEnableSet) (THIS_ DWORD) PURE;
   STDMETHOD (EchoGet)           (THIS_ BOOL *) PURE;
   STDMETHOD (EchoSet)           (THIS_ BOOL) PURE;
   STDMETHOD (EnergyFloorGet)    (THIS_ WORD *) PURE;
   STDMETHOD (EnergyFloorSet)    (THIS_ WORD) PURE;
   STDMETHOD (MicrophoneGet)     (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD (MicrophoneSet)     (THIS_ PCSTR) PURE;
   STDMETHOD (RealTimeGet)       (THIS_ DWORD *) PURE;
   STDMETHOD (RealTimeSet)       (THIS_ DWORD) PURE;
   STDMETHOD (SpeakerGet)        (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD (SpeakerSet)        (THIS_ PCSTR) PURE;
   STDMETHOD (TimeOutGet)        (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD (TimeOutSet)        (THIS_ DWORD, DWORD) PURE;
   STDMETHOD (ThresholdGet)      (THIS_ DWORD *) PURE;
   STDMETHOD (ThresholdSet)      (THIS_ DWORD) PURE;
   };

typedef ISRAttributesA FAR * PISRATTRIBUTESA;


#ifdef _S_UNICODE
 #define ISRAttributes        ISRAttributesW
 #define IID_ISRAttributes    IID_ISRAttributesW
 #define PISRATTRIBUTES       PISRATTRIBUTESW

#else
 #define ISRAttributes        ISRAttributesA
 #define IID_ISRAttributes    IID_ISRAttributesA
 #define PISRATTRIBUTES       PISRATTRIBUTESA

#endif // _S_UNICODE



/*
 * ISRCentral
 */

#undef   INTERFACE
#define  INTERFACE   ISRCentralW

DEFINE_GUID(IID_ISRCentralW, 0xB9BD3860L, 0x44DB, 0x101B, 0x90, 0xA8, 0x00, 0xAA, 0x00, 0x3E, 0x4B, 0x50);

DECLARE_INTERFACE_ (ISRCentralW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRCentralW members
   STDMETHOD (ModeGet)        (THIS_ PSRMODEINFOW) PURE;
   STDMETHOD (GrammarLoad)    (THIS_ SRGRMFMT, SDATA, PVOID, IID, LPUNKNOWN *) PURE;
   STDMETHOD (Pause)          (THIS) PURE;
   STDMETHOD (PosnGet)        (THIS_ PQWORD) PURE;
   STDMETHOD (Resume)         (THIS) PURE;
   STDMETHOD (ToFileTime)     (THIS_ PQWORD, FILETIME *) PURE;
   STDMETHOD (Register)       (THIS_ PVOID, IID, DWORD*) PURE;
   STDMETHOD (UnRegister)     (THIS_ DWORD) PURE;
   };

typedef ISRCentralW FAR * PISRCENTRALW;


#undef   INTERFACE
#define  INTERFACE   ISRCentralA

DEFINE_GUID(IID_ISRCentralA, 0x2F26B9C2L, 0xDB31, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRCentralA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRCentralA members
   STDMETHOD (ModeGet)        (THIS_ PSRMODEINFOA) PURE;
   STDMETHOD (GrammarLoad)    (THIS_ SRGRMFMT, SDATA, PVOID, IID, LPUNKNOWN *) PURE;
   STDMETHOD (Pause)          (THIS) PURE;
   STDMETHOD (PosnGet)        (THIS_ PQWORD) PURE;
   STDMETHOD (Resume)         (THIS) PURE;
   STDMETHOD (ToFileTime)     (THIS_ PQWORD, FILETIME *) PURE;
   STDMETHOD (Register)       (THIS_ PVOID, IID, DWORD*) PURE;
   STDMETHOD (UnRegister)     (THIS_ DWORD) PURE;
   };

typedef ISRCentralA FAR * PISRCENTRALA;


#ifdef _S_UNICODE
 #define ISRCentral           ISRCentralW
 #define IID_ISRCentral       IID_ISRCentralW
 #define PISRCENTRAL          PISRCENTRALW

#else
 #define ISRCentral           ISRCentralA
 #define IID_ISRCentral       IID_ISRCentralA
 #define PISRCENTRAL          PISRCENTRALA

#endif   // _S_UNICODE



/*
 * ISRDialogs
 */

#undef   INTERFACE
#define  INTERFACE   ISRDialogsW

DEFINE_GUID(IID_ISRDialogsW, 0xBCFB4C60L, 0x44DB, 0x101B, 0x90, 0xA8, 0x00, 0xAA, 0x00, 0x3E, 0x4B, 0x50);

DECLARE_INTERFACE_ (ISRDialogsW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRDialogsW members
   STDMETHOD (AboutDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (GeneralDlg)     (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (LexiconDlg)     (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainMicDlg)    (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainGeneralDlg)(THIS_ HWND, PCWSTR) PURE;
   };

typedef ISRDialogsW FAR * PISRDIALOGSW;


#undef   INTERFACE
#define  INTERFACE   ISRDialogsA

DEFINE_GUID(IID_ISRDialogsA, 0x05EB6C60L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRDialogsA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRDialogsA members
   STDMETHOD (AboutDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (GeneralDlg)     (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (LexiconDlg)     (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainMicDlg)    (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainGeneralDlg)(THIS_ HWND, PCSTR) PURE;
   };

typedef ISRDialogsA FAR * PISRDIALOGSA;


#ifdef _S_UNICODE
 #define ISRDialogs        ISRDialogsW
 #define IID_ISRDialogs    IID_ISRDialogsW
 #define PISRDIALOGS       PISRDIALOGSW

#else
 #define ISRDialogs        ISRDialogsA
 #define IID_ISRDialogs    IID_ISRDialogsA
 #define PISRDIALOGS       PISRDIALOGSA

#endif



/*
 *  ISREnum
 */

#undef   INTERFACE
#define  INTERFACE   ISREnumW

DEFINE_GUID(IID_ISREnumW, 0xBFA9F1A0L, 0x44DB, 0x101B, 0x90, 0xA8, 0x00, 0xAA, 0x00, 0x3E, 0x4B, 0x50);

DECLARE_INTERFACE_ (ISREnumW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISREnumW members
   STDMETHOD (Next)           (THIS_ ULONG, PSRMODEINFOW, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ ISREnumW * FAR *) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PISRCENTRALW *, LPUNKNOWN) PURE;
   };

typedef ISREnumW FAR * PISRENUMW;


#undef   INTERFACE
#define  INTERFACE   ISREnumA

DEFINE_GUID(IID_ISREnumA, 0x05EB6C61L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISREnumA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISREnumA members
   STDMETHOD (Next)           (THIS_ ULONG, PSRMODEINFOA, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ ISREnumA * FAR *) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PISRCENTRALA *, LPUNKNOWN) PURE;
   };

typedef ISREnumA FAR * PISRENUMA;


#ifdef _S_UNICODE
 #define ISREnum           ISREnumW
 #define IID_ISREnum       IID_ISREnumW
 #define PISRENUM          PISRENUMW

#else
 #define ISREnum           ISREnumA
 #define IID_ISREnum       IID_ISREnumA
 #define PISRENUM          PISRENUMA

#endif



/*
 * ISRFind
 */

#undef   INTERFACE
#define  INTERFACE   ISRFindW

DEFINE_GUID(IID_ISRFindW, 0xC2835060L, 0x44DB, 0x101B, 0x90, 0xA8, 0x00, 0xAA, 0x00, 0x3E, 0x4B, 0x50);

DECLARE_INTERFACE_ (ISRFindW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRFindW members
   STDMETHOD (Find)           (THIS_ PSRMODEINFOW, PSRMODEINFORANK, PSRMODEINFOW) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PISRCENTRALW *, LPUNKNOWN) PURE;
   };

typedef ISRFindW FAR * PISRFINDW;


#undef   INTERFACE
#define  INTERFACE   ISRFindA

DEFINE_GUID(IID_ISRFindA, 0x05EB6C62L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRFindA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRFindA members
   STDMETHOD (Find)           (THIS_ PSRMODEINFOA, PSRMODEINFORANK, PSRMODEINFOA) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PISRCENTRALA *, LPUNKNOWN) PURE;
   };

typedef ISRFindA FAR * PISRFINDA;


#ifdef _S_UNICODE
 #define ISRFind           ISRFindW
 #define IID_ISRFind       IID_ISRFindW
 #define PISRFIND          PISRFINDW

#else
 #define ISRFind           ISRFindA
 #define IID_ISRFind       IID_ISRFindA
 #define PISRFIND          PISRFINDA

#endif



/*
 * ISRGramCommon
 */

#undef   INTERFACE
#define  INTERFACE   ISRGramCommonW

DEFINE_GUID(IID_ISRGramCommonW, 0xe8c3e160, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (ISRGramCommonW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // ISRGramCommonW members
   STDMETHOD (Activate)         (THIS_ HWND, BOOL, PCWSTR) PURE;
   STDMETHOD (Archive)          (THIS_ BOOL, PVOID, DWORD, DWORD *) PURE;
   STDMETHOD (BookMark)         (THIS_ QWORD, DWORD) PURE;
   STDMETHOD (Deactivate)       (THIS_ PCWSTR) PURE;
   STDMETHOD (DeteriorationGet) (THIS_ DWORD *, DWORD *, DWORD *) PURE;
   STDMETHOD (DeteriorationSet) (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (TrainDlg)         (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainPhrase)      (THIS_ DWORD, PSDATA) PURE;
   STDMETHOD (TrainQuery)       (THIS_ DWORD *) PURE;
   };

typedef ISRGramCommonW FAR * PISRGRAMCOMMONW;


#undef   INTERFACE
#define  INTERFACE   ISRGramCommonA

DEFINE_GUID(IID_ISRGramCommonA, 0x05EB6C63L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRGramCommonA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // ISRGramCommonA members
   STDMETHOD (Activate)         (THIS_ HWND, BOOL, PCSTR) PURE;
   STDMETHOD (Archive)          (THIS_ BOOL, PVOID, DWORD, DWORD *) PURE;
   STDMETHOD (BookMark)         (THIS_ QWORD, DWORD) PURE;
   STDMETHOD (Deactivate)       (THIS_ PCSTR) PURE;
   STDMETHOD (DeteriorationGet) (THIS_ DWORD *, DWORD *, DWORD *) PURE;
   STDMETHOD (DeteriorationSet) (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (TrainDlg)         (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainPhrase)      (THIS_ DWORD, PSDATA) PURE;
   STDMETHOD (TrainQuery)       (THIS_ DWORD *) PURE;
   };

typedef ISRGramCommonA FAR * PISRGRAMCOMMONA;


#ifdef _S_UNICODE
 #define ISRGramCommon        ISRGramCommonW
 #define IID_ISRGramCommon    IID_ISRGramCommonW
 #define PISRGRAMCOMMON       PISRGRAMCOMMONW

#else
 #define ISRGramCommon        ISRGramCommonA
 #define IID_ISRGramCommon    IID_ISRGramCommonA
 #define PISRGRAMCOMMON       PISRGRAMCOMMONA

#endif



/*
 * ISRGramCFG
 */

#undef   INTERFACE
#define  INTERFACE   ISRGramCFGW

DEFINE_GUID(IID_ISRGramCFGW, 0xecc0b180, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (ISRGramCFGW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramCFGW members
   STDMETHOD (LinkQuery)      (THIS_ PCWSTR, BOOL *) PURE;
   STDMETHOD (ListAppend)     (THIS_ PCWSTR, SDATA) PURE;
   STDMETHOD (ListGet)        (THIS_ PCWSTR, PSDATA) PURE;
   STDMETHOD (ListRemove)     (THIS_ PCWSTR, SDATA) PURE;
   STDMETHOD (ListSet)        (THIS_ PCWSTR, SDATA) PURE;
   STDMETHOD (ListQuery)      (THIS_ PCWSTR, BOOL *) PURE;
   };

typedef ISRGramCFGW FAR * PISRGRAMCFGW;


#undef   INTERFACE
#define  INTERFACE   ISRGramCFGA

DEFINE_GUID(IID_ISRGramCFGA, 0x05EB6C64L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRGramCFGA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramCFGA members
   STDMETHOD (LinkQuery)      (THIS_ PCSTR, BOOL *) PURE;
   STDMETHOD (ListAppend)     (THIS_ PCSTR, SDATA) PURE;
   STDMETHOD (ListGet)        (THIS_ PCSTR, PSDATA) PURE;
   STDMETHOD (ListRemove)     (THIS_ PCSTR, SDATA) PURE;
   STDMETHOD (ListSet)        (THIS_ PCSTR, SDATA) PURE;
   STDMETHOD (ListQuery)      (THIS_ PCSTR, BOOL *) PURE;
   };

typedef ISRGramCFGA FAR * PISRGRAMCFGA;


#ifdef _S_UNICODE
 #define ISRGramCFG        ISRGramCFGW
 #define IID_ISRGramCFG    IID_ISRGramCFGW
 #define PISRGRAMCFG       PISRGRAMCFGW

#else
 #define ISRGramCFG        ISRGramCFGA
 #define IID_ISRGramCFG    IID_ISRGramCFGA
 #define PISRGRAMCFG       PISRGRAMCFGA

#endif



/* 
 * ISRGramDictation
 */

#undef   INTERFACE
#define  INTERFACE   ISRGramDictationW

DEFINE_GUID(IID_ISRGramDictationW, 0x090CD9A3, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRGramDictationW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramDictationW members
   STDMETHOD (Context)        (THIS_ PCWSTR, PCWSTR) PURE;
   STDMETHOD (Hint)           (THIS_ PCWSTR) PURE;
   STDMETHOD (Words)          (THIS_ PCWSTR) PURE;
   };

typedef ISRGramDictationW FAR *PISRGRAMDICTATIONW;


#undef   INTERFACE
#define  INTERFACE   ISRGramDictationA

DEFINE_GUID(IID_ISRGramDictationA, 0x05EB6C65L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRGramDictationA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramDictationA members
   STDMETHOD (Context)        (THIS_ PCSTR, PCSTR) PURE;
   STDMETHOD (Hint)           (THIS_ PCSTR) PURE;
   STDMETHOD (Words)          (THIS_ PCSTR) PURE;
   };

typedef ISRGramDictationA FAR *PISRGRAMDICTATIONA;


#ifdef _S_UNICODE
 #define ISRGramDictation        ISRGramDictationW
 #define IID_ISRGramDictation    IID_ISRGramDictationW
 #define PISRGRAMDICTATION       PISRGRAMDICTATIONW

#else
 #define ISRGramDictation        ISRGramDictationA
 #define IID_ISRGramDictation    IID_ISRGramDictationA
 #define PISRGRAMDICTATION       PISRGRAMDICTATIONA

#endif



// ISRGramInsertionGUI
// This does not need an ANSI/UNICODE interface because no characters are passed
#undef   INTERFACE
#define  INTERFACE   ISRGramInsertionGUI

// {090CD9A4-DA1A-11CD-B3CA-00AA0047BA4F}
DEFINE_GUID(IID_ISRGramInsertionGUI,
0x090CD9A4, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRGramInsertionGUI, IUnknown) {
   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramInsertionGUI members
   STDMETHOD (Hide)           (THIS) PURE;
   STDMETHOD (Move)           (THIS_ RECT) PURE;
   STDMETHOD (Show)           (THIS_ HWND) PURE;
   };

typedef ISRGramInsertionGUI FAR *PISRGRAMINSERTIONGUI;



/*
 * ISRGramNotifySink
 */

#undef   INTERFACE
#define  INTERFACE   ISRGramNotifySinkW

DEFINE_GUID(IID_ISRGramNotifySinkW,  0xf106bfa0, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

// {B1AAC561-75D6-11cf-8D15-00A0C9034A7E}
DEFINE_GUID(IID_ISRGramNotifySinkMW, 0xb1aac561, 0x75d6, 0x11cf, 0x8d, 0x15, 0x0, 0xa0, 0xc9, 0x3, 0x4a, 0x7e);

DECLARE_INTERFACE_ (ISRGramNotifySinkW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramNotifySinkW members
   STDMETHOD (BookMark)       (THIS_ DWORD) PURE;
   STDMETHOD (Paused)         (THIS) PURE;
   STDMETHOD (PhraseFinish)   (THIS_ DWORD, QWORD, QWORD, PSRPHRASEW, LPUNKNOWN) PURE;
   STDMETHOD (PhraseHypothesis)(THIS_ DWORD, QWORD, QWORD, PSRPHRASEW, LPUNKNOWN) PURE;
   STDMETHOD (PhraseStart)    (THIS_ QWORD) PURE;
   STDMETHOD (ReEvaluate)     (THIS_ LPUNKNOWN) PURE;
   STDMETHOD (Training)       (THIS_ DWORD) PURE;
   STDMETHOD (UnArchive)      (THIS_ LPUNKNOWN) PURE;
   };

typedef ISRGramNotifySinkW FAR * PISRGRAMNOTIFYSINKW;


// ISRGramNotifySinkA
#undef   INTERFACE
#define  INTERFACE   ISRGramNotifySinkA

// {EFEEA350-CE5E-11cd-9D96-00AA002FC7C9}
DEFINE_GUID(IID_ISRGramNotifySinkA, 
0xefeea350, 0xce5e, 0x11cd, 0x9d, 0x96, 0x0, 0xaa, 0x0, 0x2f, 0xc7, 0xc9);

// {B1AAC562-75D6-11cf-8D15-00A0C9034A7E}
DEFINE_GUID(IID_ISRGramNotifySinkMA, 
0xb1aac562, 0x75d6, 0x11cf, 0x8d, 0x15, 0x0, 0xa0, 0xc9, 0x3, 0x4a, 0x7e);

DECLARE_INTERFACE_ (ISRGramNotifySinkA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRGramNotifySinkA members
   STDMETHOD (BookMark)       (THIS_ DWORD) PURE;
   STDMETHOD (Paused)         (THIS) PURE;
   STDMETHOD (PhraseFinish)   (THIS_ DWORD, QWORD, QWORD, PSRPHRASEA, LPUNKNOWN) PURE;
   STDMETHOD (PhraseHypothesis)(THIS_ DWORD, QWORD, QWORD, PSRPHRASEA, LPUNKNOWN) PURE;
   STDMETHOD (PhraseStart)    (THIS_ QWORD) PURE;
   STDMETHOD (ReEvaluate)     (THIS_ LPUNKNOWN) PURE;
   STDMETHOD (Training)       (THIS_ DWORD) PURE;
   STDMETHOD (UnArchive)      (THIS_ LPUNKNOWN) PURE;
   };

typedef ISRGramNotifySinkA FAR * PISRGRAMNOTIFYSINKA;


#ifdef _S_UNICODE
 #define ISRGramNotifySink       ISRGramNotifySinkW
 #define IID_ISRGramNotifySink   IID_ISRGramNotifySinkW
 #define IID_ISRGramNotifySinkM  IID_ISRGramNotifySinkMW
 #define PISRGRAMNOTIFYSINK      PISRGRAMNOTIFYSINKW

#else
 #define ISRGramNotifySink       ISRGramNotifySinkA
 #define IID_ISRGramNotifySink   IID_ISRGramNotifySinkA
 #define IID_ISRGramNotifySinkM  IID_ISRGramNotifySinkMA
 #define PISRGRAMNOTIFYSINK      PISRGRAMNOTIFYSINKA

#endif   // _S_UNICODE



// ISRNotifySink
// This does not need an ANSI/UNICODE interface because no characters are passed
#undef   INTERFACE
#define  INTERFACE   ISRNotifySink

DEFINE_GUID(IID_ISRNotifySink,
0x090CD9B0L, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRNotifySink, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRNotifySink members
   STDMETHOD (AttribChanged)  (THIS_ DWORD) PURE;
   STDMETHOD (Interference)   (THIS_ QWORD, QWORD, DWORD) PURE;
   STDMETHOD (Sound)          (THIS_ QWORD, QWORD) PURE;
   STDMETHOD (UtteranceBegin) (THIS_ QWORD) PURE;
   STDMETHOD (UtteranceEnd)   (THIS_ QWORD, QWORD) PURE;
   STDMETHOD (VUMeter)        (THIS_ QWORD, WORD) PURE;
   };

typedef ISRNotifySink FAR *PISRNOTIFYSINK;

// Just in case anyone uses the wide/ansi versions
#define ISRNotifySinkW       ISRNotifySink
#define IID_ISRNotifySinkW   IID_ISRNotifySink
#define PISRNOTIFYSINKW      PISRNOTIFYSINK
#define ISRNotifySinkA       ISRNotifySink
#define IID_ISRNotifySinkA   IID_ISRNotifySink
#define PISRNOTIFYSINKA      PISRNOTIFYSINK


/*
 * ISRResBasic
 */

#undef   INTERFACE
#define  INTERFACE   ISRResBasicW

DEFINE_GUID(IID_ISRResBasicW, 0x090CD9A5, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResBasicW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResBasicW members
   STDMETHOD (PhraseGet)      (THIS_ DWORD, PSRPHRASEW, DWORD,  DWORD *) PURE;
   STDMETHOD (Identify)       (THIS_ GUID *) PURE;
   STDMETHOD (TimeGet)        (THIS_ PQWORD, PQWORD) PURE;
   STDMETHOD (FlagsGet)       (THIS_ DWORD, DWORD *) PURE;
   };

typedef ISRResBasicW FAR *PISRRESBASICW;


#undef   INTERFACE
#define  INTERFACE   ISRResBasicA

DEFINE_GUID(IID_ISRResBasicA, 0x05EB6C66L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResBasicA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResBasicA members
   STDMETHOD (PhraseGet)      (THIS_ DWORD, PSRPHRASEA, DWORD,  DWORD *) PURE;
   STDMETHOD (Identify)       (THIS_ GUID *) PURE;
   STDMETHOD (TimeGet)        (THIS_ PQWORD, PQWORD) PURE;
   STDMETHOD (FlagsGet)       (THIS_ DWORD, DWORD *) PURE;
   };

typedef ISRResBasicA FAR *PISRRESBASICA;


#ifdef _S_UNICODE
 #define ISRResBasic             ISRResBasicW
 #define IID_ISRResBasic         IID_ISRResBasicW
 #define PISRRESBASIC            PISRRESBASICW

#else
 #define ISRResBasic             ISRResBasicA
 #define IID_ISRResBasic         IID_ISRResBasicA
 #define PISRRESBASIC            PISRRESBASICA

#endif   // _S_UNICODE


/*
 * ISRResScore
 * This does not need an ANSI/UNICODE interface because no characters are passed
 */

#undef INTERFACE
#define INTERFACE       ISRResScores


// {0B37F1E0-B8DE-11cf-B22E-00AA00A215ED}
DEFINE_GUID(IID_ISRResScores, 0xb37f1e0, 0xb8de, 0x11cf, 0xb2, 0x2e, 0x0, 0xaa, 0x0, 0xa2, 0x15, 0xed);

DECLARE_INTERFACE_ (ISRResScores, IUnknown) {

	// IUnknown members
	STDMETHOD (QueryInterface)      (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
	STDMETHOD_(ULONG,Release)  (THIS) PURE;

	// ISRResScores members
	STDMETHOD (GetPhraseScore) (THIS_ DWORD, long FAR *) PURE;
	STDMETHOD (GetWordScores)  (THIS_ DWORD, long FAR *, DWORD, LPDWORD) PURE;
};

typedef ISRResScores FAR* PISRRESSCORES;

// In case someone uses the A/W versions...

#define ISRResScoresW           ISRResScores
#define IID_ISRResScoresW       IID_ISRResScores
#define PISRRESSCORESW          PISRRESSCORES
#define ISRResScoresA           ISRResScores
#define IID_ISRResScoresA       IID_ISRResScores
#define PISRRESSCORESA          PISRRESSCORES



/*
 * ISRResMerge
 * This does not need an ANSI/UNICODE interface because no characters are passed
 */

#undef   INTERFACE
#define  INTERFACE   ISRResMerge

DEFINE_GUID(IID_ISRResMerge, 0x090CD9A6, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResMerge, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResMerge members
   STDMETHOD (Merge)          (THIS_ LPUNKNOWN, PIUNKNOWN ) PURE;
   STDMETHOD (Split)          (THIS_ QWORD, PIUNKNOWN , PIUNKNOWN ) PURE;
   };

typedef ISRResMerge FAR *PISRRESMERGE;



/*
 * ISRResAudio
 * This does not need an ANSI/UNICODE interface because no characters are passed
 */

#undef   INTERFACE
#define  INTERFACE   ISRResAudio

DEFINE_GUID(IID_ISRResAudio, 0x090CD9A7, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResAudio, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResAudio members
   STDMETHOD (GetWAV)         (THIS_ PSDATA) PURE;
   };

typedef ISRResAudio FAR *PISRRESAUDIO;



/*
 * ISRResCorrection
 */

#undef   INTERFACE
#define  INTERFACE   ISRResCorrectionW

DEFINE_GUID(IID_ISRResCorrectionW, 0x090CD9A8L, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResCorrectionW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResCorrectionW members
   STDMETHOD (Correction)         (THIS_ PSRPHRASEW, WORD) PURE;
   STDMETHOD (Validate)           (THIS_ WORD) PURE;
   };

typedef ISRResCorrectionW FAR *PISRRESCORRECTIONW;


#undef   INTERFACE
#define  INTERFACE   ISRResCorrectionA

DEFINE_GUID(IID_ISRResCorrectionA, 0x05EB6C67L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResCorrectionA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResCorrectionA members
   STDMETHOD (Correction)         (THIS_ PSRPHRASEA, WORD) PURE;
   STDMETHOD (Validate)           (THIS_ WORD) PURE;
   };

typedef ISRResCorrectionA FAR *PISRRESCORRECTIONA;


#ifdef _S_UNICODE
 #define ISRResCorrection        ISRResCorrectionW
 #define IID_ISRResCorrection    IID_ISRResCorrectionW
 #define PISRRESCORRECTION       PISRRESCORRECTIONW

#else
 #define ISRResCorrection        ISRResCorrectionA
 #define IID_ISRResCorrection    IID_ISRResCorrectionA
 #define PISRRESCORRECTION       PISRRESCORRECTIONA

#endif   // _S_UNICODE



// ISRResEval
// This does not need an ANSI/UNICODE interface because no characters are passed
#undef   INTERFACE
#define  INTERFACE   ISRResEval

// {90CD9A9-DA1A-11CD-B3CA-00AA0047BA4F}
DEFINE_GUID(IID_ISRResEval,
0x090CD9A9, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResEval, IUnknown) {
   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   //  SRResEval members
   STDMETHOD (ReEvaluate)     (THIS_ BOOL *) PURE;
   };

typedef ISRResEval FAR *PISRRESEVAL;



/*
 * ISRResGraph
 */

#undef   INTERFACE
#define  INTERFACE ISRResGraphW

DEFINE_GUID(IID_ISRResGraphW, 0x090CD9AA, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResGraphW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface)     (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)       (THIS) PURE;
   STDMETHOD_(ULONG,Release)      (THIS) PURE;

   // ISRResGraphW members         
   STDMETHOD (BestPathPhoneme)    (THIS_ DWORD, DWORD *, DWORD, DWORD *) PURE;
   STDMETHOD (BestPathWord)       (THIS_ DWORD, DWORD *, DWORD, DWORD *) PURE;
   STDMETHOD (GetPhonemeNode)     (THIS_ DWORD, PSRRESPHONEMENODE, PWCHAR, 
				   PWCHAR) PURE;
   STDMETHOD (GetWordNode)        (THIS_ DWORD, PSRRESWORDNODE, PSRWORDW, DWORD, 
				   DWORD *) PURE;
   STDMETHOD (PathScorePhoneme)   (THIS_ DWORD *, DWORD, LONG *) PURE;
   STDMETHOD (PathScoreWord)      (THIS_ DWORD *, DWORD, LONG *) PURE;
   };

typedef ISRResGraphW FAR *PISRRESGRAPHW;


#undef   INTERFACE
#define  INTERFACE ISRResGraphA

DEFINE_GUID(IID_ISRResGraphA, 0x05EB6C68L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResGraphA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface)     (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)       (THIS) PURE;
   STDMETHOD_(ULONG,Release)      (THIS) PURE;

   // ISRResGraphA members         
   STDMETHOD (BestPathPhoneme)    (THIS_ DWORD, DWORD *, DWORD, DWORD *) PURE;
   STDMETHOD (BestPathWord)       (THIS_ DWORD, DWORD *, DWORD, DWORD *) PURE;
   STDMETHOD (GetPhonemeNode)     (THIS_ DWORD, PSRRESPHONEMENODE, PWCHAR, 
				   PCHAR) PURE;
   STDMETHOD (GetWordNode)        (THIS_ DWORD, PSRRESWORDNODE, PSRWORDA, DWORD, 
				   DWORD *) PURE;
   STDMETHOD (PathScorePhoneme)   (THIS_ DWORD *, DWORD, LONG *) PURE;
   STDMETHOD (PathScoreWord)      (THIS_ DWORD *, DWORD, LONG *) PURE;
   };

typedef ISRResGraphA FAR *PISRRESGRAPHA;


#ifdef _S_UNICODE
 #define ISRResGraph             ISRResGraphW
 #define IID_ISRResGraph         IID_ISRResGraphW
 #define PISRRESGRAPH            PISRRESGRAPHW

#else
 #define ISRResGraph             ISRResGraphA
 #define IID_ISRResGraph         IID_ISRResGraphA
 #define PISRRESGRAPH            PISRRESGRAPHA

#endif   // _S_UNICODE



// ISRResMemory
// This does not need an ANSI/UNICODE interface because no characters are passed
#undef   INTERFACE
#define  INTERFACE   ISRResMemory

DEFINE_GUID(IID_ISRResMemory, 0x090CD9AB, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResMemory, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRResMemory members
   STDMETHOD (Free)           (THIS_ DWORD) PURE;
   STDMETHOD (Get)            (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD (LockGet)        (THIS_ BOOL *) PURE;
   STDMETHOD (LockSet)        (THIS_ BOOL) PURE;
   };

typedef ISRResMemory FAR *PISRRESMEMORY;



// ISRResModifyGUI
// This does not need an ANSI/UNICODE interface because no characters are passed
#undef   INTERFACE
#define  INTERFACE   ISRResModifyGUI

DEFINE_GUID(IID_ISRResModifyGUI, 0x090CD9AC, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResModifyGUI, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)      (THIS) PURE;
   STDMETHOD_(ULONG,Release)     (THIS) PURE;

   // ISRResModifyGUI members
   STDMETHOD (Hide)              (THIS) PURE;
   STDMETHOD (Move)              (THIS_ RECT *) PURE;
   STDMETHOD (Show)              (THIS_ HWND) PURE;
   };

typedef ISRResModifyGUI FAR *PISRRESMODIFYGUI;



/*
 * ISRResSpeakerW
 */

#undef   INTERFACE
#define  INTERFACE   ISRResSpeakerW

DEFINE_GUID(IID_ISRResSpeakerW, 0x090CD9AD, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResSpeakerW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)      (THIS) PURE;
   STDMETHOD_(ULONG,Release)     (THIS) PURE;

   // ISRResSpeakerW members
   STDMETHOD (Correction)        (THIS_ PCWSTR, WORD) PURE;
   STDMETHOD (Validate)          (THIS_ WORD) PURE;
   STDMETHOD (Identify)          (THIS_ DWORD, PWSTR, DWORD, DWORD *, 
				  LONG *) PURE;
   STDMETHOD (IdentifyForFree)   (THIS_ BOOL *) PURE;
   };

typedef ISRResSpeakerW FAR *PISRRESSPEAKERW;


#undef   INTERFACE
#define  INTERFACE   ISRResSpeakerA

DEFINE_GUID(IID_ISRResSpeakerA, 0x05EB6C69L, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRResSpeakerA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)      (THIS) PURE;
   STDMETHOD_(ULONG,Release)     (THIS) PURE;

   // ISRResSpeakerA members
   STDMETHOD (Correction)        (THIS_ PCSTR, WORD) PURE;
   STDMETHOD (Validate)          (THIS_ WORD) PURE;
   STDMETHOD (Identify)          (THIS_ DWORD, PSTR, DWORD, DWORD *, 
				  LONG *) PURE;
   STDMETHOD (IdentifyForFree)   (THIS_ BOOL *) PURE;
   };

typedef ISRResSpeakerA FAR *PISRRESSPEAKERA;


#ifdef _S_UNICODE
 #define ISRResSpeaker           ISRResSpeakerW
 #define IID_ISRResSpeaker       IID_ISRResSpeakerW
 #define PISRRESSPEAKER          PISRRESSPEAKERW

#else
 #define ISRResSpeaker           ISRResSpeakerA
 #define IID_ISRResSpeaker       IID_ISRResSpeakerA
 #define PISRRESSPEAKER          PISRRESSPEAKERA

#endif   // _S_UNICODE



/*
 * ISRSpeaker
 */

#undef   INTERFACE
#define  INTERFACE   ISRSpeakerW

DEFINE_GUID(IID_ISRSpeakerW, 0x090CD9AE, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRSpeakerW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRSpeakerW members
   STDMETHOD (Delete)         (THIS_ PCWSTR) PURE;
   STDMETHOD (Enum)           (THIS_ PWSTR *, DWORD *) PURE;
   STDMETHOD (Merge)          (THIS_ PCWSTR, PVOID, DWORD) PURE;
   STDMETHOD (New)            (THIS_ PCWSTR) PURE;
   STDMETHOD (Query)          (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (Read)           (THIS_ PCWSTR, PVOID *, DWORD *) PURE;
   STDMETHOD (Revert)         (THIS_ PCWSTR) PURE;
   STDMETHOD (Select)         (THIS_ PCWSTR, BOOL) PURE;
   STDMETHOD (Write)          (THIS_ PCWSTR, PVOID, DWORD) PURE;
   };

typedef ISRSpeakerW FAR *PISRSPEAKERW;


#undef   INTERFACE
#define  INTERFACE   ISRSpeakerA

DEFINE_GUID(IID_ISRSpeakerA, 0x090CD9AF, 0xDA1A, 0x11CD, 0xB3, 0xCA, 0x0, 0xAA, 0x0, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ISRSpeakerA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ISRSpeakerA members
   STDMETHOD (Delete)         (THIS_ PCSTR) PURE;
   STDMETHOD (Enum)           (THIS_ PSTR *, DWORD *) PURE;
   STDMETHOD (Merge)          (THIS_ PCSTR, PVOID, DWORD) PURE;
   STDMETHOD (New)            (THIS_ PCSTR) PURE;
   STDMETHOD (Query)          (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD (Read)           (THIS_ PCSTR, PVOID *, DWORD *) PURE;
   STDMETHOD (Revert)         (THIS_ PCSTR) PURE;
   STDMETHOD (Select)         (THIS_ PCSTR, BOOL) PURE;
   STDMETHOD (Write)          (THIS_ PCSTR, PVOID, DWORD) PURE;
   };

typedef ISRSpeakerA FAR *PISRSPEAKERA;


#ifdef _S_UNICODE
 #define ISRSpeaker              ISRSpeakerW
 #define IID_ISRSpeaker          IID_ISRSpeakerW
 #define PISRSPEAKER             PISRSPEAKERW

#else
 #define ISRSpeaker              ISRSpeakerA
 #define IID_ISRSpeaker          IID_ISRSpeakerA
 #define PISRSPEAKER             PISRSPEAKERA

#endif   // _S_UNICODE



/************************************************************************
Low-Level text-to-speech API
*/


/************************************************************************
defines */

#define  TTSI_NAMELEN                   SVFN_LEN
#define  TTSI_STYLELEN                  SVFN_LEN

#define  GENDER_NEUTRAL                 (0)
#define  GENDER_FEMALE                  (1)
#define  GENDER_MALE                    (2)

#define  TTSFEATURE_ANYWORD             SETBIT(0)
#define  TTSFEATURE_VOLUME              SETBIT(1)
#define  TTSFEATURE_SPEED               SETBIT(2)
#define  TTSFEATURE_PITCH               SETBIT(3)
#define  TTSFEATURE_TAGGED              SETBIT(4)
#define  TTSFEATURE_IPAUNICODE          SETBIT(5)
#define  TTSFEATURE_VISUAL              SETBIT(6)
#define  TTSFEATURE_WORDPOSITION        SETBIT(7)
#define  TTSFEATURE_PCOPTIMIZED         SETBIT(8)
#define  TTSFEATURE_PHONEOPTIMIZED      SETBIT(9)

#define  TTSI_ILEXPRONOUNCE             SETBIT(0)
#define  TTSI_ITTSATTRIBUTES            SETBIT(1)
#define  TTSI_ITTSCENTRAL               SETBIT(2)
#define  TTSI_ITTSDIALOGS               SETBIT(3)

#define  TTSDATAFLAG_TAGGED             SETBIT(0)

#define   TTSBNS_ABORTED                   SETBIT(0)

// ITTSNotifySink
#define   TTSNSAC_REALTIME               0
#define   TTSNSAC_PITCH                  1
#define   TTSNSAC_SPEED                  2
#define   TTSNSAC_VOLUME                 3


#define   TTSNSHINT_QUESTION             SETBIT(0)
#define   TTSNSHINT_STATEMENT            SETBIT(1)
#define   TTSNSHINT_COMMAND              SETBIT(2)
#define   TTSNSHINT_EXCLAMATION          SETBIT(3)
#define   TTSNSHINT_EMPHASIS             SETBIT(4)


// Ages
#define  TTSAGE_BABY                   1
#define  TTSAGE_TODDLER                3
#define  TTSAGE_CHILD                  6
#define  TTSAGE_ADOLESCENT             14
#define  TTSAGE_ADULT                  30
#define  TTSAGE_ELDERLY                70

// Attribute minimums and maximums
#define  TTSATTR_MINPITCH              0
#define  TTSATTR_MAXPITCH              0xffff
#define  TTSATTR_MINREALTIME           0
#define  TTSATTR_MAXREALTIME           0xffffffff
#define  TTSATTR_MINSPEED              0
#define  TTSATTR_MAXSPEED              0xffffffff
#define  TTSATTR_MINVOLUME             0
#define  TTSATTR_MAXVOLUME             0xffffffff


/************************************************************************
typedefs */

typedef struct {
   BYTE     bMouthHeigght;
   BYTE     bMouthWidth;
   BYTE     bMouthUpturn;
   BYTE     bJawOpen;
   BYTE     bTeethUpperVisible;
   BYTE     bTeethLowerVisible;
   BYTE     bTonguePosn;
   BYTE     bLipTension;
   } TTSMOUTH, *PTTSMOUTH;



typedef struct {
   GUID       gEngineID;
   WCHAR      szMfgName[TTSI_NAMELEN];
   WCHAR      szProductName[TTSI_NAMELEN];
   GUID       gModeID;
   WCHAR      szModeName[TTSI_NAMELEN];
   LANGUAGEW  language;
   WCHAR      szSpeaker[TTSI_NAMELEN];
   WCHAR      szStyle[TTSI_STYLELEN];
   WORD       wGender;
   WORD       wAge;
   DWORD      dwFeatures;
   DWORD      dwInterfaces;
   DWORD      dwEngineFeatures;
   } TTSMODEINFOW, *PTTSMODEINFOW;

typedef struct {
   GUID       gEngineID;
   CHAR       szMfgName[TTSI_NAMELEN];
   CHAR       szProductName[TTSI_NAMELEN];
   GUID       gModeID;
   CHAR       szModeName[TTSI_NAMELEN];
   LANGUAGEA  language;
   CHAR       szSpeaker[TTSI_NAMELEN];
   CHAR       szStyle[TTSI_STYLELEN];
   WORD       wGender;
   WORD       wAge;
   DWORD      dwFeatures;
   DWORD      dwInterfaces;
   DWORD      dwEngineFeatures;
   } TTSMODEINFOA, *PTTSMODEINFOA;

#ifdef _S_UNICODE
 #define TTSMODEINFO         TTSMODEINFOW
 #define PTTSMODEINFO        PTTSMODEINFOW

#else
 #define TTSMODEINFO         TTSMODEINFOA
 #define PTTSMODEINFO        PTTSMODEINFOA

#endif   // _S_UNICODE



typedef struct {
   DWORD      dwEngineID;
   DWORD      dwMfgName;
   DWORD      dwProductName;
   DWORD      dwModeID;
   DWORD      dwModeName;
   DWORD      dwLanguage;
   DWORD      dwDialect;
   DWORD      dwSpeaker;
   DWORD      dwStyle;
   DWORD      dwGender;
   DWORD      dwAge;
   DWORD      dwFeatures;
   DWORD      dwInterfaces;
   DWORD      dwEngineFeatures;
   } TTSMODEINFORANK, * PTTSMODEINFORANK;

/************************************************************************
Class IDs */
// {D67C0280-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(CLSID_TTSEnumerator, 
0xd67c0280, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

/************************************************************************
interfaces */

// ITTSAttributes

#undef   INTERFACE
#define  INTERFACE   ITTSAttributesW

DEFINE_GUID(IID_ITTSAttributesW, 0x1287A280L, 0x4A47, 0x101B, 0x93, 0x1A, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSAttributesW, IUnknown) {

// IUnknown members

   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

// ITTSAttributes members

   STDMETHOD (PitchGet)       (THIS_ WORD *) PURE;
   STDMETHOD (PitchSet)       (THIS_ WORD) PURE;  
   STDMETHOD (RealTimeGet)    (THIS_ DWORD *) PURE;
   STDMETHOD (RealTimeSet)    (THIS_ DWORD) PURE;  
   STDMETHOD (SpeedGet)       (THIS_ DWORD *) PURE;
   STDMETHOD (SpeedSet)       (THIS_ DWORD) PURE;  
   STDMETHOD (VolumeGet)      (THIS_ DWORD *) PURE;
   STDMETHOD (VolumeSet)      (THIS_ DWORD) PURE;  
   };

typedef ITTSAttributesW FAR * PITTSATTRIBUTESW;


#undef   INTERFACE
#define  INTERFACE   ITTSAttributesA

DEFINE_GUID(IID_ITTSAttributesA,
0x0FD6E2A1L, 0xE77D, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSAttributesA, IUnknown) {

// IUnknown members

   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

// ITTSAttributes members

   STDMETHOD (PitchGet)       (THIS_ WORD *) PURE;
   STDMETHOD (PitchSet)       (THIS_ WORD) PURE;  
   STDMETHOD (RealTimeGet)    (THIS_ DWORD *) PURE;
   STDMETHOD (RealTimeSet)    (THIS_ DWORD) PURE;  
   STDMETHOD (SpeedGet)       (THIS_ DWORD *) PURE;
   STDMETHOD (SpeedSet)       (THIS_ DWORD) PURE;  
   STDMETHOD (VolumeGet)      (THIS_ DWORD *) PURE;
   STDMETHOD (VolumeSet)      (THIS_ DWORD) PURE;  
   };

typedef ITTSAttributesA FAR * PITTSATTRIBUTESA;


#ifdef _S_UNICODE
 #define ITTSAttributes          ITTSAttributesW
 #define IID_ITTSAttributes      IID_ITTSAttributesW
 #define PITTSATTRIBUTES         PITTSATTRIBUTESW

#else
 #define ITTSAttributes          ITTSAttributesA
 #define IID_ITTSAttributes      IID_ITTSAttributesA
 #define PITTSATTRIBUTES         PITTSATTRIBUTESA

#endif   // _S_UNICODE



// ITTSBufNotifySink

#undef   INTERFACE
#define  INTERFACE   ITTSBufNotifySink

// {E4963D40-C743-11cd-80E5-00AA003E4B50}
DEFINE_GUID(IID_ITTSBufNotifySink, 
0xe4963d40, 0xc743, 0x11cd, 0x80, 0xe5, 0x0, 0xaa, 0x0, 0x3e, 0x4b, 0x50);

DECLARE_INTERFACE_ (ITTSBufNotifySink, IUnknown) {

// IUnknown members

   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

// ITTSBufNotifySink members

   STDMETHOD (TextDataDone)   (THIS_ QWORD, DWORD) PURE;
   STDMETHOD (TextDataStarted)(THIS_ QWORD) PURE;
   STDMETHOD (BookMark)       (THIS_ QWORD, DWORD) PURE;  
   STDMETHOD (WordPosition)   (THIS_ QWORD, DWORD) PURE;
   };

typedef ITTSBufNotifySink FAR * PITTSBUFNOTIFYSINK;

// In case anyone uses the W or A interface
#define ITTSBufNotifySinkW          ITTSBufNotifySink
#define IID_ITTSBufNotifySinkW      IID_ITTSBufNotifySink
#define PITTSBUFNOTIFYSINKW         PITTSBUFNOTIFYSINK
#define ITTSBufNotifySinkA          ITTSBufNotifySink
#define IID_ITTSBufNotifySinkA      IID_ITTSBufNotifySink
#define PITTSBUFNOTIFYSINKA         PITTSBUFNOTIFYSINK



/*
 * ITTSCentral
 */

#undef   INTERFACE
#define  INTERFACE   ITTSCentralW

DEFINE_GUID(IID_ITTSCentralW, 0x28016060L, 0x4A47, 0x101B, 0x93, 0x1A, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSCentralW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSCentralW members
   STDMETHOD (Inject)         (THIS_ PCWSTR) PURE;
   STDMETHOD (ModeGet)        (THIS_ PTTSMODEINFOW) PURE;
   STDMETHOD (Phoneme)        (THIS_ VOICECHARSET, DWORD, SDATA, PSDATA) PURE;
   STDMETHOD (PosnGet)        (THIS_ PQWORD) PURE;
   STDMETHOD (TextData)       (THIS_ VOICECHARSET, DWORD, SDATA, PVOID, IID) PURE;
   STDMETHOD (ToFileTime)     (THIS_ PQWORD, FILETIME *) PURE;
   STDMETHOD (AudioPause)     (THIS) PURE;
   STDMETHOD (AudioResume)    (THIS) PURE;
   STDMETHOD (AudioReset)     (THIS) PURE;
   STDMETHOD (Register)       (THIS_ PVOID, IID, DWORD*) PURE;
   STDMETHOD (UnRegister)     (THIS_ DWORD) PURE;
   };

typedef ITTSCentralW FAR * PITTSCENTRALW;


#undef   INTERFACE
#define  INTERFACE   ITTSCentralA

DEFINE_GUID(IID_ITTSCentralA, 0x05EB6C6AL, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSCentralA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSCentralA members
   STDMETHOD (Inject)         (THIS_ PCSTR) PURE;
   STDMETHOD (ModeGet)        (THIS_ PTTSMODEINFOA) PURE;
   STDMETHOD (Phoneme)        (THIS_ VOICECHARSET, DWORD, SDATA, PSDATA) PURE;
   STDMETHOD (PosnGet)        (THIS_ PQWORD) PURE;
   STDMETHOD (TextData)       (THIS_ VOICECHARSET, DWORD, SDATA, PVOID, IID) PURE;
   STDMETHOD (ToFileTime)     (THIS_ PQWORD, FILETIME *) PURE;
   STDMETHOD (AudioPause)     (THIS) PURE;
   STDMETHOD (AudioResume)    (THIS) PURE;
   STDMETHOD (AudioReset)     (THIS) PURE;
   STDMETHOD (Register)       (THIS_ PVOID, IID, DWORD*) PURE;
   STDMETHOD (UnRegister)     (THIS_ DWORD) PURE;
   };

typedef ITTSCentralA FAR * PITTSCENTRALA;


#ifdef _S_UNICODE
 #define ITTSCentral             ITTSCentralW
 #define IID_ITTSCentral         IID_ITTSCentralW
 #define PITTSCENTRAL            PITTSCENTRALW

#else
 #define ITTSCentral             ITTSCentralA
 #define IID_ITTSCentral         IID_ITTSCentralA
 #define PITTSCENTRAL            PITTSCENTRALA

#endif   // _S_UNICODE



/*
 * ITTSDialogsW
 */

#undef   INTERFACE
#define  INTERFACE   ITTSDialogsW

DEFINE_GUID(IID_ITTSDialogsW, 0x47F59D00L, 0x4A47, 0x101B, 0x93, 0x1A, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSDialogsW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSDialogsW members
   STDMETHOD (AboutDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (LexiconDlg)     (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (GeneralDlg)     (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TranslateDlg)   (THIS_ HWND, PCWSTR) PURE;
   };

typedef ITTSDialogsW FAR * PITTSDIALOGSW;


#undef   INTERFACE
#define  INTERFACE   ITTSDialogsA

DEFINE_GUID(IID_ITTSDialogsA, 0x05EB6C6BL, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSDialogsA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSDialogsA members
   STDMETHOD (AboutDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (LexiconDlg)     (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (GeneralDlg)     (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TranslateDlg)   (THIS_ HWND, PCSTR) PURE;
   };

typedef ITTSDialogsA FAR * PITTSDIALOGSA;


#ifdef _S_UNICODE
 #define ITTSDialogs          ITTSDialogsW
 #define IID_ITTSDialogs      IID_ITTSDialogsW
 #define PITTSDIALOGS         PITTSDIALOGSW

#else
 #define ITTSDialogs          ITTSDialogsA
 #define IID_ITTSDialogs      IID_ITTSDialogsA
 #define PITTSDIALOGS         PITTSDIALOGSA

#endif



/*
 * ITTSEnum
 */

#undef   INTERFACE
#define  INTERFACE   ITTSEnumW

DEFINE_GUID(IID_ITTSEnumW, 0x6B837B20L, 0x4A47, 0x101B, 0x93, 0x1A, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSEnumW, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSEnumW members
   STDMETHOD (Next)           (THIS_ ULONG, PTTSMODEINFOW, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ ITTSEnumW * FAR *) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PITTSCENTRALW *, LPUNKNOWN) PURE;
   };

typedef ITTSEnumW FAR * PITTSENUMW;


#undef   INTERFACE
#define  INTERFACE   ITTSEnumA

DEFINE_GUID(IID_ITTSEnumA, 0x05EB6C6DL, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSEnumA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSEnumA members
   STDMETHOD (Next)           (THIS_ ULONG, PTTSMODEINFOA, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ ITTSEnumA * FAR *) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PITTSCENTRALA *, LPUNKNOWN) PURE;
   };

typedef ITTSEnumA FAR * PITTSENUMA;


#ifdef _S_UNICODE
 #define ITTSEnum             ITTSEnumW
 #define IID_ITTSEnum         IID_ITTSEnumW
 #define PITTSENUM            PITTSENUMW

#else
 #define ITTSEnum             ITTSEnumA
 #define IID_ITTSEnum         IID_ITTSEnumA
 #define PITTSENUM            PITTSENUMA

#endif



/*
 * ITTSFind
 */

#undef   INTERFACE
#define  INTERFACE   ITTSFindW

DEFINE_GUID(IID_ITTSFindW, 0x7AA42960L, 0x4A47, 0x101B, 0x93, 0x1A, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSFindW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSFindW members
   STDMETHOD (Find)           (THIS_ PTTSMODEINFOW, PTTSMODEINFORANK, PTTSMODEINFOW) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PITTSCENTRALW *, LPUNKNOWN) PURE;
   };

typedef ITTSFindW FAR * PITTSFINDW;


#undef   INTERFACE
#define  INTERFACE   ITTSFindA

DEFINE_GUID(IID_ITTSFindA, 0x05EB6C6EL, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSFindA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSFindA members
   STDMETHOD (Find)           (THIS_ PTTSMODEINFOA, PTTSMODEINFORANK, PTTSMODEINFOA) PURE;
   STDMETHOD (Select)         (THIS_ GUID, PITTSCENTRALA *, LPUNKNOWN) PURE;
   };

typedef ITTSFindA FAR * PITTSFINDA;


#ifdef _S_UNICODE
 #define ITTSFind             ITTSFindW
 #define IID_ITTSFind         IID_ITTSFindW
 #define PITTSFIND            PITTSFINDW

#else
 #define ITTSFind             ITTSFindA
 #define IID_ITTSFind         IID_ITTSFindA
 #define PITTSFIND            PITTSFINDA

#endif



/*
 * ITTSNotifySink
 */

#undef   INTERFACE
#define  INTERFACE   ITTSNotifySinkW

DEFINE_GUID(IID_ITTSNotifySinkW, 0xC0FA8F40L, 0x4A46, 0x101B, 0x93, 0x1A, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSNotifySinkW, IUnknown) {

// IUnknown members

   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

// ITTSNotifySinkW members

   STDMETHOD (AttribChanged)  (THIS_ DWORD) PURE;
   STDMETHOD (AudioStart)     (THIS_ QWORD) PURE;
   STDMETHOD (AudioStop)      (THIS_ QWORD) PURE;
   STDMETHOD (Visual)         (THIS_ QWORD, WCHAR, WCHAR, DWORD, PTTSMOUTH) PURE;
   };

typedef ITTSNotifySinkW FAR * PITTSNOTIFYSINKW;


#undef   INTERFACE
#define  INTERFACE   ITTSNotifySinkA

DEFINE_GUID(IID_ITTSNotifySinkA, 0x05EB6C6FL, 0xDBAB, 0x11CD, 0xB3, 0xCA, 0x00, 0xAA, 0x00, 0x47, 0xBA, 0x4F);

DECLARE_INTERFACE_ (ITTSNotifySinkA, IUnknown) {

   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // ITTSNotifySinkA members
   STDMETHOD (AttribChanged)  (THIS_ DWORD) PURE;
   STDMETHOD (AudioStart)     (THIS_ QWORD) PURE;
   STDMETHOD (AudioStop)      (THIS_ QWORD) PURE;
   STDMETHOD (Visual)         (THIS_ QWORD, CHAR, CHAR, DWORD, PTTSMOUTH) PURE;
   };

typedef ITTSNotifySinkA FAR * PITTSNOTIFYSINKA;


#ifdef _S_UNICODE
 #define ITTSNotifySink       ITTSNotifySinkW
 #define IID_ITTSNotifySink   IID_ITTSNotifySinkW
 #define PITTSNOTIFYSINK      PITTSNOTIFYSINKW

#else
 #define ITTSNotifySink       ITTSNotifySinkA
 #define IID_ITTSNotifySink   IID_ITTSNotifySinkA
 #define PITTSNOTIFYSINK      PITTSNOTIFYSINKA

#endif



/************************************************************************
High-Level command and control speech recognition API
*/

/************************************************************************
defines */


// VCMDNAME member lengths
#define VCMD_APPLEN             ((DWORD)32)
#define VCMD_STATELEN           VCMD_APPLEN
#define VCMD_MICLEN             VCMD_APPLEN
#define VCMD_SPEAKERLEN         VCMD_APPLEN

// dwFlags parameter of IVoiceCmd::MenuCreate
#define  VCMDMC_CREATE_TEMP     0x00000001
#define  VCMDMC_CREATE_NEW      0x00000002
#define  VCMDMC_CREATE_ALWAYS   0x00000004
#define  VCMDMC_OPEN_ALWAYS     0x00000008
#define  VCMDMC_OPEN_EXISTING   0x00000010

// dwFlags parameter of IVoiceCmd::Register
#define  VCMDRF_NOMESSAGES      0
#define  VCMDRF_ALLBUTVUMETER   0x00000001
#define  VCMDRF_VUMETER         0x00000002
#define  VCMDRF_ALLMESSAGES     (VCMDRF_ALLBUTVUMETER | VCMDRF_VUMETER)

// dwFlags parameter of IVoiceCmd::MenuEnum
#define  VCMDEF_DATABASE        0x00000000
#define  VCMDEF_ACTIVE          0x00000001
#define  VCMDEF_SELECTED        0x00000002
#define  VCMDEF_PERMANENT       0x00000004
#define  VCMDEF_TEMPORARY       0x00000008

// dwFlags parameter of IVCmdMenu::Activate
#define  VWGFLAG_ASLEEP         0x00000001

// wPriority parameter of IVCmdMenu::Activate
#define  VCMDACT_NORMAL         (0x8000)
#define  VCMDACT_LOW            (0x4000)
#define  VCMDACT_HIGH           (0xC000)

// dwFlags of the VCMDCOMMAND structure
#define  VCMDCMD_VERIFY         0x00000001
#define  VCMDCMD_DISABLED_TEMP  0x00000002
#define  VCMDCMD_DISABLED_PERM  0x00000004

// parameter to any function that processes individual commands
#define  VCMD_BY_POSITION       0x00000001
#define  VCMD_BY_IDENTIFIER     0x00000002


// values for dwAttributes field of IVCmdNotifySink::AttribChanged
#define  IVCNSAC_AUTOGAINENABLE 0x00000001
#define  IVCNSAC_ENABLED        0x00000002
#define  IVCNSAC_AWAKE          0x00000004
#define  IVCNSAC_DEVICE         0x00000008
#define  IVCNSAC_MICROPHONE     0x00000010
#define  IVCNSAC_SPEAKER        0x00000020
#define  IVCNSAC_SRMODE         0x00000040
#define  IVCNSAC_THRESHOLD      0x00000080
#define  IVCNSAC_ORIGINAPP      0x00010000

// values for dwAttributes field of IVTxtNotifySink::AttribChanged
#define  IVTNSAC_DEVICE         0x00000001
#define  IVTNSAC_ENABLED        0x00000002
#define  IVTNSAC_SPEED          0x00000004
#define  IVTNSAC_VOLUME         0x00000008
#define  IVTNSAC_TTSMODE        0x00000010


// values used by IVXxxAttributes::SetMode to set the global speech
// recognition mode
#define  VSRMODE_DISABLED       0x00000001
#define  VSRMODE_OFF            0x00000002
#define  VSRMODE_CMDPAUSED      0x00000004
#define  VSRMODE_CMDNOTPAUSED   0x00000008
#define  VSRMODE_CMDONLY        0x00000010
#define  VSRMODE_DCTONLY        0x00000020
#define  VSRMODE_CMDANDDCT      0x00000040


/************************************************************************
typedefs */

// voice command structure - passed to command menu functions (IVCmdMenu)
typedef struct {
    DWORD   dwSize;         // size of struct including amount of abAction
    DWORD   dwFlags;
    DWORD   dwID;           // Command ID
    DWORD   dwCommand;      // DWORD aligned offset of command string
    DWORD   dwDescription;  // DWORD aligned offset of description string
    DWORD   dwCategory;     // DWORD aligned offset of category string
    DWORD   dwCommandText;  // DWORD aligned offset of command text string
    DWORD   dwAction;       // DWORD aligned offset of action data
    DWORD   dwActionSize;   // size of the action data (could be string or binary)
    BYTE    abData[1];      // command, description, category, and action data
			    // (action data is NOT interpreted by voice command)
} VCMDCOMMAND, * PVCMDCOMMAND;



// site information structure - possible parameter to IVoiceCmd::Register
typedef struct {
    DWORD   dwAutoGainEnable;
    DWORD   dwAwakeState;
    DWORD   dwThreshold;
    DWORD   dwDevice;
    DWORD   dwEnable;
    WCHAR   szMicrophone[VCMD_MICLEN];
    WCHAR   szSpeaker[VCMD_SPEAKERLEN];
    GUID    gModeID;
} VCSITEINFOW, *PVCSITEINFOW;

typedef struct {
    DWORD   dwAutoGainEnable;
    DWORD   dwAwakeState;
    DWORD   dwThreshold;
    DWORD   dwDevice;
    DWORD   dwEnable;
    CHAR    szMicrophone[VCMD_MICLEN];
    CHAR    szSpeaker[VCMD_SPEAKERLEN];
    GUID    gModeID;
} VCSITEINFOA, *PVCSITEINFOA;



// menu name structure
typedef struct {
    WCHAR   szApplication[VCMD_APPLEN]; // unique application name
    WCHAR   szState[VCMD_STATELEN];     // unique application state
} VCMDNAMEW, *PVCMDNAMEW;

typedef struct {
    CHAR    szApplication[VCMD_APPLEN]; // unique application name
    CHAR    szState[VCMD_STATELEN];     // unique application state
} VCMDNAMEA, *PVCMDNAMEA;



#ifdef  _S_UNICODE
 #define VCSITEINFO  VCSITEINFOW
 #define PVCSITEINFO PVCSITEINFOW
 #define VCMDNAME    VCMDNAMEW
 #define PVCMDNAME   PVCMDNAMEW
#else
 #define VCSITEINFO  VCSITEINFOA
 #define PVCSITEINFO PVCSITEINFOA
 #define VCMDNAME    VCMDNAMEA
 #define PVCMDNAME   PVCMDNAMEA
#endif  // _S_UNICODE

/************************************************************************
interfaces */

/*
 *  IVCmdNotifySink
 */
#undef   INTERFACE
#define  INTERFACE   IVCmdNotifySinkW

DEFINE_GUID(IID_IVCmdNotifySinkW, 0xCCFD7A60L, 0x604D, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);

DECLARE_INTERFACE_ (IVCmdNotifySinkW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdNotifySink members

   STDMETHOD (CommandRecognize) (THIS_ DWORD, PVCMDNAMEW, DWORD, DWORD, PVOID, 
				 DWORD, PWSTR, PWSTR) PURE;
   STDMETHOD (CommandOther)     (THIS_ PVCMDNAMEW, PWSTR) PURE;
   STDMETHOD (CommandStart)     (THIS) PURE;
   STDMETHOD (MenuActivate)     (THIS_ PVCMDNAMEW, BOOL) PURE;
   STDMETHOD (UtteranceBegin)   (THIS) PURE;
   STDMETHOD (UtteranceEnd)     (THIS) PURE;
   STDMETHOD (VUMeter)          (THIS_ WORD) PURE;
   STDMETHOD (AttribChanged)    (THIS_ DWORD) PURE;
   STDMETHOD (Interference)     (THIS_ DWORD) PURE;
};

typedef IVCmdNotifySinkW FAR * PIVCMDNOTIFYSINKW;


#undef   INTERFACE
#define  INTERFACE   IVCmdNotifySinkA

// {80B25CC0-5540-11b9-C000-5611722E1D15}
DEFINE_GUID(IID_IVCmdNotifySinkA, 0x80b25cc0, 0x5540, 0x11b9, 0xc0, 0x0, 0x56, 0x11, 0x72, 0x2e, 0x1d, 0x15);

DECLARE_INTERFACE_ (IVCmdNotifySinkA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdNotifySinkA members

   STDMETHOD (CommandRecognize) (THIS_ DWORD, PVCMDNAMEA, DWORD, DWORD, PVOID, 
				 DWORD, PSTR, PSTR) PURE;
   STDMETHOD (CommandOther)     (THIS_ PVCMDNAMEA, PSTR) PURE;
   STDMETHOD (CommandStart)     (THIS) PURE;
   STDMETHOD (MenuActivate)     (THIS_ PVCMDNAMEA, BOOL) PURE;
   STDMETHOD (UtteranceBegin)   (THIS) PURE;
   STDMETHOD (UtteranceEnd)     (THIS) PURE;
   STDMETHOD (VUMeter)          (THIS_ WORD) PURE;
   STDMETHOD (AttribChanged)    (THIS_ DWORD) PURE;
   STDMETHOD (Interference)     (THIS_ DWORD) PURE;
};

typedef IVCmdNotifySinkA FAR * PIVCMDNOTIFYSINKA;


#ifdef _S_UNICODE
 #define IVCmdNotifySink        IVCmdNotifySinkW
 #define IID_IVCmdNotifySink    IID_IVCmdNotifySinkW
 #define PIVCMDNOTIFYSINK       PIVCMDNOTIFYSINKW

#else
 #define IVCmdNotifySink        IVCmdNotifySinkA
 #define IID_IVCmdNotifySink    IID_IVCmdNotifySinkA
 #define PIVCMDNOTIFYSINK       PIVCMDNOTIFYSINKA

#endif // _S_UNICODE


/*
 *  IVCmdNotifySinkEx
 */
#undef   INTERFACE
#define  INTERFACE   IVCmdNotifySinkExW

// {2F440FB4-CE01-11cf-B234-00AA00A215ED}
DEFINE_GUID(IID_IVCmdNotifySinkExW, 
0x2f440fb4, 0xce01, 0x11cf, 0xb2, 0x34, 0x0, 0xaa, 0x0, 0xa2, 0x15, 0xed);

DECLARE_INTERFACE_ (IVCmdNotifySinkExW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdNotifySinkExW members

   STDMETHOD (CommandRecognizeEx)
				(THIS_ DWORD, PVCMDNAMEW, DWORD, DWORD, PVOID, 
				 DWORD, PWSTR, PWSTR, LPUNKNOWN) PURE;
   STDMETHOD (CommandOther)     (THIS_ PVCMDNAMEW, PWSTR) PURE;
   STDMETHOD (CommandStart)     (THIS) PURE;
   STDMETHOD (MenuActivate)     (THIS_ PVCMDNAMEW, BOOL) PURE;
   STDMETHOD (UtteranceBegin)   (THIS) PURE;
   STDMETHOD (UtteranceEnd)     (THIS) PURE;
   STDMETHOD (VUMeter)          (THIS_ WORD) PURE;
   STDMETHOD (AttribChanged)    (THIS_ DWORD) PURE;
   STDMETHOD (Interference)     (THIS_ DWORD) PURE;
};

typedef IVCmdNotifySinkExW FAR * PIVCMDNOTIFYSINKEXW;


#undef   INTERFACE
#define  INTERFACE   IVCmdNotifySinkExA

// {2F440FB8-CE01-11cf-B234-00AA00A215ED}
DEFINE_GUID(IID_IVCmdNotifySinkExA, 
0x2f440fb8, 0xce01, 0x11cf, 0xb2, 0x34, 0x0, 0xaa, 0x0, 0xa2, 0x15, 0xed);

DECLARE_INTERFACE_ (IVCmdNotifySinkExA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdNotifySinkExA members

   STDMETHOD (CommandRecognizeEx)
				(THIS_ DWORD, PVCMDNAMEA, DWORD, DWORD, PVOID, 
				 DWORD, PSTR, PSTR, LPUNKNOWN) PURE;
   STDMETHOD (CommandOther)     (THIS_ PVCMDNAMEA, PSTR) PURE;
   STDMETHOD (CommandStart)     (THIS) PURE;
   STDMETHOD (MenuActivate)     (THIS_ PVCMDNAMEA, BOOL) PURE;
   STDMETHOD (UtteranceBegin)   (THIS) PURE;
   STDMETHOD (UtteranceEnd)     (THIS) PURE;
   STDMETHOD (VUMeter)          (THIS_ WORD) PURE;
   STDMETHOD (AttribChanged)    (THIS_ DWORD) PURE;
   STDMETHOD (Interference)     (THIS_ DWORD) PURE;
};

typedef IVCmdNotifySinkExA FAR * PIVCMDNOTIFYSINKEXA;


#ifdef _S_UNICODE
 #define IVCmdNotifySinkEx        IVCmdNotifySinkExW
 #define IID_IVCmdNotifySinkEx    IID_IVCmdNotifySinkExW
 #define PIVCMDNOTIFYSINKEX       PIVCMDNOTIFYSINKEXW

#else
 #define IVCmdNotifySinkEx        IVCmdNotifySinkExA
 #define IID_IVCmdNotifySinkEx    IID_IVCmdNotifySinkExA
 #define PIVCMDNOTIFYSINKEX       PIVCMDNOTIFYSINKEXA

#endif // _S_UNICODE


/*
 *  IVCmdEnum
 */
#undef   INTERFACE
#define  INTERFACE   IVCmdEnumW

DEFINE_GUID(IID_IVCmdEnumW, 0xD3CC0820L, 0x604D, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);

DECLARE_INTERFACE_ (IVCmdEnumW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdEnum members
   STDMETHOD (Next)           (THIS_ ULONG, PVCMDNAMEW, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ IVCmdEnumW * FAR *) PURE;
};
typedef IVCmdEnumW FAR * PIVCMDENUMW;


#undef   INTERFACE
#define  INTERFACE   IVCmdEnumA

// {E86F9540-DCA2-11CD-A166-00AA004CD65C}
DEFINE_GUID(IID_IVCmdEnumA, 
0xE86F9540, 0xDCA2, 0x11CD, 0xA1, 0x66, 0x0, 0xAA, 0x0, 0x4C, 0xD6, 0x5C);

DECLARE_INTERFACE_ (IVCmdEnumA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdEnum members
   STDMETHOD (Next)           (THIS_ ULONG, PVCMDNAMEA, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ IVCmdEnumA * FAR *) PURE;
};
typedef IVCmdEnumA FAR * PIVCMDENUMA;


#ifdef _S_UNICODE
 #define IVCmdEnum              IVCmdEnumW
 #define IID_IVCmdEnum          IID_IVCmdEnumW
 #define PIVCMDENUM             PIVCMDENUMW

#else
 #define IVCmdEnum              IVCmdEnumA
 #define IID_IVCmdEnum          IID_IVCmdEnumA
 #define PIVCMDENUM             PIVCMDENUMA

#endif // _S_UNICODE


 
 
/*
 *  IEnumSRShare
 */
#undef   INTERFACE
#define  INTERFACE   IEnumSRShareW

// {E97F05C0-81B3-11ce-B763-00AA004CD65C}
DEFINE_GUID(IID_IEnumSRShareW,
0xe97f05c0, 0x81b3, 0x11ce, 0xb7, 0x63, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IEnumSRShareW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IEnumSRShare members
   STDMETHOD (Next)           (THIS_ ULONG, PSRSHAREW, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ IEnumSRShareW * FAR *) PURE;
   STDMETHOD (New)            (THIS_ DWORD, GUID, PISRCENTRALW *, QWORD *) PURE;
   STDMETHOD (Share)          (THIS_ QWORD, PISRCENTRALW *) PURE;
   STDMETHOD (Detach)         (THIS_ QWORD) PURE;
};
typedef IEnumSRShareW FAR * PIENUMSRSHAREW;


#undef   INTERFACE
#define  INTERFACE   IEnumSRShareA

// {E97F05C1-81B3-11ce-B763-00AA004CD65C}
DEFINE_GUID(IID_IEnumSRShareA,
0xe97f05c1, 0x81b3, 0x11ce, 0xb7, 0x63, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IEnumSRShareA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IEnumSRShare members
   STDMETHOD (Next)           (THIS_ ULONG, PSRSHAREA, ULONG *) PURE;
   STDMETHOD (Skip)           (THIS_ ULONG) PURE;
   STDMETHOD (Reset)          (THIS) PURE;
   STDMETHOD (Clone)          (THIS_ IEnumSRShareA * FAR *) PURE;
   STDMETHOD (New)            (THIS_ DWORD, GUID, PISRCENTRALA *, QWORD *) PURE;
   STDMETHOD (Share)          (THIS_ QWORD, PISRCENTRALA *) PURE;
   STDMETHOD (Detach)         (THIS_ QWORD) PURE;
};
typedef IEnumSRShareA FAR * PIENUMSRSHAREA;


#ifdef _S_UNICODE
 #define IEnumSRShare              IEnumSRShareW
 #define IID_IEnumSRShare          IID_IEnumSRShareW
 #define PIENUMSRSHARE             PIENUMSRSHAREW

#else
 #define IEnumSRShare              IEnumSRShareA
 #define IID_IEnumSRShare          IID_IEnumSRShareA
 #define PIENUMSRSHARE             PIENUMSRSHAREA

#endif // _S_UNICODE


 
 
/*
 *  IVCmdMenu
 */
#undef   INTERFACE
#define  INTERFACE   IVCmdMenuW

DEFINE_GUID(IID_IVCmdMenuW, 0xDAC54F60L, 0x604D, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);

DECLARE_INTERFACE_ (IVCmdMenuW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdMenu members
   STDMETHOD (Activate)       (THIS_ HWND, DWORD) PURE;
   STDMETHOD (Deactivate)     (THIS) PURE;
   STDMETHOD (TrainMenuDlg)   (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (Num)            (THIS_ DWORD *) PURE;
   STDMETHOD (Get)            (THIS_ DWORD, DWORD, DWORD, PSDATA, DWORD *) PURE;
   STDMETHOD (Set)            (THIS_ DWORD, DWORD, DWORD, SDATA) PURE;
   STDMETHOD (Add)            (THIS_ DWORD, SDATA, DWORD *) PURE;
   STDMETHOD (Remove)         (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (ListGet)        (THIS_ PCWSTR, PSDATA, DWORD *) PURE;
   STDMETHOD (ListSet)        (THIS_ PCWSTR, DWORD, SDATA) PURE;
   STDMETHOD (EnableItem)     (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (SetItem)        (THIS_ DWORD, DWORD, DWORD) PURE;
};

typedef IVCmdMenuW FAR * PIVCMDMENUW;


#undef   INTERFACE
#define  INTERFACE   IVCmdMenuA

// {746141E0-5543-11b9-C000-5611722E1D15}
DEFINE_GUID(IID_IVCmdMenuA, 0x746141e0, 0x5543, 0x11b9, 0xc0, 0x0, 0x56, 0x11, 0x72, 0x2e, 0x1d, 0x15);

DECLARE_INTERFACE_ (IVCmdMenuA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdMenu members
   STDMETHOD (Activate)       (THIS_ HWND, DWORD) PURE;
   STDMETHOD (Deactivate)     (THIS) PURE;
   STDMETHOD (TrainMenuDlg)   (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (Num)            (THIS_ DWORD *) PURE;
   STDMETHOD (Get)            (THIS_ DWORD, DWORD, DWORD, PSDATA, DWORD *) PURE;
   STDMETHOD (Set)            (THIS_ DWORD, DWORD, DWORD, SDATA) PURE;
   STDMETHOD (Add)            (THIS_ DWORD, SDATA, DWORD *) PURE;
   STDMETHOD (Remove)         (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (ListGet)        (THIS_ PCSTR, PSDATA, DWORD *) PURE;
   STDMETHOD (ListSet)        (THIS_ PCSTR, DWORD, SDATA) PURE;
   STDMETHOD (EnableItem)     (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (SetItem)        (THIS_ DWORD, DWORD, DWORD) PURE;
};

typedef IVCmdMenuA FAR * PIVCMDMENUA;


#ifdef _S_UNICODE
 #define IVCmdMenu      IVCmdMenuW
 #define IID_IVCmdMenu  IID_IVCmdMenuW
 #define PIVCMDMENU     PIVCMDMENUW

#else
 #define IVCmdMenu      IVCmdMenuA
 #define IID_IVCmdMenu  IID_IVCmdMenuA
 #define PIVCMDMENU     PIVCMDMENUA

#endif // _S_UNICODE



/*
 *  IVoiceCmd
 */
#undef   INTERFACE
#define  INTERFACE   IVoiceCmdW

DEFINE_GUID(IID_IVoiceCmdW, 0xE0DCC220L, 0x604D, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);

DECLARE_INTERFACE_ (IVoiceCmdW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVoiceCmd members
   STDMETHOD (Register)       (THIS_ PCWSTR, LPUNKNOWN, GUID, DWORD,
				     PVCSITEINFOW) PURE;
   STDMETHOD (MenuEnum)       (THIS_ DWORD, PCWSTR, PCWSTR, PIVCMDENUMW *) PURE;
   STDMETHOD (MenuCreate)     (THIS_ PVCMDNAMEW, PLANGUAGEW, DWORD,
				     PIVCMDMENUW *) PURE;
   STDMETHOD (MenuDelete)     (THIS_ PVCMDNAMEW) PURE;
   STDMETHOD (CmdMimic)       (THIS_ PVCMDNAMEW, PCWSTR) PURE;
};

typedef IVoiceCmdW FAR * PIVOICECMDW;


#undef   INTERFACE
#define  INTERFACE   IVoiceCmdA

// {C63A2B30-5543-11b9-C000-5611722E1D15}
DEFINE_GUID(IID_IVoiceCmdA, 0xc63a2b30, 0x5543, 0x11b9, 0xc0, 0x0, 0x56, 0x11, 0x72, 0x2e, 0x1d, 0x15);

DECLARE_INTERFACE_ (IVoiceCmdA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVoiceCmd members
   STDMETHOD (Register)       (THIS_ PCSTR, LPUNKNOWN, GUID, DWORD,
				     PVCSITEINFOA) PURE;
   STDMETHOD (MenuEnum)       (THIS_ DWORD, PCSTR, PCSTR, PIVCMDENUMA *) PURE;
   STDMETHOD (MenuCreate)     (THIS_ PVCMDNAMEA, PLANGUAGEA, DWORD,
				     PIVCMDMENUA *) PURE;
   STDMETHOD (MenuDelete)     (THIS_ PVCMDNAMEA) PURE;
   STDMETHOD (CmdMimic)       (THIS_ PVCMDNAMEA, PCSTR) PURE;
};

typedef IVoiceCmdA FAR * PIVOICECMDA;


#ifdef _S_UNICODE
 #define IVoiceCmd      IVoiceCmdW
 #define IID_IVoiceCmd  IID_IVoiceCmdW
 #define PIVOICECMD     PIVOICECMDW

#else
 #define IVoiceCmd      IVoiceCmdA
 #define IID_IVoiceCmd  IID_IVoiceCmdA
 #define PIVOICECMD     PIVOICECMDA

#endif //_S_UNICODE


/*
 *  IVCmdAttributes
 */
#undef   INTERFACE
#define  INTERFACE   IVCmdAttributesW

DEFINE_GUID(IID_IVCmdAttributesW, 0xE5F24680L, 0x6053, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);

DECLARE_INTERFACE_ (IVCmdAttributesW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdAttributes members
   STDMETHOD (AutoGainEnableGet) (THIS_ DWORD *) PURE;
   STDMETHOD (AutoGainEnableSet) (THIS_ DWORD) PURE;
   STDMETHOD (AwakeStateGet)     (THIS_ DWORD *) PURE;
   STDMETHOD (AwakeStateSet)     (THIS_ DWORD) PURE;
   STDMETHOD (ThresholdGet)      (THIS_ DWORD *) PURE;
   STDMETHOD (ThresholdSet)      (THIS_ DWORD) PURE;
   STDMETHOD (DeviceGet)         (THIS_ DWORD *) PURE;
   STDMETHOD (DeviceSet)         (THIS_ DWORD) PURE;
   STDMETHOD (EnabledGet)        (THIS_ DWORD *) PURE;
   STDMETHOD (EnabledSet)        (THIS_ DWORD) PURE;
   STDMETHOD (MicrophoneGet)     (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (MicrophoneSet)     (THIS_ PCWSTR) PURE;
   STDMETHOD (SpeakerGet)        (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD (SpeakerSet)        (THIS_ PCWSTR) PURE;
   STDMETHOD (SRModeGet)         (THIS_ GUID *) PURE;
   STDMETHOD (SRModeSet)         (THIS_ GUID) PURE;
};

typedef IVCmdAttributesW FAR * PIVCMDATTRIBUTESW;


#undef   INTERFACE
#define  INTERFACE   IVCmdAttributesA

// {FFF5DF80-5544-11b9-C000-5611722E1D15}
DEFINE_GUID(IID_IVCmdAttributesA, 0xfff5df80, 0x5544, 0x11b9, 0xc0, 0x0, 0x56, 0x11, 0x72, 0x2e, 0x1d, 0x15);

DECLARE_INTERFACE_ (IVCmdAttributesA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVCmdAttributes members
   STDMETHOD (AutoGainEnableGet) (THIS_ DWORD *) PURE;
   STDMETHOD (AutoGainEnableSet) (THIS_ DWORD) PURE;
   STDMETHOD (AwakeStateGet)     (THIS_ DWORD *) PURE;
   STDMETHOD (AwakeStateSet)     (THIS_ DWORD) PURE;
   STDMETHOD (ThresholdGet)      (THIS_ DWORD *) PURE;
   STDMETHOD (ThresholdSet)      (THIS_ DWORD) PURE;
   STDMETHOD (DeviceGet)         (THIS_ DWORD *) PURE;
   STDMETHOD (DeviceSet)         (THIS_ DWORD) PURE;
   STDMETHOD (EnabledGet)        (THIS_ DWORD *) PURE;
   STDMETHOD (EnabledSet)        (THIS_ DWORD) PURE;
   STDMETHOD (MicrophoneGet)     (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD (MicrophoneSet)     (THIS_ PCSTR) PURE;
   STDMETHOD (SpeakerGet)        (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD (SpeakerSet)        (THIS_ PCSTR) PURE;
   STDMETHOD (SRModeGet)         (THIS_ GUID *) PURE;
   STDMETHOD (SRModeSet)         (THIS_ GUID) PURE;
};

typedef IVCmdAttributesA FAR * PIVCMDATTRIBUTESA;


#ifdef _S_UNICODE
 #define IVCmdAttributes        IVCmdAttributesW
 #define IID_IVCmdAttributes    IID_IVCmdAttributesW
 #define PIVCMDATTRIBUTES       PIVCMDATTRIBUTESW

#else
 #define IVCmdAttributes        IVCmdAttributesA
 #define IID_IVCmdAttributes    IID_IVCmdAttributesA
 #define PIVCMDATTRIBUTES       PIVCMDATTRIBUTESA

#endif // _S_UNICODE



/*
 *  IVCmdDialog
 */
#undef   INTERFACE
#define  INTERFACE   IVCmdDialogsW

DEFINE_GUID(IID_IVCmdDialogsW, 0xEE39B8A0L, 0x6053, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);

DECLARE_INTERFACE_ (IVCmdDialogsW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // IVCmdDialogs members
   STDMETHOD (AboutDlg)         (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (GeneralDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (LexiconDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainGeneralDlg)  (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainMicDlg)      (THIS_ HWND, PCWSTR) PURE;
};

typedef IVCmdDialogsW FAR * PIVCMDDIALOGSW;


#undef   INTERFACE
#define  INTERFACE   IVCmdDialogsA

// {AA8FE260-5545-11b9-C000-5611722E1D15}
DEFINE_GUID(IID_IVCmdDialogsA, 0xaa8fe260, 0x5545, 0x11b9, 0xc0, 0x0, 0x56, 0x11, 0x72, 0x2e, 0x1d, 0x15);

DECLARE_INTERFACE_ (IVCmdDialogsA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // IVCmdDialogs members
   STDMETHOD (AboutDlg)         (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (GeneralDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (LexiconDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainGeneralDlg)  (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainMicDlg)      (THIS_ HWND, PCSTR) PURE;
};

typedef IVCmdDialogsA FAR * PIVCMDDIALOGSA;


#ifdef _S_UNICODE
 #define IVCmdDialogs       IVCmdDialogsW
 #define IID_IVCmdDialogs   IID_IVCmdDialogsW
 #define PIVCMDDIALOGS      PIVCMDDIALOGSW

#else
 #define IVCmdDialogs       IVCmdDialogsA
 #define IID_IVCmdDialogs   IID_IVCmdDialogsA
 #define PIVCMDDIALOGS      PIVCMDDIALOGSA

#endif // _S_UNICODE



/************************************************************************
class guids */

// DEFINE_GUID(CLSID_VCmd, 0x93898800L, 0x604D, 0x101B, 0x99, 0x26, 0x00, 0xAA, 0x00, 0x3C, 0xFC, 0x2C);
// {6D40D820-0BA7-11ce-A166-00AA004CD65C}
DEFINE_GUID(CLSID_VCmd, 
0x6d40d820, 0xba7, 0x11ce, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);
// {89F70C30-8636-11ce-B763-00AA004CD65C}
DEFINE_GUID(CLSID_SRShare, 
0x89f70c30, 0x8636, 0x11ce, 0xb7, 0x63, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);



/************************************************************************
High-Level dictation speech recognition API
*/

/************************************************************************
defines */
#define     VDCT_TOPICNAMELEN       32

// bit flags for the dwReason field of IVDctNotifySink::TextChanged
#define     VDCT_TEXTADDED          0x00000001
#define     VDCT_TEXTREMOVED        0x00000002
#define     VDCT_TEXTREPLACED       0x00000004

// bit flags for the dwReason field of IVDctText::TextRemove/TextSet
#define     VDCT_TEXTCLEAN          0x00010000
#define     VDCT_TEXTKEEPRESULTS    0x00020000

// bit flags for dwFlags of IVDctGUI::FlagsSet
#define     VDCTGUIF_VISIBLE        0x00000001
#define     VDCTGUIF_DONTMOVE       0x00000002

/************************************************************************
typedefs */

// site information structure - used for IVoiceDictation::SiteInfoGet/Set
typedef struct {
    DWORD   dwAutoGainEnable;
    DWORD   dwAwakeState;
    DWORD   dwThreshold;
    DWORD   dwDevice;
    DWORD   dwEnable;
    WCHAR   szMicrophone[VCMD_MICLEN];
    WCHAR   szSpeaker[VCMD_SPEAKERLEN];
    GUID    gModeID;
} VDSITEINFOW, *PVDSITEINFOW;

typedef struct {
    DWORD   dwAutoGainEnable;
    DWORD   dwAwakeState;
    DWORD   dwThreshold;
    DWORD   dwDevice;
    DWORD   dwEnable;
    CHAR    szMicrophone[VCMD_MICLEN];
    CHAR    szSpeaker[VCMD_SPEAKERLEN];
    GUID    gModeID;
} VDSITEINFOA, *PVDSITEINFOA;


// topic structure used by the IVoiceDictation object
typedef struct {
    WCHAR       szTopic[VDCT_TOPICNAMELEN];
    LANGUAGEW   language;
} VDCTTOPICW, *PVDCTTOPICW;

typedef struct {
    CHAR        szTopic[VDCT_TOPICNAMELEN];
    LANGUAGEA   language;
} VDCTTOPICA, *PVDCTTOPICA;


#ifdef  _S_UNICODE
 #define VDSITEINFO  VDSITEINFOW
 #define PVDSITEINFO PVDSITEINFOW
 #define VDCTTOPIC   VDCTTOPICW
 #define PVDCTTOPIC  PVDCTTOPICW
#else
 #define VDSITEINFO  VDSITEINFOA
 #define PVDSITEINFO PVDSITEINFOA
 #define VDCTTOPIC   VDCTTOPICA
 #define PVDCTTOPIC  PVDCTTOPICA
#endif  // _S_UNICODE


// memory maintenance structure used by MemoryGet/Set in IVDctAttributes
typedef struct {
    DWORD   dwMaxRAM;
    DWORD   dwMaxTime;
    DWORD   dwMaxWords;
    BOOL    fKeepAudio;
    BOOL    fKeepCorrection;
    BOOL    fKeepEval;
} VDCTMEMORY, *PVDCTMEMORY;


// bookmark definition
typedef struct {
    DWORD   dwID;
    DWORD   dwStartChar;
    DWORD   dwNumChars;
} VDCTBOOKMARK, *PVDCTBOOKMARK;



/************************************************************************
interfaces */

/*
 *  IVDctNotifySink
 */
#undef   INTERFACE
#define  INTERFACE   IVDctNotifySinkW

// {5FEB8800-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctNotifySinkW,
0x5feb8800, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctNotifySinkW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctNotifySink members
   STDMETHOD (CommandBuiltIn)       (THIS_ PWSTR) PURE;
   STDMETHOD (CommandOther)         (THIS_ PWSTR) PURE;
   STDMETHOD (CommandRecognize)     (THIS_ DWORD, DWORD, DWORD, PVOID, PWSTR) PURE;
   STDMETHOD (TextSelChanged)       (THIS) PURE;
   STDMETHOD (TextChanged)          (THIS_ DWORD) PURE;
   STDMETHOD (TextBookmarkChanged)  (THIS_ DWORD) PURE;
   STDMETHOD (PhraseStart)          (THIS) PURE;
   STDMETHOD (PhraseFinish)         (THIS_ DWORD, PSRPHRASEW) PURE;
   STDMETHOD (PhraseHypothesis)     (THIS_ DWORD, PSRPHRASEW) PURE;
   STDMETHOD (UtteranceBegin)       (THIS) PURE;
   STDMETHOD (UtteranceEnd)         (THIS) PURE;
   STDMETHOD (VUMeter)              (THIS_ WORD) PURE;
   STDMETHOD (AttribChanged)        (THIS_ DWORD) PURE;
   STDMETHOD (Interference)         (THIS_ DWORD) PURE;
   STDMETHOD (Training)             (THIS_ DWORD) PURE;
   STDMETHOD (Dictating)            (THIS_ PCWSTR, BOOL) PURE;
};

typedef IVDctNotifySinkW FAR * PIVDCTNOTIFYSINKW;


#undef   INTERFACE
#define  INTERFACE   IVDctNotifySinkA

// {88AD7DC0-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctNotifySinkA,
0x88ad7dc0, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctNotifySinkA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctNotifySinkA members
   STDMETHOD (CommandBuiltIn)       (THIS_ PSTR) PURE;
   STDMETHOD (CommandOther)         (THIS_ PSTR) PURE;
   STDMETHOD (CommandRecognize)     (THIS_ DWORD, DWORD, DWORD, PVOID, PSTR) PURE;
   STDMETHOD (TextSelChanged)       (THIS) PURE;
   STDMETHOD (TextChanged)          (THIS_ DWORD) PURE;
   STDMETHOD (TextBookmarkChanged)  (THIS_ DWORD) PURE;
   STDMETHOD (PhraseStart)          (THIS) PURE;
   STDMETHOD (PhraseFinish)         (THIS_ DWORD, PSRPHRASEA) PURE;
   STDMETHOD (PhraseHypothesis)     (THIS_ DWORD, PSRPHRASEA) PURE;
   STDMETHOD (UtteranceBegin)       (THIS) PURE;
   STDMETHOD (UtteranceEnd)         (THIS) PURE;
   STDMETHOD (VUMeter)              (THIS_ WORD) PURE;
   STDMETHOD (AttribChanged)        (THIS_ DWORD) PURE;
   STDMETHOD (Interference)         (THIS_ DWORD) PURE;
   STDMETHOD (Training)             (THIS_ DWORD) PURE;
   STDMETHOD (Dictating)            (THIS_ PCSTR, BOOL) PURE;
};

typedef IVDctNotifySinkA FAR * PIVDCTNOTIFYSINKA;


#ifdef _S_UNICODE
 #define IVDctNotifySink        IVDctNotifySinkW
 #define IID_IVDctNotifySink    IID_IVDctNotifySinkW
 #define PIVDCTNOTIFYSINK       PIVDCTNOTIFYSINKW

#else
 #define IVDctNotifySink        IVDctNotifySinkA
 #define IID_IVDctNotifySink    IID_IVDctNotifySinkA
 #define PIVDCTNOTIFYSINK       PIVDCTNOTIFYSINKA

#endif // _S_UNICODE



 
/*
 *  IVoiceDictation
 */
#undef   INTERFACE
#define  INTERFACE   IVoiceDictationW

// {88AD7DC3-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVoiceDictationW,
0x88ad7dc3, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVoiceDictationW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVoiceDictation members
   STDMETHOD (Register)         (THIS_ PCWSTR, PCWSTR, LPSTORAGE, PCWSTR,
				 PIVDCTNOTIFYSINK, GUID, DWORD) PURE;
   STDMETHOD (SiteInfoGet)      (THIS_ PCWSTR, PVDSITEINFOW) PURE;
   STDMETHOD (SiteInfoSet)      (THIS_ PCWSTR, PVDSITEINFOW) PURE;
   STDMETHOD (SessionSerialize) (THIS_ LPSTORAGE) PURE;
   STDMETHOD (TopicEnum)        (THIS_ PVDCTTOPICW *, DWORD *) PURE;
   STDMETHOD (TopicAdd)         (THIS_ PCWSTR, LANGUAGEW *, PCWSTR *, PCWSTR *,
				 PCWSTR *, PCWSTR *) PURE;
   STDMETHOD (TopicRemove)      (THIS_ PCWSTR) PURE;
   STDMETHOD (TopicSerialize)   (THIS_ LPSTORAGE) PURE;
   STDMETHOD (TopicDeserialize) (THIS_ LPSTORAGE) PURE;
   STDMETHOD (Activate)         (THIS_ HWND) PURE;
   STDMETHOD (Deactivate)       (THIS) PURE;
   STDMETHOD (TopicAddGrammar)  (THIS_ PCWSTR, SDATA) PURE;
};

typedef IVoiceDictationW FAR * PIVOICEDICTATIONW;


#undef   INTERFACE
#define  INTERFACE   IVoiceDictationA

// {88AD7DC4-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVoiceDictationA,
0x88ad7dc4, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVoiceDictationA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVoiceDictation members
   STDMETHOD (Register)         (THIS_ PCSTR, PCSTR, LPSTORAGE, PCSTR,
				 PIVDCTNOTIFYSINK, GUID, DWORD) PURE;
   STDMETHOD (SiteInfoGet)      (THIS_ PCSTR, PVDSITEINFOA) PURE;
   STDMETHOD (SiteInfoSet)      (THIS_ PCSTR, PVDSITEINFOA) PURE;
   STDMETHOD (SessionSerialize) (THIS_ LPSTORAGE) PURE;
   STDMETHOD (TopicEnum)        (THIS_ PVDCTTOPICA *, DWORD *) PURE;
   STDMETHOD (TopicAdd)         (THIS_ PCSTR, LANGUAGEA *, PCSTR *, PCSTR *,
				 PCSTR *, PCSTR *) PURE;
   STDMETHOD (TopicRemove)      (THIS_ PCSTR) PURE;
   STDMETHOD (TopicSerialize)   (THIS_ LPSTORAGE) PURE;
   STDMETHOD (TopicDeserialize) (THIS_ LPSTORAGE) PURE;
   STDMETHOD (Activate)         (THIS_ HWND) PURE;
   STDMETHOD (Deactivate)       (THIS) PURE;
   STDMETHOD (TopicAddGrammar)  (THIS_ PCSTR, SDATA) PURE;
};

typedef IVoiceDictationA FAR * PIVOICEDICTATIONA;


#ifdef _S_UNICODE
 #define IVoiceDictation     IVoiceDictationW
 #define IID_IVoiceDictation IID_IVoiceDictationW
 #define PIVOICEDICTATION    PIVOICEDICTATIONW

#else
 #define IVoiceDictation     IVoiceDictationA
 #define IID_IVoiceDictation IID_IVoiceDictationA
 #define PIVOICEDICTATION    PIVOICEDICTATIONA

#endif //_S_UNICODE



/*
 *  IVDctText
 */
#undef   INTERFACE
#define  INTERFACE   IVDctTextW

// {6D62B3A0-6893-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctTextW,
0x6d62b3a0, 0x6893, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctTextW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctText members
   STDMETHOD (Lock)             (THIS) PURE;
   STDMETHOD (UnLock)           (THIS) PURE;
   STDMETHOD (TextGet)          (THIS_ DWORD, DWORD, PSDATA) PURE;
   STDMETHOD (TextSet)          (THIS_ DWORD, DWORD, PCWSTR, DWORD) PURE;
   STDMETHOD (TextMove)         (THIS_ DWORD, DWORD, DWORD, DWORD) PURE;
   STDMETHOD (TextRemove)       (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (TextSelSet)       (THIS_ DWORD, DWORD) PURE;
   STDMETHOD (TextSelGet)       (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD (TextLengthGet)    (THIS_ DWORD *) PURE;
   STDMETHOD (GetChanges)       (THIS_ DWORD *, DWORD *, DWORD *, DWORD *) PURE;
   STDMETHOD (BookmarkAdd)      (THIS_ PVDCTBOOKMARK) PURE;
   STDMETHOD (BookmarkRemove)   (THIS_ DWORD) PURE;
   STDMETHOD (BookmarkMove)     (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (BookmarkQuery)    (THIS_ DWORD, PVDCTBOOKMARK) PURE;
   STDMETHOD (BookmarkEnum)     (THIS_ DWORD, DWORD, PVDCTBOOKMARK *,
				       DWORD *) PURE;
   STDMETHOD (Hint)             (THIS_ PCWSTR) PURE;
   STDMETHOD (Words)            (THIS_ PCWSTR) PURE;
   STDMETHOD (ResultsGet)       (THIS_ DWORD, DWORD, DWORD *, DWORD *,
				       LPUNKNOWN *) PURE;
};
typedef IVDctTextW FAR * PIVDCTTEXTW;


#undef   INTERFACE
#define  INTERFACE   IVDctTextA

// {6D62B3A1-6893-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctTextA,
0x6d62b3a1, 0x6893, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctTextA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctText members
   STDMETHOD (Lock)             (THIS) PURE;
   STDMETHOD (UnLock)           (THIS) PURE;
   STDMETHOD (TextGet)          (THIS_ DWORD, DWORD, PSDATA) PURE;
   STDMETHOD (TextSet)          (THIS_ DWORD, DWORD, PCSTR, DWORD) PURE;
   STDMETHOD (TextMove)         (THIS_ DWORD, DWORD, DWORD, DWORD) PURE;
   STDMETHOD (TextRemove)       (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (TextSelSet)       (THIS_ DWORD, DWORD) PURE;
   STDMETHOD (TextSelGet)       (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD (TextLengthGet)    (THIS_ DWORD *) PURE;
   STDMETHOD (GetChanges)       (THIS_ DWORD *, DWORD *, DWORD *, DWORD *) PURE;
   STDMETHOD (BookmarkAdd)      (THIS_ PVDCTBOOKMARK) PURE;
   STDMETHOD (BookmarkRemove)   (THIS_ DWORD) PURE;
   STDMETHOD (BookmarkMove)     (THIS_ DWORD, DWORD, DWORD) PURE;
   STDMETHOD (BookmarkQuery)    (THIS_ DWORD, PVDCTBOOKMARK) PURE;
   STDMETHOD (BookmarkEnum)     (THIS_ DWORD, DWORD, PVDCTBOOKMARK *,
				       DWORD *) PURE;
   STDMETHOD (Hint)             (THIS_ PCSTR) PURE;
   STDMETHOD (Words)            (THIS_ PCSTR) PURE;
   STDMETHOD (ResultsGet)       (THIS_ DWORD, DWORD, DWORD *, DWORD *,
				       LPUNKNOWN *) PURE;
};
typedef IVDctTextA FAR * PIVDCTTEXTA;


#ifdef _S_UNICODE
 #define IVDctText      IVDctTextW
 #define IID_IVDctText  IID_IVDctTextW
 #define PIVDCTTEXT     PIVDCTTEXTW

#else
 #define IVDctText      IVDctTextA
 #define IID_IVDctText  IID_IVDctTextA
 #define PIVDCTTEXT     PIVDCTTEXTA

#endif //_S_UNICODE



/*
 *  IVDctAttributes
 */
#undef   INTERFACE
#define  INTERFACE   IVDctAttributesW

// {88AD7DC5-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctAttributesW,
0x88ad7dc5, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctAttributesW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctAttributes members
   STDMETHOD    (AutoGainEnableGet)  (THIS_ DWORD *) PURE;
   STDMETHOD    (AutoGainEnableSet)  (THIS_ DWORD) PURE;
   STDMETHOD    (AwakeStateGet)      (THIS_ DWORD *) PURE;
   STDMETHOD    (AwakeStateSet)      (THIS_ DWORD) PURE;
   STDMETHOD    (ThresholdGet)       (THIS_ DWORD *) PURE;
   STDMETHOD    (ThresholdSet)       (THIS_ DWORD) PURE;
   STDMETHOD    (EchoGet)            (THIS_ BOOL *) PURE;
   STDMETHOD    (EchoSet)            (THIS_ BOOL) PURE;
   STDMETHOD    (EnergyFloorGet)     (THIS_ WORD *) PURE;
   STDMETHOD    (EnergyFloorSet)     (THIS_ WORD) PURE;
   STDMETHOD    (RealTimeGet)        (THIS_ DWORD *) PURE;
   STDMETHOD    (RealTimeSet)        (THIS_ DWORD) PURE;
   STDMETHOD    (TimeOutGet)         (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD    (TimeOutSet)         (THIS_ DWORD, DWORD) PURE;
   STDMETHOD    (EnabledGet)         (THIS_ DWORD *) PURE;
   STDMETHOD    (EnabledSet)         (THIS_ DWORD) PURE;
   STDMETHOD    (BuiltInFeaturesGet) (THIS_ DWORD *) PURE;
   STDMETHOD    (BuiltInFeaturesSet) (THIS_ DWORD) PURE;
   STDMETHOD    (MemoryGet)          (THIS_ VDCTMEMORY *) PURE;
   STDMETHOD    (MemorySet)          (THIS_ VDCTMEMORY *) PURE;
   STDMETHOD    (DictatingNowGet)    (THIS_ DWORD *) PURE;
   STDMETHOD    (DictatingNowSet)    (THIS_ DWORD) PURE;
   STDMETHOD    (IsAnyoneDictating)  (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD    (MicrophoneGet)      (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD    (MicrophoneSet)      (THIS_ PCWSTR) PURE;
   STDMETHOD    (SpeakerGet)         (THIS_ PWSTR, DWORD, DWORD *) PURE;
   STDMETHOD    (SpeakerSet)         (THIS_ PCWSTR) PURE;
};

typedef IVDctAttributesW FAR * PIVDCTATTRIBUTESW;


#undef   INTERFACE
#define  INTERFACE   IVDctAttributesA

// {88AD7DC6-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctAttributesA,
0x88ad7dc6, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctAttributesA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctAttributes members
   STDMETHOD    (AutoGainEnableGet)  (THIS_ DWORD *) PURE;
   STDMETHOD    (AutoGainEnableSet)  (THIS_ DWORD) PURE;
   STDMETHOD    (AwakeStateGet)      (THIS_ DWORD *) PURE;
   STDMETHOD    (AwakeStateSet)      (THIS_ DWORD) PURE;
   STDMETHOD    (ThresholdGet)       (THIS_ DWORD *) PURE;
   STDMETHOD    (ThresholdSet)       (THIS_ DWORD) PURE;
   STDMETHOD    (EchoGet)            (THIS_ BOOL *) PURE;
   STDMETHOD    (EchoSet)            (THIS_ BOOL) PURE;
   STDMETHOD    (EnergyFloorGet)     (THIS_ WORD *) PURE;
   STDMETHOD    (EnergyFloorSet)     (THIS_ WORD) PURE;
   STDMETHOD    (RealTimeGet)        (THIS_ DWORD *) PURE;
   STDMETHOD    (RealTimeSet)        (THIS_ DWORD) PURE;
   STDMETHOD    (TimeOutGet)         (THIS_ DWORD *, DWORD *) PURE;
   STDMETHOD    (TimeOutSet)         (THIS_ DWORD, DWORD) PURE;
   STDMETHOD    (EnabledGet)         (THIS_ DWORD *) PURE;
   STDMETHOD    (EnabledSet)         (THIS_ DWORD) PURE;
   STDMETHOD    (BuiltInFeaturesGet) (THIS_ DWORD *) PURE;
   STDMETHOD    (BuiltInFeaturesSet) (THIS_ DWORD) PURE;
   STDMETHOD    (MemoryGet)          (THIS_ VDCTMEMORY *) PURE;
   STDMETHOD    (MemorySet)          (THIS_ VDCTMEMORY *) PURE;
   STDMETHOD    (DictatingNowGet)    (THIS_ DWORD *) PURE;
   STDMETHOD    (DictatingNowSet)    (THIS_ DWORD) PURE;
   STDMETHOD    (IsAnyoneDictating)  (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD    (MicrophoneGet)      (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD    (MicrophoneSet)      (THIS_ PCSTR) PURE;
   STDMETHOD    (SpeakerGet)         (THIS_ PSTR, DWORD, DWORD *) PURE;
   STDMETHOD    (SpeakerSet)         (THIS_ PCSTR) PURE;
};

typedef IVDctAttributesA FAR * PIVDCTATTRIBUTESA;


#ifdef _S_UNICODE
 #define IVDctAttributes        IVDctAttributesW
 #define IID_IVDctAttributes    IID_IVDctAttributesW
 #define PIVDCTATTRIBUTES       PIVDCTATTRIBUTESW

#else
 #define IVDctAttributes        IVDctAttributesA
 #define IID_IVDctAttributes    IID_IVDctAttributesA
 #define PIVDCTATTRIBUTES       PIVDCTATTRIBUTESA

#endif // _S_UNICODE



/*
 *  IVDctCommands
 */
#undef   INTERFACE
#define  INTERFACE   IVDctCommandsW

// {A02C2CA0-AE50-11cf-833A-00AA00A21A29}
DEFINE_GUID(IID_IVDctCommandsW,
0xA02C2CA0, 0xAE50, 0x11cf, 0x83, 0x3A, 0x00, 0xAA, 0x00, 0xA2, 0x1A, 0x29);

DECLARE_INTERFACE_ (IVDctCommandsW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctCommands members
   STDMETHOD    (Add)      (THIS_ BOOL, DWORD, SDATA, DWORD *) PURE;
   STDMETHOD    (Remove)   (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Get)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA*, DWORD *) PURE;
   STDMETHOD    (Set)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA) PURE;
   STDMETHOD    (EnableItem) (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Num)      (THIS_ BOOL, DWORD *) PURE;
};

typedef IVDctCommandsW FAR * PIVDCTCOMMANDSW;


#undef   INTERFACE
#define  INTERFACE   IVDctCommandsA

// {A02C2CA1-AE50-11cf-833A-00AA00A21A29}
DEFINE_GUID(IID_IVDctCommandsA,
0xA02C2CA1, 0xAE50, 0x11cf, 0x83, 0x3A, 0x00, 0xAA, 0x00, 0xA2, 0x1A, 0x29);

DECLARE_INTERFACE_ (IVDctCommandsA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctCommands members
   STDMETHOD    (Add)      (THIS_ BOOL, DWORD, SDATA, DWORD *) PURE;
   STDMETHOD    (Remove)   (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Get)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA*, DWORD *) PURE;
   STDMETHOD    (Set)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA) PURE;
   STDMETHOD    (EnableItem) (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Num)      (THIS_ BOOL, DWORD *) PURE;
};

typedef IVDctCommandsA FAR * PIVDCTCOMMANDSA;


#ifdef _S_UNICODE
 #define IVDctCommands        IVDctCommandsW
 #define IID_IVDctCommands    IID_IVDctCommandsW
 #define PIVDCTCOMMANDS       PIVDCTCOMMANDSW

#else
 #define IVDctCommands        IVDctCommandsA
 #define IID_IVDctCommands    IID_IVDctCommandsA
 #define PIVDCTCOMMANDS       PIVDCTCOMMANDSA

#endif // _S_UNICODE



/*
 *  IVDctGlossary
 */
#undef   INTERFACE
#define  INTERFACE   IVDctGlossaryW

// {A02C2CA2-AE50-11cf-833A-00AA00A21A29}
DEFINE_GUID(IID_IVDctGlossaryW,
0xA02C2CA2, 0xAE50, 0x11cf, 0x83, 0x3A, 0x00, 0xAA, 0x00, 0xA2, 0x1A, 0x29);

DECLARE_INTERFACE_ (IVDctGlossaryW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctGlossary members
   STDMETHOD    (Add)      (THIS_ BOOL, DWORD, SDATA, DWORD *) PURE;
   STDMETHOD    (Remove)   (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Get)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA*, DWORD *) PURE;
   STDMETHOD    (Set)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA) PURE;
   STDMETHOD    (EnableItem) (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Num)      (THIS_ BOOL, DWORD *) PURE;
};

typedef IVDctGlossaryW FAR * PIVDCTGLOSSARYW;


#undef   INTERFACE
#define  INTERFACE   IVDctGlossaryA

// {A02C2CA3-AE50-11cf-833A-00AA00A21A29}
DEFINE_GUID(IID_IVDctGlossaryA,
0xA02C2CA3, 0xAE50, 0x11cf, 0x83, 0x3A, 0x00, 0xAA, 0x00, 0xA2, 0x1A, 0x29);

DECLARE_INTERFACE_ (IVDctGlossaryA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVDctGlossary members
   STDMETHOD    (Add)      (THIS_ BOOL, DWORD, SDATA, DWORD *) PURE;
   STDMETHOD    (Remove)   (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Get)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA*, DWORD *) PURE;
   STDMETHOD    (Set)      (THIS_ BOOL, DWORD, DWORD, DWORD, SDATA) PURE;
   STDMETHOD    (EnableItem) (THIS_ BOOL, DWORD, DWORD, DWORD) PURE;
   STDMETHOD    (Num)      (THIS_ BOOL, DWORD *) PURE;
};

typedef IVDctGlossaryA FAR * PIVDCTGLOSSARYA;


#ifdef _S_UNICODE
 #define IVDctGlossary        IVDctGlossaryW
 #define IID_IVDctGlossary    IID_IVDctGlossaryW
 #define PIVDCTGLOSSARY       PIVDCTGLOSSARYW

#else
 #define IVDctGlossary        IVDctGlossaryA
 #define IID_IVDctGlossary    IID_IVDctGlossaryA
 #define PIVDCTGLOSSARY       PIVDCTGLOSSARYA

#endif // _S_UNICODE




/*
 *  IVDctDialog
 */
#undef   INTERFACE
#define  INTERFACE   IVDctDialogsW

// {88AD7DC7-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctDialogsW,
0x88ad7dc7, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctDialogsW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // IVDctDialogs members
   STDMETHOD (AboutDlg)         (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (GeneralDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (LexiconDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainGeneralDlg)  (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TrainMicDlg)      (THIS_ HWND, PCWSTR) PURE;
};

typedef IVDctDialogsW FAR * PIVDCTDIALOGSW;


#undef   INTERFACE
#define  INTERFACE   IVDctDialogsA

// {88AD7DC8-67D5-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(IID_IVDctDialogsA,
0x88ad7dc8, 0x67d5, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);

DECLARE_INTERFACE_ (IVDctDialogsA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // IVDctDialogs members
   STDMETHOD (AboutDlg)         (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (GeneralDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (LexiconDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainGeneralDlg)  (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TrainMicDlg)      (THIS_ HWND, PCSTR) PURE;
};

typedef IVDctDialogsA FAR * PIVDCTDIALOGSA;


#ifdef _S_UNICODE
 #define IVDctDialogs       IVDctDialogsW
 #define IID_IVDctDialogs   IID_IVDctDialogsW
 #define PIVDCTDIALOGS      PIVDCTDIALOGSW

#else
 #define IVDctDialogs       IVDctDialogsA
 #define IID_IVDctDialogs   IID_IVDctDialogsA
 #define PIVDCTDIALOGS      PIVDCTDIALOGSA

#endif // _S_UNICODE


#undef   INTERFACE
#define  INTERFACE   IVDctGUI

// {8953F1A0-7E80-11cf-8D15-00A0C9034A7E}
DEFINE_GUID(IID_IVDctGUI,
0x8953f1a0, 0x7e80, 0x11cf, 0x8d, 0x15, 0x0, 0xa0, 0xc9, 0x3, 0x4a, 0x7e);

DECLARE_INTERFACE_ (IVDctGUI, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)    (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)     (THIS) PURE;
   STDMETHOD_(ULONG,Release)    (THIS) PURE;

   // IVDctDialogs members
   STDMETHOD (SetSelRect)       (THIS_ RECT *) PURE;
   STDMETHOD (FlagsSet)         (THIS_ DWORD) PURE;
   STDMETHOD (FlagsGet)         (THIS_ DWORD *) PURE;
};

typedef IVDctGUI FAR * PIVDCTGUI;



/************************************************************************
class guids */

// {25522CA0-67CE-11cf-9B8B-08005AFC3A41}
DEFINE_GUID(CLSID_VDct, 
0x25522ca0, 0x67ce, 0x11cf, 0x9b, 0x8b, 0x8, 0x0, 0x5a, 0xfc, 0x3a, 0x41);



/************************************************************************
High-Level text-to-speech API
*/


/************************************************************************
defines */

#define  ONE                    (1)

// dwFlags parameter of IVoiceText::Register
#define  VTXTF_ALLMESSAGES      (ONE<<0)

/*
 *   dwFlags parameter of IVoiceText::Speak
 */

// type of speech
#define  VTXTST_STATEMENT       0x00000001
#define  VTXTST_QUESTION        0x00000002
#define  VTXTST_COMMAND         0x00000004
#define  VTXTST_WARNING         0x00000008
#define  VTXTST_READING         0x00000010
#define  VTXTST_NUMBERS         0x00000020
#define  VTXTST_SPREADSHEET     0x00000040

// priorities
#define  VTXTSP_VERYHIGH        0x00000080
#define  VTXTSP_HIGH            0x00000100
#define  VTXTSP_NORMAL          0x00000200

/************************************************************************
typedefs */

// possible parameter to IVoiceText::Register
typedef struct {
    DWORD   dwDevice;
    DWORD   dwEnable;
    DWORD   dwSpeed;
    GUID    gModeID;
} VTSITEINFO, *PVTSITEINFO;


/************************************************************************
interfaces */

/*
 *  IVCmdNotifySink
 */
#undef   INTERFACE
#define  INTERFACE   IVTxtNotifySinkW

// {FD3A2430-E090-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVTxtNotifySinkW, 0xfd3a2430, 0xe090, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVTxtNotifySinkW, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVTxtNotifySinkW members
   STDMETHOD (AttribChanged)    (THIS_ DWORD) PURE;
   STDMETHOD (Visual)           (THIS_ WCHAR, WCHAR, DWORD, PTTSMOUTH) PURE;
   STDMETHOD (Speak)            (THIS_ PWSTR, PWSTR, DWORD) PURE;
   STDMETHOD (SpeakingStarted)  (THIS) PURE;
   STDMETHOD (SpeakingDone)     (THIS) PURE;
};

typedef IVTxtNotifySinkW FAR * PIVTXTNOTIFYSINKW;


#undef   INTERFACE
#define  INTERFACE   IVTxtNotifySinkA

// {D2C840E0-E092-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVTxtNotifySinkA, 0xd2c840e0, 0xe092, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVTxtNotifySinkA, IUnknown) {

   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVTxtNotifySinkA members
   STDMETHOD (AttribChanged)    (THIS_ DWORD) PURE;
   STDMETHOD (Visual)           (THIS_ WCHAR, CHAR, DWORD, PTTSMOUTH) PURE;
   STDMETHOD (Speak)            (THIS_ PSTR, PSTR, DWORD) PURE;
   STDMETHOD (SpeakingStarted)  (THIS) PURE;
   STDMETHOD (SpeakingDone)     (THIS) PURE;
};

typedef IVTxtNotifySinkA FAR * PIVTXTNOTIFYSINKA;


#ifdef _S_UNICODE
 #define IVTxtNotifySink        IVTxtNotifySinkW
 #define IID_IVTxtNotifySink    IID_IVTxtNotifySinkW
 #define PIVTXTNOTIFYSINK       PIVTXTNOTIFYSINKW

#else
 #define IVTxtNotifySink        IVTxtNotifySinkA
 #define IID_IVTxtNotifySink    IID_IVTxtNotifySinkA
 #define PIVTXTNOTIFYSINK       PIVTXTNOTIFYSINKA

#endif // _S_UNICODE



/*
 *  IVoiceText
 */
#undef   INTERFACE
#define  INTERFACE   IVoiceTextW

// {C4FE8740-E093-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVoiceTextW, 0xc4fe8740, 0xe093, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVoiceTextW, IUnknown) {
    // IUnknown members
    STDMETHOD(QueryInterface)   (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG,Release)   (THIS) PURE;

    // IVoiceText members

    STDMETHOD (Register)         (THIS_ PCWSTR, PCWSTR, 
					PIVTXTNOTIFYSINK, GUID,
					DWORD, PVTSITEINFO) PURE;
    STDMETHOD (Speak)            (THIS_ PCWSTR, DWORD, PCWSTR) PURE;
    STDMETHOD (StopSpeaking)     (THIS) PURE;
    STDMETHOD (AudioFastForward) (THIS) PURE;
    STDMETHOD (AudioPause)       (THIS) PURE;
    STDMETHOD (AudioResume)      (THIS) PURE;
    STDMETHOD (AudioRewind)      (THIS) PURE;
};

typedef IVoiceTextW FAR * PIVOICETEXTW;


#undef   INTERFACE
#define  INTERFACE   IVoiceTextA

// {E1B7A180-E093-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVoiceTextA, 0xe1b7a180, 0xe093, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVoiceTextA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVoiceText members

    STDMETHOD (Register)         (THIS_ PCSTR, PCSTR, 
					PIVTXTNOTIFYSINK, GUID,
					DWORD, PVTSITEINFO) PURE;
    STDMETHOD (Speak)            (THIS_ PCSTR, DWORD, PCSTR) PURE;
    STDMETHOD (StopSpeaking)     (THIS) PURE;
    STDMETHOD (AudioFastForward) (THIS) PURE;
    STDMETHOD (AudioPause)       (THIS) PURE;
    STDMETHOD (AudioResume)      (THIS) PURE;
    STDMETHOD (AudioRewind)      (THIS) PURE;
};

typedef IVoiceTextA FAR * PIVOICETEXTA;


#ifdef _S_UNICODE
 #define IVoiceText      IVoiceTextW
 #define IID_IVoiceText  IID_IVoiceTextW
 #define PIVOICETEXT     PIVOICETEXTW

#else
 #define IVoiceText      IVoiceTextA
 #define IID_IVoiceText  IID_IVoiceTextA
 #define PIVOICETEXT     PIVOICETEXTA

#endif //_S_UNICODE



/*
 *  IVTxtAttributes
 */
#undef   INTERFACE
#define  INTERFACE   IVTxtAttributesW

// {6A8D6140-E095-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVTxtAttributesW, 0x6a8d6140, 0xe095, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVTxtAttributesW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVTxtAttributes members
   STDMETHOD (DeviceGet)         (THIS_ DWORD *) PURE;
   STDMETHOD (DeviceSet)         (THIS_ DWORD) PURE;
   STDMETHOD (EnabledGet)        (THIS_ DWORD *) PURE;
   STDMETHOD (EnabledSet)        (THIS_ DWORD) PURE;
   STDMETHOD (IsSpeaking)        (THIS_ BOOL *) PURE;
   STDMETHOD (SpeedGet)          (THIS_ DWORD *) PURE;
   STDMETHOD (SpeedSet)          (THIS_ DWORD) PURE;
   STDMETHOD (TTSModeGet)        (THIS_ GUID *) PURE;
   STDMETHOD (TTSModeSet)        (THIS_ GUID) PURE;
};

typedef IVTxtAttributesW FAR * PIVTXTATTRIBUTESW;


#undef   INTERFACE
#define  INTERFACE   IVTxtAttributesA

// {8BE9CC30-E095-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVTxtAttributesA, 0x8be9cc30, 0xe095, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVTxtAttributesA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVTxtAttributes members
   STDMETHOD (DeviceGet)         (THIS_ DWORD *) PURE;
   STDMETHOD (DeviceSet)         (THIS_ DWORD) PURE;
   STDMETHOD (EnabledGet)        (THIS_ DWORD *) PURE;
   STDMETHOD (EnabledSet)        (THIS_ DWORD) PURE;
   STDMETHOD (IsSpeaking)        (THIS_ BOOL *) PURE;
   STDMETHOD (SpeedGet)          (THIS_ DWORD *) PURE;
   STDMETHOD (SpeedSet)          (THIS_ DWORD) PURE;
   STDMETHOD (TTSModeGet)        (THIS_ GUID *) PURE;
   STDMETHOD (TTSModeSet)        (THIS_ GUID) PURE;
};

typedef IVTxtAttributesA FAR * PIVTXTATTRIBUTESA;


#ifdef _S_UNICODE
 #define IVTxtAttributes        IVTxtAttributesW
 #define IID_IVTxtAttributes    IID_IVTxtAttributesW
 #define PIVTXTATTRIBUTES       PIVTXTATTRIBUTESW

#else
 #define IVTxtAttributes        IVTxtAttributesA
 #define IID_IVTxtAttributes    IID_IVTxtAttributesA
 #define PIVTXTATTRIBUTES       PIVTXTATTRIBUTESA

#endif // _S_UNICODE



/*
 *  IVTxtDialog
 */
#undef   INTERFACE
#define  INTERFACE   IVTxtDialogsW

// {D6469210-E095-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVTxtDialogsW, 0xd6469210, 0xe095, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVTxtDialogsW, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVTxtDialogs members

   STDMETHOD (AboutDlg)       (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (LexiconDlg)     (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (GeneralDlg)     (THIS_ HWND, PCWSTR) PURE;
   STDMETHOD (TranslateDlg)   (THIS_ HWND, PCWSTR) PURE;
};

typedef IVTxtDialogsW FAR * PIVTXTDIALOGSW;


#undef   INTERFACE
#define  INTERFACE   IVTxtDialogsA

// {E8F6FA20-E095-11cd-A166-00AA004CD65C}
DEFINE_GUID(IID_IVTxtDialogsA, 0xe8f6fa20, 0xe095, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);

DECLARE_INTERFACE_ (IVTxtDialogsA, IUnknown) {
   // IUnknown members
   STDMETHOD(QueryInterface)  (THIS_ REFIID riid, LPVOID FAR* ppvObj) PURE;
   STDMETHOD_(ULONG,AddRef)   (THIS) PURE;
   STDMETHOD_(ULONG,Release)  (THIS) PURE;

   // IVTxtDialogs members
   STDMETHOD (AboutDlg)       (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (LexiconDlg)     (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (GeneralDlg)     (THIS_ HWND, PCSTR) PURE;
   STDMETHOD (TranslateDlg)   (THIS_ HWND, PCSTR) PURE;
};

typedef IVTxtDialogsA FAR * PIVTXTDIALOGSA;


#ifdef _S_UNICODE
 #define IVTxtDialogs       IVTxtDialogsW
 #define IID_IVTxtDialogs   IID_IVTxtDialogsW
 #define PIVTXTDIALOGS      PIVTXTDIALOGSW

#else
 #define IVTxtDialogs       IVTxtDialogsA
 #define IID_IVTxtDialogs   IID_IVTxtDialogsA
 #define PIVTXTDIALOGS      PIVTXTDIALOGSA

#endif // _S_UNICODE



/************************************************************************
class guids */

// {080EB9D0-E096-11cd-A166-00AA004CD65C}
// DEFINE_GUID(CLSID_VTxt, 0x80eb9d0, 0xe096, 0x11cd, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);
// {F1DC95A0-0BA7-11ce-A166-00AA004CD65C}
DEFINE_GUID(CLSID_VTxt, 
0xf1dc95a0, 0xba7, 0x11ce, 0xa1, 0x66, 0x0, 0xaa, 0x0, 0x4c, 0xd6, 0x5c);



#endif    // _SPEECH_
