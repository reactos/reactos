<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdinmal" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdinmal.dll">
	<importlibrary definition="kbdinmal.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdinmal.c</file>
	<file>kbdinmal.rc</file>
</module>
