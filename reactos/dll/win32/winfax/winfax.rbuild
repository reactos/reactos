<module name="winfax" type="win32dll" baseaddress="${BASEADDRESS_WINFAX}" installbase="system32" installname="winfax.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="winfax.def" />
        <include base="winfax">.</include>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>winfax.c</file>
        <file>winfax.rc</file>
</module>