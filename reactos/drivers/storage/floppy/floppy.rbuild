<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="floppy" type="kernelmodedriver" installbase="system32/drivers" installname="floppy.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="floppy">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>csq</library>
	<file>csqrtns.c</file>
	<file>floppy.c</file>
	<file>hardware.c</file>
	<file>ioctl.c</file>
	<file>readwrite.c</file>
	<file>floppy.rc</file>
</module>
