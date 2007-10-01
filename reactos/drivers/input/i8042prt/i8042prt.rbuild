<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="i8042prt" type="kernelmodedriver" installbase="system32/drivers" installname="i8042prt.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="i8042prt">.</include>
	<define name="__USE_W32API" />
	<define name="__REACTOS__" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>i8042prt.c</file>
	<file>keyboard.c</file>
	<file>mouse.c</file>
	<file>ps2pp.c</file>
	<file>registry.c</file>
	<file>i8042prt.rc</file>
</module>
