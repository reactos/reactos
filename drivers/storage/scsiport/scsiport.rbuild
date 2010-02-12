<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="scsiport" type="kernelmodedriver" installbase="system32/drivers" installname="scsiport.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<define name="_SCSIPORT_" />
	<importlibrary definition="scsiport.spec" />
	<include base="scsiport">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>scsiport.c</file>
	<file>scsiport.rc</file>
</module>
