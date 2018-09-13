/*
 * db.c - Twin database module description.
 */


/* Types
 ********/

/* database header version numbers */

#define HEADER_MAJOR_VER         (0x0001)
#define HEADER_MINOR_VER         (0x0005)

/* old (but supported) version numbers */

#define HEADER_M8_MINOR_VER      (0x0004)


typedef struct _dbversion
{
    DWORD dwMajorVer;
    DWORD dwMinorVer;
}
DBVERSION;
DECLARE_STANDARD_TYPES(DBVERSION);


/* Prototypes
 *************/

/* db.c */

extern TWINRESULT WriteTwinDatabase(HCACHEDFILE, HBRFCASE);
extern TWINRESULT ReadTwinDatabase(HBRFCASE, HCACHEDFILE);
extern TWINRESULT WriteDBSegmentHeader(HCACHEDFILE, LONG, PCVOID, UINT);
extern TWINRESULT TranslateFCRESULTToTWINRESULT(FCRESULT);

/* path.c */

extern TWINRESULT WritePathList(HCACHEDFILE, HPATHLIST);
extern TWINRESULT ReadPathList(HCACHEDFILE, HPATHLIST, PHHANDLETRANS);

/* brfcase.c */

extern TWINRESULT WriteBriefcaseInfo(HCACHEDFILE, HBRFCASE);
extern TWINRESULT ReadBriefcaseInfo(HCACHEDFILE, HBRFCASE, HHANDLETRANS);

/* string.c */

extern TWINRESULT WriteStringTable(HCACHEDFILE, HSTRINGTABLE);
extern TWINRESULT ReadStringTable(HCACHEDFILE, HSTRINGTABLE, PHHANDLETRANS);

/* twin.c */

extern TWINRESULT WriteTwinFamilies(HCACHEDFILE, HPTRARRAY);
extern TWINRESULT ReadTwinFamilies(HCACHEDFILE, HBRFCASE, PCDBVERSION, HHANDLETRANS, HHANDLETRANS);

/* foldtwin.c */

extern TWINRESULT WriteFolderPairList(HCACHEDFILE, HPTRARRAY);
extern TWINRESULT ReadFolderPairList(HCACHEDFILE, HBRFCASE, HHANDLETRANS, HHANDLETRANS);

