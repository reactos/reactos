<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="atl" type="win32dll" baseaddress="${BASEADDRESS_ATL}" installbase="system32" installname="atl.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="atl.spec" />
	<include base="atl">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="_WIN32_WINNT">0x600</define>
	<file>atl_ax.c</file>
	<file>atl_main.c</file>
	<file>registrar.c</file>
	<file>rsrc.rc</file>
	<include base="atl" root="intermediate">.</include>
	<file>atl.spec</file>
	<library>wine</library>
	<library>uuid</library>
	<library>ole32</library>
	<library>oleaut32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<dependency>atl_atliface_header</dependency>
</module>
<module name="atl_atliface_header" type="idlheader" allowwarnings="true">
	<file>atliface.idl</file>
</module>
</group>
