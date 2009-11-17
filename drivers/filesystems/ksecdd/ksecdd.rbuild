<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="ksecdd" type="kernelmodedriver" installbase="system32/drivers" installname="ksecdd.sys">
	<bootstrap installbase="$(CDOUTPUT)" />
	<include base="ksecdd">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>dispatch.c</file>
	<file>ksecdd.c</file>
	<file>ksecdd.rc</file>
	<pch>ksecdd.h</pch>
</module>
