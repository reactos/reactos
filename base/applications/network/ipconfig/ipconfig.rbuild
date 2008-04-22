<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ipconfig" type="win32cui" installbase="system32" installname="ipconfig.exe" allowwarnings="true">
	<include base="ipconfig">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>iphlpapi</library>
	<library>advapi32</library>
	<file>ipconfig.c</file>
	<file>ipconfig.rc</file>
</module>
