<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="actxprxy" type="win32dll" baseaddress="${BASEADDRESS_ACTXPRXY}" installbase="system32" entrypoint="0" installname="actxprxy.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="actxprxy.spec" />
	<include base="actxprxy">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<include base="actxprxy" root="intermediate">.</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>actxprxy_interface</library>
	<library>actxprxy_proxy</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>pseh</library>
	<file>usrmarshal.c</file>
	<compilerflag compilerset="gcc">-fno-unit-at-a-time</compilerflag>
</module>
<module name="actxprxy_interface" type="idlinterface">
	<file>actxprxy_servprov.idl</file>
</module>
<module name="actxprxy_proxy" type="rpcproxy">
	<define name="__WINESRC__" />
	<define name="REGISTER_PROXY_DLL" />
	<define name="PROXY_CLSID_IS">"{ 0xb8da6310, 0xe19b, 0x11d0, { 0x93, 0x3c, 0x00, 0xa0, 0xc9, 0x0d, 0xca, 0xa9 } }"</define>
	<file>actxprxy_servprov.idl</file>
</module>
</group>
