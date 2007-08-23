// See license at end of file

/*++

Abstract:

    This module implements the wrapper for the P1275 boot firmware client
    program interface. There is a wrapper routine for each of the client
    interface service. The wrapper routine constructs a client interface
    argument array as illustraed in the figure below, places its address
    in r3 and transfers control to the client interface handler. The
    return address of the wrapper routine is placed in lr register.
    
    The Client interface handler performs the service specified in the
    argument array and return to wrapper routine which in turn return
    to the client program. The client interface handler returns an
    overall success or failure code to the caller as a subroutine
    return value (%o0 for SPARC, %r3 for PowerPC, %eax for x86).
    

	Layout of the argument array

	+--------------------------------------+
	| Name of the client interface service |
	+--------------------------------------+
	| Number of input arguments            |
	+--------------------------------------+	
	| Number of return values              |
	+--------------------------------------+	
	| Input arguments (arg1, ..., argN)    |
	+--------------------------------------+	
	| Returned values (ret1, ..., retN)    |
	+--------------------------------------+

--*/

#include "1275.h"

#ifdef SPRO
typedef long long cell_t;
#else
typedef unsigned long cell_t ;
#endif

#ifdef CIF64
#define LOW(index) ((index*2) + 1)
#else
#define LOW(index) (index)
#endif

extern int call_firmware(ULONG *);
extern void warn(char *fmt, ...);

#ifdef CIF64
#define CIF_HANDLER_IN 6
#else
#define CIF_HANDLER_IN 3
#endif

// Device tree routines

//
// Peer() - This routines outputs the identifier(phandle) of the device node that is
//          the next sibling of the specified device node.
//
//   Inputs:
//           phandle - identifier of a device node
//
//   Outputs:
//           sibling_phandle - identifier of the next sibling.
//                             Zero if there are no more siblings.
//

phandle
OFPeer(phandle device_id)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"peer", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"peer",1,1,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = device_id;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle)argarray[CIF_HANDLER_IN+LOW(1)]);
}

phandle
OFChild(phandle device_id)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"child", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"child",1,1,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = device_id;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle)argarray[CIF_HANDLER_IN+LOW(1)]);
}

phandle
OFParent(phandle device_id)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"parent", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"parent", 1,1,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = device_id;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle)argarray[CIF_HANDLER_IN+LOW(1)]);
}

long
OFGetproplen(
    phandle device_id,
    char *name
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"getproplen", 0,2, 0,1, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"getproplen", 2,1,0,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = (long)device_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (long)name;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(2)]);
}

long
OFGetprop(
    phandle device_id,
    char *name,
    char *buf,
    ULONG buflen
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"getprop", 0,4, 0,1, 0,0, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"getprop", 4,1,0,0,0,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = (long)device_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (long)name;
	argarray[CIF_HANDLER_IN+LOW(2)] = (long)buf;
	argarray[CIF_HANDLER_IN+LOW(3)] = buflen;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(4)]);
}

long
OFNextprop(
    phandle device_id,
    char *name,
    char *buf
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"nextprop", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"nextprop", 3,1,0,0,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = (long)device_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (long)name;
	argarray[CIF_HANDLER_IN+LOW(2)] = (long)buf;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(3)]);
}

long
OFSetprop(
    phandle device_id,
    char *name,
    char *buf,
    ULONG buflen
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"setprop", 0,4, 0,1, 0,0, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"setprop", 4,1,0,0,0,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = (long)device_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (long)name;
	argarray[CIF_HANDLER_IN+LOW(2)] = (long)buf;
	argarray[CIF_HANDLER_IN+LOW(3)] = buflen;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(4)]);
}

phandle
OFFinddevice( char *devicename)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"finddevice", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"finddevice", 1,1,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (long)devicename;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle) argarray[CIF_HANDLER_IN+LOW(1)]);
}

ihandle
OFOpen( char *devicename)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"open", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"open", 1,1,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (long)devicename;
	if (call_firmware(argarray) != 0)
	{
		return (ihandle)0;
	}
	return ((ihandle) argarray[CIF_HANDLER_IN+LOW(1)]);
}

void
OFClose(ihandle id)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"close", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"close", 1,1,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = (long)id;
	if (call_firmware(argarray) != 0)
	{
#ifdef notdef
		warn("OFClose(%x) failed\n", id);
#endif
	}
	
}

long
OFRead(
    ihandle instance_id,
    char *addr,
    ULONG len
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"read", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"read", 3,1,0,0,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (long) instance_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (cell_t)addr;
	argarray[CIF_HANDLER_IN+LOW(2)] = len;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(3)]);
}

long
OFWrite(
    ihandle instance_id,
    char *addr,
    ULONG len
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"write", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"write", 3,1,0,0,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (long) instance_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (cell_t)addr;
	argarray[CIF_HANDLER_IN+LOW(2)] = len;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(3)]);
}

long
OFSeek(
    ihandle instance_id,
    ULONG poshi,
    ULONG poslo
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"seek", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"seek", 3,1,0,0,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (long) instance_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = poshi;
	argarray[CIF_HANDLER_IN+LOW(2)] = poslo;
	if (call_firmware(argarray) != 0) {
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+LOW(3)]);
}

ULONG
OFClaim(
    char *addr,
    ULONG size,
    ULONG align
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"claim", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"claim", 3,1,0,0,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)addr;
	argarray[CIF_HANDLER_IN+LOW(1)] = size;
	argarray[CIF_HANDLER_IN+LOW(2)] = align;
	if (call_firmware(argarray) != 0)
	{
		return (ULONG)0;
	}
	return (argarray[CIF_HANDLER_IN+LOW(3)]);
}

VOID
OFRelease(
    char *addr,
    ULONG size
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"release", 0,2, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"release", 2,0,0,0};
#endif
	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)addr;
	argarray[CIF_HANDLER_IN+LOW(1)] = size;
	call_firmware(argarray); 
}

long
OFPackageToPath(
    phandle device_id,
    char *addr,
    ULONG buflen
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"package-to-path", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"package-to-path", 3,1,0,0,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)device_id;
	argarray[CIF_HANDLER_IN+LOW(1)] = (cell_t)addr;
	argarray[CIF_HANDLER_IN+LOW(2)] = buflen;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+LOW(3)]);
}

phandle
OFInstanceToPackage(ihandle ih)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"instance-to-package", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"instance-to-package", 1,1,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)ih;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+LOW(1)]);
}

long
OFCallMethod(
    char *method,
    ihandle id,
    ULONG arg
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"call-method", 0,3, 0,1, 0,0, 0,0, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"call-method", 3,1,0,0,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)method;
	argarray[CIF_HANDLER_IN+LOW(1)] = (cell_t)id;
	argarray[CIF_HANDLER_IN+LOW(2)] = arg;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+LOW(3)]);
}

long
OFInterpret0(
    char *cmd
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"interpret", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"interpret", 1,1,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)cmd;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+LOW(1)]);
}

ULONG
OFMilliseconds( VOID )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"milliseconds", 0,0, 0,1, 0,0};
#else
	cell_t argarray[] = { (cell_t)"milliseconds", 0,1,0};
#endif
	if (call_firmware(argarray) != 0)
	{
		return (ULONG)0;
	}
	return (argarray[CIF_HANDLER_IN+LOW(0)]);
}

void (*OFSetCallback(void (*func)(void)))(void)
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"set-callback", 0,1, 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"set-callback", 1,1,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)func;
	if (call_firmware(argarray) != 0)
	{
		return (NULL);
	}
	return ((void (*)(void))argarray[CIF_HANDLER_IN+LOW(1)]);
}

long
OFBoot(
    char *bootspec
    )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"boot", 0,1, 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"boot", 1,0,0};
#endif

	argarray[CIF_HANDLER_IN+LOW(0)] = (cell_t)bootspec;
	call_firmware(argarray);
}

VOID
OFEnter( VOID )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"enter", 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"enter", 0,0};
#endif

	call_firmware(argarray);
}

/* volatile VOID */
 VOID
OFExit( VOID )
{
#ifdef CIF64
	ULONG argarray[] = { 0,(ULONG)"exit", 0,0, 0,0};
#else
	cell_t argarray[] = { (cell_t)"exit", 0,0};
#endif
	call_firmware(argarray);
}

// LICENSE_BEGIN
// Copyright (c) 2006 FirmWorks
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
// 
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// LICENSE_END
