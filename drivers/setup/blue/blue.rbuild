<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="blue" type="kernelmodedriver" installbase="system32/drivers" installname="blue.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="ReactOS">include/reactos/drivers</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>blue.c</file>
	<file>font.c</file>
	<file>blue.rc</file>
</module>
