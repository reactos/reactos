<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="libsamplerate" type="staticlibrary" allowwarnings="true">
	<define name="HAVE_LRINT" />
	<define name="HAVE_LRINTF" />
	<include base="libsamplerate">.</include>
	<file>samplerate.c</file>
	<file>src_linear.c</file>
	<file>src_sinc.c</file>
	<file>src_zoh.c</file>
</module>
