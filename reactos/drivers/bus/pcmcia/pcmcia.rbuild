<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="pcmcia" type="kernelmodedriver" installbase="system32/drivers" installname="pcmcia.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="pcmcia">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>fdo.c</file>
	<file>pcmcia.c</file>
	<file>pdo.c</file>
	<file>pcmcia.rc</file>
</module>
