<module name="lsdd" type="win32cui" installbase="system32" installname="lsdd.exe">
	<define name="_WIN32_IE">0x0501</define>
	<define name="_WIN32_WINNT">0x0501</define>
	<library>win32err</library>
	<library>kernel32</library>
	<library>user32</library>

	<file>lsdd.c</file>
	<file>lsdd.rc</file>
</module>
