<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="inetcomm" type="win32dll" baseaddress="${BASEADDRESS_INETCOMM}" installbase="system32" installname="inetcomm.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="inetcomm.spec" />
	<include base="inetcomm">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<file>imaptransport.c</file>
	<file>inetcomm_main.c</file>
	<file>internettransport.c</file>
	<file>mimeintl.c</file>
	<file>mimeole.c</file>
	<file>pop3transport.c</file>
	<file>regsvr.c</file>
	<file>smtptransport.c</file>
	<file>inetcomm.spec</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>ws2_32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
