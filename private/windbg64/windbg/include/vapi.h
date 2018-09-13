/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/***    gda_t -- global data area
 * FIELDS
 *  gda_batflg  batch mode compile
 *  gda_wExit   VAP exit status
 *  gda_wRet    VAP API extra return value
 *  gda_lRet    VAP API extra return value
 *  gda_syserr  analog to errno for VAPI, etc.
 *  gda_fdErr   VIO fd for error 'file'
 *  gda_lpszC1Path  pointer to C1.ERR path spec
 *  gda_lpbInclude  far pointer to INCLUDE path
 *  gda_lpQcini far pointer to QCINI struct
 *  gda_fAsFeed Feedback from the assembler (flags)
 *
 * BUGS
 *  M00WARN gda_s is in vapi.h and vapi.inc.
 */
struct gda_s {
    uint        gda_batflg:1;   /* batch mode compile */
    uint        gda_CcO1:1; /* -O1 */
    uint        gda_fInVap:1;   /* Vap currently active? */
    uint        gda_flg0:13;    /* unused */
    int         gda_wExit;  /* VAP exit status */
    ushort      gda_wRet;   /* VAP API extra return value */
    ulong       gda_lRet;   /* VAP API extra return value */
    int         gda_syserr; /* analog to errno */
    int         gda_fdErr;  /* error file descriptor */
    char far *      gda_lpszC1Path; /* pointer to C1.ERR path spec */
    char far *      gda_lpbInclude; /* far pointer to INCLUDE path */
    char far *      gda_lpQcini;    /* far pointer to QCINI struct */
    int         gda_fsAsFeed;   /* Flag feedback from the assembler */
};

typedef struct gda_s        gda_t;
typedef struct gda_s FAR *  gdap_t;

extern gdap_t FAR PASCAL GdaGet( void );

/* VAP (stack-switching, etc.) entry points */
extern gdap_t FAR PASCAL VapGdaGet( void );

/*
**  These flags are used in the gda_fAsfeed field
*/

#define ASF_STARTUP 0x1     /* .Startup is present      */
#define ASF_EXIT    0x2     /* .exit is present     */
#define ASF_TINY    0x4     /* .model tiny is present   */
#define ASF_CHECKERRORS 0x8     /* Check for related errors */
