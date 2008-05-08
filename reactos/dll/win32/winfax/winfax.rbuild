<module name="winfax" type="win32dll" baseaddress="${BASEADDRESS_WINFAX}" installbase="system32" installname="winfax.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="winfax.def" />
        <include base="winfax">.</include>
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>winfax.c</file>
        <file>winfax.rc</file>
</module>