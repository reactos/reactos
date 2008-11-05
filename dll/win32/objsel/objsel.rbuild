<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="objsel" type="win32dll" baseaddress="${BASEADDRESS_OBJSEL}" installbase="system32" installname="objsel.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="objsel.spec" />
	<include base="objsel">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>ole32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<file>factory.c</file>
	<file>objsel.c</file>
	<file>regsvr.c</file>
	<file>objsel.rc</file>
</module>
</group>
