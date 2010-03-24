<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cmbatt" type="kernelmodedriver" installbase="system32/drivers" installname="cmbatt.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>battc</library>
	<include base="cmbatt">.</include>
	<file>cmbatt.c</file>
	<file>miniclass.c</file>
	<file>cmbatt.rc</file>
</module>
