<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../tools/rbuild/project.dtd">
<module name="widl" type="buildtool" allowwarnings="true">
	<define name="INT16">SHORT</define>
	<define name="YYDEBUG">1</define>
	<include base="wpp">.</include>
	<library>wpp</library>
	<file>client.c</file>
	<file>expr.c</file>
	<file>hash.c</file>
	<file>header.c</file>
	<file>proxy.c</file>
	<file>server.c</file>
	<file>typegen.c</file>
	<file>typelib.c</file>
	<file>utils.c</file>
	<file>widl.c</file>
	<file>write_msft.c</file>
	<file>parser.yy.c</file>
	<file>parser.tab.c</file>
	<directory name="port">
		<file>mkstemps.c</file>
	</directory>
</module>
