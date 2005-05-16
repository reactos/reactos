PATH_TO_TOP = ../../..

TARGET_TYPE = program

TARGET_APPTYPE = windows

TARGET_NAME = reporterror

TARGET_INSTALLDIR = system32

TARGET_CFLAGS = -D__USE_W32API -DUNICODE -D_WIN32_IE=0x0501 -D_WIN32_WINNT=0x0501

TARGET_SDKLIBS = advapi32.a comctl32.a comdlg32.a shell32.a ws2_32.a

TARGET_OBJECTS = \
  $(TARGET_NAME).o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk
