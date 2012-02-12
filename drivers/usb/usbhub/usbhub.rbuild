<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbhub" type="kernelmodedriver" installbase="system32/drivers" installname="usbhub.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>usbd</library>
	<library>pseh</library>
	<file>fdo.c</file>
	<file>misc.c</file>
	<file>pdo.c</file>
	<file>usbhub.c</file>
	<file>usbhub.rc</file>
</module>
