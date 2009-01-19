<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="browseui" type="win32dll" baseaddress="${BASEADDRESS_BROWSEUI}" installbase="system32" installname="browseui.dll">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="browseui.spec" />
	<include base="browseui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="explorer_new">.</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>aclmulti.c</file>
	<file>bandsite.c</file>
	<file>bandsitemenu.c</file>
	<file>browseui_main.c</file>
	<file>regsvr.c</file>
	<file>version.rc</file>
</module>
