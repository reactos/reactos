<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="magnify" type="win32gui" installbase="system32" installname="magnify.exe">
	<include base="magnify">.</include>
	<library>user32</library>
	<library>gdi32</library>
	<library>advapi32</library>
	<library>shell32</library>
	<library>kernel32</library>
	<file>magnifier.c</file>
	<file>settings.c</file>
	<file>magnify.rc</file>
</module>
