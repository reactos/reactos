<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="cmbattx" type="kernelmodedriver" installbase="system32/drivers" installname="cmbattx.sys">
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>battc</library>
	<include base="cmbatt">.</include>
	<file>cmbatt.c</file>
	<file>cmexec.c</file>
	<file>cmbpnp.c</file>
	<file>cmbwmi.c</file>
	<file>cmbatt.rc</file>
</module>
