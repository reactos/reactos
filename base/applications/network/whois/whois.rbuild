<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="whois" type="win32cui" installbase="system32" installname="whois.exe">
	<include base="whois">.</include>
	<library>ws2_32</library>
	<file>whois.c</file>
	<file>whois.rc</file>
</module>
