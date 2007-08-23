/*
 * callofw.c - Open Firmware client interface for 32-bit systems.
 * This code is intended to be portable to any 32-bit Open Firmware
 * implementation with a standard client interface that can be
 * called when Linux is running.
 */

#include <stdarg.h>
#include <asm/callofw.h>
#include <linux/types.h>
#include <linux/spinlock.h>

u32 (*call_firmware)(u32 *);

static DEFINE_SPINLOCK(prom_lock);

#define MAXARGS 20
int callofw(char *name, int numargs, int numres, ...)
{
	va_list ap;
	u32 argarray[MAXARGS+3];
	int argnum = 3;
	int retval;
	int *intp;
	unsigned long flags;

	if (call_firmware == NULL)
		return -1;

	argarray[0] = (u32)name;
	argarray[1] = numargs;
	argarray[2] = numres;

	if ((numargs + numres) > MAXARGS)
		return -1;

	va_start(ap, numres);
	while (numargs--)
		argarray[argnum++] = va_arg(ap, int);

	spin_lock_irqsave(&prom_lock, flags);
	retval = call_firmware(argarray);
	spin_unlock_irqrestore(&prom_lock, flags);

	if (retval == 0) {
		while (numres--) {
			intp = va_arg(ap, int *);
			*intp = argarray[argnum++];
		}
	}
	va_end(ap);
	return retval;
}

/*
The return value from callofw in all cases is 0 if the attempt to call the
function succeeded, nonzero otherwise.  That return value is from the
gateway function only.  Any results from the called function are returned
via output argument pointers.

Here are call templates for all the standard OFW client services.

callofw("test", 1, 1, namestr, &missing);
callofw("peer", 1, 1, phandle, &sibling_phandle);
callofw("child", 1, 1, phandle, &child_phandle);
callofw("parent", 1, 1, phandle, &parent_phandle);
callofw("instance-to-package", 1, 1, ihandle, &phandle);
callofw("getproplen", 2, 1, phandle, namestr, &proplen);
callofw("getprop", 4, 1, phandle, namestr, bufaddr, buflen, &size);
callofw("nextprop", 3, 1, phandle, previousstr, bufaddr, &flag);
callofw("setprop", 4, 1, phandle, namestr, bufaddr, len, &size);
callofw("canon", 3, 1, devspecstr, bufaddr, buflen, &length);
callofw("finddevice", 1, 1, devspecstr, &phandle);
callofw("instance-to-path", 3, 1, ihandle, bufaddr, buflen, &length);
callofw("instance-to-interposed-path", 3, 1, ihandle, bufaddr, buflen, &length);
callofw("package-to-path", 3, 1, phandle, bufaddr, buflen, &length);
callofw("call-method", numin, numout, in0, in1, ..., &out0, &out1, ...);
callofw("open", 1, 1, devspecstr, &ihandle);
callofw("close", 1, 0, ihandle);
callofw("read", 3, 1, ihandle, addr, len, &actual);
callofw("write", 3, 1, ihandle, addr, len, &actual);
callofw("seek", 3, 1, ihandle, pos_hi, pos_lo, &status);
callofw("claim", 3, 1, virtaddr, size, align, &baseaddr);
callofw("release", 2, 0, virtaddr, size);
callofw("boot", 1, 0, bootspecstr);
callofw("enter", 0, 0);
callofw("exit", 0, 0);
callofw("chain", 5, 0, virtaddr, size, entryaddr, argsaddr, len);
callofw("interpret", numin+1, numout+1, cmdstr, in0, ..., &catchres, &out0, ...);
callofw("set-callback", 1, 1, newfuncaddr, &oldfuncaddr);
callofw("set-symbol-lookup", 2, 0, symtovaladdr, valtosymaddr);
callofw("milliseconds", 0, 1, &ms);
*/
