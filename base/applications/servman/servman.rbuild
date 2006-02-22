<?xml version="1.0"?>
<rbuild xmlns:xi="http://www.w3.org/2001/XInclude">
  <module name="servman" type="win32gui" installbase="system32" installname="servman.exe">
    <include base="servman">.</include>
    <define name="UNICODE" />
    <define name="_UNICODE" />
    <define name="__REACTOS__" />
    <define name="__USE_W32API" />
    <define name="_WIN32_IE">0x600</define>
    <define name="_WIN32_WINNT">0x501</define>
    <library>kernel32</library>
    <library>user32</library>
    <library>advapi32</library>
    <library>version</library>
    <library>comctl32</library>
    <library>shell32</library>
    <library>comdlg32</library>
    <compilationunit name="unit.c">
      <file>about.c</file>
      <file>control.c</file>
      <file>create.c</file>
      <file>delete.c</file>
      <file>export.c</file>
      <file>geterror.c</file>
      <file>progress.c</file>
      <file>propsheet.c</file>
      <file>query.c</file>
      <file>servman.c</file>
      <file>start.c</file>		
    </compilationunit>
    <file>servman.rc</file>
    <pch>servman.h</pch>
  </module>
</rbuild>
