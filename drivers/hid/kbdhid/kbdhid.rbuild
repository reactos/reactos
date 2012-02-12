<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdhid" type="kernelmodedriver" installbase="system32/drivers" installname="kbdhid.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>hidparse</library>
	<file>kbdhid.c</file>
	<file>kbdhid.rc</file>
</module>
