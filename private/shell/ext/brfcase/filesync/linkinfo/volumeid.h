/*
 * volumeid.h - Volume ID ADT module description.
 */


/* Types
 ********/

typedef struct _volumeid
{
   int nUnused;
}
VOLUMEID;
DECLARE_STANDARD_TYPES(VOLUMEID);

/* SearchForLocalPath() input flags */

typedef enum _searchforlocalpathinflags
{
   /* Search matching local devices for missing volume. */

   SFLP_IFL_LOCAL_SEARCH = 0x0001,

   ALL_SFLP_IFLAGS = SFLP_IFL_LOCAL_SEARCH
}
SEARCHFORLOCALPATHINFLAGS;


/* Prototypes
 *************/

/* volumeid.c */

extern BOOL CreateVolumeID(LPCTSTR, PVOLUMEID *, PUINT);
extern void DestroyVolumeID(PVOLUMEID);
extern COMPARISONRESULT CompareVolumeIDs(PCVOLUMEID, PCVOLUMEID);
extern BOOL SearchForLocalPath(PCVOLUMEID, LPCTSTR, DWORD, LPTSTR);
extern UINT GetVolumeIDLen(PCVOLUMEID);
extern BOOL GetVolumeSerialNumber(PCVOLUMEID, PCDWORD *);
extern BOOL GetVolumeDriveType(PCVOLUMEID, PCUINT *);
extern BOOL GetVolumeLabel(PCVOLUMEID, LPCSTR *);
#ifdef UNICODE
extern BOOL GetVolumeLabelW(PCVOLUMEID, LPCWSTR *);
#endif
extern COMPARISONRESULT CompareDWORDs(DWORD, DWORD);

#if defined(DEBUG) || defined (VSTF)

extern BOOL IsValidPCVOLUMEID(PCVOLUMEID);

#endif

#ifdef DEBUG

extern void DumpVolumeID(PCVOLUMEID);

#endif
