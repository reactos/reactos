<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="oleaut32" type="win32dll" baseaddress="${BASEADDRESS_OLEAUT32}" installbase="system32" installname="oleaut32.dll" allowwarnings="true" crt="msvcrt">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="oleaut32.spec" />
	<include base="oleaut32">.</include>
	<include base="ReactOS">include/reactos/libs/libjpeg</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<define name="PROXY_CLSID">CLSID_PSDispatch</define>
	<define name="COM_NO_WINDOWS_H"/>
	<define name="_OLEAUT32_"/>
	<define name="PROXY_DELEGATION"/>
	<define name="REGISTER_PROXY_DLL"/>
	<define name="ENTRY_PREFIX">OLEAUTPS_</define>
	<file>connpt.c</file>
	<file>dispatch.c</file>
	<file>hash.c</file>
	<file>oleaut.c</file>
	<file>olefont.c</file>
	<file>olepicture.c</file>
	<file>recinfo.c</file>
	<file>regsvr.c</file>
	<file>safearray.c</file>
	<file>stubs.c</file>
	<file>tmarshal.c</file>
	<file>typelib.c</file>
	<file>typelib2.c</file>
	<file>ungif.c</file>
	<file>usrmarshal.c</file>
	<file>varformat.c</file>
	<file>variant.c</file>
	<file>vartype.c</file>
	<file>oleaut32.rc</file>
	<file>oleaut32_oaidl.idl</file>
	<include base="oleaut32" root="intermediate">.</include>
	<library>oleaut32_proxy</library>
	<library>wine</library>
	<library>wineldr</library>
	<library>libjpeg</library>
	<library>ole32</library>
	<library>rpcrt4</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>comctl32</library>
	<library>urlmon</library>
	<library>uuid</library>
	<library>pseh</library>
</module>
<module name="oleaut32_proxy" type="rpcproxy" allowwarnings="true">
	<define name="COM_NO_WINDOWS_H"/>
	<define name="PROXY_CLSID">CLSID_PSDispatch</define>
	<define name="_OLEAUT32_"/>
	<define name="PROXY_DELEGATION"/>
	<define name="REGISTER_PROXY_DLL"/>
	<define name="ENTRY_PREFIX">OLEAUTPS_</define>
	<file>oleaut32_oaidl.idl</file>
	<file>oleaut32_ocidl.idl</file>
</module>
</group>
