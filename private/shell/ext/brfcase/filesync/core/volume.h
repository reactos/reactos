/*
 * volume.h - Volume ADT module description.
 */


/* Types
 ********/

/* handles */

DECLARE_HANDLE(HVOLUMELIST);
DECLARE_STANDARD_TYPES(HVOLUMELIST);

DECLARE_HANDLE(HVOLUME);
DECLARE_STANDARD_TYPES(HVOLUME);

/* volume results returned by AddVolume() */

typedef enum _volumeresult
{
   VR_SUCCESS,

   VR_UNAVAILABLE_VOLUME,

   VR_OUT_OF_MEMORY,

   VR_INVALID_PATH
}
VOLUMERESULT;
DECLARE_STANDARD_TYPES(VOLUMERESULT);


/* Prototypes
 *************/

/* volume.c */

extern BOOL CreateVolumeList(DWORD, HWND, PHVOLUMELIST);
extern void DestroyVolumeList(HVOLUMELIST);
extern void InvalidateVolumeListInfo(HVOLUMELIST);
PUBLIC_CODE void ClearVolumeListInfo(HVOLUMELIST);
extern VOLUMERESULT AddVolume(HVOLUMELIST, LPCTSTR, PHVOLUME, LPTSTR);
extern void DeleteVolume(HVOLUME);
extern COMPARISONRESULT CompareVolumes(HVOLUME, HVOLUME);
extern BOOL CopyVolume(HVOLUME, HVOLUMELIST, PHVOLUME);
extern BOOL IsVolumeAvailable(HVOLUME);
extern void GetVolumeRootPath(HVOLUME, LPTSTR);

#ifdef DEBUG

extern LPTSTR DebugGetVolumeRootPath(HVOLUME, LPTSTR);
extern ULONG GetVolumeCount(HVOLUMELIST);

#endif

extern void DescribeVolume(HVOLUME, PVOLUMEDESC);
extern TWINRESULT WriteVolumeList(HCACHEDFILE, HVOLUMELIST);
extern TWINRESULT ReadVolumeList(HCACHEDFILE, HVOLUMELIST, PHHANDLETRANS);
extern BOOL IsValidHVOLUME(HVOLUME);

#if defined(DEBUG) || defined(VSTF)

extern BOOL IsValidHVOLUMELIST(HVOLUMELIST);

#endif

