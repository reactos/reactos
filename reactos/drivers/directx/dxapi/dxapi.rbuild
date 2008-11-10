<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dxapi" type="kernelmodedriver"
installbase="system32/drivers" installname="dxapi.sys">
	<importlibrary definition="dxapi.spec" />
	<include base="dxapi">.</include>
	<define name="__USE_W32API" />
	<define name="_DXAPI_" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>videoprt</library>
	<file>main.c</file>
	<file>dxapi.rc</file>
</module>
