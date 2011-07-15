<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="nmidebug" type="kernelmodedriver" installbase="system32/drivers" installname="nmidebug.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<include base="null">.</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>nmidebug.c</file>
	<file>nmidebug.rc</file>
</module>
