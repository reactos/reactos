<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rsaenh" type="win32dll" baseaddress="${BASEADDRESS_RSAENH}" installbase="system32" installname="rsaenh.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="rsaenh.spec" />
	<include base="rsaenh">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<file>aes.c</file>
	<file>des.c</file>
	<file>handle.c</file>
	<file>implglue.c</file>
	<file>md2.c</file>
	<file>mpi.c</file>
	<file>rc2.c</file>
	<file>rc4.c</file>
	<file>rsa.c</file>
	<file>rsaenh.c</file>
	<file>version.rc</file>
	<file>rsaenh.spec</file>
	<library>wine</library>
	<library>crypt32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
