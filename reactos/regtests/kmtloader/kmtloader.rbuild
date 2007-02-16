<module name="kmtloader" type="win32cui" installbase="system32" installname="kmtloader.exe">
        <define name="__USE_W32API" />
	<library>kernel32</library>
	<library>advapi32</library>
	<file>kmtloader.c</file>
</module>
