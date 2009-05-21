<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="kbda2" type="keyboardlayout" entrypoint="0" installbase="system32" installname="kbda2.dll">
	<importlibrary definition="kbda2.spec" />
	<include base="ntoskrnl">include</include>
	<file>kbda2.c</file>
	<file>kbda2.rc</file>
</module>
