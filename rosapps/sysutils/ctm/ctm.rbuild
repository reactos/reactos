<module name="ctm" type="win32cui" installbase="system32" installname="ctm.exe">
	<define name="__USE_W32API" />
	<define name="UNICODE" />
	<define name="_UNICODE" />
	<library>epsapi</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>ctm.c</file>
	<file>ctm.rc</file>
</module>
