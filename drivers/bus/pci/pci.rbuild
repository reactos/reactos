<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="pci" type="kernelmodedriver" installbase="system32/drivers" installname="pci.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="pci">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>fdo.c</file>
	<file>pci.c</file>
	<file>pdo.c</file>
	<file>pci.rc</file>
</module>
