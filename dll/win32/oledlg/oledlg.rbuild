<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="oledlg" type="win32dll" baseaddress="${BASEADDRESS_OLEDLG}" installbase="system32" installname="oledlg.dll" allowwarnings="true">
	<importlibrary definition="oledlg.spec" />
	<include base="oledlg">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<library>wine</library>
	<library>ole32</library>
	<library>comdlg32</library>
	<library>user32</library>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<file>insobjdlg.c</file>
	<file>oledlg_main.c</file>
	<file>pastespl.c</file>
	<file>rsrc.rc</file>
</module>
</group>
