/* Missing Defines and structures 
 * These are either missing from the w32api package
 * the ReactOS build system is broken and needs to
 * fixed.
 */
#ifndef _ROS_WINE_PORT
#define _ROS_WINE_PORT

typedef short     INT16;
#define HFILE_ERROR ((HFILE)-1) /* Already in winbase.h - ros is fubar */

#define strncasecmp strncmp
#define snprintf _snprintf
#define strcasecmp _stricmp

/* Wine debugging porting */
#include "debugtools.h"

#endif /* _ROS_WINE_PORT */
