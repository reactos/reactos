<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="rsabase" type="win32dll" baseaddress="${BASEADDRESS_RSABASE}" installbase="system32" installname="rsabase.dll" allowwarnings="true" entrypoint="0">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="rsabase.spec.def" />
	<include base="rsabase">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>rsaenh</library>
	<library>kernel32</library>
	<file>rsabase.spec</file>
</module>
