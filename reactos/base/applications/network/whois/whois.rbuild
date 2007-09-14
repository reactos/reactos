<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="whois" type="win32cui" installbase="system32" installname="whois.exe">
	<define name="__USE_W32API" />
	<include base="whois">.</include>
	<library>kernel32</library>
	<library>ws2_32</library>
	<file>whois.c</file>
	<file>whois.rc</file>
</module>
