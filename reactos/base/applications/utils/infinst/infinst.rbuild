<module name="infinst" type="win32cui" installbase="bin" installname="infinst.exe" >
	<define name="__USE_W32API" />
	<library>msvcrt</library>
	<library>kernel32</library>
	<library>ntdll</library>
	<library>user32</library>
	<library>setupapi</library>
	<library>comdlg32</library>
	<file>infinst.c</file>
</module>