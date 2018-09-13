/* 
** fiber.h
** 9/23/96
** Fiber Debugging Support
*/

typedef enum {
    ecreate_fiber=1,
    edelete_fiber
} EFBR;

#define EXCEPTION_FIBER_DEBUG (0x8EEFFAD5)
