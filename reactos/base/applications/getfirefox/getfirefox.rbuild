<?xml version="1.0"?>
<!DOCTYPE project SYSTEM "tools/rbuild/project.dtd">
<module name="getfirefox" type="win32gui" installbase="system32" installname="getfirefox.exe">
    <include base="getfirefox">.</include>
    <define name="UNICODE" />
    <define name="_UNICODE" />
    <define name="__USE_W32API" />
    <define name="WINVER">0x0501</define>
    <define name="_WIN32_IE>0x0600</define>
    <library>comctl32</library>
    <library>ntdll</library>
    <library>shell32</library>
    <library>shlwapi</library>
    <library>urlmon</library>
    <library>uuid</library>
    <pch>precomp.h</pch>
    <file>getfirefox.c</file>
    <file>getfirefox.rc</file>
</module>
