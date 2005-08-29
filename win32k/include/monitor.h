#ifndef _WIN32K_MONITOR_H
#define _WIN32K_MONITOR_H

struct GDIDEVICE;

/* monitor object */
typedef struct _MONITOR_OBJECT
{
/*---------- USER_OBJECT_HDR --------------*/
  HMONITOR hSelf;
  LONG refs;
  BYTE hdrFlags;
  
/*---------- USER_OBJECT_HDR --------------*/

	BOOL           IsPrimary;  /* wether this is the primary monitor */
	UNICODE_STRING DeviceName; /* name of the monitor */
	GDIDEVICE     *GdiDevice;  /* pointer to the GDI device to
	                              which this monitor is attached */
	struct _MONITOR_OBJECT *Prev, *Next; /* doubly linked list */
} MONITOR_OBJECT, *PMONITOR_OBJECT;


#endif /* _WIN32K_MONITOR_H */

/* EOF */
