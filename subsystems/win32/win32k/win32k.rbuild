<?xml version="1.0"?>
<!DOCTYPE group SYSTEM "../../../tools/rbuild/project.dtd">
<group>
<module name="win32k" type="kernelmodedriver" installbase="system32" installname="win32k.sys" allowwarnings="true">
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
	<directory name="eng">
		<file>engblt.c</file>
		<file>engbrush.c</file>
		<file>engclip.c</file>
		<file>engdev.c</file>
		<file>engdrv.c</file>
		<file>engevent.c</file>
		<file>engerror.c</file>
		<file>engfile.c</file>
		<file>engfloat.c</file>
		<file>engfont.c</file>
		<file>engmem.c</file>
		<file>engmisc.c</file>
		<file>engpaint.c</file>
		<file>engpal.c</file>
		<file>engpath.c</file>
		<file>engpoint.c</file>
		<file>engprint.c</file>
		<file>engquery.c</file>
		<file>engrtl.c</file>
		<file>engsem.c</file>
		<file>engsurf.c</file>
		<file>engtext.c</file>
		<file>engwnd.c</file>
		<file>engxform.c</file>
		<file>engxlate.c</file>
	</directory>
	<directory name="gre">
		<file>init.c</file>
	</directory>
	<directory name="ntddraw">
		<file>ddeng.c</file>
	</directory>
	<directory name="ntgdi">
		<file>gdistubs.c</file>
	</directory>
	<directory name="ntuser">
		<file>usrstubs.c</file>
	</directory>
	<file>win32k.rc</file>
</module>
</group>