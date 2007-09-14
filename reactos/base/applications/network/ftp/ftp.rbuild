<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ftp" type="win32cui" installbase="system32" installname="ftp.exe" allowwarnings="true">
	<include base="ftp">.</include>
	<define name="__USE_W32API" />
	<define name="lint" />
	<library>kernel32</library>
	<library>ws2_32</library>
	<library>crtdll</library>
	<library>iphlpapi</library>
	<file>cmds.c</file>
	<file>cmdtab.c</file>
	<file>domacro.c</file>
	<file>fake.c</file>
	<file>ftp.c</file>
	<file>main.c</file>
	<file>ruserpass.c</file>
	<file>ftp.rc</file>
</module>
