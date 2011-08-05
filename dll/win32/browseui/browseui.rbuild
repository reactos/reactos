<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="browseui" type="win32dll" baseaddress="${BASEADDRESS_BROWSEUI}" installbase="system32" installname="browseui.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="browseui.spec" />
	<include base="browseui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="atlnew">.</include>
	<define name="__WINESRC__" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<define name="ROS_Headers" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>wine</library>
	<library>shlwapi</library>
	<library>shell32</library>
	<library>comctl32</library>
	<library>gdi32</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>atlnew</library>
	<library>msvcrt</library>
	<file>aclmulti.cpp</file>
	<file>addressband.cpp</file>
	<file>addresseditbox.cpp</file>
	<file>bandproxy.cpp</file>
	<file>bandsite.cpp</file>
	<file>bandsitemenu.cpp</file>
	<file>basebar.cpp</file>
	<file>basebarsite.cpp</file>
	<file>brandband.cpp</file>
	<file>browseui.cpp</file>
	<file>browseuiord.cpp</file>
	<file>commonbrowser.cpp</file>
	<file>globalfoldersettings.cpp</file>
	<file>internettoolbar.cpp</file>
	<file>regtreeoptions.cpp</file>
	<file>shellbrowser.cpp</file>
	<file>toolsband.cpp</file>
	<file>travellog.cpp</file>
	<file>utility.cpp</file>
	<file>dllinstall.c</file>
	<file>browseui.rc</file>
	<pch>precomp.h</pch>
</module>
