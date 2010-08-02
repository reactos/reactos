<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="compbatt" type="kernelmodedriver" installbase="system32/drivers" installname="compbatt.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>battc</library>
	<include base="compbatt">.</include>
	<file>compbatt.c</file>
	<file>compmisc.c</file>
	<file>comppnp.c</file>
	<file>compbatt.rc</file>
	<pch>compbatt.h</pch>
</module>
