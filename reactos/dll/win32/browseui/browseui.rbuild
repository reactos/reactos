<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="browseui" type="win32dll" baseaddress="${BASEADDRESS_BROWSEUI}" installbase="system32" installname="browseui.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="Both" />
	<importlibrary definition="browseui.spec.def" />
	<include base="browseui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__REACTOS__" />
	<define name="__WINESRC__" />
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<define name="WINVER">0x600</define>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>aclmulti.c</file>
	<file>browseui_main.c</file>
	<file>regsvr.c</file>
	<file>version.rc</file>
	<file>browseui.spec</file>
</module>
