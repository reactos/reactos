<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<group>
<module name="drawcap" type="win32cui" installbase="system32" installname="drawcap.exe" unicode="true">
	<include base="drawcap">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>drawcap.c</file>
	<file>drawcap.rc</file>
</module>

<module name="capicon" type="win32cui" installbase="system32" installname="capicon.exe" unicode="true">
	<include base="capicon">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
	<file>capicon.c</file>
	<file>capicon.rc</file>
</module>
</group>