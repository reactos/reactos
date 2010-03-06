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
	<library>actxprxy_proxy</library>
	<library>ntdll</library>
	<library>rpcrt4</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>uuid</library>
	<library>pseh</library>
	<file>usrmarshal.c</file>
</module>
<module name="actxprxy_proxy" type="rpcproxy">
	<define name="__WINESRC__" />
	<define name="REGISTER_PROXY_DLL" />
	<define name="PROXY_DELEGATION" />
	<file>actxprxy_activscp.idl</file>
	<file>actxprxy_comcat.idl</file>
	<file>actxprxy_docobj.idl</file>
	<file>actxprxy_hlink.idl</file>
	<file>actxprxy_htiframe.idl</file>
	<file>actxprxy_objsafe.idl</file>
	<file>actxprxy_ocmm.idl</file>
	<file>actxprxy_servprov.idl</file>
	<!-- file>actxprxy_shobjidl.idl</file -->
	<file>actxprxy_urlhist.idl
</file>
</module>
</group>
