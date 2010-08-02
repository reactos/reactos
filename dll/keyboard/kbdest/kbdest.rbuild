<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdest" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdest.dll">
	<importlibrary definition="kbdest.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdest.c</file>
	<file>kbdest.rc</file>
</module>
