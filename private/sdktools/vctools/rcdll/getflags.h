/*
 * the format strings may be:
 *              .*                      must match exactly
 *              .*\*.*          head and tail must match, with wild card in middle
 *              .*#                     head must match. tail can either be adjacent or next word
 *      at the moment '-' is not treated specialy
 *  note that 'middle' may be at either end since '.*' matches null.
 */

#define TAKESARG  0x20           /* tag to indicate argument or not */
#define FLAG      1              /* set the flag */
#define STRING    2 | TAKESARG   /* set the string, either from here, or next word */
#define SUBSTR    3 | TAKESARG   /* set single letter flags from substring */
#define NUMBER    4 | TAKESARG   /* set read in the number */
#define UNFLAG    5              /* turn the flag off */
#define PSHSTR    6 | TAKESARG   /* like string, but puts it on a LIST structure */
#define NOVSTR    7 | TAKESARG   /* like string, but complains about overwriting */

#define NUM_ARGS                50   /* Limit of flags passed from driver to pass */
#define MSC_ENVFLAGS            "MSC_CMD_FLAGS"
            /* Environment variable flags passed in, used for getenv() */
#define PUT_MSC_ENVFLAGS        "MSC_CMD_FLAGS="
            /* Environment variable flags passed in, used for putenv() */

/* return values from getflags */
#define R_SWITCH        1
#define R_CFILE         2
#define R_ASMFILE       3
#define R_OBJFILE       4
#define R_ERROR         5
#define R_FILE          6
#define R_EXIT          7
#define R_FFILE         8
#define R_PFILE         9
#define R_AFILE         10
#define R_OFILE         11
/* r. nevin, 1/11/85 */
#define R_HELP          12
/* b. nguyen, 4/7/86 */
#define R_HELPC         13
#define R_HELPF         14

struct cmdtab
        {
        WCHAR *format;           /* format matching string */
        char  *flag;             /* pointer to what to fill in */
/*  this is really a
 *      union
 *              {
 *              WCHAR **str;
 *              int  *flag;
 *              struct subtab *sub;
 *              struct LIST *list;
 *              } cm;
 *      but you cant initialize unions so we have to fake it.
 */
        char retval;            /* crack_cmd will return whatever is here */
        UCHAR type;             /* control mask */
        };

struct subtab
        {
        int letter;
        int type;
        int *flag;
        };

