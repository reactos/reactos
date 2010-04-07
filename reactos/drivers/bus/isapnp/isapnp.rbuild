<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="isapnp" type="kernelmodedriver" installbase="system32/drivers" installname="isapnp.sys">
	<bootstrap installbase="$(CDOUTPUT)"/>
	<include base="isapnp">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>isapnp.c</file>
	<file>pdo.c</file>
	<file>fdo.c</file>
	<file>hardware.c</file>
	<file>isapnp.rc</file>
</module>
