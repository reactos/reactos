/*++ BUILD Version: 0001    // Increment this if a change has global effects
---*/
/***    SBCONST.H - Systemwide sb handles
*
* GLOBAL
*   None
*
* LOCAL
*   None
*
* DESCRIPTION
*   Constant sb handles
*
* HISTORY
*   26-Oct-90   [mannyv}    Distilled for NATSYS
*   26-Jan-88   [jimsch]    Added some kernel handles
*   18-Dec-87   [mattg]     Created
*
* NOTES
*   Tabs 8.
*/

/*
**  system (CW)
*/

#define sbNil       0       /* Always */
#define sbDds       1       /* DGROUP */

/*
**  Global Routines from kernel
*/

#define sbAtomIndex 2       /* See ATOM.ASM */
#define sbAtomStrings   3       /* See ATOM.ASM */
#define sbKernFixed 4       /* See KERNEL.C */
#define sbUserProg  sbKernFixed
#define sbMrk1      5       /* See NOTEPAD/MARK.C */
#define sbMrk2      6       /* See NOTEPAD/MARK.C */
#define sbCompRE    7       /* See NOTEPAD/SEARCH.C */
#define sbCvPublics 8       /* Code View Public Symbols */
#define sbScratch   9       /* General-purpose scratch SB */
#define sbHelpBuf   10      /* See HELP directory */
#define sbCvInfo    11      /* Points to Code View Info SB's */
#define sbUserScreen    12      /* User Screen Region Save buffer */
#define sbCwScreen  13      /* Cw Screen Region Save buffer */
#define sbUserTape  14      /* User Tape buffer */
#define sbProgramList   15      /* Main Program List buffer */
#define sbProgDepList   16      /* Dependency Program List buffer */
#define sbMakDepHandle  17      /* Program List dependency handles */
#define sbMakDepHeap    18      /* Program List dependency heap    */
#define sbMakRuleList   19      /* Program List rules list  */
#define sbMrk3      20      /* */
#define sbMrk4      21      /* */
#define sbMrkErrors 22      /* Mark table for error window lines */
#define sbQWatch    23      /* Quick Watch tree */

/*
**  Buffers for EDITMGR
*/

#define sbShScrap   25

/*
**  Buffers for PIO
*/

#define csbPIO      12          /* Number of SBs for PIO */

#define sbPIOMin    26          /* First SB for PIO routines */
#define sbPIOMax    (sbPIOMin + csbPIO - 1) /* Last SB for PIO routines */

/*
**  Buffers for HELP
*/

#define csbHelp     16

#define sbHelpMin   (sbPIOMax + 1)
#define sbHelpMax   (sbHelpMin + csbHelp - 1)

/*
**  The following may be reused since multiple vaps may not be running
**  at the same time.
*/

#define sbVapMin    sbHelpMax + 1

/*
**  Cc VAP
*/

#define sbCcMin     sbVapMin
#define sbCcDgroup  (sbCcMin)   /* DGROUP handle for Compiler */
#define sbCcPreload (sbCcMin + 1)   /* First based heap */
#define sbCcPreloadMax  (sbCcPreload + 3) /* Last based heap + 1 */
#define sbCcLpt     (sbCcPreloadMax + 0)    /* loop table */
#define sbCcLps     (sbCcPreloadMax + 1)    /* loop stack */
#define sbCcDtyp    (sbCcPreloadMax + 2)    /* debug types */
#define sbMdt       (sbCcPreloadMax + 3)
#define sbMdtNames  (sbCcPreloadMax + 4)
#define sbCcMax     (sbCcPreloadMax + 5)

/*
**  Constants releated to SbScanNext
*/

#define sbReservedMax   (sbVapMin + 20)
