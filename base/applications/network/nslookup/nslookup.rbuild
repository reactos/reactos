<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="nslookup" type="win32cui" installbase="system32" installname="nslookup.exe" allowwarnings="true">
	<include base="nslookup">include</include>
	<include base="isc">include</include>
	<include base="isc">noatomic/include</include>
	<include base="isc">win32</include>
	<include base="isc">win32/include</include>
	<include base="dns">include</include>
	<include base="dns">win32</include>
	<include base="dns">win32/include</include>
	<include base="lwres">include</include>
	<include base="lwres">win32</include>
	<include base="lwres">win32/include</include>
	<include base="bind9">include</include>
	<include base="bind9">win32</include>
	<include base="bind9">win32/include</include>
	<define name="WIN32" />
	<define name="ISC_PLATFORM_HAVEIN6PKTINFO" />
	<library>isc</library>
	<library>dns</library>
	<library>ws2_32</library>
	<library>bind9</library>
	<library>lwres</library>
	<file>dighost.c</file>
	<file>nslookup.c</file>
</module>
