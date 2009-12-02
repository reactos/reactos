<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="zlib" type="staticlibrary">
	<include base="zlib">.</include>
	<file>adler32.c</file>
	<file>compress.c</file>
	<file>crc32.c</file>
	<file>gzio.c</file>
	<file>uncompr.c</file>
	<file>deflate.c</file>
	<file>trees.c</file>
	<file>zutil.c</file>
	<file>inflate.c</file>
	<file>infback.c</file>
	<file>inftrees.c</file>
	<file>inffast.c</file>
</module>
<module name="zlibhost" type="hoststaticlibrary">
	<include base="zlibhost">.</include>
	<file>adler32.c</file>
	<file>compress.c</file>
	<file>crc32.c</file>
	<file>gzio.c</file>
	<file>uncompr.c</file>
	<file>deflate.c</file>
	<file>trees.c</file>
	<file>zutil.c</file>
	<file>inflate.c</file>
	<file>infback.c</file>
	<file>inftrees.c</file>
	<file>inffast.c</file>
</module>
</group>
