OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o

all: ext2.o

ext2.o: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o ext2fs.sys

include ../../../rules.mak

