/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Disk Redirection
   Copyright (C) Jeroen Meijer 2003

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "disk.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>		/* open, close */
#include <dirent.h>		/* opendir, closedir, readdir */
#include <fnmatch.h>
#include <errno.h>		/* errno */
#include <stdio.h>

#include <utime.h>
#include <time.h>		/* ctime */

#if (defined(HAVE_DIRFD) || (HAVE_DECL_DIRFD == 1))
#define DIRFD(a) (dirfd(a))
#else
#define DIRFD(a) ((a)->DIR_FD_MEMBER_NAME)
#endif

/* TODO: Fix mntent-handling for solaris
 * #include <sys/mntent.h> */
#if (defined(HAVE_MNTENT_H) && defined(HAVE_SETMNTENT))
#include <mntent.h>
#define MNTENT_PATH "/etc/mtab"
#define USE_SETMNTENT
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#include "rdesktop.h"

#ifdef STAT_STATFS3_OSF1
#define STATFS_FN(path, buf) (statfs(path,buf,sizeof(buf)))
#define STATFS_T statfs
#define USE_STATFS
#endif

#ifdef STAT_STATVFS
#define STATFS_FN(path, buf) (statvfs(path,buf))
#define STATFS_T statvfs
#define USE_STATVFS
#endif

#ifdef STAT_STATVFS64
#define STATFS_FN(path, buf) (statvfs64(path,buf))
#define STATFS_T statvfs64
#define USE_STATVFS
#endif

#if (defined(STAT_STATFS2_FS_DATA) || defined(STAT_STATFS2_BSIZE) || defined(STAT_STATFS2_FSIZE))
#define STATFS_FN(path, buf) (statfs(path,buf))
#define STATFS_T statfs
#define USE_STATFS
#endif

#ifdef STAT_STATFS4
#define STATFS_FN(path, buf) (statfs(path,buf,sizeof(buf),0))
#define STATFS_T statfs
#define USE_STATFS
#endif

#if ((defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_NAMEMAX)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_NAMEMAX)))
#define F_NAMELEN(buf) ((buf).f_namemax)
#endif

#if ((defined(USE_STATFS) && defined(HAVE_STRUCT_STATFS_F_NAMELEN)) || (defined(USE_STATVFS) && defined(HAVE_STRUCT_STATVFS_F_NAMELEN)))
#define F_NAMELEN(buf) ((buf).f_namelen)
#endif

#ifndef F_NAMELEN
#define F_NAMELEN(buf) (255)
#endif

/* Dummy statfs fallback */
#ifndef STATFS_T
struct dummy_statfs_t
{
	long f_bfree;
	long f_bsize;
	long f_blocks;
	int f_namelen;
	int f_namemax;
};

static int
dummy_statfs(struct dummy_statfs_t *buf)
{
	buf->f_blocks = 262144;
	buf->f_bfree = 131072;
	buf->f_bsize = 512;
	buf->f_namelen = 255;
	buf->f_namemax = 255;

	return 0;
}

#define STATFS_T dummy_statfs_t
#define STATFS_FN(path,buf) (dummy_statfs(buf))
#endif

typedef struct
{
	char name[PATH_MAX];
	char label[PATH_MAX];
	unsigned long serial;
	char type[PATH_MAX];
} FsInfoType;

static NTSTATUS NotifyInfo(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, NOTIFY * p);

static time_t
get_create_time(struct stat *st)
{
	time_t ret, ret1;

	ret = MIN(st->st_ctime, st->st_mtime);
	ret1 = MIN(ret, st->st_atime);

	if (ret1 != (time_t) 0)
		return ret1;

	return ret;
}

/* Convert seconds since 1970 to a filetime */
static void
seconds_since_1970_to_filetime(time_t seconds, uint32 * high, uint32 * low)
{
	unsigned long long ticks;

	ticks = (seconds + 11644473600LL) * 10000000;
	*low = (uint32) ticks;
	*high = (uint32) (ticks >> 32);
}

/* Convert seconds since 1970 back to filetime */
static time_t
convert_1970_to_filetime(uint32 high, uint32 low)
{
	unsigned long long ticks;
	time_t val;

	ticks = low + (((unsigned long long) high) << 32);
	ticks /= 10000000;
	ticks -= 11644473600LL;

	val = (time_t) ticks;
	return (val);

}

/* A wrapper for ftruncate which supports growing files, even if the
   native ftruncate doesn't. This is needed on Linux FAT filesystems,
   for example. */
static int
ftruncate_growable(int fd, off_t length)
{
	int ret;
	off_t pos;
	static const char zero = 0;

	/* Try the simple method first */
	if ((ret = ftruncate(fd, length)) != -1)
	{
		return ret;
	}

	/*
	 * Some kind of error. Perhaps we were trying to grow. Retry
	 * in a safe way.
	 */

	/* Get current position */
	if ((pos = lseek(fd, 0, SEEK_CUR)) == -1)
	{
		perror("lseek");
		return -1;
	}

	/* Seek to new size */
	if (lseek(fd, length, SEEK_SET) == -1)
	{
		perror("lseek");
		return -1;
	}

	/* Write a zero */
	if (write(fd, &zero, 1) == -1)
	{
		perror("write");
		return -1;
	}

	/* Truncate. This shouldn't fail. */
	if (ftruncate(fd, length) == -1)
	{
		perror("ftruncate");
		return -1;
	}

	/* Restore position */
	if (lseek(fd, pos, SEEK_SET) == -1)
	{
		perror("lseek");
		return -1;
	}

	return 0;
}

/* Just like open(2), but if a open with O_EXCL fails, retry with
   GUARDED semantics. This might be necessary because some filesystems
   (such as NFS filesystems mounted from a unfsd server) doesn't
   support O_EXCL. GUARDED semantics are subject to race conditions,
   but we can live with that.
*/
static int
open_weak_exclusive(const char *pathname, int flags, mode_t mode)
{
	int ret;
	struct stat statbuf;

	ret = open(pathname, flags, mode);
	if (ret != -1 || !(flags & O_EXCL))
	{
		/* Success, or not using O_EXCL */
		return ret;
	}

	/* An error occured, and we are using O_EXCL. In case the FS
	   doesn't support O_EXCL, some kind of error will be
	   returned. Unfortunately, we don't know which one. Linux
	   2.6.8 seems to return 524, but I cannot find a documented
	   #define for this case. So, we'll return only on errors that
	   we know aren't related to O_EXCL. */
	switch (errno)
	{
		case EACCES:
		case EEXIST:
		case EINTR:
		case EISDIR:
		case ELOOP:
		case ENAMETOOLONG:
		case ENOENT:
		case ENOTDIR:
			return ret;
	}

	/* Retry with GUARDED semantics */
	if (stat(pathname, &statbuf) != -1)
	{
		/* File exists */
		errno = EEXIST;
		return -1;
	}
	else
	{
		return open(pathname, flags & ~O_EXCL, mode);
	}
}

/* Enumeration of devices from rdesktop.c        */
/* returns numer of units found and initialized. */
/* optarg looks like ':h=/mnt/floppy,b=/mnt/usbdevice1' */
/* when it arrives to this function.             */
int
disk_enum_devices(RDPCLIENT * This, uint32 * id, char *optarg)
{
	char *pos = optarg;
	char *pos2;
	int count = 0;

	/* skip the first colon */
	optarg++;
	while ((pos = next_arg(optarg, ',')) && *id < RDPDR_MAX_DEVICES)
	{
		pos2 = next_arg(optarg, '=');

		strncpy(This->rdpdr_device[*id].name, optarg, sizeof(This->rdpdr_device[*id].name) - 1);
		if (strlen(optarg) > (sizeof(This->rdpdr_device[*id].name) - 1))
			fprintf(stderr, "share name %s truncated to %s\n", optarg,
				This->rdpdr_device[*id].name);

		This->rdpdr_device[*id].local_path = (char *) xmalloc(strlen(pos2) + 1);
		strcpy(This->rdpdr_device[*id].local_path, pos2);
		This->rdpdr_device[*id].device_type = DEVICE_TYPE_DISK;
		count++;
		(*id)++;

		optarg = pos;
	}
	return count;
}

/* Opens or creates a file or directory */
static NTSTATUS
disk_create(RDPCLIENT * This, uint32 device_id, uint32 accessmask, uint32 sharemode, uint32 create_disposition,
	    uint32 flags_and_attributes, char *filename, NTHANDLE * phandle)
{
	NTHANDLE handle;
	DIR *dirp;
	int flags, mode;
	char path[PATH_MAX];
	struct stat filestat;

	handle = 0;
	dirp = NULL;
	flags = 0;
	mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

	if (*filename && filename[strlen(filename) - 1] == '/')
		filename[strlen(filename) - 1] = 0;
	sprintf(path, "%s%s", This->rdpdr_device[device_id].local_path, filename);

	switch (create_disposition)
	{
		case CREATE_ALWAYS:

			/* Delete existing file/link. */
			unlink(path);
			flags |= O_CREAT;
			break;

		case CREATE_NEW:

			/* If the file already exists, then fail. */
			flags |= O_CREAT | O_EXCL;
			break;

		case OPEN_ALWAYS:

			/* Create if not already exists. */
			flags |= O_CREAT;
			break;

		case OPEN_EXISTING:

			/* Default behaviour */
			break;

		case TRUNCATE_EXISTING:

			/* If the file does not exist, then fail. */
			flags |= O_TRUNC;
			break;
	}

	/*printf("Open: \"%s\"  flags: %X, accessmask: %X sharemode: %X create disp: %X\n", path, flags_and_attributes, accessmask, sharemode, create_disposition); */

	/* Get information about file and set that flag ourselfs */
	if ((stat(path, &filestat) == 0) && (S_ISDIR(filestat.st_mode)))
	{
		if (flags_and_attributes & FILE_NON_DIRECTORY_FILE)
			return STATUS_FILE_IS_A_DIRECTORY;
		else
			flags_and_attributes |= FILE_DIRECTORY_FILE;
	}

	if (flags_and_attributes & FILE_DIRECTORY_FILE)
	{
		if (flags & O_CREAT)
		{
			mkdir(path, mode);
		}

		dirp = opendir(path);
		if (!dirp)
		{
			switch (errno)
			{
				case EACCES:

					return STATUS_ACCESS_DENIED;

				case ENOENT:

					return STATUS_NO_SUCH_FILE;

				default:

					perror("opendir");
					return STATUS_NO_SUCH_FILE;
			}
		}
		handle = DIRFD(dirp);
	}
	else
	{

		if (accessmask & GENERIC_ALL
		    || (accessmask & GENERIC_READ && accessmask & GENERIC_WRITE))
		{
			flags |= O_RDWR;
		}
		else if ((accessmask & GENERIC_WRITE) && !(accessmask & GENERIC_READ))
		{
			flags |= O_WRONLY;
		}
		else
		{
			flags |= O_RDONLY;
		}

		handle = open_weak_exclusive(path, flags, mode);
		if (handle == -1)
		{
			switch (errno)
			{
				case EISDIR:

					return STATUS_FILE_IS_A_DIRECTORY;

				case EACCES:

					return STATUS_ACCESS_DENIED;

				case ENOENT:

					return STATUS_NO_SUCH_FILE;
				case EEXIST:

					return STATUS_OBJECT_NAME_COLLISION;
				default:

					perror("open");
					return STATUS_NO_SUCH_FILE;
			}
		}

		/* all read and writes of files should be non blocking */
		if (fcntl(handle, F_SETFL, O_NONBLOCK) == -1)
			perror("fcntl");
	}

	if (handle >= MAX_OPEN_FILES)
	{
		error("Maximum number of open files (%s) reached. Increase MAX_OPEN_FILES!\n",
		      handle);
		exit(1);
	}

	if (dirp)
		This->fileinfo[handle].pdir = dirp;
	else
		This->fileinfo[handle].pdir = NULL;

	This->fileinfo[handle].device_id = device_id;
	This->fileinfo[handle].flags_and_attributes = flags_and_attributes;
	This->fileinfo[handle].accessmask = accessmask;
	strncpy(This->fileinfo[handle].path, path, PATH_MAX - 1);
	This->fileinfo[handle].delete_on_close = False;
	This->notify_stamp = True;

	*phandle = handle;
	return STATUS_SUCCESS;
}

static NTSTATUS
disk_close(RDPCLIENT * This, NTHANDLE handle)
{
	struct fileinfo *pfinfo;

	pfinfo = &(This->fileinfo[handle]);

	This->notify_stamp = True;

	rdpdr_abort_io(This, handle, 0, STATUS_CANCELLED);

	if (pfinfo->pdir)
	{
		if (closedir(pfinfo->pdir) < 0)
		{
			perror("closedir");
			return STATUS_INVALID_HANDLE;
		}

		if (pfinfo->delete_on_close)
			if (rmdir(pfinfo->path) < 0)
			{
				perror(pfinfo->path);
				return STATUS_ACCESS_DENIED;
			}
		pfinfo->delete_on_close = False;
	}
	else
	{
		if (close(handle) < 0)
		{
			perror("close");
			return STATUS_INVALID_HANDLE;
		}
		if (pfinfo->delete_on_close)
			if (unlink(pfinfo->path) < 0)
			{
				perror(pfinfo->path);
				return STATUS_ACCESS_DENIED;
			}

		pfinfo->delete_on_close = False;
	}

	return STATUS_SUCCESS;
}

static NTSTATUS
disk_read(RDPCLIENT * This, NTHANDLE handle, uint8 * data, uint32 length, uint32 offset, uint32 * result)
{
	int n;

#if 0
	/* browsing dir ????        */
	/* each request is 24 bytes */
	if (This->fileinfo[handle].flags_and_attributes & FILE_DIRECTORY_FILE)
	{
		*result = 0;
		return STATUS_SUCCESS;
	}
#endif

	lseek(handle, offset, SEEK_SET);

	n = read(handle, data, length);

	if (n < 0)
	{
		*result = 0;
		switch (errno)
		{
			case EISDIR:
				/* Implement 24 Byte directory read ??
				   with STATUS_NOT_IMPLEMENTED server doesn't read again */
				/* return STATUS_FILE_IS_A_DIRECTORY; */
				return STATUS_NOT_IMPLEMENTED;
			default:
				perror("read");
				return STATUS_INVALID_PARAMETER;
		}
	}

	*result = n;

	return STATUS_SUCCESS;
}

static NTSTATUS
disk_write(RDPCLIENT * This, NTHANDLE handle, uint8 * data, uint32 length, uint32 offset, uint32 * result)
{
	int n;

	lseek(handle, offset, SEEK_SET);

	n = write(handle, data, length);

	if (n < 0)
	{
		perror("write");
		*result = 0;
		switch (errno)
		{
			case ENOSPC:
				return STATUS_DISK_FULL;
			default:
				return STATUS_ACCESS_DENIED;
		}
	}

	*result = n;

	return STATUS_SUCCESS;
}

NTSTATUS
disk_query_information(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, STREAM out)
{
	uint32 file_attributes, ft_high, ft_low;
	struct stat filestat;
	char *path, *filename;

	path = This->fileinfo[handle].path;

	/* Get information about file */
	if (fstat(handle, &filestat) != 0)
	{
		perror("stat");
		out_uint8(out, 0);
		return STATUS_ACCESS_DENIED;
	}

	/* Set file attributes */
	file_attributes = 0;
	if (S_ISDIR(filestat.st_mode))
		file_attributes |= FILE_ATTRIBUTE_DIRECTORY;

	filename = 1 + strrchr(path, '/');
	if (filename && filename[0] == '.')
		file_attributes |= FILE_ATTRIBUTE_HIDDEN;

	if (!file_attributes)
		file_attributes |= FILE_ATTRIBUTE_NORMAL;

	if (!(filestat.st_mode & S_IWUSR))
		file_attributes |= FILE_ATTRIBUTE_READONLY;

	/* Return requested data */
	switch (info_class)
	{
		case FileBasicInformation:
			seconds_since_1970_to_filetime(get_create_time(&filestat), &ft_high,
						       &ft_low);
			out_uint32_le(out, ft_low);	/* create_access_time */
			out_uint32_le(out, ft_high);

			seconds_since_1970_to_filetime(filestat.st_atime, &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* last_access_time */
			out_uint32_le(out, ft_high);

			seconds_since_1970_to_filetime(filestat.st_mtime, &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* last_write_time */
			out_uint32_le(out, ft_high);

			seconds_since_1970_to_filetime(filestat.st_ctime, &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* last_change_time */
			out_uint32_le(out, ft_high);

			out_uint32_le(out, file_attributes);
			break;

		case FileStandardInformation:

			out_uint32_le(out, filestat.st_size);	/* Allocation size */
			out_uint32_le(out, 0);
			out_uint32_le(out, filestat.st_size);	/* End of file */
			out_uint32_le(out, 0);
			out_uint32_le(out, filestat.st_nlink);	/* Number of links */
			out_uint8(out, 0);	/* Delete pending */
			out_uint8(out, S_ISDIR(filestat.st_mode) ? 1 : 0);	/* Directory */
			break;

		case FileObjectIdInformation:

			out_uint32_le(out, file_attributes);	/* File Attributes */
			out_uint32_le(out, 0);	/* Reparse Tag */
			break;

		default:

			unimpl("IRP Query (File) Information class: 0x%x\n", info_class);
			return STATUS_INVALID_PARAMETER;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
disk_set_information(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, STREAM in, STREAM out)
{
	uint32 length, file_attributes, ft_high, ft_low, delete_on_close;
	char newname[PATH_MAX], fullpath[PATH_MAX];
	struct fileinfo *pfinfo;
	int mode;
	struct stat filestat;
	time_t write_time, change_time, access_time, mod_time;
	struct utimbuf tvs;
	struct STATFS_T stat_fs;

	pfinfo = &(This->fileinfo[handle]);
	This->notify_stamp = True;

	switch (info_class)
	{
		case FileBasicInformation:
			write_time = change_time = access_time = 0;

			in_uint8s(in, 4);	/* Handle of root dir? */
			in_uint8s(in, 24);	/* unknown */

			/* CreationTime */
			in_uint32_le(in, ft_low);
			in_uint32_le(in, ft_high);

			/* AccessTime */
			in_uint32_le(in, ft_low);
			in_uint32_le(in, ft_high);
			if (ft_low || ft_high)
				access_time = convert_1970_to_filetime(ft_high, ft_low);

			/* WriteTime */
			in_uint32_le(in, ft_low);
			in_uint32_le(in, ft_high);
			if (ft_low || ft_high)
				write_time = convert_1970_to_filetime(ft_high, ft_low);

			/* ChangeTime */
			in_uint32_le(in, ft_low);
			in_uint32_le(in, ft_high);
			if (ft_low || ft_high)
				change_time = convert_1970_to_filetime(ft_high, ft_low);

			in_uint32_le(in, file_attributes);

			if (fstat(handle, &filestat))
				return STATUS_ACCESS_DENIED;

			tvs.modtime = filestat.st_mtime;
			tvs.actime = filestat.st_atime;
			if (access_time)
				tvs.actime = access_time;


			if (write_time || change_time)
				mod_time = MIN(write_time, change_time);
			else
				mod_time = write_time ? write_time : change_time;

			if (mod_time)
				tvs.modtime = mod_time;


			if (access_time || write_time || change_time)
			{
#if WITH_DEBUG_RDP5
				printf("FileBasicInformation access       time %s",
				       ctime(&tvs.actime));
				printf("FileBasicInformation modification time %s",
				       ctime(&tvs.modtime));
#endif
				if (utime(pfinfo->path, &tvs) && errno != EPERM)
					return STATUS_ACCESS_DENIED;
			}

			if (!file_attributes)
				break;	/* not valid */

			mode = filestat.st_mode;

			if (file_attributes & FILE_ATTRIBUTE_READONLY)
				mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);
			else
				mode |= S_IWUSR;

			mode &= 0777;
#if WITH_DEBUG_RDP5
			printf("FileBasicInformation set access mode 0%o", mode);
#endif

			if (fchmod(handle, mode))
				return STATUS_ACCESS_DENIED;

			break;

		case FileRenameInformation:

			in_uint8s(in, 4);	/* Handle of root dir? */
			in_uint8s(in, 0x1a);	/* unknown */
			in_uint32_le(in, length);

			if (length && (length / 2) < 256)
			{
				rdp_in_unistr(This, in, newname, length);
				convert_to_unix_filename(newname);
			}
			else
			{
				return STATUS_INVALID_PARAMETER;
			}

			sprintf(fullpath, "%s%s", This->rdpdr_device[pfinfo->device_id].local_path,
				newname);

			if (rename(pfinfo->path, fullpath) != 0)
			{
				perror("rename");
				return STATUS_ACCESS_DENIED;
			}
			break;

		case FileDispositionInformation:
			/* As far as I understand it, the correct
			   thing to do here is to *schedule* a delete,
			   so it will be deleted when the file is
			   closed. Subsequent
			   FileDispositionInformation requests with
			   DeleteFile set to FALSE should unschedule
			   the delete. See
			   https://www.osronline.com/article.cfm%5earticle=245.htm . */

			in_uint32_le(in, delete_on_close);

			if (delete_on_close ||
			    (pfinfo->
			     accessmask & (FILE_DELETE_ON_CLOSE | FILE_COMPLETE_IF_OPLOCKED)))
			{
				pfinfo->delete_on_close = True;
			}

			break;

		case FileAllocationInformation:
			/* Fall through to FileEndOfFileInformation,
			   which uses ftrunc. This is like Samba with
			   "strict allocation = false", and means that
			   we won't detect out-of-quota errors, for
			   example. */

		case FileEndOfFileInformation:
			in_uint8s(in, 28);	/* unknown */
			in_uint32_le(in, length);	/* file size */

			/* prevents start of writing if not enough space left on device */
			if (STATFS_FN(This->rdpdr_device[pfinfo->device_id].local_path, &stat_fs) == 0)
				if (stat_fs.f_bfree * stat_fs.f_bsize < length)
					return STATUS_DISK_FULL;

			if (ftruncate_growable(handle, length) != 0)
			{
				return STATUS_DISK_FULL;
			}

			break;
		default:

			unimpl("IRP Set File Information class: 0x%x\n", info_class);
			return STATUS_INVALID_PARAMETER;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
disk_check_notify(RDPCLIENT * This, NTHANDLE handle)
{
	struct fileinfo *pfinfo;
	NTSTATUS status = STATUS_PENDING;

	NOTIFY notify;

	pfinfo = &(This->fileinfo[handle]);
	if (!pfinfo->pdir)
		return STATUS_INVALID_DEVICE_REQUEST;



	status = NotifyInfo(This, handle, pfinfo->info_class, &notify);

	if (status != STATUS_PENDING)
		return status;

	if (memcmp(&pfinfo->notify, &notify, sizeof(NOTIFY)))
	{
		/*printf("disk_check_notify found changed event\n"); */
		memcpy(&pfinfo->notify, &notify, sizeof(NOTIFY));
		status = STATUS_NOTIFY_ENUM_DIR;
	}

	return status;


}

NTSTATUS
disk_create_notify(RDPCLIENT * This, NTHANDLE handle, uint32 info_class)
{

	struct fileinfo *pfinfo;
	NTSTATUS ret = STATUS_PENDING;

	/* printf("start disk_create_notify info_class %X\n", info_class); */

	pfinfo = &(This->fileinfo[handle]);
	pfinfo->info_class = info_class;

	ret = NotifyInfo(This, handle, info_class, &pfinfo->notify);

	if (info_class & 0x1000)
	{			/* ???? */
		if (ret == STATUS_PENDING)
			return STATUS_SUCCESS;
	}

	/* printf("disk_create_notify: num_entries %d\n", pfinfo->notify.num_entries); */


	return ret;

}

static NTSTATUS
NotifyInfo(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, NOTIFY * p)
{
	struct fileinfo *pfinfo;
	struct stat buf;
	struct dirent *dp;
	char *fullname;
	DIR *dpr;

	pfinfo = &(This->fileinfo[handle]);
	if (fstat(handle, &buf) < 0)
	{
		perror("NotifyInfo");
		return STATUS_ACCESS_DENIED;
	}
	p->modify_time = buf.st_mtime;
	p->status_time = buf.st_ctime;
	p->num_entries = 0;
	p->total_time = 0;


	dpr = opendir(pfinfo->path);
	if (!dpr)
	{
		perror("NotifyInfo");
		return STATUS_ACCESS_DENIED;
	}


	while ((dp = readdir(dpr)))
	{
		if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
			continue;
		p->num_entries++;
		fullname = (char *) xmalloc(strlen(pfinfo->path) + strlen(dp->d_name) + 2);
		sprintf(fullname, "%s/%s", pfinfo->path, dp->d_name);

		if (!stat(fullname, &buf))
		{
			p->total_time += (buf.st_mtime + buf.st_ctime);
		}

		xfree(fullname);
	}
	closedir(dpr);

	return STATUS_PENDING;
}

static FsInfoType *
FsVolumeInfo(char *fpath)
{

	static FsInfoType info;
#ifdef USE_SETMNTENT
	FILE *fdfs;
	struct mntent *e;
#endif

	/* initialize */
	memset(&info, 0, sizeof(info));
	strcpy(info.label, "RDESKTOP");
	strcpy(info.type, "RDPFS");

#ifdef USE_SETMNTENT
	fdfs = setmntent(MNTENT_PATH, "r");
	if (!fdfs)
		return &info;

	while ((e = getmntent(fdfs)))
	{
		if (str_startswith(e->mnt_dir, fpath))
		{
			strcpy(info.type, e->mnt_type);
			strcpy(info.name, e->mnt_fsname);
			if (strstr(e->mnt_opts, "vfat") || strstr(e->mnt_opts, "iso9660"))
			{
				int fd = open(e->mnt_fsname, O_RDONLY);
				if (fd >= 0)
				{
					unsigned char buf[512];
					memset(buf, 0, sizeof(buf));
					if (strstr(e->mnt_opts, "vfat"))
						 /*FAT*/
					{
						strcpy(info.type, "vfat");
						read(fd, buf, sizeof(buf));
						info.serial =
							(buf[42] << 24) + (buf[41] << 16) +
							(buf[40] << 8) + buf[39];
						strncpy(info.label, buf + 43, 10);
						info.label[10] = '\0';
					}
					else if (lseek(fd, 32767, SEEK_SET) >= 0)	/* ISO9660 */
					{
						read(fd, buf, sizeof(buf));
						strncpy(info.label, buf + 41, 32);
						info.label[32] = '\0';
						/* info.Serial = (buf[128]<<24)+(buf[127]<<16)+(buf[126]<<8)+buf[125]; */
					}
					close(fd);
				}
			}
		}
	}
	endmntent(fdfs);
#else
	/* initialize */
	memset(&info, 0, sizeof(info));
	strcpy(info.label, "RDESKTOP");
	strcpy(info.type, "RDPFS");

#endif
	return &info;
}


NTSTATUS
disk_query_volume_information(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, STREAM out)
{
	struct STATFS_T stat_fs;
	struct fileinfo *pfinfo;
	FsInfoType *fsinfo;

	pfinfo = &(This->fileinfo[handle]);

	if (STATFS_FN(pfinfo->path, &stat_fs) != 0)
	{
		perror("statfs");
		return STATUS_ACCESS_DENIED;
	}

	fsinfo = FsVolumeInfo(pfinfo->path);

	switch (info_class)
	{
		case FileFsVolumeInformation:

			out_uint32_le(out, 0);	/* volume creation time low */
			out_uint32_le(out, 0);	/* volume creation time high */
			out_uint32_le(out, fsinfo->serial);	/* serial */

			out_uint32_le(out, 2 * strlen(fsinfo->label));	/* length of string */

			out_uint8(out, 0);	/* support objects? */
			rdp_out_unistr(This, out, fsinfo->label, 2 * strlen(fsinfo->label) - 2);
			break;

		case FileFsSizeInformation:

			out_uint32_le(out, stat_fs.f_blocks);	/* Total allocation units low */
			out_uint32_le(out, 0);	/* Total allocation high units */
			out_uint32_le(out, stat_fs.f_bfree);	/* Available allocation units */
			out_uint32_le(out, 0);	/* Available allowcation units */
			out_uint32_le(out, stat_fs.f_bsize / 0x200);	/* Sectors per allocation unit */
			out_uint32_le(out, 0x200);	/* Bytes per sector */
			break;

		case FileFsAttributeInformation:

			out_uint32_le(out, FS_CASE_SENSITIVE | FS_CASE_IS_PRESERVED);	/* fs attributes */
			out_uint32_le(out, F_NAMELEN(stat_fs));	/* max length of filename */

			out_uint32_le(out, 2 * strlen(fsinfo->type));	/* length of fs_type */
			rdp_out_unistr(This, out, fsinfo->type, 2 * strlen(fsinfo->type) - 2);
			break;

		case FileFsLabelInformation:
		case FileFsDeviceInformation:
		case FileFsControlInformation:
		case FileFsFullSizeInformation:
		case FileFsObjectIdInformation:
		case FileFsMaximumInformation:

		default:

			unimpl("IRP Query Volume Information class: 0x%x\n", info_class);
			return STATUS_INVALID_PARAMETER;
	}
	return STATUS_SUCCESS;
}

NTSTATUS
disk_query_directory(RDPCLIENT * This, NTHANDLE handle, uint32 info_class, char *pattern, STREAM out)
{
	uint32 file_attributes, ft_low, ft_high;
	char *dirname, fullpath[PATH_MAX];
	DIR *pdir;
	struct dirent *pdirent;
	struct stat fstat;
	struct fileinfo *pfinfo;

	pfinfo = &(This->fileinfo[handle]);
	pdir = pfinfo->pdir;
	dirname = pfinfo->path;
	file_attributes = 0;

	switch (info_class)
	{
		case FileBothDirectoryInformation:

			/* If a search pattern is received, remember this pattern, and restart search */
			if (pattern[0] != 0)
			{
				strncpy(pfinfo->pattern, 1 + strrchr(pattern, '/'), PATH_MAX - 1);
				rewinddir(pdir);
			}

			/* find next dirent matching pattern */
			pdirent = readdir(pdir);
			while (pdirent && fnmatch(pfinfo->pattern, pdirent->d_name, 0) != 0)
				pdirent = readdir(pdir);

			if (pdirent == NULL)
				return STATUS_NO_MORE_FILES;

			/* Get information for directory entry */
			sprintf(fullpath, "%s/%s", dirname, pdirent->d_name);

			if (stat(fullpath, &fstat))
			{
				switch (errno)
				{
					case ENOENT:
					case ELOOP:
					case EACCES:
						/* These are non-fatal errors. */
						memset(&fstat, 0, sizeof(fstat));
						break;
					default:
						/* Fatal error. By returning STATUS_NO_SUCH_FILE,
						   the directory list operation will be aborted */
						perror(fullpath);
						out_uint8(out, 0);
						return STATUS_NO_SUCH_FILE;
				}
			}

			if (S_ISDIR(fstat.st_mode))
				file_attributes |= FILE_ATTRIBUTE_DIRECTORY;
			if (pdirent->d_name[0] == '.')
				file_attributes |= FILE_ATTRIBUTE_HIDDEN;
			if (!file_attributes)
				file_attributes |= FILE_ATTRIBUTE_NORMAL;
			if (!(fstat.st_mode & S_IWUSR))
				file_attributes |= FILE_ATTRIBUTE_READONLY;

			/* Return requested information */
			out_uint8s(out, 8);	/* unknown zero */

			seconds_since_1970_to_filetime(get_create_time(&fstat), &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* create time */
			out_uint32_le(out, ft_high);

			seconds_since_1970_to_filetime(fstat.st_atime, &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* last_access_time */
			out_uint32_le(out, ft_high);

			seconds_since_1970_to_filetime(fstat.st_mtime, &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* last_write_time */
			out_uint32_le(out, ft_high);

			seconds_since_1970_to_filetime(fstat.st_ctime, &ft_high, &ft_low);
			out_uint32_le(out, ft_low);	/* change_write_time */
			out_uint32_le(out, ft_high);

			out_uint32_le(out, fstat.st_size);	/* filesize low */
			out_uint32_le(out, 0);	/* filesize high */
			out_uint32_le(out, fstat.st_size);	/* filesize low */
			out_uint32_le(out, 0);	/* filesize high */
			out_uint32_le(out, file_attributes);
			out_uint8(out, 2 * strlen(pdirent->d_name) + 2);	/* unicode length */
			out_uint8s(out, 7);	/* pad? */
			out_uint8(out, 0);	/* 8.3 file length */
			out_uint8s(out, 2 * 12);	/* 8.3 unicode length */
			rdp_out_unistr(This, out, pdirent->d_name, 2 * strlen(pdirent->d_name));
			break;

		default:
			/* FIXME: Support FileDirectoryInformation,
			   FileFullDirectoryInformation, and
			   FileNamesInformation */

			unimpl("IRP Query Directory sub: 0x%x\n", info_class);
			return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}



static NTSTATUS
disk_device_control(RDPCLIENT * This, NTHANDLE handle, uint32 request, STREAM in, STREAM out)
{
	if (((request >> 16) != 20) || ((request >> 16) != 9))
		return STATUS_INVALID_PARAMETER;

	/* extract operation */
	request >>= 2;
	request &= 0xfff;

	printf("DISK IOCTL %d\n", request);

	switch (request)
	{
		case 25:	/* ? */
		case 42:	/* ? */
		default:
			unimpl("DISK IOCTL %d\n", request);
			return STATUS_INVALID_PARAMETER;
	}

	return STATUS_SUCCESS;
}

DEVICE_FNS disk_fns = {
	disk_create,
	disk_close,
	disk_read,
	disk_write,
	disk_device_control	/* device_control */
};
