<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="sb16_nt4" type="kernelmodedriver" installbase="system32/drivers" installname="sndblst.sys">
	<include base="sb16_nt4">.</include>
	<include base="sb16_nt4">..</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>main.c</file>
	<file>control.c</file>
	<file>interrupt.c</file>
</module>
