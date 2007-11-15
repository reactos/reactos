<?xml version="1.0"?>
<!DOCTYPE module SYSTEM "../../../../tools/rbuild/project.dtd">
<module name="more" type="win32cui" installbase="system32" installname="more.exe">
	<define name="__USE_W32API" />
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<family>applications</family>
	<family>cuiapplications</family>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>user32</library>
	<file>more.c</file>
	<file>more.rc</file>
</module>
