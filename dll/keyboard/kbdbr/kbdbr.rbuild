<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbdbr" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbdbr.dll">
	<importlibrary definition="kbdbr.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbdbr.c</file>
	<file>kbdbr.rc</file>
</module>
