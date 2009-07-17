<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="win32k" type="kernelmodedriver" installbase="system32" installname="win32k.sys">
	<importlibrary definition="win32k.def" />
	<define name="_WIN32K_" />
	<include base="win32k">include</include>
	<include base="win32k" root="intermediate"></include>
	<include base="win32k" root="intermediate">include</include>
	<include base="ReactOS">include/reactos/subsys</include>
	<library>ntoskrnl</library>
	<library>hal</library>
	<library>freetype</library>
	<library>libcntpr</library>
	<library>pseh</library>
	<directory name="include">
		<pch>win32k.h</pch>
	</directory>
	<directory name="gre">
		<file>init.c</file>
	</directory>
	<directory name="wine">
		<file>main.c</file>
	</directory>
	<file>win32k.rc</file>
</module>
</group>