/*
 * Various useful prototypes
 */

#ifndef __KERNEL_H
#define __KERNEL_H

#include <windows.h>
#include <ddk/ntddk.h>

#include <internal/linkage.h>
#include <stdarg.h>

/*
 * Use these to place a function in a specific section of the executable
 */
#define PLACE_IN_SECTION(s) __attribute__((section (s)))
#define INIT_FUNCTION (PLACE_IN_SECTION("init"))
#define PAGE_LOCKED_FUNCTION (PLACE_IN_SECTION("pagelk"))
#define PAGE_UNLOCKED_FUNCTION (PLACE_IN_SECTION("pagepo"))

/*
 * Maximum size of the kmalloc area (this is totally arbitary)
 */
#define NONPAGED_POOL_SIZE   (4*1024*1024)

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
void __putchar(char c);
void __goxy(unsigned x, unsigned y);
unsigned __wherex (void);
unsigned __wherey (void);
void __getscreensize (unsigned *maxx, unsigned *maxy);
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
void MmInitialize(boot_param* bp);
void HalInit(boot_param* bp);
void IoInit(void);
void ObInit(void);
void PsInit(void);
void TstBegin(void);
VOID KeInit(VOID);
VOID HalInitConsole(boot_param* bp);

/*
 * Loader functions (called in main())
 */
VOID LdrInitModuleManagement(VOID);
VOID LdrLoadAutoConfigDrivers(VOID);

#endif
