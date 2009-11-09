<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="qmgrprxy" type="win32dll" baseaddress="${BASEADDRESS_QMGRPRXY}" installbase="system32" entrypoint="0" installname="qmgrprxy.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="qmgrprxy.spec" />
	<include base="qmgrprxy">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="qmgrprxy" root="intermediate">.</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>qmgrprxy_interface</library>
	<library>qmgrprxy_proxy</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>version.rc</file> <!-- we need at least one file in the module -->
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
<module name="qmgrprxy_interface" type="idlinterface">
	<file>qmgrprxy.idl</file>
</module>
<module name="qmgrprxy_proxy" type="rpcproxy">
	<define name="__WINESRC__" />
	<define name="REGISTER_PROXY_DLL" />
	<file>qmgrprxy.idl</file>
</module>
</group>
