<module name="kdcom" type="kernelmodedll" entrypoint="DriverEntry@8" installbase="system32/drivers" installname="kdcom.dll">
	<importlibrary definition="kdcom.def"></importlibrary>
	<bootstrap base="$(CDOUTPUT)" nameoncd="kdcom.dll" />
	<include base="kdcom">.</include>
	<define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
	<file>kdbg.c</file>
</module>
