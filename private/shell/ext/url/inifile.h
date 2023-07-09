/*
 * inifile.h - Initialization file processing module description.
 */


/* Types
 ********/

#ifdef DEBUG

/* .ini switch types */

typedef enum _iniswitchtype
{
   IST_BOOL,
   IST_DEC_INT,
   IST_UNS_DEC_INT
}
INISWITCHTYPE;
DECLARE_STANDARD_TYPES(INISWITCHTYPE);

/* boolean .ini switch */

typedef struct _booliniswitch
{
   INISWITCHTYPE istype;      /* must be IST_BOOL */

   PCSTR pcszKeyName;

   PDWORD pdwParentFlags;

   DWORD dwFlag;
}
BOOLINISWITCH;
DECLARE_STANDARD_TYPES(BOOLINISWITCH);

/* decimal integer .ini switch */

typedef struct _decintiniswitch
{
   INISWITCHTYPE istype;      /* must be IST_DEC_INT */

   PCSTR pcszKeyName;

   PINT pnValue;
}
DECINTINISWITCH;
DECLARE_STANDARD_TYPES(DECINTINISWITCH);

/* unsigned decimal integer .ini switch */

typedef struct _unsdecintiniswitch
{
   INISWITCHTYPE istype;      /* must be IST_UNS_DEC_INT */

   PCSTR pcszKeyName;

   PUINT puValue;
}
UNSDECINTINISWITCH;
DECLARE_STANDARD_TYPES(UNSDECINTINISWITCH);

#endif


/* Global Variables
 *******************/

#ifdef DEBUG

/* defined by client */

extern PCSTR g_pcszIniFile;
extern PCSTR g_pcszIniSection;

#endif


/* Prototypes
 *************/

#ifdef DEBUG

/* inifile.c */

extern BOOL SetIniSwitches(const PCVOID *, UINT);

#endif

