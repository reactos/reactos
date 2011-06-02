<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="fdc" type="kernelmodedriver" installbase="system32/drivers" installname="fdc.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<include base="fdc">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>fdc.c</file>
	<file>fdo.c</file>
	<file>pdo.c</file>
	<file>fdc.rc</file>
</module>
