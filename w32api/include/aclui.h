/*
 * aclui.h
 *
 * Access Control List Editor definitions
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __ACLUI_H
#define __ACLUI_H
#if __GNUC__ >=3
#pragma GCC system_header
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <unknwn.h>
#include <accctrl.h>
#include <commctrl.h>

DEFINE_GUID(IID_ISecurityInformation, 0x965fc360, 0x16ff, 0x11d0, 0x0091, 0xcb,0x00,0xaa,0x00,0xbb,0xb7,0x23);
DEFINE_GUID(IID_ISecurityInformation2, 0xc3ccfdb4, 0x6f88, 0x11d2, 0x00a3, 0xce,0x00,0xc0,0x4f,0xb1,0x78,0x2a);
DEFINE_GUID(IID_IEffectivePermission, 0x3853dc76, 0x9f35, 0x407c, 0x0088, 0xa1,0xd1,0x93,0x44,0x36,0x5f,0xbc);
DEFINE_GUID(IID_ISecurityObjectTypeInfo, 0xfc3066eb, 0x79ef, 0x444b, 0x0091, 0x11,0xd1,0x8a,0x75,0xeb,0xf2,0xfa);

typedef interface ISecurityInformation *LPSECURITYINFO;
typedef interface ISecurityInformation2 *LPSECURITYINFO2;
typedef interface IEffectivePermission *LPEFFECTIVEPERMISSION;
typedef interface ISecurityObjectTypeInfo *LPSecurityObjectTypeInfo;

#undef INTERFACE
EXTERN_C const IID IID_ISecurityInformation;
#define INTERFACE ISecurityInformation
DECLARE_INTERFACE_(ISecurityInformation,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* ISecurityInformation */
	STDMETHOD(GetObjectInformation)(THIS_ PSI_OBJECT_INFO) PURE;
	STDMETHOD(GetSecurity)(THIS_ SECURITY_INFORMATION,PSECURITY_DESCRIPTOR*,BOOL) PURE;
	STDMETHOD(SetSecurity)(THIS_ SECURITY_INFORMATION,PSECURITY_DESCRIPTOR) PURE;
	STDMETHOD(GetAccessRights)(THIS_ GUID*,DWORD,PSI_ACCESS*,ULONG*,ULONG*) PURE;
	STDMETHOD(MapGeneric)(THIS_ GUID*,UCHAR*,PSI_ACCESS*) PURE;
	STDMETHOD(GetInheritTypes)(THIS_ PSI_INHERIT_TYPE*,ULONG*) PURE;
	STDMETHOD(PropertySheetPageCallback)(THIS_ HWND,UINT,SI_PAGE_TYPE) PURE;
};
#undef INTERFACE

#undef INTERFACE
#define INTERFACE ISecurityInformation2
DECLARE_INTERFACE_(ISecurityInformation2,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* ISecurityInformation2 */
	STDMETHOD(IsDaclCanonical)(THIS_ PACL) PURE;
	STDMETHOD(LookupSids)(THIS_ ULONG,PSID*,LPDATAOBJECT*) PURE;
};
#undef INTERFACE

#undef INTERFACE
#define INTERFACE IEffectivePermission
DECLARE_INTERFACE_(IEffectivePermission,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* IEffectivePermission */
	STDMETHOD(GetEffectivePermission)(THIS_ GUID*,PSID,LPCWSTR,PSECURITY_DESCRIPTOR,POBJECT_TYPE_LIST*,ULONG*,PACCESS_MASK*,ULONG*) PURE;
};
#undef INTERFACE

#undef INTERFACE
#define INTERFACE ISecurityObjectTypeInfo
DECLARE_INTERFACE_(ISecurityObjectTypeInfo,IUnknown)
{
        /* IUnknown */
        STDMETHOD(QueryInterface)(THIS_ REFIID,PVOID*) PURE;
	STDMETHOD_(ULONG,AddRef)(THIS) PURE;
	STDMETHOD_(ULONG,Release)(THIS) PURE;

	/* ISecurityObjectTypeInfo */
	STDMETHOD(GetInheritSource)(THIS_ SECURITY_INFORMATION,PACL,PINHERITED_FROM*) PURE;
};
#undef INTERFACE

HPROPSHEETPAGE WINAPI
CreateSecurityPage(LPSECURITYINFO psi);

BOOL WINAPI
EditSecurity(HWND hwndOwner, LPSECURITYINFO psi);

#ifdef __cplusplus
}
#endif
#endif /* __ACLUI_H */

/* EOF */
