/*
 * SPSTUP.H - SwapFile interface for Setup. routines exported from cpwin386.cpl
 *
 * BUGBUG: this can go away once we do swapfiles right...
 */

/*
 * Structure used with SetupSwapFile API.
 */
typedef struct
{
    unsigned long StpResBytes;          /* The number of bytes that
                                         * Setup needs to reserve on the
                                         * indicated drive for further
                                         * Setup operations.
                                         */

    unsigned long PartFileSizeBytes;    /* The size set (output), or the size
                                         * to be set (input) for the paging
                                         * SwapFile.
                                         */

    unsigned int  PartFlags;            /* Paging file flags, see bit
                                         * definitions below.
                                         */

    unsigned int  Win300SpCopied;       /* This is a BOOL.  It controls the
                                         * user warning about the fact that
                                         * partition files are inherently
                                         * GLOBAL and that if the USER
                                         * has both a 3.00 and 3.10
                                         * installation he is going to stomp
                                         * the 3.00 installation unless he
                                         * behaves.
                                         *
                                         * NOTE that the display of this
                                         * warning is NOT controlled by the
                                         * Interact setting.
                                         *
                                         * NOTE that SetupSwapFile may change
                                         * this variable from TRUE -> FALSE
                                         * and that Setup should preserve
                                         * this change across subsequent
                                         * calls.
                                         *
                                         * Normally this is FALSE.  Setup
                                         * should set this to TRUE iff SETUP
                                         * has copied SPART.PAR from a 3.00
                                         * directory as part of the setup
                                         * process.  NOTE that this implies
                                         * several things:
                                         *
                                         *   SETUP found a 3.00 dir.
                                         *
                                         *   This is a new installation of
                                         *   3.10 into a directory DIFFERENT
                                         *   than the 3.00 directory.
                                         *
                                         *   Setup found an SPART.PAR in the
                                         *   3.00 directory to copy.
                                         *
                                         *   An SPART.PAR did not already
                                         *   exist in the 3.10 directory.
                                         */

    /*
     * The following two fields are UPPER-CASE DOS DRIVE LETTERS.
     */

    unsigned char StpResDrv;            /* DOS drive of StpResBytes. */

    unsigned char PartDrv;              /* DOS drive of PartFileSizeBytes. */
    LPCSTR szWinDir;                    /* NEW FOR CHICAGO: windows dir where we are installing too */
} SprtData;

/*
 * Structure used with SetupGetCurSetting API:
 */
typedef struct
{
    unsigned long PartCurSizeBytes;     /* The size of the current SwapFile
                                         * (output).
                                         */

    unsigned int  PartFlags;            /* Paging file flags (output), see
                                         * bit definitions below.
                                         */

    unsigned char PartDrv;              /* DOS drive of CurSizeBytes (output).
                                         */
} SprtSetData;

/*
 * Bit definitions for PartFlags bit field:
 *
 * For SetupSwapFile() API only
 *
 *      PART_IS_TEMP
 *      PART_IS_PERM
 *      PART_OFF
 *      PART_NO_SPACE
 *      PART_UCANCEL
 *
 * are valid.  For SetupGetCurSetting() API only
 *
 *      PART_IS_TEMP
 *      PART_IS_PERM
 *      PART_OFF
 *      PART_FAST
 *
 * are valid.
 */
#define PART_IS_TEMP    0x0001  /* Setting is temporary swapfile. */
#define PART_IS_PERM    0x0002  /* Setting is permanent swapfile. */
#define PART_OFF        0x0004  /* USER requested paging OFF. */
#define PART_FAST       0x0008  /* Only through SetupGetCurSetting API -
                                 *  if PART_IS_PERM means USING 32-BIT
                                 *  ACCESS.
                                 */
#define PART_NO_SPACE   0x0010  /* No setting, insufficient disk space. */
#define PART_UCANCEL    0x0020  /* USER CANCELED in interactive mode. */


typedef BOOL (FAR PASCAL *SWAPCURSETPROC)(SprtSetData FAR *SetData);
typedef BOOL (FAR PASCAL *SWAPFILEPROC)(HWND, BOOL bInteract, BOOL bCreate, SprtData FAR *);

