<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="localui" type="win32dll" baseaddress="${BASEADDRESS_LOCALUI}" installbase="system32" installname="localui.dll" allowwarnings="true">
	<importlibrary definition="localui.spec" />
	<include base="localui">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<define name="__WINESRC__" />
	<file>localui.c</file>
	<file>localui.rc</file>
	<file>ui_Da.rc</file>
	<file>ui_De.rc</file>
	<file>ui_En.rc</file>
	<file>ui_Fr.rc</file>
	<file>ui_Ko.rc</file>
	<file>ui_Lt.rc</file>
	<file>ui_Nl.rc</file>
	<file>ui_No.rc</file>
	<file>ui_Pl.rc</file>
	<file>ui_Pt.rc</file>
	<file>ui_Ro.rc</file>
	<file>ui_Ru.rc</file>
	<file>ui_Si.rc</file>
	<file>ui_Sv.rc</file>
	<file>ui_Zh.rc</file>
	<library>wine</library>
	<library>winspool</library>
	<library>user32</library>
	<library>kernel32</library>
	<library>uuid</library>
	<library>ntdll</library>
</module>
</group>
