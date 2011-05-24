<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="fullfat" type="staticlibrary" >
	<include base="ReactOS">include/reactos/libs/fullfat</include>
	<define name="__NTDRIVER__" />

	<file>ff_blk.c</file>
	<file>ff_crc.c</file>
	<file>ff_dir.c</file>
	<file>ff_error.c</file>
	<file>ff_fat.c</file>
	<file>ff_file.c</file>
	<file>ff_format.c</file>
	<file>ff_hash.c</file>
	<file>ff_ioman.c</file>
	<file>ff_memory.c</file>
	<file>ff_safety.c</file>
	<file>ff_string.c</file>
	<file>ff_time.c</file>
	<file>ff_unicode.c</file>
</module>
