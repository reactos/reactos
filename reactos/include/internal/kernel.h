/*
 * Various useful prototypes
 */

#ifndef __KERNEL_H
#define __KERNEL_H

#include <windows.h>
#include <ddk/ntddk.h>

#include <internal/linkage.h>
#include <stdarg.h>

VOID KiInterruptDispatch(unsigned int irq);
VOID KiDispatchInterrupt(unsigned int irq);
VOID KeTimerInterrupt(VOID);

/*
 * Defines a descriptor as it appears in the processor tables
 */
typedef struct
{
        unsigned int a;
        unsigned int b;
} descriptor;

extern descriptor idt[256];
extern descriptor gdt[256];

/*
 * printf style functions
 */
asmlinkage void printk(const char* fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char* buf, const char* fmt, ...);

typedef struct
{
        /*
         * Magic value (useless really)
         */
        unsigned int magic;

        /*
         * Cursor position
         */
        unsigned int cursorx;
        unsigned int cursory;

        /*
         * Number of files (including the kernel) loaded
         */
        unsigned int nr_files;

        /*
         * Range of physical memory being used by the system
         */
        unsigned int start_mem;
        unsigned int end_mem;

        /*
         * List of module lengths (terminated by a 0)
         */
        unsigned int module_length[64];
} boot_param;


/*
 * Initalization functions (called once by main())
 */
void MmInitalize(boot_param* bp);
void InitalizeExceptions(void);
void InitalizeIRQ(void);
void InitializeTimer(void);
void InitConsole(boot_param* bp);
void KeInitDpc(void);
void HalInit(boot_param* bp);
void IoInit(void);
void ObjNamespcInit(void);
void PsMgrInit(void);


/*
 * FUNCTION: Called to execute queued dpcs
 */
void KeDrainDpcQueue(void);

void KeExpireTimers(void);

typedef unsigned int (exception_hook)(CONTEXT* c, unsigned int exp);
asmlinkage unsigned int ExHookException(exception_hook fn, UINT exp);

#endif
