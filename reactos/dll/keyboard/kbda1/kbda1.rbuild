<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbda1" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbda1.dll">
	<importlibrary definition="kbda1.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbda1.c</file>
	<file>kbda1.rc</file>
</module>
