/*
 *  dragdrop.c -
 *
 *  Code for Drag/Drop API's.
 *
 *  This code assumes something else does all the dragging work; it just
 *  takes a list of files after all the extra stuff.
 *
 *  The File Manager is responsible for doing the drag loop, determining
 *  what files will be dropped, formatting the file list, and posting
 *  the WM_DROPFILES message.
 *
 *  The list of files is a sequence of zero terminated filenames, fully
 *  qualified, ended by an empty name (double NUL).  The memory is allocated
 *  DDESHARE.
 */

#include "shprv.h"

#undef  Assert
#define Assert(x)
#include "..\shell32\rdrag.c"
