#pragma once

#define DBG 1
// #define DBG 1
// #define KDBG
#define _WINKD_
// #define AUTO_ENABLE_BOCHS
// #define CONFIG_SMP

#define __REACTOS__
#define _X86_
#define __i386__

#define KERNEL_STACK_SIZE 0x6000
#define KERNEL_LARGE_STACK_SIZE 0xf000
#define KERNEL_LARGE_STACK_COMMIT 0x6000

#include <osver.h>
#include <platf.h>
