<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="iexplore" type="win32gui" installbase="system32" installname="iexplore.exe" unicode="no">
	<include base="iexplore">.</include>
	<library>kernel32</library>
	<library>user32</library>
	<library>gdi32</library>
        <library>shdocvw</library>
	<file>main.c</file>
	<file>version.rc</file>
</module>
