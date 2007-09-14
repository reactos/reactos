<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="vbemp" type="kernelmodedriver" installbase="system32/drivers" installname="vbemp.sys">
	<include base="vbemp">.</include>
	<define name="__USE_W32API" />
	<library>videoprt</library>
	<file>edid.c</file>
	<file>vbemp.c</file>
	<file>vbemp.rc</file>
</module>
