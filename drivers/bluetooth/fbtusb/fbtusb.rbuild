<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="fbtusb" type="kernelmodedriver" installbase="system32/drivers" installname="fbtusb.sys" allowwarnings="true">
	<include base="fbtusb">include</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>usbd</library>
	<file>fbtdev.c</file>
	<file>fbtpnp.c</file>
	<file>fbtpwr.c</file>
	<file>fbtrwr.c</file>
	<file>fbtusb.c</file>
	<!--file>fbtwmi.c</file-->
	<file>fbtusb.rc</file>
</module>
