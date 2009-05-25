<module name="msacm32.drv" type="win32dll" entrypoint="0" extension=".drv" baseaddress="${BASEADDRESS_WAVEMAP}" installbase="system32" installname="msacm32.drv" unicode="yes">
	<importlibrary definition="msacm32.drv.spec" />
	<include base="msacm32.drv">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>msacm32</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>wavemap.c</file>
	<file>wavemap.rc</file>
</module>
