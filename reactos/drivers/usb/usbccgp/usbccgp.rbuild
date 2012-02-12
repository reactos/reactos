<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usbccgp" type="kernelmodedriver" installbase="system32/drivers" installname="usbccgp.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<define name="DEBUG_MODE" />
	<include base="ntoskrnl">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
    <library>usbd</library>
    <library>pseh</library>
	<file>descriptor.c</file>
	<file>fdo.c</file>
	<file>function.c</file>
	<file>misc.c</file>
	<file>pdo.c</file>
	<file>usbccgp.c</file>
	<file>usbccgp.rc</file>
</module>
