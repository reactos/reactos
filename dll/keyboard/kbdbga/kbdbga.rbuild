<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbga" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbga.dll">
	<importlibrary definition="kbdbga.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdbga.c</file>
	<file>kbdbga.rc</file>
</module>
