/* this call (GetDriverFlags) is not documented, nor the flags returned.
 * here are Wine only definitions
*/

#ifndef __INCLUDES_ROS_WINMM_INTERNAL__
#define __INCLUDES_ROS_WINMM_INTERNAL__

#ifdef __WINE_FOR_REACTOS__

#define WINE_GDF_EXIST    0x80000000
#define WINE_GDF_16BIT    0x1000000

#define DRV_SUCCESS		0x0001
#define DRV_FAILURE		0x0000

#define WARN printf
#define FIXME printf
#define TRACE printf
#define ERR printf
#define debugstr_a printf

#define HEAP_strdupWtoA strdupWtoA

#endif  /* __WINE_FOR_REACTOS__ */

#endif /* __INCLUDES_ROS_WINMM_INTERNAL__ */
