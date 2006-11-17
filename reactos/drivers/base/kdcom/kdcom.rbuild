<module name="kdcom" type="kernelmodedll" entrypoint="DriverEntry@8" installbase="system32/drivers" installname="kdcom.sys">
	<importlibrary definition="kdcom.def"></importlibrary>
    <bootstrap base="reactos" nameoncd="kdcom.sys" />
    <include base="kdcom">.</include>
        <define name="__USE_W32API" />
	<library>ntoskrnl</library>
	<library>hal</library>
    <file>kdbg.c</file>
</module>
