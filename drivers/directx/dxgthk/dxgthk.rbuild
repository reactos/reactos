<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dxgthk" type="kernelmodedriver"
installbase="system32/drivers" installname="dxgthk.sys">
	<importlibrary definition="dxgthk.spec" />
	<include base="dxgthk">.</include>
	<library>win32k</library>
	<file>main.c</file>
	<file>dxgthk.rc</file>
</module>
