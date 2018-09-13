/****************************************************************************
    hlguids.h

    Copyright (c) 1995-1998 Microsoft Corporation

    This file defines or declares (according to standard DEFINE_GUID protocol)
    the GUIDs used to interact with hyperlinks.

    NOTE: This header file is used by non-Office as well as Office parties to
    access functionality provided by hlink dll and hlinkprx dll.
****************************************************************************/

#ifndef HLGUIDS_H
#define HLGUIDS_H

/****************************************************************************
    hlink GUIDs
****************************************************************************/

// On Windows, we get these from uuid3.lib
#if MAC

/* 79eac9c0-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IBinding,
    0x79eac9c0,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c1-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IBindStatusCallback,
    0x79eac9c1,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c9-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IPersistMoniker,
    0x79eac9c9,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

#endif /* MAC */

/* 79eac9c2-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IHlinkSite,
    0x79eac9c2,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c3-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IHlink,
    0x79eac9c3,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c4-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IHlinkTarget,
    0x79eac9c4,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c5-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IHlinkFrame,
    0x79eac9c5,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c6-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IEnumHLITEM,
    0x79eac9c6,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9c7-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IHlinkBrowseContext,
    0x79eac9c7,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9cb-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(IID_IExtensionServices,
    0x79eac9cb,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9d0-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(CLSID_StdHlink,
    0x79eac9d0,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* 79eac9d1-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(CLSID_StdHlinkBrowseContext,
    0x79eac9d1,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);

/* The GUID of the service SID_SHlinkFrame is the same as IID_IHlinkFrame */
/* 79eac9c5-baf9-11ce-8c82-00aa004ba90b */
#ifndef SID_SHlinkFrame                   /* Usually #defined in hlink.h */
DEFINE_GUID(SID_SHlinkFrame,
    0x79eac9c5,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);
#endif /* ! SID_SHlinkFrame */

/* The GUID of the service SID_SContainer */
/* 79eac9c4-baf9-11ce-8c82-00aa004ba90b */
DEFINE_GUID(SID_SContainer,
    0x79eac9c4,
    0xbaf9, 0x11ce,
    0x8c, 0x82,
    0x00, 0xaa, 0x00, 0x4b, 0xa9, 0x0b);
#endif // HLGUIDS_H


