<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="hidusb" type="kernelmodedriver" installbase="system32/drivers" installname="hidusb.sys">
	<bootstrap installbase="$(CDOUTPUT)/system32/drivers" />
	<library>ntoskrnl</library>
	<library>hidclass</library>
	<library>usbd</library>
	<library>hal</library>
	<file>hidusb.c</file>
	<file>hidusb.rc</file>
</module>
