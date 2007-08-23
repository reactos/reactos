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
extern int call_firmware(ULONG *);
extern void warn(char *fmt, ...);

#define CIF_HANDLER_IN 3

// Device tree routines

// Peer() - This routines outputs the identifier(phandle) of the device node that is
//          the next sibling of the specified device node.
//
//   Inputs:
//           phandle - identifier of a device node
//
//   Outputs:
//           sibling_phandle - identifier of the next sibling.
//                             Zero if there are no more siblings.

phandle
OFPeer(phandle device_id)
{
	ULONG argarray[] = { (ULONG)"peer",1,1,0,0};
	argarray[CIF_HANDLER_IN+0] = device_id;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle)argarray[CIF_HANDLER_IN+1]);
}

phandle
OFChild(phandle device_id)
{
	ULONG argarray[] = { (ULONG)"child",1,1,0,0};
	argarray[CIF_HANDLER_IN+0] = device_id;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle)argarray[CIF_HANDLER_IN+1]);
}

phandle
OFParent(phandle device_id)
{
	ULONG argarray[] = { (ULONG)"parent",1,1,0,0};
	argarray[CIF_HANDLER_IN+0] = device_id;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle)argarray[CIF_HANDLER_IN+1]);
}

long
OFGetproplen(
    phandle device_id,
    char *name
    )
{
	ULONG argarray[] = { (ULONG)"getproplen",2,1,0,0,0};
	argarray[CIF_HANDLER_IN+0] = (long)device_id;
	argarray[CIF_HANDLER_IN+1] = (long)name;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+2]);
}

long
OFGetprop(
    phandle device_id,
    char *name,
    char *buf,
    ULONG buflen
    )
{
	ULONG argarray[] = { (ULONG)"getprop",4,1,0,0,0,0,0};
	argarray[CIF_HANDLER_IN+0] = (long)device_id;
	argarray[CIF_HANDLER_IN+1] = (long)name;
	argarray[CIF_HANDLER_IN+2] = (long)buf;
	argarray[CIF_HANDLER_IN+3] = buflen;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+4]);
}

long
OFNextprop(
    phandle device_id,
    char *name,
    char *buf
    )
{
	ULONG argarray[] = { (ULONG)"nextprop",3,1,0,0,0,0};
	argarray[CIF_HANDLER_IN+0] = (long)device_id;
	argarray[CIF_HANDLER_IN+1] = (long)name;
	argarray[CIF_HANDLER_IN+2] = (long)buf;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+3]);
}

long
OFSetprop(
    phandle device_id,
    char *name,
    char *buf,
    ULONG buflen
    )
{
	ULONG argarray[] = { (ULONG)"setprop",4,1,0,0,0,0,0};
	argarray[CIF_HANDLER_IN+0] = (long)device_id;
	argarray[CIF_HANDLER_IN+1] = (long)name;
	argarray[CIF_HANDLER_IN+2] = (long)buf;
	argarray[CIF_HANDLER_IN+3] = buflen;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+4]);
}

phandle
OFFinddevice( char *devicename)
{
	ULONG argarray[] = { (ULONG)"finddevice",1,1,0,0};

	argarray[CIF_HANDLER_IN+0] = (long)devicename;
	if (call_firmware(argarray) != 0)
	{
		return (phandle)0;
	}
	return ((phandle) argarray[CIF_HANDLER_IN+1]);
}

ihandle
OFOpen( char *devicename)
{
	ULONG argarray[] = { (ULONG)"open",1,1,0,0};

	argarray[CIF_HANDLER_IN+0] = (long)devicename;
	if (call_firmware(argarray) != 0)
	{
		return (ihandle)0;
	}
	return ((ihandle) argarray[CIF_HANDLER_IN+1]);
}

void
OFClose(ihandle id)
{
	ULONG argarray[] = { (ULONG)"close",1,1,0,0};
	argarray[CIF_HANDLER_IN+0] = (long)id;
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
	ULONG argarray[] = { (ULONG)"read",3,1,0,0,0,0};

	argarray[CIF_HANDLER_IN+0] = (long) instance_id;
	argarray[CIF_HANDLER_IN+1] = (ULONG)addr;
	argarray[CIF_HANDLER_IN+2] = len;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+3]);
}

long
OFWrite(
    ihandle instance_id,
    char *addr,
    ULONG len
    )
{
	ULONG argarray[] = { (ULONG)"write",3,1,0,0,0,0};

	argarray[CIF_HANDLER_IN+0] = (long) instance_id;
	argarray[CIF_HANDLER_IN+1] = (ULONG)addr;
	argarray[CIF_HANDLER_IN+2] = len;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+3]);
}

long
OFSeek(
    ihandle instance_id,
    ULONG poshi,
    ULONG poslo
    )
{
	ULONG argarray[] = { (ULONG)"seek",3,1,0,0,0,0};

	argarray[CIF_HANDLER_IN+0] = (long) instance_id;
	argarray[CIF_HANDLER_IN+1] = poshi;
	argarray[CIF_HANDLER_IN+2] = poslo;
	if (call_firmware(argarray) != 0) {
		return (-1);
	}
	return (argarray[CIF_HANDLER_IN+3]);
}

ULONG
OFClaim(
    char *addr,
    ULONG size,
    ULONG align
    )
{
	ULONG argarray[] = { (ULONG)"claim",3,1,0,0,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)addr;
	argarray[CIF_HANDLER_IN+1] = size;
	argarray[CIF_HANDLER_IN+2] = align;
	if (call_firmware(argarray) != 0)
	{
		return (ULONG)0;
	}
	return (argarray[CIF_HANDLER_IN+3]);
}

VOID
OFRelease(
    char *addr,
    ULONG size
    )
{
	ULONG argarray[] = { (ULONG)"release",2,0,0,0};
	argarray[CIF_HANDLER_IN+0] = (ULONG)addr;
	argarray[CIF_HANDLER_IN+1] = size;
	call_firmware(argarray); 
}

long
OFPackageToPath(
    phandle device_id,
    char *addr,
    ULONG buflen
    )
{
	ULONG argarray[] = { (ULONG)"package-to-path",3,1,0,0,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)device_id;
	argarray[CIF_HANDLER_IN+1] = (ULONG)addr;
	argarray[CIF_HANDLER_IN+2] = buflen;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+3]);
}

phandle
OFInstanceToPackage(ihandle ih)
{
	ULONG argarray[] = { (ULONG)"instance-to-package",1,1,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)ih;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+1]);
}

long
OFCallMethod(
    char *method,
    ihandle id,
    ULONG arg
    )
{
	ULONG argarray[] = { (ULONG)"call-method",3,1,0,0,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)method;
	argarray[CIF_HANDLER_IN+1] = (ULONG)id;
	argarray[CIF_HANDLER_IN+2] = arg;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+3]);
}

long
OFInterpret0(
    char *cmd
    )
{
	ULONG argarray[] = { (ULONG)"interpret",1,1,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)cmd;
	if (call_firmware(argarray) != 0)
	{
		return (-1);
	}
	return ((LONG)argarray[CIF_HANDLER_IN+1]);
}

ULONG
OFMilliseconds( VOID )
{
	ULONG argarray[] = { (ULONG)"milliseconds",0,1,0};
	if (call_firmware(argarray) != 0)
	{
		return (ULONG)0;
	}
	return (argarray[CIF_HANDLER_IN+0]);
}

void (*OFSetCallback(void (*func)(void)))(void)
{
	ULONG argarray[] = { (ULONG)"set-callback",1,1,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)func;
	if (call_firmware(argarray) != 0)
	{
		return (NULL);
	}
	return ((void (*)(void))argarray[CIF_HANDLER_IN+1]);
}

long
OFBoot(
    char *bootspec
    )
{
	ULONG argarray[] = { (ULONG)"boot",1,0,0};

	argarray[CIF_HANDLER_IN+0] = (ULONG)bootspec;
	call_firmware(argarray);
}

VOID
OFEnter( VOID )
{
	ULONG argarray[] = { (ULONG)"enter",0,0};

	call_firmware(argarray);
}

/* volatile VOID */
 VOID
OFExit( VOID )
{
#ifdef DEBUG
	ULONG argarray[] = { (ULONG)"enter",0,0};
#else
	ULONG argarray[] = { (ULONG)"exit",0,0};
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
