<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cryptdlg" type="win32dll" baseaddress="${BASEADDRESS_CRYPTDLG}" installbase="system32" installname="cryptdlg.dll" allowwarnings="true">
	<autoregister infsection="OleControlDlls" type="DllRegisterServer" />
	<importlibrary definition="cryptdlg.spec" />
	<include base="cryptdlg">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>crypt32</library>
	<library>cryptui</library>
	<library>wintrust</library>
	<library>ntdll</library>
	<file>main.c</file>
	<file>cryptdlg_De.rc</file>
	<file>cryptdlg_En.rc</file>
	<file>cryptdlg_Fr.rc</file>
	<file>cryptdlg_Ko.rc</file>
	<file>cryptdlg_Lt.rc</file>
	<file>cryptdlg_Nl.rc</file>
	<file>cryptdlg_Ro.rc</file>
</module>
