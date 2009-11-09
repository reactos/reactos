<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdes" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdes.dll">
	<importlibrary definition="kbdes.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdes.c</file>
	<file>kbdes.rc</file>
</module>
