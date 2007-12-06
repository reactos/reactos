<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="usetup" type="nativecui" installbase="system32" installname="usetup.exe" allowwarnings="false">
	<bootstrap installbase="$(CDOUTPUT)/system32" nameoncd="smss.exe" />
	<include base="usetup">.</include>
	<include base="zlib">.</include>
	<include base="inflib">.</include>
	<include base="ReactOS">include/reactos/drivers</include>
	<define name="_DISABLE_TIDENTS" />
	<define name="_WIN32_WINNT">0x0502</define>
	<define name="__NO_CTYPE_INLINES" />
	<linkerflag>-lgcc</linkerflag>
	<library>zlib</library>
	<library>inflib</library>
	<library>vfatlib</library>
	<library>ntdll</library>
	<directory name="interface">
		<file>consup.c</file>
		<file>devinst.c</file>
		<file>usetup.c</file>
	</directory>
	<directory name="native">
		<directory name="utils">
			<file>console.c</file>
			<file>keytrans.c</file>
		</directory>
		<file>console.c</file>
		<file>fslist.c</file>
	</directory>
	<file>bootsup.c</file>
	<file>cabinet.c</file>
	<file>chkdsk.c</file>
	<file>drivesup.c</file>
	<file>filequeue.c</file>
	<file>filesup.c</file>
	<file>format.c</file>
	<file>fslist.c</file>
	<file>genlist.c</file>
	<file>inffile.c</file>
	<file>inicache.c</file>
	<file>partlist.c</file>
	<file>progress.c</file>
	<file>registry.c</file>
	<file>settings.c</file>
	<file>usetup.rc</file>
</module>
