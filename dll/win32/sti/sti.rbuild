<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="sti" type="win32dll" baseaddress="${BASEADDRESS_STI}" installbase="system32" installname="sti.dll" allowwarnings="true" entrypoint="0">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="sti.spec" />
	<include base="sti">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="__WINESRC__" />
	<define name="ENTRY_PREFIX">STI_</define>
	<define name="PROXY_DELEGATION" />
	<define name="REGISTER_PROXY_DLL" />

	<file>regsvr.c</file>
	<file>sti.c</file>
	<file>sti_main.c</file>
	<file>sti_wia.idl</file>
	<library>wine</library>
	<library>sti_proxy</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>rpcrt4</library>
	<library>advapi32</library>
	<library>pseh</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
<module name="sti_proxy" type="rpcproxy" allowwarnings="true">
	<define name="__WINESRC__" />
	<define name="ENTRY_PREFIX">STI_</define>
	<define name="REGISTER_PROXY_DLL"/>
	<define name="PROXY_DELEGATION" />
	<file>sti_wia.idl</file>
</module>
</group>
