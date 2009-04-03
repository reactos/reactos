<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="libjpeg" type="win32dll" entrypoint="0" installbase="system32" installname="libjpeg.dll" allowwarnings="true" crt="msvcrt">
	<define name="HAVE_CONFIG_H" />
	<define name="WIN32" />
	<define name="_WINDOWS" />
	<define name="_MBCS" />
	<define name="JPEG_DLL" />
	<include base="libjpeg">.</include>
	<file>jcapimin.c</file>
	<file>jcapistd.c</file>
	<file>jccoefct.c</file>
	<file>jccolor.c</file>
	<file>jcdctmgr.c</file>
	<file>jchuff.c</file>
	<file>jcinit.c</file>
	<file>jcmainct.c</file>
	<file>jcmarker.c</file>
	<file>jcmaster.c</file>
	<file>jcomapi.c</file>
	<file>jcparam.c</file>
	<file>jcphuff.c</file>
	<file>jcprepct.c</file>
	<file>jcsample.c</file>
	<file>jctrans.c</file>
	<file>jdapimin.c</file>
	<file>jdapistd.c</file>
	<file>jdatadst.c</file>
	<file>jdatasrc.c</file>
	<file>jdcoefct.c</file>
	<file>jdcolor.c</file>
	<file>jddctmgr.c</file>
	<file>jdhuff.c</file>
	<file>jdinput.c</file>
	<file>jdmainct.c</file>
	<file>jdmarker.c</file>
	<file>jdmaster.c</file>
	<file>jdmerge.c</file>
	<file>jdphuff.c</file>
	<file>jdpostct.c</file>
	<file>jdsample.c</file>
	<file>jdtrans.c</file>
	<file>jerror.c</file>
	<file>jfdctflt.c</file>
	<file>jfdctfst.c</file>
	<file>jfdctint.c</file>
	<file>jidctflt.c</file>
	<file>jidctfst.c</file>
	<file>jidctint.c</file>
	<file>jidctred.c</file>
	<file>jquant1.c</file>
	<file>jquant2.c</file>
	<file>jutils.c</file>
	<file>jmemmgr.c</file>
	<file>jmemnobs.c</file>
</module>
