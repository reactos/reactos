PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = regedt32

TARGET_CFLAGS = -DGCC \
  -D__USE_W32API \
  -D__REACTOS__ \
  -D_WIN32_IE=0x600 \
  -D_WIN32_WINNT=0x501 \
  -DWINVER=0x501

TARGET_RCFLAGS = -DGCC -D_WIN32_IE=0x0400 -D__USE_W32API

TARGET_SDKLIBS = \
  kernel32.a \
  user32.a \
  gdi32.a \
  advapi32.a

TARGET_GCCLIBS = comctl32 comdlg32

TARGET_OBJECTS = \
  framewnd.o \
  childwnd.o \
  listview.o \
  treeview.o \
  settings.o \
  main.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
