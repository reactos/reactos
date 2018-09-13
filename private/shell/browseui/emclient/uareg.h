//***   uareg.h -- UAssist registry settings and flags
//

//***   SZ_* -- registry locations
//
#define SZ_UASSIST          TEXT("UserAssist")
#define SZ_UASSIST2         TEXT("UserAssist2")
  #define SZ_SETTINGS         TEXT("Settings")
    #define SZ_SESSTIME         TEXT("SessionTime")
    #define SZ_IDLETIME         TEXT("IdleTime")
    #define SZ_CLEANTIME        TEXT("CleanupTime")
    #define SZ_NOPURGE          TEXT("NoPurge")     // (debug) don't nuke 0s
    #define SZ_BACKUP           TEXT("Backup")      // (debug) simulate deletes
    #define SZ_NOLOG            TEXT("NoLog")
    #define SZ_INSTRUMENT       TEXT("Instrument")
    #define SZ_NOENCRYPT        TEXT("NoEncrypt")   // (debug) don't encrypt
//{guid}
    #define SZ_UAVERSION      TEXT("Version")
    #define SZ_COUNT          TEXT("Count")
//    #define SZ_CTLSESSION     TEXT("UEME_CTLSESSION")
//    #define SZ_CUACount_ctor  TEXT("UEME_CTLCUACount:ctor")

//***   UA*F_* -- flags
// standard, shared by some of {CUserAssist,CUADbase,CUACount}
#define UAXF_NOPURGE    0x01000000
#define UAXF_BACKUP     0x02000000
#define UAXF_NOENCRYPT  0x04000000
#define UAXF_NODECAY    0x08000000
#define UAXF_RESERVED2  0x10000000
#define UAXF_RESERVED3  0x20000000
#define UAXF_RESERVED4  0x40000000
#define UAXF_RESERVED5  0x80000000

#define UAXF_XMASK      0xff000000

// for CUserAssist API
#define UAAF_NOLOG      0x01
#define UAAF_INSTR      0x02

// for CUADbase
#define UADF_UNUSED     0x01

// for CUACount
#define UACF_UNUSED     0x01


//***
//
#define UEMIND_NINSTR   0
#ifdef UAAF_INSTR
#undef  UEMIND_NINSTR
#define UEMIND_NINSTR   2

#define UEMIND_SHELL2   (UEMIND_SHELL + UEMIND_NINSTR)
#define UEMIND_BROWSER2 (UEMIND_BROWSER + UEMIND_NINSTR)
#endif

#define IND_NONINSTR(iGrp) \
    (!IND_ISINSTR(iGrp) ? (iGrp) : ((iGrp) - UEMIND_NINSTR))
#define IND_ISINSTR(iGrp)   (UEMIND_NINSTR && iSvr >= UEMIND_NINSTR)
