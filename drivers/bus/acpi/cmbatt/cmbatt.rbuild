<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cmbatt" type="kernelmodedriver" installbase="system32/drivers" installname="cmbatt.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>battc</library>
	<library>wmilib</library>
	<include base="cmbatt">.</include>
	<file>cmbatt.c</file>
	<file>cmexec.c</file>
	<file>cmbpnp.c</file>
	<file>cmbwmi.c</file>
	<file>cmbatt.rc</file>
	<pch>cmbatt.h</pch>
</module>
