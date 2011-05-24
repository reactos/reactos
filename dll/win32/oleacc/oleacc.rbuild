<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="oleacc" type="win32dll" baseaddress="${BASEADDRESS_OLEACC}" installbase="system32" installname="oleacc.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="oleacc.spec" />
	<include base="oleacc">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<file>oleacc.rc</file>
	<library>wine</library>
	<library>user32</library>
	<library>ntdll</library>
</module>
</group>
