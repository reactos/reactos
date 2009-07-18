<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="win32k" type="kernelmodedriver" installbase="system32" installname="win32k.sys">
	<importlibrary definition="win32k.def" />
	<define name="_WIN32K_" />

	<include base="win32k">.</include>
	<include base="win32k">include</include>
	<include base="win32k" root="intermediate">.</include>
	<include base="ntoskrnl">include</include>
	<include base="freetype">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<include base="ReactOS">include/reactos/drivers</include>

	<compilerflag compilerset="gcc">-fms-extensions</compilerflag>

	<library>ntoskrnl</library>
	<library>hal</library>
	<library>freetype</library>
	<library>libcntpr</library>
	<library>pseh</library>
	<directory name="include">
		<pch>win32k.h</pch>
	</directory>
	<directory name="main">
		<file>err.c</file>
		<file>init.c</file>
		<file>usrheap.c</file>
	</directory>
	<directory name="wine">
		<file>atom.c</file>
		<file>class.c</file>
		<file>directory.c</file>
		<file>handle.c</file>
		<file>hook.c</file>
		<file>main.c</file>
		<file>object.c</file>
		<file>queue.c</file>
		<file>region.c</file>
		<file>stubs.c</file>
		<file>user.c</file>
		<file>window.c</file>
		<file>winesup.c</file>
		<file>winstation.c</file>
	</directory>
	<file>win32k.rc</file>
</module>
</group>