<module name="wavemap" type="win32dll" entrypoint="0" extension=".drv" baseaddress="${BASEADDRESS_WAVEMAP}" installbase="system32" installname="msacm32.drv" allowwarnings="true" unicode="yes">
	<importlibrary definition="msacm.spec" />
	<include base="wavemap">.</include>
	<include base="ReactOS">include/reactos/wine</include>
	<library>wine</library>
	<library>msacm32</library>
	<library>uuid</library>
	<library>ntdll</library>
	<library>kernel32</library>
	<library>advapi32</library>
	<library>user32</library>
	<library>winmm</library>
	<file>wavemap.c</file>
	<file>wavemap.rc</file>
</module>
