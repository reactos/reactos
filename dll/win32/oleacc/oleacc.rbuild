<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="oleacc" type="win32dll" entrypoint="0" baseaddress="${BASEADDRESS_OLEACC}" installbase="system32" installname="oleacc.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="oleacc.spec.def" />
	<include base="oleacc">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<define name="WINVER">0x600</define>
	<define name="_WIN32_WINNT">0x600</define>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>main.c</file>
	<file>oleacc.spec</file>
</module>
