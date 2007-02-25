<module name="bzip2" type="kernelmodedll" entrypoint="0" installbase="system32/drivers" installname="bzip2.dll">
	<importlibrary definition="unbzip2.def" />
	<define name="BZ_NO_STDIO" />
	<define name="BZ_DECOMPRESS_ONLY" />
	<define name="__USE_W32API" />
	<linkerflag>-lgcc</linkerflag>
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>bzlib.c</file>
	<file>randtable.c</file>
	<file>crctable.c</file>
	<file>decompress.c</file>
	<file>huffman.c</file>
	<file>dllmain.c</file>
</module>
