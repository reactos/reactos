/* DO NOT USE THE PRECOMPILED HEADER FOR THIS FILE! */

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
#include <initguid.h>
#include <xmllite.h>

/* reader.c */
DEFINE_GUID(IID_IXmlReaderInput, 0x0b3ccc9b, 0x9214, 0x428b, 0xa2, 0xae, 0xef, 0x3a, 0xa8, 0x71, 0xaf, 0xda);
/* writer.c */
DEFINE_GUID(IID_IXmlWriterOutput, 0xc1131708, 0x0f59, 0x477f, 0x93, 0x59, 0x7d, 0x33, 0x24, 0x51, 0xbc, 0x1a);

/* NO CODE HERE, THIS IS JUST REQUIRED FOR THE GUID DEFINITIONS */
