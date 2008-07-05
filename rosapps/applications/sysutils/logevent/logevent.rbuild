<?xml version="1.0"?>

<module name="logevent" type="win32cui" installbase="system32" installname="logevent.exe" allowwarnings="true">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>advapi32</library>
	<file>logevent.c</file>
	<file>logevent.rc</file>
</module>
