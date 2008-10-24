<module name="printui" type="win32dll" baseaddress="${BASEADDRESS_PRINTUI}" installbase="system32" installname="printui.dll" allowwarnings="true" entrypoint="0">
        <importlibrary definition="printui.spec" />
        <include base="printui">.</include>
        <include base="ReactOS">include/reactos/wine</include>
        <define name="__WINESRC__" />
        <define name="__USE_W32API" />
        <define name="_WIN32_IE">0x600</define>
        <define name="_WIN32_WINNT">0x501</define>
        <define name="WINVER">0x501</define>
        <library>wine</library>
        <library>shell32</library>
        <library>kernel32</library>
        <library>ntdll</library>
        <file>printui.c</file>
        <file>printui.rc</file>
        <file>printui.spec</file>
</module>
