<module name="binpatch" type="win32cui" installbase="bin" installname="binpatch.exe" >
	<define name="__USE_W32API" />
	<library>kernel32</library>
	<library>ntdll</library>
	<file>patch.c</file>
</module>