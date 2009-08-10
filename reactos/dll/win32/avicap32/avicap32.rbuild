<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../tools/rbuild/project.dtd">
<module name="avicap32" type="win32dll" baseaddress="${BASEADDRESS_AVICAP32}" installbase="system32" installname="avicap32.dll" unicode="yes">
	<importlibrary definition="avicap32.spec" />
	<include base="avicap32">.</include>
	<library>advapi32</library>
	<library>kernel32</library>
	<library>msvfw32</library>
	<library>user32</library>
	<library>gdi32</library>
	<library>wine</library>
	<library>winmm</library>
	<library>version</library>
	<file>avicap32.c</file>
	<file>avicap32.rc</file>
</module>
