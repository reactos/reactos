#ifndef _INC_DSKQUOTA_GUIDSP_H
#define _INC_DSKQUOTA_GUIDSP_H
///////////////////////////////////////////////////////////////////////////////
/*  File: guidsp.h

    Description: Private class and interface ID declarations/definitions.
        These GUIDs are for private (dskquota project) use only and are not 
        distributed to public clients.  
        GUIDs are DEFINED if initguids.h is included prior to this header.
        Otherwise, they are declared.

    Revision History:

    Date        Description                                          Programmer
    --------    ---------------------------------------------------  ----------
    05/22/96    Initial creation.                                    BrianAu
    05/23/97    Added GUID_NtDiskQuotaStream                         BrianAu
    08/19/97    Reserved dispatch IID's.                             BrianAu
*/
///////////////////////////////////////////////////////////////////////////////
//
// Class IDs.
//

// {7988B573-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(CLSID_DiskQuotaUI, 
0x7988b573, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);

//
// Interface IDs
//
// {7988B578-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(IID_ISidNameResolver, 
0x7988b578, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);

//
// This GUID is the unique identifier for the disk quota export/import stream.
// It's text equivalent is used as the name of the stream in the doc file.
//
// {8A44DF21-D2C9-11d0-80EA-00A0C90637D0}
DEFINE_GUID(GUID_NtDiskQuotaStream, 
0x8a44df21, 0xd2c9, 0x11d0, 0x80, 0xea, 0x0, 0xa0, 0xc9, 0x6, 0x37, 0xd0);


// {7988B575-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(IID_DIDiskQuotaControl, 
0x7988b575, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
// {7988B57A-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(IID_DIDiskQuotaUser, 
0x7988b57a, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
// {7988B57C-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(LIBID_DiskQuotaTypeLibrary, 
0x7988b57c, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);


//
// Events used though OLE automation.
//
// {7988B581-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(IID_DIDiskQuotaControlEvents, 
0x7988b581, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
// {7988B580-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(IID_IDDiskQuotaControlEvents,
0x7988b580, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);

#ifdef POLICY_MMC_SNAPIN
//
// Used for MMC Snapin Node IDs.
//
// {313a692a-9f28-11d1-91b7-00c04fb6cbb3}
DEFINE_GUID(NODEID_DiskQuotaRoot,
0x313a692a, 0x9f28, 0x11d1, 0x91, 0xb7, 0x00, 0xc0, 0x4f, 0xb6, 0xcb, 0xb3);
// {3c58d64e-9f28-11d1-91b7-00c04fb6cbb3}
DEFINE_GUID(NODEID_DiskQuotaSettings,
0x3c58d64e, 0x9f28, 0x11d1, 0x91, 0xb7, 0x00, 0xc0, 0x4f, 0xb6, 0xcb, 0xb3);
// {E268F17A-A167-11d1-91B7-00C04FB6CBB3}
DEFINE_GUID(IID_IDiskQuotaSnapInData, 
0xe268f17a, 0xa167, 0x11d1, 0x91, 0xb7, 0x0, 0xc0, 0x4f, 0xb6, 0xcb, 0xb3);

// {A9E1E46F-A260-11d1-91B7-00C04FB6CBB3}
DEFINE_GUID(IID_ISnapInPropSheetExt, 
0xa9e1e46f, 0xa260, 0x11d1, 0x91, 0xb7, 0x0, 0xc0, 0x4f, 0xb6, 0xcb, 0xb3);
#endif // POLICY_MMC_SNAPIN

// {F82FEAC6-A340-11d1-91B8-00C04FB6CBB3}
DEFINE_GUID(IID_IDiskQuotaPolicy, 
0xf82feac6, 0xa340, 0x11d1, 0x91, 0xb8, 0x0, 0xc0, 0x4f, 0xb6, 0xcb, 0xb3);



#ifdef __USED_IN_MIDL_FILE__
//
// These guids are merely reserved for use by in dispatch.idl.
//
// {7988B57B-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(GUID_QuotaStateConstant, 
0x7988b57b, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
// {7988B57D-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(GUID_UserFilterFlags, 
0x7988b57d, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
// {7988B57E-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(GUID_NameResolutionConstant, 
0x7988b57e, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
// {7988B57F-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(GUID_InitResult, 
0x7988b57f, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);
#endif

#ifdef __DSKQUOTA_UNUSED_GUIDS__
//
// These GUIDs were allocated consecutively so it is easier to recognize them
// in the registry.
// If you need another ID for the disk quota project, take it from
// this set.  They may be used as either public or private.
//
// {7988B579-EC89-11cf-9C00-00AA00A14F56}
DEFINE_GUID(<<name>>, 
0x7988b579, 0xec89, 0x11cf, 0x9c, 0x0, 0x0, 0xaa, 0x0, 0xa1, 0x4f, 0x56);

#endif // __DSKQUOTA_UNUSED_GUIDS__
#endif // _INC_DSKQUOTA_GUIDSP_H


