; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "Extensions.h"
ResourceCount=2
Resource1=IDD_SHELLOPTIONS
ClassCount=2
Class1=ExtensionChoice
LastClass=ExtensionChoice
LastTemplate=CDialog
Class2=ShellExtensions
Resource2=IDD_EXTENSION

[DLG:IDD_EXTENSION]
Type=1
Class=ExtensionChoice
ControlCount=8
Control1=IDC_EXT_EDIT,edit,1350631552
Control2=IDC_CLASSTYPE_EDIT,edit,1350631552
Control3=IDC_CLASSDESC_EDIT,edit,1350631552
Control4=IDC_STATIC,static,1342308352
Control5=IDC_STATIC,static,1342308354
Control6=IDC_STATIC,static,1342308354
Control7=IDC_STATIC,static,1342308354
Control8=IDC_STATIC,static,1342177295

[DLG:IDD_SHELLOPTIONS]
Type=1
Class=ShellExtensions
ControlCount=10
Control1=IDC_CONTEXTMENU,button,1342242819
Control2=IDC_CONTEXTMENU3,button,1476460547
Control3=IDC_DND,button,1476460547
Control4=IDC_ICONHANDLER,button,1342242819
Control5=IDC_PROPERTYSHEET,button,1342242819
Control6=IDC_DATAOBJECT,button,1476460547
Control7=IDC_DROPTARGET,button,1476460547
Control8=IDC_INFOTIP,button,1342242819
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342177295

[CLS:ExtensionChoice]
Type=0
HeaderFile=ExtensionChoice.h
ImplementationFile=ExtensionChoice.cpp
BaseClass=CAppWizStepDlg
Filter=D
VirtualFilter=dWC

[CLS:ShellExtensions]
Type=0
HeaderFile=ShellExtensions.h
ImplementationFile=ShellExtensions.cpp
BaseClass=CAppWizStepDlg
Filter=D
VirtualFilter=dWC
LastObject=IDC_CONTEXTMENU

