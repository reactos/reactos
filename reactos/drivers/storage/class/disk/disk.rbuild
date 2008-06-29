<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="disk" type="kernelmodedriver" installbase="system32/drivers" installname="disk.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>class2</library>
	<library>scsiport</library>
	<include base="disk">..</include>
	<file>disk.c</file>
	<file>disk.rc</file>
</module>
