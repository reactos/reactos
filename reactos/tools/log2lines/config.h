#pragma once

/* Assume if an offset > ABS_TRESHOLD, then it must be absolute */
#define ABS_TRESHOLD    0x00400000L
#define INVALID_BASE    0xFFFFFFFFL

#define LOGBOTTOM       "--------"
#define SVNDB           "svndb.log"
#define SVNDB_INX       "svndb.inx"
#define DEF_RANGE       500
#define MAGIC_INX       0x494E585F //'INX_'
#define DEF_OPT_DIR     "output-i386"
#define SOURCES_ENV     "_ROSBE_ROSSOURCEDIR"
#define CACHEFILE       "log2lines.cache"
#define TRKBUILDPREFIX  "bootcd-"
#define SVN_PREFIX      "/trunk/reactos/"
#define PIPEREAD_CMD    "piperead -c"


#define CP_FMT          CP_CMD "%s %s > " DEV_NULL

#define CMD_7Z          "7z"
#define UNZIP_FMT_7Z    "%s e -y %s -o%s > " DEV_NULL
#define UNZIP_FMT       "%s x -y -r %s -o%s > " DEV_NULL
#define UNZIP_FMT_CAB \
"%s x -y -r %s" PATH_STR "reactos" PATH_STR "reactos.cab -o%s" \
PATH_STR "reactos" PATH_STR "reactos > " DEV_NULL

/* When we can't use a normal path, because it gets cleaned,
 * fallback to name mangling:
 */
#define ALT_PATH_STR    "#"

#define LINESIZE        1024
#define NAMESIZE        80

/* EOF */
