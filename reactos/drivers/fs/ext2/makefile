OBJECTS = super.o blockdev.o inode.o file.o dir.o rw.o

all: $(OBJECTS)
	$(LD) -r $(OBJECTS) -o ext2fs.sys
