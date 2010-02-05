#pragma once

#if 0
#include <ntoskrnl_bld.h>
#else
#define _NTSYSTEM_
#define _NTDLLBUILD_
#define _NTOSKRNL_
#define __NTOSKRNL__
#define _CRTIMP
#define _LIBCNT_
#define _IN_KERNEL_

#if 0
#pragma section("INIT", read, execute, shared, nopage, discard)
#define SECTTN(s,n) __pragma(alloc_text(s,n))
#define SECTB_INIT _SECTTB("INIT")
#define SECTE_INIT _SECTTE("INIT")
#define SECTD_INIT
#define SECTB_PAGL _SECTTB("pagelk")
#define SECTE_PAGL _SECTTE("pagelk")
#define SECTB_PAGU _SECTTB("pagepo")
#define SECTE_PAGU _SECTTE("pagepo")
#else
#define SECTB_INIT
#define SECTE_INIT
#define SECTD_INIT
#define SECTTN(s,n)
#endif

#include <ros.h>

#include <ntifs.h>
// #include <ntddk.h>
#include <bugcodes.h>

#include <arc.h>
#include <ntndk.h>
#include <winddk.h>
#include <ketypes.h>
#include <pseh2.h>

#include <kd\windbgkd.h>
#include <wdbgexts.h>
#include <ntoskrnli.h>
#include <kddll.h>

#include <kd.h>

#endif

#undef NTKERNELAPI
#define NTKERNELAPI
