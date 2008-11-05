<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="dxg" type="kernelmodedriver" installbase="system32/drivers" installname="dxg.sys">
	<importlibrary definition="dxg.spec" />
	<include base="dxg">.</include>
	<include base="dxg">include</include>
	<include base="ReactOS">subsystems/win32/win32k/include</include>
	<library>dxgthk</library>
	<library>ntoskrnl</library>
	<file>main.c</file>
	<file>ddhmg.c</file>
	<file>eng.c</file>
	<file>historic.c</file>
	<file>dxg.rc</file>
</module>
