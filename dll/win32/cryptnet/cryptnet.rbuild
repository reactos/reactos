<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cryptnet" type="win32dll" baseaddress="${BASEADDRESS_CRYPTNET}" installbase="system32" installname="cryptnet.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="cryptnet.spec" />
	<include base="cryptnet">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>crypt32</library>
	<library>kernel32</library>
	<library>wininet</library>
	<library>ntdll</library>
	<file>cryptnet_main.c</file>
</module>
</group>
