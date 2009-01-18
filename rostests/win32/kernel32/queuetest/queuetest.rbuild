<module name="queuetest" type="win32cui" installbase="system32" installname="queuetest.exe">
        <define name="__USE_W32API" />
	<library>kernel32</library>
	<file>queuetest.c</file>
</module>
