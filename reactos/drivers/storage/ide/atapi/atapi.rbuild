<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="atapi" type="kernelmodedriver" installbase="system32/drivers" installname="atapi.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="atapi">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>scsiport</library>
	<file>atapi.c</file>
	<file>atapi.rc</file>
</module>
