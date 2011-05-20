<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="atapi" type="kernelmodedriver" installbase="system32/drivers" installname="atapi.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<include base="atapi">.</include>
	<library>scsiport</library>
	<library>libcntpr</library>
	<file>atapi.c</file>
	<file>atapi.rc</file>
</module>
