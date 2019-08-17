/*
 * Copyright (C) 2000 Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __CGUID_H__
#define __CGUID_H__

#if __GNUC__ >= 8
#define GCC8DECLSPEC_SELECTANY DECLSPEC_SELECTANY
#else
#define GCC8DECLSPEC_SELECTANY
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern const IID GCC8DECLSPEC_SELECTANY GUID_NULL;
extern const IID GCC8DECLSPEC_SELECTANY IID_IRpcChannel;
extern const IID GCC8DECLSPEC_SELECTANY IID_IRpcStub;
extern const IID GCC8DECLSPEC_SELECTANY IID_IStubManager;
extern const IID GCC8DECLSPEC_SELECTANY IID_IRpcProxy;
extern const IID GCC8DECLSPEC_SELECTANY IID_IProxyManager;
extern const IID GCC8DECLSPEC_SELECTANY IID_IPSFactory;
extern const IID GCC8DECLSPEC_SELECTANY IID_IInternalMoniker;
extern const IID GCC8DECLSPEC_SELECTANY IID_IDfReserved1;
extern const IID GCC8DECLSPEC_SELECTANY IID_IDfReserved2;
extern const IID GCC8DECLSPEC_SELECTANY IID_IDfReserved3;
extern const CLSID CLSID_StdMarshal;
extern const CLSID CLSID_AggStdMarshal;
extern const CLSID CLSID_StdAsyncActManager;
extern const IID GCC8DECLSPEC_SELECTANY IID_IStub;
extern const IID GCC8DECLSPEC_SELECTANY IID_IProxy;
extern const IID GCC8DECLSPEC_SELECTANY IID_IEnumGeneric;
extern const IID GCC8DECLSPEC_SELECTANY IID_IEnumHolder;
extern const IID GCC8DECLSPEC_SELECTANY IID_IEnumCallback;
extern const IID GCC8DECLSPEC_SELECTANY IID_IOleManager;
extern const IID GCC8DECLSPEC_SELECTANY IID_IOlePresObj;
extern const IID GCC8DECLSPEC_SELECTANY IID_IDebug;
extern const IID GCC8DECLSPEC_SELECTANY IID_IDebugStream;
extern const CLSID CLSID_PSGenObject;
extern const CLSID CLSID_PSClientSite;
extern const CLSID CLSID_PSClassObject;
extern const CLSID CLSID_PSInPlaceActive;
extern const CLSID CLSID_PSInPlaceFrame;
extern const CLSID CLSID_PSDragDrop;
extern const CLSID CLSID_PSBindCtx;
extern const CLSID CLSID_PSEnumerators;
extern const CLSID CLSID_StaticMetafile;
extern const CLSID CLSID_StaticDib;
extern const CLSID CID_CDfsVolume;
extern const CLSID CLSID_DCOMAccessControl;
extern const CLSID CLSID_GlobalOptions;
extern const CLSID CLSID_StdGlobalInterfaceTable;
extern const CLSID CLSID_ComBinding;
extern const CLSID CLSID_StdEvent;
extern const CLSID CLSID_ManualResetEvent;
extern const CLSID CLSID_SynchronizeContainer;
extern const CLSID CLSID_CCDFormKrnl;
extern const CLSID CLSID_CCDPropertyPage;
extern const CLSID CLSID_CCDFormDialog;
extern const CLSID CLSID_CCDCommandButton;
extern const CLSID CLSID_CCDComboBox;
extern const CLSID CLSID_CCDTextBox;
extern const CLSID CLSID_CCDCheckBox;
extern const CLSID CLSID_CCDLabel;
extern const CLSID CLSID_CCDOptionButton;
extern const CLSID CLSID_CCDListBox;
extern const CLSID CLSID_CCDScrollBar;
extern const CLSID CLSID_CCDGroupBox;
extern const CLSID CLSID_CCDGeneralPropertyPage;
extern const CLSID CLSID_CCDGenericPropertyPage;
extern const CLSID CLSID_CCDFontPropertyPage;
extern const CLSID CLSID_CCDColorPropertyPage;
extern const CLSID CLSID_CCDLabelPropertyPage;
extern const CLSID CLSID_CCDCheckBoxPropertyPage;
extern const CLSID CLSID_CCDTextBoxPropertyPage;
extern const CLSID CLSID_CCDOptionButtonPropertyPage;
extern const CLSID CLSID_CCDListBoxPropertyPage;
extern const CLSID CLSID_CCDCommandButtonPropertyPage;
extern const CLSID CLSID_CCDComboBoxPropertyPage;
extern const CLSID CLSID_CCDScrollBarPropertyPage;
extern const CLSID CLSID_CCDGroupBoxPropertyPage;
extern const CLSID CLSID_CCDXObjectPropertyPage;
extern const CLSID CLSID_CStdPropertyFrame;
extern const CLSID CLSID_CFormPropertyPage;
extern const CLSID CLSID_CGridPropertyPage;
extern const CLSID CLSID_CWSJArticlePage;
extern const CLSID CLSID_CSystemPage;
extern const CLSID CLSID_IdentityUnmarshal;
extern const CLSID CLSID_InProcFreeMarshaler;
extern const CLSID CLSID_Picture_Metafile;
extern const CLSID CLSID_Picture_EnhMetafile;
extern const CLSID CLSID_Picture_Dib;
extern const GUID GUID_TRISTATE;


#ifdef __cplusplus
}
#endif

#endif /* __CGUID_H__ */
