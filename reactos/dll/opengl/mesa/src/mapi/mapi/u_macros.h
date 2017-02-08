#ifndef _U_MACROS_
#define _U_MACROS_

#define _U_STRINGIFY(x) #x
#define _U_CONCAT(x, y) x ## y
#define _U_CONCAT_STR(x, y) #x#y

#define U_STRINGIFY(x) _U_STRINGIFY(x)
#define U_CONCAT(x, y) _U_CONCAT(x, y)
#define U_CONCAT_STR(x, y) _U_CONCAT_STR(x, y)

#endif /* _U_MACROS_ */
