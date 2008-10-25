<module name="msacm32" type="win32dll" baseaddress="${BASEADDRESS_MSACM32}" installbase="system32" installname="msacm32.dll" unicode="yes">
	<importlibrary definition="msacm32.spec" />
	<include base="msacm32">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>driver.c</file>
	<file>filter.c</file>
	<file>format.c</file>
	<file>internal.c</file>
	<file>msacm32_main.c</file>
	<file>pcmconverter.c</file>
	<file>stream.c</file>
	<file>msacm32.spec</file>
</module>
