<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group xmlns:xi="http://www.w3.org/2001/XInclude">
<directory name="acpica">
	<xi:include href="acpica/acpica.rbuild" />
</directory>
<directory name="cmbatt">
	<xi:include href="cmbatt/cmbatt.rbuild" />
</directory>
</group>

<module name="acpi" type="kernelmodedriver" installbase="system32/drivers" installname="acpi.sys" allowwarnings="true">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="acpi">include</include>
	<include base="acpica">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>wdmguid</library>
	<library>acpica</library>
	<directory name="busmgr">
		<file>bus.c</file>
		<file>button.c</file>
		<file>power.c</file>
		<file>utils.c</file>
		<file>system.c</file>
	</directory>
	<file>osl.c</file>
	<file>acpienum.c</file>
	<file>eval.c</file>
	<file>interface.c</file>
	<file>pnp.c</file>
	<file>power.c</file>
	<file>buspdo.c</file>
	<file>main.c</file>
</module>
