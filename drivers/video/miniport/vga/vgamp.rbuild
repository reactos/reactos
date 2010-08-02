<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="vgamp" type="kernelmodedriver" installbase="system32/drivers" installname="vgamp.sys">
	<include base="vgamp">.</include>
	<library>videoprt</library>
	<file>initvga.c</file>
	<file>vgamp.c</file>
	<file>vgamp.rc</file>
	<pch>vgamp.h</pch>
</module>
