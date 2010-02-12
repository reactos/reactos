<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="ftp" type="win32cui" installbase="system32" installname="ftp.exe">
	<include base="ftp">.</include>
	<define name="lint" />

	<!-- FIXME: workarounds until we have a proper oldnames library -->
	<define name="chdir">_chdir</define>
	<define name="getcwd">_getcwd</define>
	<define name="mktemp">_mktemp</define>
	<define name="unlink">_unlink</define>
	<define name="close">_close</define>
	<define name="fileno">_fileno</define>
	<define name="read">_read</define>
	<define name="write">_write</define>
	<define name="lseek">_lseek</define>

	<library>ws2_32</library>
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
