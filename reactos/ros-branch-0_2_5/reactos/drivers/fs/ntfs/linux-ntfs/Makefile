# Rules for making the NTFS driver.

obj-$(CONFIG_NTFS_FS) += ntfs.o

ntfs-objs := aops.o attrib.o compress.o debug.o dir.o file.o inode.o mft.o \
	     mst.o namei.o super.o sysctl.o time.o unistr.o upcase.o

EXTRA_CFLAGS = -DNTFS_VERSION=\"2.1.5\"

ifeq ($(CONFIG_NTFS_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

ifeq ($(CONFIG_NTFS_RW),y)
EXTRA_CFLAGS += -DNTFS_RW
endif
