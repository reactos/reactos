#ifndef __VFS_H
#define __VFS_H

#if !defined(_MSC_VER)
#if !defined(SCO_FLAVOR) || !defined(_SYS_SELECT_H)
#	include <sys/time.h>	/* alex: this redefines struct timeval */
#endif /* SCO_FLAVOR */
#else
#include <time.h>
#endif

#ifdef HAVE_UTIME_H
#    include <utime.h>
#else
struct utimbuf {
	time_t actime;
	time_t modtime;
};
#endif

#ifdef USE_VFS

#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

    /* Our virtual file system layer */
    
    typedef void * vfsid;

    struct vfs_stamping;
    
    typedef struct {
	void  *(*open)(char *fname, int flags, int mode);
	int   (*close)(void *vfs_info);
	int   (*read)(void *vfs_info, char *buffer, int count);
	int   (*write)(void *vfs_info, char *buf, int count);

	void  *(*opendir)(char *dirname);
	void  *(*readdir)(void *vfs_info);
	int   (*closedir)(void *vfs_info);

	int  (*stat)(char *path, struct stat *buf);
	int  (*lstat)(char *path, struct stat *buf);
	int  (*fstat)(void *vfs_info, struct stat *buf);

	int  (*chmod)(char *path, int mode);
	int  (*chown)(char *path, int owner, int group);
	int  (*utime)(char *path, struct utimbuf *times);

	int  (*readlink)(char *path, char *buf, int size);
	int  (*symlink)(char *n1, char *n2);
	int  (*link)(char *p1, char *p2);
	int  (*unlink)(char *path);
	int  (*rename)(char *p1, char *p2);
	int  (*chdir)(char *path);
	int  (*ferrno)(void);
	int  (*lseek)(void *vfs_info, off_t offset, int whence);
	int  (*mknod)(char *path, int mode, int dev);
	
	vfsid (*getid)(char *path, struct vfs_stamping **parent);
	int  (*nothingisopen)(vfsid id);
	void (*free)(vfsid id);
	
	char *(*getlocalcopy)(char *filename);
	void (*ungetlocalcopy)(char *filename, char *local, int has_changed);

	int  (*mkdir)(char *path, mode_t mode);
	int  (*rmdir)(char *path);
	
	int  (*ctl)(void *vfs_info, int ctlop, int arg);
	int  (*setctl)(char *path, int ctlop, char *arg);
	void (*forget_about)(char *path);
#ifdef HAVE_MMAP
	caddr_t (*mmap)(caddr_t addr, size_t len, int prot, int flags, void *vfs_info, off_t offset);
	int (*munmap)(caddr_t addr, size_t len, void *vfs_info);
#endif	
    } vfs;

    /* Other file systems */
    extern vfs local_vfs_ops;
    extern vfs tarfs_vfs_ops;

    extern vfs ftpfs_vfs_ops;
    extern vfs mcfs_vfs_ops;
    
    extern vfs extfs_vfs_ops;

    extern vfs undelfs_vfs_ops;

    struct vfs_stamping {
        vfs *v;
        vfsid id;
        struct vfs_stamping *parent; /* At the moment applies to tarfs only */
        struct vfs_stamping *next;
        struct timeval time;
    };

    void vfs_init (void);
    void vfs_shut (void);

    extern int vfs_type_absolute;
    vfs *vfs_type (char *path);
    vfsid vfs_ncs_getid (vfs *nvfs, char *dir, struct vfs_stamping **par);
    void vfs_rm_parents (struct vfs_stamping *stamp);
    char *vfs_path (char *path);
    char *vfs_canon (char *path);
    char *mc_get_current_wd (char *buffer, int bufsize);
    int vfs_current_is_local (void);
    int vfs_current_is_extfs (void);
    int vfs_current_is_tarfs (void);
    int vfs_file_is_local (char *name);
    int vfs_file_is_ftp (char *filename);
    char *vfs_get_current_dir (void);
    
    void vfs_stamp (vfs *, vfsid);
    void vfs_rmstamp (vfs *, vfsid, int);
    void vfs_addstamp (vfs *, vfsid, struct vfs_stamping *);
    void vfs_add_noncurrent_stamps (vfs *, vfsid, struct vfs_stamping *);
    void vfs_add_current_stamps (void);
    void vfs_free_resources(char *path);
    void vfs_timeout_handler ();
    int vfs_timeouts ();
    void vfs_force_expire (char *pathname);

    void vfs_fill_names (void (*)(char *));

    /* Required for the vfs_canon routine */
    char *tarfs_analysis (char *inname, char **archive, int is_dir);

    void ftpfs_init(void);
    void ftpfs_done(void);
    void ftpfs_set_debug (char *file);
#ifdef USE_NETCODE
    void ftpfs_hint_reread(int reread);
    void ftpfs_flushdir(void);
#else
#   define ftpfs_flushdir()
#   define ftpfs_hint_reread(x) 
#endif
    /* They fill the file system names */
    void mcfs_fill_names (void (*)(char *));
    void ftpfs_fill_names (void (*)(char *));
    void tarfs_fill_names (void (*)(char *));
    
    char *ftpfs_gethome (char *);
    char *mcfs_gethome (char *);
    
    char *ftpfs_getupdir (char *);
    char *mcfs_getupdir (char *);

    /* Only the routines outside of the VFS module need the emulation macros */

	int mc_open (char *file, int flags, ...);
	int mc_close (int handle);
	int mc_read (int handle, char *buffer, int count);
	int mc_write (int hanlde, char *buffer, int count);
	off_t mc_lseek (int fd, off_t offset, int whence);
	int mc_chdir (char *);

	DIR *mc_opendir (char *dirname);
	struct dirent *mc_readdir(DIR *dirp);
	int mc_closedir (DIR *dir);

	int mc_stat (char *path, struct stat *buf);
	int mc_lstat (char *path, struct stat *buf);
	int mc_fstat (int fd, struct stat *buf);

	int mc_chmod (char *path, int mode);
	int mc_chown (char *path, int owner, int group);
	int mc_utime (char *path, struct utimbuf *times);
	int mc_readlink(char *path, char *buf, int bufsiz);
	int mc_unlink (char *path);
	int mc_symlink (char *name1, char *name2);
        int mc_link (char *name1, char *name2);
        int mc_mknod (char *, int, int);
	int mc_rename (char *original, char *target);
	int mc_write (int fd, char *buf, int nbyte);
        int mc_rmdir (char *path);
        int mc_mkdir (char *path, mode_t mode);
        char *mc_getlocalcopy (char *filename);
        void mc_ungetlocalcopy (char *filename, char *local, int has_changed);
        char *mc_def_getlocalcopy (char *filename);
        void mc_def_ungetlocalcopy (char *filename, char *local, int has_changed);
        int mc_ctl (int fd, int ctlop, int arg);
        int mc_setctl (char *path, int ctlop, char *arg);
#ifdef HAVE_MMAP
	    caddr_t mc_mmap (caddr_t, size_t, int, int, int, off_t);
	    int mc_unmap (caddr_t, size_t);
            int mc_munmap (caddr_t addr, size_t len);
#endif /* HAVE_MMAP */

#else

#ifdef USE_NETCODE
#    undef USE_NETCODE
#endif

#   define vfs_fill_names(x)
#   define vfs_add_current_stamps()
#   define vfs_current_is_local() 1
#   define vfs_file_is_local(x) 1
#   define vfs_file_is_ftp(x) 0
#   define vfs_current_is_tarfs() 0
#   define vfs_current_is_extfs() 0
#   define vfs_path(x) x
#   define mc_close close
#   define mc_read read
#   define mc_write write
#   define mc_lseek lseek
#   define mc_opendir opendir
#   define mc_readdir readdir
#   define mc_closedir closedir

#   define mc_get_current_wd(x,size) get_current_wd (x, size)
#   define mc_fstat fstat
#   define mc_lstat lstat

#   define mc_readlink readlink
#   define mc_symlink symlink
#   define mc_rename rename

#ifndef __os2__
#   define mc_open open
#   define mc_utime utime
#   define mc_chmod chmod
#   define mc_chown chown
#   define mc_chdir chdir
#   define mc_unlink unlink
#endif

#   define mc_mmap mmap
#   define mc_munmap munmap

#   define mc_ctl(a,b,c) 0
#   define mc_setctl(a,b,c)

#   define mc_stat stat
#   define mc_mknod mknod
#   define mc_link link
#   define mc_mkdir mkdir
#   define mc_rmdir rmdir
#   define is_special_prefix(x) 0
#   define vfs_type(x) (vfs *)(NULL)
#   define vfs_setup_wd()
#   define vfs_init()
#   define vfs_shut()
#   define vfs_canon(p) strdup (canonicalize_pathname(p))
#   define vfs_free_resources()
#   define vfs_timeout_handler()
#   define vfs_timeouts() 0
#   define vfs_force_expire () 

    typedef int vfs;
    
#   define mc_getlocalcopy(x) NULL
#   define mc_ungetlocalcopy(x,y,z)

#   define ftpfs_hint_reread(x) 
#   define ftpfs_flushdir()

#ifdef _OS_NT
#   undef mc_rmdir
#endif

#ifdef OS2_NT
#   undef mc_ctl
#   undef mc_unlink
#   define mc_ctl(a,b,c) 0
#   ifndef __EMX__
#      undef mc_mkdir
#      define mc_mkdir(a,b) mkdir(a)
#   endif
#endif

#endif /* USE_VFS */

#define mc_errno errno

#ifdef WANT_PARSE_LS_LGA
int parse_ls_lga (char *p, struct stat *s, char **filename, char **linkname);
#endif

#define MCCTL_SETREMOTECOPY	0
#define MCCTL_ISREMOTECOPY	1
#define MCCTL_REMOTECOPYCHUNK	2
#define MCCTL_FINISHREMOTE	3
#define MCCTL_FLUSHDIR          4

/* Return codes from the ${fs}_ctl routine */

#define MCERR_TARGETOPEN	-1
    /* Can't open target file */
#define MCERR_READ		-2
    /* Read error on source file */
#define MCERR_WRITE		-3
    /* Write error on target file */
#define MCERR_FINISH		-4
    /* Finished transfer */
#define MCERR_DATA_ON_STDIN     -5
    /* Data waiting on stdin to be processed */

#endif /* __VFS_H */

