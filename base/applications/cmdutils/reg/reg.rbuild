<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="reg" type="win32cui" installbase="system32" installname="reg.exe" unicode="true">
	<include base="reg">.</include>
	<redefine name="_WIN32_WINNT">0x600</redefine>
	<library>wine</library>
	<library>advapi32</library>
	<library>user32</library>
	<file>reg.c</file>
	<file>rsrc.rc</file>
</module>
