<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="rsabase" type="win32dll" baseaddress="${BASEADDRESS_RSABASE}" installbase="system32" installname="rsabase.dll" allowwarnings="true" entrypoint="0">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="rsabase.spec" />
	<include base="rsabase">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>rsaenh</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>version.rc</file>
	<file>rsabase.spec</file>
</module>
</group>
