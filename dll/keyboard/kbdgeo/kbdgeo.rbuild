<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdgeo" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdgeo.dll">
	<importlibrary definition="kbdgeo.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdgeo.c</file>
	<file>kbdgeo.rc</file>
</module>
