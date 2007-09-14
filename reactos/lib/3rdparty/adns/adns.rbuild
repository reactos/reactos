<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="adns" type="staticlibrary" allowwarnings="true">
	<include base="adns">src</include>
	<include base="adns">adns_win32</include>
	<define name="__USE_W32API" />
	<define name="ADNS_JGAA_WIN32" />
	<directory name="adns_win32">
		<file>adns_unix_calls.c</file>
	</directory>
	<directory name="src">
		<file>check.c</file>
		<file>event.c</file>
		<file>general.c</file>
		<file>parse.c</file>
		<file>poll.c</file>
		<file>query.c</file>
		<file>reply.c</file>
		<file>setup.c</file>
		<file>transmit.c</file>
		<file>types.c</file>
	</directory>
</module>
