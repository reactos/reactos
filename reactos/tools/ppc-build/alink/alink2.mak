CC = cl /Zl /c /Ox

%.obj: %.c
	$(CC) /Fo$@ $<

all: blink.exe

alink.obj coff.obj cofflib.obj combine.obj output.obj objload.obj util.obj: alink.h

blink.exe: alink.obj coff.obj cofflib.obj combine.obj output.obj objload.obj util.obj
	alink -subsys con @lib/def.rsp -o $@ $^
