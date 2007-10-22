<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dxgthk" type="kernelmodedriver" 
installbase="system32/drivers" installname="dxgthk.sys">
	<importlibrary definition="dxgthk.def" />
	<include base="dxgthk">.</include>
	<define name="__USE_W32API" />
	<file>main.c</file>
	<file>dxgthk.rc</file>
</module>
