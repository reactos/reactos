<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="cryptui" type="win32dll" baseaddress="${BASEADDRESS_CRYPTUI}" installbase="system32" installname="cryptui.dll" allowwarnings="true">
	<!--autoregister infsection="OleControlDlls" type="DllRegisterServer" /-->
	<importlibrary definition="cryptui.spec" />
	<include base="cryptui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>main.c</file>
	<file>cryptui.spec</file>
	<library>wine</library>
	<library>kernel32</library>
	<library>ntdll</library>
</module>
</group>
