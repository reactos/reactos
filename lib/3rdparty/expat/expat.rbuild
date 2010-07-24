<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="expat" type="staticlibrary">
	<include base="expat">.</include>
	<include base="expat">lib</include>
	<include base="ReactOS">include/reactos/libs/expat</include>
	<define name="HAVE_EXPAT_CONFIG_H" />
	<directory name="lib">
		<file>xmlparse.c</file>
		<file>xmlrole.c</file>
		<file>xmltok.c</file>
	</directory>
</module>
