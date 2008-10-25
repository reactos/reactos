<module name="threadwait" type="win32gui" installbase="bin" installname="threadwait.exe">
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ntdll</library>
	<file>threadwait.c</file>
</module>
