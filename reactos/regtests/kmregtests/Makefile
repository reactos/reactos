# $Id: Makefile,v 1.2 2003/11/19 05:53:37 vizzini Exp $

PATH_TO_TOP = ../..

TARGET_NORC = yes

TARGET_TYPE = driver

TARGET_NAME = kmregtests

TARGET_LIBS = $(SDK_PATH_LIB)/rtshared.a

TARGET_CFLAGS = -I../shared -Werror -Wall

TARGET_GENREGTESTS = yes

TARGET_OBJECTS = \
  _regtests.o \
  driver.o

include $(PATH_TO_TOP)/rules.mak

include $(TOOLS_PATH)/helper.mk

# EOF
