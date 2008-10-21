<module name="nddeapi" type="win32dll" baseaddress="${BASEADDRESS_NDDEAPI}" installbase="system32" installname="nddeapi.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="nddeapi.spec" />
        <include base="nddeapi">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>nddeapi.c</file>
        <file>nddeapi.spec</file>
</module>
