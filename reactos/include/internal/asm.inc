%macro DECLARE_EXTERNAL_SYMBOL 1
%ifdef coff
extern _%1
%elifdef win32
extern _%1
%elifdef elf
extern %1
_%1:
      call %1
      ret
%endif
%endmacro

%macro DECLARE_GLOBAL_SYMBOL 1
%ifdef coff
global _%1
_%1:
%elifdef win32
global _%1
_%1:
%elifdef elf
global %1
%1:
%endif
%endmacro
