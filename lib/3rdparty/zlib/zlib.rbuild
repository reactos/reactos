<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="zlib" type="staticlibrary" allowwarnings="true">
	<include base="zlib">.</include>
	<file>adler32.c</file>
	<file>compress.c</file>
	<file>crc32.c</file>
	<file>deflate.c</file>
	<file>gzclose.c</file>
	<file>gzlib.c</file>
	<file>gzread.c</file>
	<file>gzwrite.c</file>
	<file>infback.c</file>
	<file>inffast.c</file>
	<file>inflate.c</file>
	<file>inftrees.c</file>
	<file>trees.c</file>
	<file>uncompr.c</file>
	<file>zutil.c</file>
</module>
<module name="zlibhost" type="hoststaticlibrary" allowwarnings="true">
	<include base="zlibhost">.</include>
	<file>adler32.c</file>
	<file>compress.c</file>
	<file>crc32.c</file>
	<file>deflate.c</file>
	<file>gzclose.c</file>
	<file>gzlib.c</file>
	<file>gzread.c</file>
	<file>gzwrite.c</file>
	<file>infback.c</file>
	<file>inffast.c</file>
	<file>inflate.c</file>
	<file>inftrees.c</file>
	<file>trees.c</file>
	<file>uncompr.c</file>
	<file>zutil.c</file>
</module>
</group>
