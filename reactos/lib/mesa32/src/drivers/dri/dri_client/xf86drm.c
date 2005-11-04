/**
 * \file xf86drm.c 
 * User-level interface to DRM device
 *
 * \author Rickard E. (Rik) Faith <faith@valinux.com>
 * \author Kevin E. Martin <martin@valinux.com>
 */

/*
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * Copyright 2000 VA Linux Systems, Inc., Sunnyvale, California.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/xf86drm.c,v 1.36 2003/08/24 17:35:35 tsi Exp $ */

#ifdef XFree86Server
# include "xf86.h"
# include "xf86_OSproc.h"
# include "drm.h"
# include "xf86_ansic.h"
# define _DRM_MALLOC xalloc
# define _DRM_FREE   xfree
# ifndef XFree86LOADER
#  include <sys/mman.h>
# endif
#else
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <ctype.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/stat.h>
# define stat_t struct stat
# include <sys/ioctl.h>
# include <sys/mman.h>
# include <sys/time.h>
# include <stdarg.h>
# ifdef DRM_USE_MALLOC
#  define _DRM_MALLOC malloc
#  define _DRM_FREE   free
# else
#  include <X11/Xlibint.h>
#  define _DRM_MALLOC Xmalloc
#  define _DRM_FREE   Xfree
# endif
# include "drm.h"
#endif

/* No longer needed with CVS kernel modules on alpha 
#if defined(__alpha__) && defined(__linux__)
extern unsigned long _bus_base(void);
#define BUS_BASE _bus_base()
#endif
*/

/* Not all systems have MAP_FAILED defined */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "xf86drm.h"

#ifdef __FreeBSD__
#define DRM_MAJOR 145
#endif

#ifdef __NetBSD__
#define DRM_MAJOR 34
#endif

# ifdef __OpenBSD__
#  define DRM_MAJOR 81
# endif

#ifndef DRM_MAJOR
#define DRM_MAJOR 226		/* Linux */
#endif

#ifndef DRM_MAX_MINOR
#define DRM_MAX_MINOR 16
#endif

#ifdef __linux__
#include <sys/sysmacros.h>	/* for makedev() */
#endif

#ifndef makedev
				/* This definition needs to be changed on
                                   some systems if dev_t is a structure.
                                   If there is a header file we can get it
                                   from, there would be best. */
#define makedev(x,y)    ((dev_t)(((x) << 8) | (y)))
#endif

#define DRM_MSG_VERBOSITY 3

/**
 * Output a message to stderr.
 *
 * \param format printf() like format string.
 *
 * \internal
 * This function is a wrapper around vfprintf().
 */
static void
drmMsg(const char *format, ...)
{
    va_list	ap;

#ifndef XFree86Server
    const char *env;
    if ((env = getenv("LIBGL_DEBUG")) && strstr(env, "verbose"))
#endif
    {
	va_start(ap, format);
#ifdef XFree86Server
	xf86VDrvMsgVerb(-1, X_NONE, DRM_MSG_VERBOSITY, format, ap);
#else
	vfprintf(stderr, format, ap);
#endif
	va_end(ap);
    }
}

static void *drmHashTable = NULL; /* Context switch callbacks */

typedef struct drmHashEntry {
    int      fd;
    void     (*f)(int, void *, void *);
    void     *tagTable;
} drmHashEntry;

void *drmMalloc(int size)
{
    void *pt;
    if ((pt = _DRM_MALLOC(size))) memset(pt, 0, size);
    return pt;
}

void drmFree(void *pt)
{
    if (pt) _DRM_FREE(pt);
}

/* drmStrdup can't use strdup(3), since it doesn't call _DRM_MALLOC... */
static char *drmStrdup(const char *s)
{
    char *retval = NULL;

    if (s) {
	retval = _DRM_MALLOC(strlen(s)+1);
	strcpy(retval, s);
    }
    return retval;
}


static unsigned long drmGetKeyFromFd(int fd)
{
    stat_t     st;

    st.st_rdev = 0;
    fstat(fd, &st);
    return st.st_rdev;
}

static drmHashEntry *drmGetEntry(int fd)
{
    unsigned long key = drmGetKeyFromFd(fd);
    void          *value;
    drmHashEntry  *entry;

    if (!drmHashTable) drmHashTable = drmHashCreate();

    if (drmHashLookup(drmHashTable, key, &value)) {
	entry           = drmMalloc(sizeof(*entry));
	entry->fd       = fd;
	entry->f        = NULL;
	entry->tagTable = drmHashCreate();
	drmHashInsert(drmHashTable, key, entry);
    } else {
	entry = value;
    }
    return entry;
}

/**
 * Compare two busid strings
 *
 * \param first
 * \param second
 *
 * \return 1 if matched.
 *
 * \internal
 * This function compares two bus ID strings.  It understands the older
 * PCI:b:d:f format and the newer pci:oooo:bb:dd.f format.  In the format, o is
 * domain, b is bus, d is device, f is function.
 */
static int drmMatchBusID(const char *id1, const char *id2)
{
    /* First, check if the IDs are exactly the same */
    if (strcasecmp(id1, id2) == 0)
	return 1;

    /* Try to match old/new-style PCI bus IDs. */
    if (strncasecmp(id1, "pci", 3) == 0) {
	int o1, b1, d1, f1;
	int o2, b2, d2, f2;
	int ret;

	ret = sscanf(id1, "pci:%04x:%02x:%02x.%d", &o1, &b1, &d1, &f1);
	if (ret != 4) {
	    o1 = 0;
	    ret = sscanf(id1, "PCI:%d:%d:%d", &b1, &d1, &f1);
	    if (ret != 3)
		return 0;
	}

	ret = sscanf(id2, "pci:%04x:%02x:%02x.%d", &o2, &b2, &d2, &f2);
	if (ret != 4) {
	    o2 = 0;
	    ret = sscanf(id2, "PCI:%d:%d:%d", &b2, &d2, &f2);
	    if (ret != 3)
		return 0;
	}

	if ((o1 != o2) || (b1 != b2) || (d1 != d2) || (f1 != f2))
	    return 0;
	else
	    return 1;
    }
    return 0;
}

/**
 * Open the DRM device, creating it if necessary.
 *
 * \param dev major and minor numbers of the device.
 * \param minor minor number of the device.
 * 
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * Assembles the device name from \p minor and opens it, creating the device
 * special file node with the major and minor numbers specified by \p dev and
 * parent directory if necessary and was called by root.
 */
static int drmOpenDevice(long dev, int minor)
{
    stat_t          st;
    char            buf[64];
    int             fd;
    mode_t          devmode = DRM_DEV_MODE;
    int             isroot  = !geteuid();
#if defined(XFree86Server)
    uid_t           user    = DRM_DEV_UID;
    gid_t           group   = DRM_DEV_GID;
#endif

    sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, minor);
    drmMsg("drmOpenDevice: node name is %s\n", buf);

#if defined(XFree86Server)
    devmode  = xf86ConfigDRI.mode ? xf86ConfigDRI.mode : DRM_DEV_MODE;
    devmode &= ~(S_IXUSR|S_IXGRP|S_IXOTH);
    group = (xf86ConfigDRI.group >= 0) ? xf86ConfigDRI.group : DRM_DEV_GID;
#endif

    if (stat(DRM_DIR_NAME, &st)) {
	if (!isroot) return DRM_ERR_NOT_ROOT;
	mkdir(DRM_DIR_NAME, DRM_DEV_DIRMODE);
	chown(DRM_DIR_NAME, 0, 0); /* root:root */
	chmod(DRM_DIR_NAME, DRM_DEV_DIRMODE);
    }

    /* Check if the device node exists and create it if necessary. */
    if (stat(buf, &st)) {
	if (!isroot) return DRM_ERR_NOT_ROOT;
	remove(buf);
	mknod(buf, S_IFCHR | devmode, dev);
    }
#if defined(XFree86Server)
    chown(buf, user, group);
    chmod(buf, devmode);
#endif

    fd = open(buf, O_RDWR, 0);
    drmMsg("drmOpenDevice: open result is %d, (%s)\n",
		fd, fd < 0 ? strerror(errno) : "OK");
    if (fd >= 0) return fd;

    /* Check if the device node is not what we expect it to be, and recreate it
     * and try again if so.
     */
    if (st.st_rdev != dev) {
	if (!isroot) return DRM_ERR_NOT_ROOT;
	remove(buf);
	mknod(buf, S_IFCHR | devmode, dev);
#if defined(XFree86Server)
	chown(buf, user, group);
	chmod(buf, devmode);
#endif
    }
    fd = open(buf, O_RDWR, 0);
    drmMsg("drmOpenDevice: open result is %d, (%s)\n",
		fd, fd < 0 ? strerror(errno) : "OK");
    if (fd >= 0) return fd;

    drmMsg("drmOpenDevice: Open failed\n");
    remove(buf);
    return -errno;
}


/**
 * Open the DRM device
 *
 * \param minor device minor number.
 * \param create allow to create the device if set.
 *
 * \return a file descriptor on success, or a negative value on error.
 * 
 * \internal
 * Calls drmOpenDevice() if \p create is set, otherwise assembles the device
 * name from \p minor and opens it.
 */
static int drmOpenMinor(int minor, int create)
{
    int  fd;
    char buf[64];
    
    if (create) return drmOpenDevice(makedev(DRM_MAJOR, minor), minor);
    
    sprintf(buf, DRM_DEV_NAME, DRM_DIR_NAME, minor);
    if ((fd = open(buf, O_RDWR, 0)) >= 0) return fd;
    return -errno;
}


/**
 * Determine whether the DRM kernel driver has been loaded.
 * 
 * \return 1 if the DRM driver is loaded, 0 otherwise.
 *
 * \internal 
 * Determine the presence of the kernel driver by attempting to open the 0
 * minor and get version information.  For backward compatibility with older
 * Linux implementations, /proc/dri is also checked.
 */
int drmAvailable(void)
{
    drmVersionPtr version;
    int           retval = 0;
    int           fd;

    if ((fd = drmOpenMinor(0, 1)) < 0) {
#ifdef __linux__
				/* Try proc for backward Linux compatibility */
	if (!access("/proc/dri/0", R_OK)) return 1;
#endif
	return 0;
    }
    
    if ((version = drmGetVersion(fd))) {
	retval = 1;
	drmFreeVersion(version);
    }
    close(fd);

    return retval;
}


/**
 * Open the device by bus ID.
 *
 * \param busid bus ID.
 *
 * \return a file descriptor on success, or a negative value on error.
 *
 * \internal
 * This function attempts to open every possible minor (up to DRM_MAX_MINOR),
 * comparing the device bus ID with the one supplied.
 *
 * \sa drmOpenMinor() and drmGetBusid().
 */
static int drmOpenByBusid(const char *busid)
{
    int        i;
    int        fd;
    const char *buf;
    drmSetVersion sv;

    drmMsg("drmOpenByBusid: Searching for BusID %s\n", busid);
    for (i = 0; i < DRM_MAX_MINOR; i++) {
	fd = drmOpenMinor(i, 1);
	drmMsg("drmOpenByBusid: drmOpenMinor returns %d\n", fd);
	if (fd >= 0) {
	    sv.drm_di_major = 1;
	    sv.drm_di_minor = 1;
	    sv.drm_dd_major = -1;	/* Don't care */
	    drmSetInterfaceVersion(fd, &sv);
	    buf = drmGetBusid(fd);
	    drmMsg("drmOpenByBusid: drmGetBusid reports %s\n", buf);
	    if (buf && drmMatchBusID(buf, busid)) {
		drmFreeBusid(buf);
		return fd;
	    }
	    if (buf) drmFreeBusid(buf);
	    close(fd);
	}
    }
    return -1;
}


/**
 * Open the device by name.
 *
 * \param name driver name.
 * 
 * \return a file descriptor on success, or a negative value on error.
 * 
 * \internal
 * This function opens the first minor number that matches the driver name and
 * isn't already in use.  If it's in use it then it will already have a bus ID
 * assigned.
 * 
 * \sa drmOpenMinor(), drmGetVersion() and drmGetBusid().
 */
static int drmOpenByName(const char *name)
{
    int           i;
    int           fd;
    drmVersionPtr version;
    char *        id;
    
    if (!drmAvailable()) {
#if !defined(XFree86Server)
	return -1;
#else
        /* try to load the kernel module now */
        if (!xf86LoadKernelModule(name)) {
            ErrorF("[drm] failed to load kernel module \"%s\"\n",
		   name);
            return -1;
        }
#endif
    }

    /*
     * Open the first minor number that matches the driver name and isn't
     * already in use.  If it's in use it will have a busid assigned already.
     */
    for (i = 0; i < DRM_MAX_MINOR; i++) {
	if ((fd = drmOpenMinor(i, 1)) >= 0) {
	    if ((version = drmGetVersion(fd))) {
		if (!strcmp(version->name, name)) {
		    drmFreeVersion(version);
		    id = drmGetBusid(fd);
		    drmMsg("drmGetBusid returned '%s'\n", id ? id : "NULL");
		    if (!id || !*id) {
			if (id) {
			    drmFreeBusid(id);
			}
			return fd;
		    } else {
			drmFreeBusid(id);
		    }
		} else {
		    drmFreeVersion(version);
		}
	    }
	    close(fd);
	}
    }

#ifdef __linux__
				/* Backward-compatibility /proc support */
    for (i = 0; i < 8; i++) {
	char proc_name[64], buf[512];
	char *driver, *pt, *devstring;
	int  retcode;
	
	sprintf(proc_name, "/proc/dri/%d/name", i);
	if ((fd = open(proc_name, 0, 0)) >= 0) {
	    retcode = read(fd, buf, sizeof(buf)-1);
	    close(fd);
	    if (retcode) {
		buf[retcode-1] = '\0';
		for (driver = pt = buf; *pt && *pt != ' '; ++pt)
		    ;
		if (*pt) {	/* Device is next */
		    *pt = '\0';
		    if (!strcmp(driver, name)) { /* Match */
			for (devstring = ++pt; *pt && *pt != ' '; ++pt)
			    ;
			if (*pt) { /* Found busid */
			    return drmOpenByBusid(++pt);
			} else {	/* No busid */
			    return drmOpenDevice(strtol(devstring, NULL, 0),i);
			}
		    }
		}
	    }
	}
    }
#endif

    return -1;
}


/**
 * Open the DRM device.
 *
 * Looks up the specified name and bus ID, and opens the device found.  The
 * entry in /dev/dri is created if necessary and if called by root.
 *
 * \param name driver name. Not referenced if bus ID is supplied.
 * \param busid bus ID. Zero if not known.
 * 
 * \return a file descriptor on success, or a negative value on error.
 * 
 * \internal
 * It calls drmOpenByBusid() if \p busid is specified or drmOpenByName()
 * otherwise.
 */
int drmOpen(const char *name, const char *busid)
{
#ifdef XFree86Server
    if (!drmAvailable() && name != NULL) {
	/* try to load the kernel */
	if (!xf86LoadKernelModule(name)) {
	    ErrorF("[drm] failed to load kernel module \"%s\"\n",
	           name);
	    return -1;
	}
    }
#endif

    if (busid) {
	int fd;

	fd = drmOpenByBusid(busid);
	if (fd >= 0)
	    return fd;
    }
    if (name)
	return drmOpenByName(name);
    return -1;
}


/**
 * Free the version information returned by drmGetVersion().
 *
 * \param v pointer to the version information.
 *
 * \internal
 * It frees the memory pointed by \p %v as well as all the non-null strings
 * pointers in it.
 */
void drmFreeVersion(drmVersionPtr v)
{
    if (!v) return;
    if (v->name) drmFree(v->name);
    if (v->date) drmFree(v->date);
    if (v->desc) drmFree(v->desc);
    drmFree(v);
}


/**
 * Free the non-public version information returned by the kernel.
 *
 * \param v pointer to the version information.
 *
 * \internal
 * Used by drmGetVersion() to free the memory pointed by \p %v as well as all
 * the non-null strings pointers in it.
 */
static void drmFreeKernelVersion(drm_version_t *v)
{
    if (!v) return;
    if (v->name) drmFree(v->name);
    if (v->date) drmFree(v->date);
    if (v->desc) drmFree(v->desc);
    drmFree(v);
}


/**
 * Copy version information.
 * 
 * \param d destination pointer.
 * \param s source pointer.
 * 
 * \internal
 * Used by drmGetVersion() to translate the information returned by the ioctl
 * interface in a private structure into the public structure counterpart.
 */
static void drmCopyVersion(drmVersionPtr d, const drm_version_t *s)
{
    d->version_major      = s->version_major;
    d->version_minor      = s->version_minor;
    d->version_patchlevel = s->version_patchlevel;
    d->name_len           = s->name_len;
    d->name               = drmStrdup(s->name);
    d->date_len           = s->date_len;
    d->date               = drmStrdup(s->date);
    d->desc_len           = s->desc_len;
    d->desc               = drmStrdup(s->desc);
}


/**
 * Query the driver version information.
 *
 * \param fd file descriptor.
 * 
 * \return pointer to a drmVersion structure which should be freed with
 * drmFreeVersion().
 * 
 * \note Similar information is available via /proc/dri.
 * 
 * \internal
 * It gets the version information via successive DRM_IOCTL_VERSION ioctls,
 * first with zeros to get the string lengths, and then the actually strings.
 * It also null-terminates them since they might not be already.
 */
drmVersionPtr drmGetVersion(int fd)
{
    drmVersionPtr retval;
    drm_version_t *version = drmMalloc(sizeof(*version));

				/* First, get the lengths */
    version->name_len    = 0;
    version->name        = NULL;
    version->date_len    = 0;
    version->date        = NULL;
    version->desc_len    = 0;
    version->desc        = NULL;

    if (ioctl(fd, DRM_IOCTL_VERSION, version)) {
	drmFreeKernelVersion(version);
	return NULL;
    }

				/* Now, allocate space and get the data */
    if (version->name_len)
	version->name    = drmMalloc(version->name_len + 1);
    if (version->date_len)
	version->date    = drmMalloc(version->date_len + 1);
    if (version->desc_len)
	version->desc    = drmMalloc(version->desc_len + 1);

    if (ioctl(fd, DRM_IOCTL_VERSION, version)) {
	drmMsg("DRM_IOCTL_VERSION: %s\n", strerror(errno));
	drmFreeKernelVersion(version);
	return NULL;
    }

				/* The results might not be null-terminated
                                   strings, so terminate them. */

    if (version->name_len) version->name[version->name_len] = '\0';
    if (version->date_len) version->date[version->date_len] = '\0';
    if (version->desc_len) version->desc[version->desc_len] = '\0';

				/* Now, copy it all back into the
                                   client-visible data structure... */
    retval = drmMalloc(sizeof(*retval));
    drmCopyVersion(retval, version);
    drmFreeKernelVersion(version);
    return retval;
}


/**
 * Get version information for the DRM user space library.
 * 
 * This version number is driver independent.
 * 
 * \param fd file descriptor.
 *
 * \return version information.
 * 
 * \internal
 * This function allocates and fills a drm_version structure with a hard coded
 * version number.
 */
drmVersionPtr drmGetLibVersion(int fd)
{
    drm_version_t *version = drmMalloc(sizeof(*version));

    /* Version history:
     *   revision 1.0.x = original DRM interface with no drmGetLibVersion
     *                    entry point and many drm<Device> extensions
     *   revision 1.1.x = added drmCommand entry points for device extensions
     *                    added drmGetLibVersion to identify libdrm.a version
     *   revision 1.2.x = added drmSetInterfaceVersion
     *                    modified drmOpen to handle both busid and name
     */
    version->version_major      = 1;
    version->version_minor      = 2;
    version->version_patchlevel = 0;

    return (drmVersionPtr)version;
}


/**
 * Free the bus ID information.
 *
 * \param busid bus ID information string as given by drmGetBusid().
 *
 * \internal
 * This function is just frees the memory pointed by \p busid.
 */
void drmFreeBusid(const char *busid)
{
    drmFree((void *)busid);
}


/**
 * Get the bus ID of the device.
 *
 * \param fd file descriptor.
 *
 * \return bus ID string.
 *
 * \internal
 * This function gets the bus ID via successive DRM_IOCTL_GET_UNIQUE ioctls to
 * get the string length and data, passing the arguments in a drm_unique
 * structure.
 */
char *drmGetBusid(int fd)
{
    drm_unique_t u;

    u.unique_len = 0;
    u.unique     = NULL;

    if (ioctl(fd, DRM_IOCTL_GET_UNIQUE, &u)) return NULL;
    u.unique = drmMalloc(u.unique_len + 1);
    if (ioctl(fd, DRM_IOCTL_GET_UNIQUE, &u)) return NULL;
    u.unique[u.unique_len] = '\0';

    return u.unique;
}


/**
 * Set the bus ID of the device.
 *
 * \param fd file descriptor.
 * \param busid bus ID string.
 *
 * \return zero on success, negative on failure.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_SET_UNIQUE ioctl, passing
 * the arguments in a drm_unique structure.
 */
int drmSetBusid(int fd, const char *busid)
{
    drm_unique_t u;

    u.unique     = (char *)busid;
    u.unique_len = strlen(busid);

    if (ioctl(fd, DRM_IOCTL_SET_UNIQUE, &u)) {
	return -errno;
    }
    return 0;
}

int drmGetMagic(int fd, drm_magic_t * magic)
{
    drm_auth_t auth;

    *magic = 0;
    if (ioctl(fd, DRM_IOCTL_GET_MAGIC, &auth)) return -errno;
    *magic = auth.magic;
    return 0;
}

int drmAuthMagic(int fd, drm_magic_t magic)
{
    drm_auth_t auth;

    auth.magic = magic;
    if (ioctl(fd, DRM_IOCTL_AUTH_MAGIC, &auth)) return -errno;
    return 0;
}

/**
 * Specifies a range of memory that is available for mapping by a
 * non-root process.
 *
 * \param fd file descriptor.
 * \param offset usually the physical address. The actual meaning depends of
 * the \p type parameter. See below.
 * \param size of the memory in bytes.
 * \param type type of the memory to be mapped.
 * \param flags combination of several flags to modify the function actions.
 * \param handle will be set to a value that may be used as the offset
 * parameter for mmap().
 * 
 * \return zero on success or a negative value on error.
 *
 * \par Mapping the frame buffer
 * For the frame buffer
 * - \p offset will be the physical address of the start of the frame buffer,
 * - \p size will be the size of the frame buffer in bytes, and
 * - \p type will be DRM_FRAME_BUFFER.
 *
 * \par
 * The area mapped will be uncached. If MTRR support is available in the
 * kernel, the frame buffer area will be set to write combining. 
 *
 * \par Mapping the MMIO register area
 * For the MMIO register area,
 * - \p offset will be the physical address of the start of the register area,
 * - \p size will be the size of the register area bytes, and
 * - \p type will be DRM_REGISTERS.
 * \par
 * The area mapped will be uncached. 
 * 
 * \par Mapping the SAREA
 * For the SAREA,
 * - \p offset will be ignored and should be set to zero,
 * - \p size will be the desired size of the SAREA in bytes,
 * - \p type will be DRM_SHM.
 * 
 * \par
 * A shared memory area of the requested size will be created and locked in
 * kernel memory. This area may be mapped into client-space by using the handle
 * returned. 
 * 
 * \note May only be called by root.
 *
 * \internal
 * This function is a wrapper around the DRM_IOCTL_ADD_MAP ioctl, passing
 * the arguments in a drm_map structure.
 */
int drmAddMap(int fd,
	      drm_handle_t offset,
	      drmSize size,
	      drmMapType type,
	      drmMapFlags flags,
	      drm_handle_t * handle)
{
    drm_map_t map;

    map.offset  = offset;
/* No longer needed with CVS kernel modules on alpha
#ifdef __alpha__
    if (type != DRM_SHM)
	map.offset += BUS_BASE;
#endif
*/
    map.size    = size;
    map.handle  = 0;
    map.type    = type;
    map.flags   = flags;
    if (ioctl(fd, DRM_IOCTL_ADD_MAP, &map)) return -errno;
    if (handle) *handle = (drm_handle_t)map.handle;
    return 0;
}

int drmRmMap(int fd, drm_handle_t handle)
{
    drm_map_t map;

    map.handle = (void *)handle;

    if(ioctl(fd, DRM_IOCTL_RM_MAP, &map)) return -errno;
    return 0;
}

/**
 * Make buffers available for DMA transfers.
 * 
 * \param fd file descriptor.
 * \param count number of buffers.
 * \param size size of each buffer.
 * \param flags buffer allocation flags.
 * \param agp_offset offset in the AGP aperture 
 *
 * \return number of buffers allocated, negative on error.
 *
 * \internal
 * This function is a wrapper around DRM_IOCTL_ADD_BUFS ioctl.
 *
 * \sa drm_buf_desc.
 */
int drmAddBufs(int fd, int count, int size, drmBufDescFlags flags,
	       int agp_offset)
{
    drm_buf_desc_t request;

    request.count     = count;
    request.size      = size;
    request.low_mark  = 0;
    request.high_mark = 0;
    request.flags     = flags;
    request.agp_start = agp_offset;

    if (ioctl(fd, DRM_IOCTL_ADD_BUFS, &request)) return -errno;
    return request.count;
}

int drmMarkBufs(int fd, double low, double high)
{
    drm_buf_info_t info;
    int            i;

    info.count = 0;
    info.list  = NULL;

    if (ioctl(fd, DRM_IOCTL_INFO_BUFS, &info)) return -EINVAL;

    if (!info.count) return -EINVAL;

    if (!(info.list = drmMalloc(info.count * sizeof(*info.list))))
	return -ENOMEM;

    if (ioctl(fd, DRM_IOCTL_INFO_BUFS, &info)) {
	int retval = -errno;
	drmFree(info.list);
	return retval;
    }

    for (i = 0; i < info.count; i++) {
	info.list[i].low_mark  = low  * info.list[i].count;
	info.list[i].high_mark = high * info.list[i].count;
	if (ioctl(fd, DRM_IOCTL_MARK_BUFS, &info.list[i])) {
	    int retval = -errno;
	    drmFree(info.list);
	    return retval;
	}
    }
    drmFree(info.list);

    return 0;
}

/**
 * Free buffers.
 *
 * \param fd file descriptor.
 * \param count number of buffers to free.
 * \param list list of buffers to be freed.
 *
 * \return zero on success, or a negative value on failure.
 * 
 * \note This function is primarily used for debugging.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_FREE_BUFS ioctl, passing
 * the arguments in a drm_buf_free structure.
 */
int drmFreeBufs(int fd, int count, int *list)
{
    drm_buf_free_t request;

    request.count = count;
    request.list  = list;
    if (ioctl(fd, DRM_IOCTL_FREE_BUFS, &request)) return -errno;
    return 0;
}


/**
 * Close the device.
 *
 * \param fd file descriptor.
 *
 * \internal
 * This function closes the file descriptor.
 */
int drmClose(int fd)
{
    unsigned long key    = drmGetKeyFromFd(fd);
    drmHashEntry  *entry = drmGetEntry(fd);

    drmHashDestroy(entry->tagTable);
    entry->fd       = 0;
    entry->f        = NULL;
    entry->tagTable = NULL;

    drmHashDelete(drmHashTable, key);
    drmFree(entry);

    return close(fd);
}


/**
 * Map a region of memory.
 *
 * \param fd file descriptor.
 * \param handle handle returned by drmAddMap().
 * \param size size in bytes. Must match the size used by drmAddMap().
 * \param address will contain the user-space virtual address where the mapping
 * begins.
 *
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper for mmap().
 */
int drmMap(int fd,
	   drm_handle_t handle,
	   drmSize size,
	   drmAddressPtr address)
{
    static unsigned long pagesize_mask = 0;

    if (fd < 0) return -EINVAL;

    if (!pagesize_mask)
	pagesize_mask = getpagesize() - 1;

    size = (size + pagesize_mask) & ~pagesize_mask;

    *address = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, handle);
    if (*address == MAP_FAILED) return -errno;
    return 0;
}


/**
 * Unmap mappings obtained with drmMap().
 *
 * \param address address as given by drmMap().
 * \param size size in bytes. Must match the size used by drmMap().
 * 
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * This function is a wrapper for unmap().
 */
int drmUnmap(drmAddress address, drmSize size)
{
    return munmap(address, size);
}

drmBufInfoPtr drmGetBufInfo(int fd)
{
    drm_buf_info_t info;
    drmBufInfoPtr  retval;
    int            i;

    info.count = 0;
    info.list  = NULL;

    if (ioctl(fd, DRM_IOCTL_INFO_BUFS, &info)) return NULL;

    if (info.count) {
	if (!(info.list = drmMalloc(info.count * sizeof(*info.list))))
	    return NULL;

	if (ioctl(fd, DRM_IOCTL_INFO_BUFS, &info)) {
	    drmFree(info.list);
	    return NULL;
	}
				/* Now, copy it all back into the
                                   client-visible data structure... */
	retval = drmMalloc(sizeof(*retval));
	retval->count = info.count;
	retval->list  = drmMalloc(info.count * sizeof(*retval->list));
	for (i = 0; i < info.count; i++) {
	    retval->list[i].count     = info.list[i].count;
	    retval->list[i].size      = info.list[i].size;
	    retval->list[i].low_mark  = info.list[i].low_mark;
	    retval->list[i].high_mark = info.list[i].high_mark;
	}
	drmFree(info.list);
	return retval;
    }
    return NULL;
}

/**
 * Map all DMA buffers into client-virtual space.
 *
 * \param fd file descriptor.
 *
 * \return a pointer to a ::drmBufMap structure.
 *
 * \note The client may not use these buffers until obtaining buffer indices
 * with drmDMA().
 * 
 * \internal
 * This function calls the DRM_IOCTL_MAP_BUFS ioctl and copies the returned
 * information about the buffers in a drm_buf_map structure into the
 * client-visible data structures.
 */ 
drmBufMapPtr drmMapBufs(int fd)
{
    drm_buf_map_t bufs;
    drmBufMapPtr  retval;
    int           i;

    bufs.count = 0;
    bufs.list  = NULL;
    bufs.virtual = NULL;
    if (ioctl(fd, DRM_IOCTL_MAP_BUFS, &bufs)) return NULL;

    if (!bufs.count) return NULL;

	if (!(bufs.list = drmMalloc(bufs.count * sizeof(*bufs.list))))
	    return NULL;

	if (ioctl(fd, DRM_IOCTL_MAP_BUFS, &bufs)) {
	    drmFree(bufs.list);
	    return NULL;
	}
				/* Now, copy it all back into the
                                   client-visible data structure... */
	retval = drmMalloc(sizeof(*retval));
	retval->count = bufs.count;
	retval->list  = drmMalloc(bufs.count * sizeof(*retval->list));
	for (i = 0; i < bufs.count; i++) {
	    retval->list[i].idx     = bufs.list[i].idx;
	    retval->list[i].total   = bufs.list[i].total;
	    retval->list[i].used    = 0;
	    retval->list[i].address = bufs.list[i].address;
	}

	drmFree(bufs.list);
	
	return retval;
}


/**
 * Unmap buffers allocated with drmMapBufs().
 *
 * \return zero on success, or negative value on failure.
 *
 * \internal
 * Calls munmap() for every buffer stored in \p bufs and frees the
 * memory allocated by drmMapBufs().
 */
int drmUnmapBufs(drmBufMapPtr bufs)
{
    int i;

    for (i = 0; i < bufs->count; i++) {
	munmap(bufs->list[i].address, bufs->list[i].total);
    }

    drmFree(bufs->list);
    drmFree(bufs);
	
    return 0;
}


#define DRM_DMA_RETRY		16

/**
 * Reserve DMA buffers.
 *
 * \param fd file descriptor.
 * \param request 
 * 
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * Assemble the arguments into a drm_dma structure and keeps issuing the
 * DRM_IOCTL_DMA ioctl until success or until maximum number of retries.
 */
int drmDMA(int fd, drmDMAReqPtr request)
{
    drm_dma_t dma;
    int ret, i = 0;

				/* Copy to hidden structure */
    dma.context         = request->context;
    dma.send_count      = request->send_count;
    dma.send_indices    = request->send_list;
    dma.send_sizes      = request->send_sizes;
    dma.flags           = request->flags;
    dma.request_count   = request->request_count;
    dma.request_size    = request->request_size;
    dma.request_indices = request->request_list;
    dma.request_sizes   = request->request_sizes;
    dma.granted_count   = 0;

    do {
	ret = ioctl( fd, DRM_IOCTL_DMA, &dma );
    } while ( ret && errno == EAGAIN && i++ < DRM_DMA_RETRY );

    if ( ret == 0 ) {
	request->granted_count = dma.granted_count;
	return 0;
    } else {
	return -errno;
    }
}


/**
 * Obtain heavyweight hardware lock.
 *
 * \param fd file descriptor.
 * \param context context.
 * \param flags flags that determine the sate of the hardware when the function
 * returns.
 * 
 * \return always zero.
 * 
 * \internal
 * This function translates the arguments into a drm_lock structure and issue
 * the DRM_IOCTL_LOCK ioctl until the lock is successfully acquired.
 */
int drmGetLock(int fd, drm_context_t context, drmLockFlags flags)
{
    drm_lock_t lock;

    lock.context = context;
    lock.flags   = 0;
    if (flags & DRM_LOCK_READY)      lock.flags |= _DRM_LOCK_READY;
    if (flags & DRM_LOCK_QUIESCENT)  lock.flags |= _DRM_LOCK_QUIESCENT;
    if (flags & DRM_LOCK_FLUSH)      lock.flags |= _DRM_LOCK_FLUSH;
    if (flags & DRM_LOCK_FLUSH_ALL)  lock.flags |= _DRM_LOCK_FLUSH_ALL;
    if (flags & DRM_HALT_ALL_QUEUES) lock.flags |= _DRM_HALT_ALL_QUEUES;
    if (flags & DRM_HALT_CUR_QUEUES) lock.flags |= _DRM_HALT_CUR_QUEUES;

    while (ioctl(fd, DRM_IOCTL_LOCK, &lock))
	;
    return 0;
}

/**
 * Release the hardware lock.
 *
 * \param fd file descriptor.
 * \param context context.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_UNLOCK ioctl, passing the
 * argument in a drm_lock structure.
 */
int drmUnlock(int fd, drm_context_t context)
{
    drm_lock_t lock;

    lock.context = context;
    lock.flags   = 0;
    return ioctl(fd, DRM_IOCTL_UNLOCK, &lock);
}

drm_context_t * drmGetReservedContextList(int fd, int *count)
{
    drm_ctx_res_t res;
    drm_ctx_t     *list;
    drm_context_t * retval;
    int           i;

    res.count    = 0;
    res.contexts = NULL;
    if (ioctl(fd, DRM_IOCTL_RES_CTX, &res)) return NULL;

    if (!res.count) return NULL;

    if (!(list   = drmMalloc(res.count * sizeof(*list)))) return NULL;
    if (!(retval = drmMalloc(res.count * sizeof(*retval)))) {
	drmFree(list);
	return NULL;
    }

    res.contexts = list;
    if (ioctl(fd, DRM_IOCTL_RES_CTX, &res)) return NULL;

    for (i = 0; i < res.count; i++) retval[i] = list[i].handle;
    drmFree(list);

    *count = res.count;
    return retval;
}

void drmFreeReservedContextList(drm_context_t * pt)
{
    drmFree(pt);
}

/**
 * Create context.
 *
 * Used by the X server during GLXContext initialization. This causes
 * per-context kernel-level resources to be allocated.
 *
 * \param fd file descriptor.
 * \param handle is set on success. To be used by the client when requesting DMA
 * dispatch with drmDMA().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \note May only be called by root.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_ADD_CTX ioctl, passing the
 * argument in a drm_ctx structure.
 */
int drmCreateContext(int fd, drm_context_t * handle)
{
    drm_ctx_t ctx;

    ctx.flags = 0;	/* Modified with functions below */
    if (ioctl(fd, DRM_IOCTL_ADD_CTX, &ctx)) return -errno;
    *handle = ctx.handle;
    return 0;
}

int drmSwitchToContext(int fd, drm_context_t context)
{
    drm_ctx_t ctx;

    ctx.handle = context;
    if (ioctl(fd, DRM_IOCTL_SWITCH_CTX, &ctx)) return -errno;
    return 0;
}

int drmSetContextFlags(int fd, drm_context_t context, drm_context_tFlags flags)
{
    drm_ctx_t ctx;

				/* Context preserving means that no context
                                   switched are done between DMA buffers
                                   from one context and the next.  This is
                                   suitable for use in the X server (which
                                   promises to maintain hardware context,
                                   or in the client-side library when
                                   buffers are swapped on behalf of two
                                   threads. */
    ctx.handle = context;
    ctx.flags  = 0;
    if (flags & DRM_CONTEXT_PRESERVED) ctx.flags |= _DRM_CONTEXT_PRESERVED;
    if (flags & DRM_CONTEXT_2DONLY)    ctx.flags |= _DRM_CONTEXT_2DONLY;
    if (ioctl(fd, DRM_IOCTL_MOD_CTX, &ctx)) return -errno;
    return 0;
}

int drmGetContextFlags(int fd, drm_context_t context, drm_context_tFlagsPtr flags)
{
    drm_ctx_t ctx;

    ctx.handle = context;
    if (ioctl(fd, DRM_IOCTL_GET_CTX, &ctx)) return -errno;
    *flags = 0;
    if (ctx.flags & _DRM_CONTEXT_PRESERVED) *flags |= DRM_CONTEXT_PRESERVED;
    if (ctx.flags & _DRM_CONTEXT_2DONLY)    *flags |= DRM_CONTEXT_2DONLY;
    return 0;
}

/**
 * Destroy context.
 *
 * Free any kernel-level resources allocated with drmCreateContext() associated
 * with the context.
 * 
 * \param fd file descriptor.
 * \param handle handle given by drmCreateContext().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \note May only be called by root.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_RM_CTX ioctl, passing the
 * argument in a drm_ctx structure.
 */
int drmDestroyContext(int fd, drm_context_t handle)
{
    drm_ctx_t ctx;
    ctx.handle = handle;
    if (ioctl(fd, DRM_IOCTL_RM_CTX, &ctx)) return -errno;
    return 0;
}

int drmCreateDrawable(int fd, drm_drawable_t * handle)
{
    drm_draw_t draw;
    if (ioctl(fd, DRM_IOCTL_ADD_DRAW, &draw)) return -errno;
    *handle = draw.handle;
    return 0;
}

int drmDestroyDrawable(int fd, drm_drawable_t handle)
{
    drm_draw_t draw;
    draw.handle = handle;
    if (ioctl(fd, DRM_IOCTL_RM_DRAW, &draw)) return -errno;
    return 0;
}

/**
 * Acquire the AGP device.
 *
 * Must be called before any of the other AGP related calls.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ACQUIRE ioctl.
 */
int drmAgpAcquire(int fd)
{
    if (ioctl(fd, DRM_IOCTL_AGP_ACQUIRE, NULL)) return -errno;
    return 0;
}


/**
 * Release the AGP device.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_RELEASE ioctl.
 */
int drmAgpRelease(int fd)
{
    if (ioctl(fd, DRM_IOCTL_AGP_RELEASE, NULL)) return -errno;
    return 0;
}


/**
 * Set the AGP mode.
 *
 * \param fd file descriptor.
 * \param mode AGP mode.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ENABLE ioctl, passing the
 * argument in a drm_agp_mode structure.
 */
int drmAgpEnable(int fd, unsigned long mode)
{
    drm_agp_mode_t m;

    m.mode = mode;
    if (ioctl(fd, DRM_IOCTL_AGP_ENABLE, &m)) return -errno;
    return 0;
}


/**
 * Allocate a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param size requested memory size in bytes. Will be rounded to page boundary.
 * \param type type of memory to allocate.
 * \param address if not zero, will be set to the physical address of the
 * allocated memory.
 * \param handle on success will be set to a handle of the allocated memory.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_ALLOC ioctl, passing the
 * arguments in a drm_agp_buffer structure.
 */
int drmAgpAlloc(int fd, unsigned long size, unsigned long type,
		unsigned long *address, unsigned long *handle)
{
    drm_agp_buffer_t b;

    *handle = DRM_AGP_NO_HANDLE;
    b.size   = size;
    b.handle = 0;
    b.type   = type;
    if (ioctl(fd, DRM_IOCTL_AGP_ALLOC, &b)) return -errno;
    if (address != 0UL) *address = b.physical;
    *handle = b.handle;
    return 0;
}


/**
 * Free a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_FREE ioctl, passing the
 * argument in a drm_agp_buffer structure.
 */
int drmAgpFree(int fd, unsigned long handle)
{
    drm_agp_buffer_t b;

    b.size   = 0;
    b.handle = handle;
    if (ioctl(fd, DRM_IOCTL_AGP_FREE, &b)) return -errno;
    return 0;
}


/**
 * Bind a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * \param offset offset in bytes. It will round to page boundary.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_BIND ioctl, passing the
 * argument in a drm_agp_binding structure.
 */
int drmAgpBind(int fd, unsigned long handle, unsigned long offset)
{
    drm_agp_binding_t b;

    b.handle = handle;
    b.offset = offset;
    if (ioctl(fd, DRM_IOCTL_AGP_BIND, &b)) return -errno;
    return 0;
}


/**
 * Unbind a chunk of AGP memory.
 *
 * \param fd file descriptor.
 * \param handle handle to the allocated memory, as given by drmAgpAllocate().
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_UNBIND ioctl, passing
 * the argument in a drm_agp_binding structure.
 */
int drmAgpUnbind(int fd, unsigned long handle)
{
    drm_agp_binding_t b;

    b.handle = handle;
    b.offset = 0;
    if (ioctl(fd, DRM_IOCTL_AGP_UNBIND, &b)) return -errno;
    return 0;
}


/**
 * Get AGP driver major version number.
 *
 * \param fd file descriptor.
 * 
 * \return major version number on success, or a negative value on failure..
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
int drmAgpVersionMajor(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return -errno;
    return i.agp_version_major;
}


/**
 * Get AGP driver minor version number.
 *
 * \param fd file descriptor.
 * 
 * \return minor version number on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
int drmAgpVersionMinor(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return -errno;
    return i.agp_version_minor;
}


/**
 * Get AGP mode.
 *
 * \param fd file descriptor.
 * 
 * \return mode on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpGetMode(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.mode;
}


/**
 * Get AGP aperture base.
 *
 * \param fd file descriptor.
 * 
 * \return aperture base on success, zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpBase(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.aperture_base;
}


/**
 * Get AGP aperture size.
 *
 * \param fd file descriptor.
 * 
 * \return aperture size on success, zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpSize(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.aperture_size;
}


/**
 * Get used AGP memory.
 *
 * \param fd file descriptor.
 * 
 * \return memory used on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpMemoryUsed(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.memory_used;
}


/**
 * Get available AGP memory.
 *
 * \param fd file descriptor.
 * 
 * \return memory available on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned long drmAgpMemoryAvail(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.memory_allowed;
}


/**
 * Get hardware vendor ID.
 *
 * \param fd file descriptor.
 * 
 * \return vendor ID on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned int drmAgpVendorId(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.id_vendor;
}


/**
 * Get hardware device ID.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or zero on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_AGP_INFO ioctl, getting the
 * necessary information in a drm_agp_info structure.
 */
unsigned int drmAgpDeviceId(int fd)
{
    drm_agp_info_t i;

    if (ioctl(fd, DRM_IOCTL_AGP_INFO, &i)) return 0;
    return i.id_device;
}

int drmScatterGatherAlloc(int fd, unsigned long size, unsigned long *handle)
{
    drm_scatter_gather_t sg;

    *handle = 0;
    sg.size   = size;
    sg.handle = 0;
    if (ioctl(fd, DRM_IOCTL_SG_ALLOC, &sg)) return -errno;
    *handle = sg.handle;
    return 0;
}

int drmScatterGatherFree(int fd, unsigned long handle)
{
    drm_scatter_gather_t sg;

    sg.size   = 0;
    sg.handle = handle;
    if (ioctl(fd, DRM_IOCTL_SG_FREE, &sg)) return -errno;
    return 0;
}

/**
 * Wait for VBLANK.
 *
 * \param fd file descriptor.
 * \param vbl pointer to a drmVBlank structure.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_WAIT_VBLANK ioctl.
 */
int drmWaitVBlank(int fd, drmVBlankPtr vbl)
{
    int ret;

    do {
       ret = ioctl(fd, DRM_IOCTL_WAIT_VBLANK, vbl);
       vbl->request.type &= ~DRM_VBLANK_RELATIVE;
    } while (ret && errno == EINTR);

    return ret;
}

int drmError(int err, const char *label)
{
    switch (err) {
    case DRM_ERR_NO_DEVICE: fprintf(stderr, "%s: no device\n", label);   break;
    case DRM_ERR_NO_ACCESS: fprintf(stderr, "%s: no access\n", label);   break;
    case DRM_ERR_NOT_ROOT:  fprintf(stderr, "%s: not root\n", label);    break;
    case DRM_ERR_INVALID:   fprintf(stderr, "%s: invalid args\n", label);break;
    default:
	if (err < 0) err = -err;
	fprintf( stderr, "%s: error %d (%s)\n", label, err, strerror(err) );
	break;
    }

    return 1;
}

/**
 * Install IRQ handler.
 *
 * \param fd file descriptor.
 * \param irq IRQ number.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_CONTROL ioctl, passing the
 * argument in a drm_control structure.
 */
int drmCtlInstHandler(int fd, int irq)
{
    drm_control_t ctl;

    ctl.func  = DRM_INST_HANDLER;
    ctl.irq   = irq;
    if (ioctl(fd, DRM_IOCTL_CONTROL, &ctl)) return -errno;
    return 0;
}


/**
 * Uninstall IRQ handler.
 *
 * \param fd file descriptor.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_CONTROL ioctl, passing the
 * argument in a drm_control structure.
 */
int drmCtlUninstHandler(int fd)
{
    drm_control_t ctl;

    ctl.func  = DRM_UNINST_HANDLER;
    ctl.irq   = 0;
    if (ioctl(fd, DRM_IOCTL_CONTROL, &ctl)) return -errno;
    return 0;
}

int drmFinish(int fd, int context, drmLockFlags flags)
{
    drm_lock_t lock;

    lock.context = context;
    lock.flags   = 0;
    if (flags & DRM_LOCK_READY)      lock.flags |= _DRM_LOCK_READY;
    if (flags & DRM_LOCK_QUIESCENT)  lock.flags |= _DRM_LOCK_QUIESCENT;
    if (flags & DRM_LOCK_FLUSH)      lock.flags |= _DRM_LOCK_FLUSH;
    if (flags & DRM_LOCK_FLUSH_ALL)  lock.flags |= _DRM_LOCK_FLUSH_ALL;
    if (flags & DRM_HALT_ALL_QUEUES) lock.flags |= _DRM_HALT_ALL_QUEUES;
    if (flags & DRM_HALT_CUR_QUEUES) lock.flags |= _DRM_HALT_CUR_QUEUES;
    if (ioctl(fd, DRM_IOCTL_FINISH, &lock)) return -errno;
    return 0;
}

/**
 * Get IRQ from bus ID.
 *
 * \param fd file descriptor.
 * \param busnum bus number.
 * \param devnum device number.
 * \param funcnum function number.
 * 
 * \return IRQ number on success, or a negative value on failure.
 * 
 * \internal
 * This function is a wrapper around the DRM_IOCTL_IRQ_BUSID ioctl, passing the
 * arguments in a drm_irq_busid structure.
 */
int drmGetInterruptFromBusID(int fd, int busnum, int devnum, int funcnum)
{
    drm_irq_busid_t p;

    p.busnum  = busnum;
    p.devnum  = devnum;
    p.funcnum = funcnum;
    if (ioctl(fd, DRM_IOCTL_IRQ_BUSID, &p)) return -errno;
    return p.irq;
}

int drmAddContextTag(int fd, drm_context_t context, void *tag)
{
    drmHashEntry  *entry = drmGetEntry(fd);

    if (drmHashInsert(entry->tagTable, context, tag)) {
	drmHashDelete(entry->tagTable, context);
	drmHashInsert(entry->tagTable, context, tag);
    }
    return 0;
}

int drmDelContextTag(int fd, drm_context_t context)
{
    drmHashEntry  *entry = drmGetEntry(fd);

    return drmHashDelete(entry->tagTable, context);
}

void *drmGetContextTag(int fd, drm_context_t context)
{
    drmHashEntry  *entry = drmGetEntry(fd);
    void          *value;

    if (drmHashLookup(entry->tagTable, context, &value)) return NULL;

    return value;
}

int drmAddContextPrivateMapping(int fd, drm_context_t ctx_id, drm_handle_t handle)
{
    drm_ctx_priv_map_t map;

    map.ctx_id = ctx_id;
    map.handle = (void *)handle;

    if (ioctl(fd, DRM_IOCTL_SET_SAREA_CTX, &map)) return -errno;
    return 0;
}

int drmGetContextPrivateMapping(int fd, drm_context_t ctx_id, drm_handle_t * handle)
{
    drm_ctx_priv_map_t map;

    map.ctx_id = ctx_id;

    if (ioctl(fd, DRM_IOCTL_GET_SAREA_CTX, &map)) return -errno;
    if (handle) *handle = (drm_handle_t)map.handle;

    return 0;
}

int drmGetMap(int fd, int idx, drm_handle_t *offset, drmSize *size,
	      drmMapType *type, drmMapFlags *flags, drm_handle_t *handle,
	      int *mtrr)
{
    drm_map_t map;

    map.offset = idx;
    if (ioctl(fd, DRM_IOCTL_GET_MAP, &map)) return -errno;
    *offset = map.offset;
    *size   = map.size;
    *type   = map.type;
    *flags  = map.flags;
    *handle = (unsigned long)map.handle;
    *mtrr   = map.mtrr;
    return 0;
}

int drmGetClient(int fd, int idx, int *auth, int *pid, int *uid,
		 unsigned long *magic, unsigned long *iocs)
{
    drm_client_t client;

    client.idx = idx;
    if (ioctl(fd, DRM_IOCTL_GET_CLIENT, &client)) return -errno;
    *auth      = client.auth;
    *pid       = client.pid;
    *uid       = client.uid;
    *magic     = client.magic;
    *iocs      = client.iocs;
    return 0;
}

int drmGetStats(int fd, drmStatsT *stats)
{
    drm_stats_t s;
    int         i;

    if (ioctl(fd, DRM_IOCTL_GET_STATS, &s)) return -errno;

    stats->count = 0;
    memset(stats, 0, sizeof(*stats));
    if (s.count > sizeof(stats->data)/sizeof(stats->data[0]))
	return -1;

#define SET_VALUE                              \
    stats->data[i].long_format = "%-20.20s";   \
    stats->data[i].rate_format = "%8.8s";      \
    stats->data[i].isvalue     = 1;            \
    stats->data[i].verbose     = 0

#define SET_COUNT                              \
    stats->data[i].long_format = "%-20.20s";   \
    stats->data[i].rate_format = "%5.5s";      \
    stats->data[i].isvalue     = 0;            \
    stats->data[i].mult_names  = "kgm";        \
    stats->data[i].mult        = 1000;         \
    stats->data[i].verbose     = 0

#define SET_BYTE                               \
    stats->data[i].long_format = "%-20.20s";   \
    stats->data[i].rate_format = "%5.5s";      \
    stats->data[i].isvalue     = 0;            \
    stats->data[i].mult_names  = "KGM";        \
    stats->data[i].mult        = 1024;         \
    stats->data[i].verbose     = 0


    stats->count = s.count;
    for (i = 0; i < s.count; i++) {
	stats->data[i].value = s.data[i].value;
	switch (s.data[i].type) {
	case _DRM_STAT_LOCK:
	    stats->data[i].long_name = "Lock";
	    stats->data[i].rate_name = "Lock";
	    SET_VALUE;
	    break;
	case _DRM_STAT_OPENS:
	    stats->data[i].long_name = "Opens";
	    stats->data[i].rate_name = "O";
	    SET_COUNT;
	    stats->data[i].verbose   = 1;
	    break;
	case _DRM_STAT_CLOSES:
	    stats->data[i].long_name = "Closes";
	    stats->data[i].rate_name = "Lock";
	    SET_COUNT;
	    stats->data[i].verbose   = 1;
	    break;
	case _DRM_STAT_IOCTLS:
	    stats->data[i].long_name = "Ioctls";
	    stats->data[i].rate_name = "Ioc/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_LOCKS:
	    stats->data[i].long_name = "Locks";
	    stats->data[i].rate_name = "Lck/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_UNLOCKS:
	    stats->data[i].long_name = "Unlocks";
	    stats->data[i].rate_name = "Unl/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_IRQ:
	    stats->data[i].long_name = "IRQs";
	    stats->data[i].rate_name = "IRQ/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_PRIMARY:
	    stats->data[i].long_name = "Primary Bytes";
	    stats->data[i].rate_name = "PB/s";
	    SET_BYTE;
	    break;
	case _DRM_STAT_SECONDARY:
	    stats->data[i].long_name = "Secondary Bytes";
	    stats->data[i].rate_name = "SB/s";
	    SET_BYTE;
	    break;
	case _DRM_STAT_DMA:
	    stats->data[i].long_name = "DMA";
	    stats->data[i].rate_name = "DMA/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_SPECIAL:
	    stats->data[i].long_name = "Special DMA";
	    stats->data[i].rate_name = "dma/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_MISSED:
	    stats->data[i].long_name = "Miss";
	    stats->data[i].rate_name = "Ms/s";
	    SET_COUNT;
	    break;
	case _DRM_STAT_VALUE:
	    stats->data[i].long_name = "Value";
	    stats->data[i].rate_name = "Value";
	    SET_VALUE;
	    break;
	case _DRM_STAT_BYTE:
	    stats->data[i].long_name = "Bytes";
	    stats->data[i].rate_name = "B/s";
	    SET_BYTE;
	    break;
	case _DRM_STAT_COUNT:
	default:
	    stats->data[i].long_name = "Count";
	    stats->data[i].rate_name = "Cnt/s";
	    SET_COUNT;
	    break;
	}
    }
    return 0;
}

/**
 * Issue a set-version ioctl.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data source pointer of the data to be read and written.
 * \param size size of the data to be read and written.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a read-write ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmSetInterfaceVersion(int fd, drmSetVersion *version )
{
    int retcode = 0;
    drm_set_version_t sv;

    sv.drm_di_major = version->drm_di_major;
    sv.drm_di_minor = version->drm_di_minor;
    sv.drm_dd_major = version->drm_dd_major;
    sv.drm_dd_minor = version->drm_dd_minor;

    if (ioctl(fd, DRM_IOCTL_SET_VERSION, &sv)) {
	retcode = -errno;
    }

    version->drm_di_major = sv.drm_di_major;
    version->drm_di_minor = sv.drm_di_minor;
    version->drm_dd_major = sv.drm_dd_major;
    version->drm_dd_minor = sv.drm_dd_minor;

    return retcode;
}

/**
 * Send a device-specific command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandNone(int fd, unsigned long drmCommandIndex)
{
    void *data = NULL; /* dummy */
    unsigned long request;

    request = DRM_IO( DRM_COMMAND_BASE + drmCommandIndex);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}


/**
 * Send a device-specific read command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data destination pointer of the data to be read.
 * \param size size of the data to be read.
 * 
 * \return zero on success, or a negative value on failure.
 *
 * \internal
 * It issues a read ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandRead(int fd, unsigned long drmCommandIndex,
                   void *data, unsigned long size )
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ, DRM_IOCTL_BASE, 
	DRM_COMMAND_BASE + drmCommandIndex, size);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}


/**
 * Send a device-specific write command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data source pointer of the data to be written.
 * \param size size of the data to be written.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a write ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandWrite(int fd, unsigned long drmCommandIndex,
                   void *data, unsigned long size )
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_WRITE, DRM_IOCTL_BASE, 
	DRM_COMMAND_BASE + drmCommandIndex, size);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}


/**
 * Send a device-specific read-write command.
 *
 * \param fd file descriptor.
 * \param drmCommandIndex command index 
 * \param data source pointer of the data to be read and written.
 * \param size size of the data to be read and written.
 * 
 * \return zero on success, or a negative value on failure.
 * 
 * \internal
 * It issues a read-write ioctl given by 
 * \code DRM_COMMAND_BASE + drmCommandIndex \endcode.
 */
int drmCommandWriteRead(int fd, unsigned long drmCommandIndex,
                   void *data, unsigned long size )
{
    unsigned long request;

    request = DRM_IOC( DRM_IOC_READ|DRM_IOC_WRITE, DRM_IOCTL_BASE, 
	DRM_COMMAND_BASE + drmCommandIndex, size);

    if (ioctl(fd, request, data)) {
	return -errno;
    }
    return 0;
}

#if defined(XFree86Server)
static void drmSIGIOHandler(int interrupt, void *closure)
{
    unsigned long key;
    void          *value;
    ssize_t       count;
    drm_ctx_t     ctx;
    typedef void  (*_drmCallback)(int, void *, void *);
    char          buf[256];
    drm_context_t    old;
    drm_context_t    new;
    void          *oldctx;
    void          *newctx;
    char          *pt;
    drmHashEntry  *entry;

    if (!drmHashTable) return;
    if (drmHashFirst(drmHashTable, &key, &value)) {
	entry = value;
	do {
#if 0
	    fprintf(stderr, "Trying %d\n", entry->fd);
#endif
	    if ((count = read(entry->fd, buf, sizeof(buf))) > 0) {
		buf[count] = '\0';
#if 0
		fprintf(stderr, "Got %s\n", buf);
#endif

		for (pt = buf; *pt != ' '; ++pt); /* Find first space */
		++pt;
		old    = strtol(pt, &pt, 0);
		new    = strtol(pt, NULL, 0);
		oldctx = drmGetContextTag(entry->fd, old);
		newctx = drmGetContextTag(entry->fd, new);
#if 0
		fprintf(stderr, "%d %d %p %p\n", old, new, oldctx, newctx);
#endif
		((_drmCallback)entry->f)(entry->fd, oldctx, newctx);
		ctx.handle = new;
		ioctl(entry->fd, DRM_IOCTL_NEW_CTX, &ctx);
	    }
	} while (drmHashNext(drmHashTable, &key, &value));
    }
}

int drmInstallSIGIOHandler(int fd, void (*f)(int, void *, void *))
{
    drmHashEntry     *entry;

    entry     = drmGetEntry(fd);
    entry->f  = f;

    return xf86InstallSIGIOHandler(fd, drmSIGIOHandler, 0);
}

int drmRemoveSIGIOHandler(int fd)
{
    drmHashEntry     *entry = drmGetEntry(fd);

    entry->f = NULL;

    return xf86RemoveSIGIOHandler(fd);
}
#endif
