<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="libmpg123" type="staticlibrary" allowwarnings="true" crt="dll">
	<define name="OPT_I386"/>
	<define name="REAL_IS_FLOAT"/>
	<define name="NOXFERMEM"/>
	<define name="HAVE_CONFIG_H"/>
	<define name="EOVERFLOW">75</define>
	<include base="libmpg123">.</include>
	<include base="ReactOS">include/reactos/libs/libmpg123</include>
	<!-- FIXME: workarounds until we have a proper oldnames library -->
    <define name="lseek">_lseek</define>
    <define name="read">_read</define>
    <define name="strdup">_strdup</define>
	<file>compat.c</file>
	<file>dct64.c</file>
	<file>dct64_i386.c</file>
	<file>equalizer.c</file>
	<file>feature.c</file>
	<file>format.c</file>
	<file>frame.c</file>
	<file>icy.c</file>
	<file>icy2utf8.c</file>
	<file>id3.c</file>
	<file>index.c</file>
	<file>layer1.c</file>
	<file>layer2.c</file>
	<file>layer3.c</file>
	<file>libmpg123.c</file>
	<file>ntom.c</file>
	<file>optimize.c</file>
	<file>parse.c</file>
	<file>readers.c</file>
	<file>stringbuf.c</file>
	<file>synth.c</file>
	<file>synth_8bit.c</file>
	<file>synth_real.c</file>
	<file>synth_s32.c</file>
	<file>tabinit.c</file>
</module>
</group>
