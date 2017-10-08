/* NFSv4.1 client for Windows
 * Copyright © 2012 The Regents of the University of Michigan
 *
 * Olga Kornievskaia <aglo@umich.edu>
 * Casey Bodley <cbodley@umich.edu>
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at
 * your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 */

#ifndef __NFS41_NFS_CONST_H__
#define __NFS41_NFS_CONST_H__


/*
 * Sizes
 */
#define NFS4_FHSIZE             128
#define NFS4_VERIFIER_SIZE      8
#define NFS4_OPAQUE_LIMIT       1024
#define NFS4_SESSIONID_SIZE     16
#define NFS4_STATEID_OTHER      12
#define NFS4_EASIZE             256
#define NFS4_EANAME_SIZE        128


#define NFS41_MAX_FILEIO_SIZE   (1024 * 1024)
#define NFS41_MAX_SERVER_CACHE  1024
#define NFS41_MAX_RPC_REQS      128

#define UPCALL_BUF_SIZE         2048

/* MaximumComponentNameLength reported for FileFsAttributeInformation */
#define NFS41_MAX_COMPONENT_LEN     256
#define NFS41_MAX_PATH_LEN          1280

#define NFS41_HOSTNAME_LEN          64
#define NFS41_ADDRS_PER_SERVER      4

/* max length of ipv6 address       48
 * sizeof(".255.255")              + 8 */
#define NFS41_UNIVERSAL_ADDR_LEN    56

/* "udp" "tcp" "udp6" "tcp6" */
#define NFS41_NETWORK_ID_LEN        4

/* msdn: There is a maximum of 31 reparse points (and
 * therefore symbolic links) allowed in a particular path. */
#define NFS41_MAX_SYMLINK_DEPTH     31


/* 424 bytes: max rpc header for reply with data */
/* 32 bytes: max COMPOUND response */
/* 40 bytes: max SEQUENCE response */
/* 4 bytes: max PUTFH response */
/* 12 bytes: max READ response */
#define READ_OVERHEAD       512

/* 840 bytes: max rpc header for call */
/* 32 bytes: max COMPOUND request */
/* 32 bytes: max SEQUENCE request */
/* 132 bytes: max PUTFH request */
/* 32 bytes: max WRITE request */
#define WRITE_OVERHEAD      1068


#define NFS41_RPC_PROGRAM   100003
#define NFS41_RPC_VERSION   4
#define NFS41_RPC_CBPROGRAM 0x2358


/*
 * Error status
 */
enum nfsstat4 {
    NFS4_OK                     = 0,        /* everything is okay      */
    NFS4ERR_PERM                = 1,        /* caller not privileged   */
    NFS4ERR_NOENT               = 2,        /* no such file/directory  */
    NFS4ERR_IO                  = 5,        /* hard I/O error          */
    NFS4ERR_NXIO                = 6,        /* no such device          */
    NFS4ERR_ACCESS              = 13,       /* access denied           */
    NFS4ERR_EXIST               = 17,       /* file already exists     */
    NFS4ERR_XDEV                = 18,       /* different filesystems   */

    NFS4ERR_NOTDIR              = 20,       /* should be a directory   */
    NFS4ERR_ISDIR               = 21,       /* should not be directory */
    NFS4ERR_INVAL               = 22,       /* invalid argument        */
    NFS4ERR_FBIG                = 27,       /* file exceeds server max */
    NFS4ERR_NOSPC               = 28,       /* no space on filesystem  */
    NFS4ERR_ROFS                = 30,       /* read-only filesystem    */
    NFS4ERR_MLINK               = 31,       /* too many hard links     */
    NFS4ERR_NAMETOOLONG         = 63,       /* name exceeds server max */
    NFS4ERR_NOTEMPTY            = 66,       /* directory not empty     */
    NFS4ERR_DQUOT               = 69,       /* hard quota limit reached*/
    NFS4ERR_STALE               = 70,       /* file no longer exists   */
    NFS4ERR_BADHANDLE           = 10001,    /* Illegal filehandle      */
    NFS4ERR_BAD_COOKIE          = 10003,    /* READDIR cookie is stale */
    NFS4ERR_NOTSUPP             = 10004,    /* operation not supported */
    NFS4ERR_TOOSMALL            = 10005,    /* response limit exceeded */
    NFS4ERR_SERVERFAULT         = 10006,    /* undefined server error  */
    NFS4ERR_BADTYPE             = 10007,    /* type invalid for CREATE */
    NFS4ERR_DELAY               = 10008,    /* file "busy" - retry     */
    NFS4ERR_SAME                = 10009,    /* nverify says attrs same */
    NFS4ERR_DENIED              = 10010,    /* lock unavailable        */
    NFS4ERR_EXPIRED             = 10011,    /* lock lease expired      */
    NFS4ERR_LOCKED              = 10012,    /* I/O failed due to lock  */
    NFS4ERR_GRACE               = 10013,    /* in grace period         */
    NFS4ERR_FHEXPIRED           = 10014,    /* filehandle expired      */
    NFS4ERR_SHARE_DENIED        = 10015,    /* share reserve denied    */
    NFS4ERR_WRONGSEC            = 10016,    /* wrong security flavor   */
    NFS4ERR_CLID_INUSE          = 10017,    /* clientid in use         */

    /* NFS4ERR_RESOURCE is not a valid error in NFSv4.1 */
    NFS4ERR_RESOURCE            = 10018,    /* resource exhaustion     */
    NFS4ERR_MOVED               = 10019,    /* filesystem relocated    */
    NFS4ERR_NOFILEHANDLE        = 10020,    /* current FH is not set   */
    NFS4ERR_MINOR_VERS_MISMATCH = 10021,    /* minor vers not supp     */
    NFS4ERR_STALE_CLIENTID      = 10022,    /* server has rebooted     */
    NFS4ERR_STALE_STATEID       = 10023,    /* server has rebooted     */
    NFS4ERR_OLD_STATEID         = 10024,    /* state is out of sync    */
    NFS4ERR_BAD_STATEID         = 10025,    /* incorrect stateid       */
    NFS4ERR_BAD_SEQID           = 10026,    /* request is out of seq.  */
    NFS4ERR_NOT_SAME            = 10027,    /* verify - attrs not same */
    NFS4ERR_LOCK_RANGE          = 10028,    /* overlapping lock range  */
    NFS4ERR_SYMLINK             = 10029,    /* should be file/directory*/
    NFS4ERR_RESTOREFH           = 10030,    /* no saved filehandle     */
    NFS4ERR_LEASE_MOVED         = 10031,    /* some filesystem moved   */
    NFS4ERR_ATTRNOTSUPP         = 10032,    /* recommended attr not sup*/
    NFS4ERR_NO_GRACE            = 10033,    /* reclaim outside of grace*/
    NFS4ERR_RECLAIM_BAD         = 10034,    /* reclaim error at server */
    NFS4ERR_RECLAIM_CONFLICT    = 10035,    /* conflict on reclaim     */
    NFS4ERR_BADXDR              = 10036,    /* XDR decode failed       */
    NFS4ERR_LOCKS_HELD          = 10037,    /* file locks held at CLOSE*/
    NFS4ERR_OPENMODE            = 10038,    /* conflict in OPEN and I/O*/
    NFS4ERR_BADOWNER            = 10039,    /* owner translation bad   */
    NFS4ERR_BADCHAR             = 10040,    /* utf-8 char not supported*/
    NFS4ERR_BADNAME             = 10041,    /* name not supported      */
    NFS4ERR_BAD_RANGE           = 10042,    /* lock range not supported*/
    NFS4ERR_LOCK_NOTSUPP        = 10043,    /* no atomic up/downgrade  */
    NFS4ERR_OP_ILLEGAL          = 10044,    /* undefined operation     */
    NFS4ERR_DEADLOCK            = 10045,    /* file locking deadlock   */
    NFS4ERR_FILE_OPEN           = 10046,    /* open file blocks op.    */
    NFS4ERR_ADMIN_REVOKED       = 10047,    /* lockowner state revoked */
    NFS4ERR_CB_PATH_DOWN        = 10048,    /* callback path down      */

    /* NFSv4.1 errors start here. */
    NFS4ERR_BADIOMODE           = 10049,
    NFS4ERR_BADLAYOUT           = 10050,
    NFS4ERR_BAD_SESSION_DIGEST  = 10051,
    NFS4ERR_BADSESSION          = 10052,
    NFS4ERR_BADSLOT             = 10053,
    NFS4ERR_COMPLETE_ALREADY    = 10054,
    NFS4ERR_CONN_NOT_BOUND_TO_SESSION = 10055,
    NFS4ERR_DELEG_ALREADY_WANTED = 10056,
    NFS4ERR_BACK_CHAN_BUSY      = 10057,    /*backchan reqs outstanding*/
    NFS4ERR_LAYOUTTRYLATER      = 10058,
    NFS4ERR_LAYOUTUNAVAILABLE   = 10059,
    NFS4ERR_NOMATCHING_LAYOUT   = 10060,
    NFS4ERR_RECALLCONFLICT      = 10061,
    NFS4ERR_UNKNOWN_LAYOUTTYPE  = 10062,
    NFS4ERR_SEQ_MISORDERED      = 10063,    /* unexpected seq.ID in req*/
    NFS4ERR_SEQUENCE_POS        = 10064,    /* [CB_]SEQ. op not 1st op */
    NFS4ERR_REQ_TOO_BIG         = 10065,    /* request too big         */
    NFS4ERR_REP_TOO_BIG         = 10066,    /* reply too big           */
    NFS4ERR_REP_TOO_BIG_TO_CACHE = 10067,   /* rep. not all cached     */
    NFS4ERR_RETRY_UNCACHED_REP  = 10068,    /* retry & rep. uncached   */
    NFS4ERR_UNSAFE_COMPOUND     = 10069,    /* retry/recovery too hard */
    NFS4ERR_TOO_MANY_OPS        = 10070,    /*too many ops in [CB_]COMP*/
    NFS4ERR_OP_NOT_IN_SESSION   = 10071,    /* op needs [CB_]SEQ. op   */
    NFS4ERR_HASH_ALG_UNSUPP     = 10072,    /* hash alg. not supp.     */
                                            /* Error 10073 is unused.  */
    NFS4ERR_CLIENTID_BUSY       = 10074,    /* clientid has state      */
    NFS4ERR_PNFS_IO_HOLE        = 10075,    /* IO to _SPARSE file hole */
    NFS4ERR_SEQ_FALSE_RETRY     = 10076,    /* Retry != original req.  */
    NFS4ERR_BAD_HIGH_SLOT       = 10077,    /* req has bad highest_slot*/
    NFS4ERR_DEADSESSION         = 10078,    /*new req sent to dead sess*/
    NFS4ERR_ENCR_ALG_UNSUPP     = 10079,    /* encr alg. not supp.     */
    NFS4ERR_PNFS_NO_LAYOUT      = 10080,    /* I/O without a layout    */
    NFS4ERR_NOT_ONLY_OP         = 10081,    /* addl ops not allowed    */
    NFS4ERR_WRONG_CRED          = 10082,    /* op done by wrong cred   */
    NFS4ERR_WRONG_TYPE          = 10083,    /* op on wrong type object */
    NFS4ERR_DIRDELEG_UNAVAIL    = 10084,    /* delegation not avail.   */
    NFS4ERR_REJECT_DELEG        = 10085,    /* cb rejected delegation  */
    NFS4ERR_RETURNCONFLICT      = 10086,    /* layout get before return*/
    NFS4ERR_DELEG_REVOKED       = 10087     /* deleg./layout revoked   */
};

#define MAKE_WORD0(XXX) (1 << XXX)
#define MAKE_WORD1(XXX) (1 << (XXX-32))
#define MAKE_WORD2(XXX) (1 << (XXX-64))

enum {
/*
 * Mandatory Attributes
 */
    FATTR4_WORD0_SUPPORTED_ATTRS    = MAKE_WORD0(0),
    FATTR4_WORD0_TYPE               = MAKE_WORD0(1),
    FATTR4_WORD0_FH_EXPIRE_TYPE     = MAKE_WORD0(2),
    FATTR4_WORD0_CHANGE             = MAKE_WORD0(3),
    FATTR4_WORD0_SIZE               = MAKE_WORD0(4),
    FATTR4_WORD0_LINK_SUPPORT       = MAKE_WORD0(5),
    FATTR4_WORD0_SYMLINK_SUPPORT    = MAKE_WORD0(6),
    FATTR4_WORD0_NAMED_ATTR         = MAKE_WORD0(7),
    FATTR4_WORD0_FSID               = MAKE_WORD0(8),
    FATTR4_WORD0_UNIQUE_HANDLES     = MAKE_WORD0(9),
    FATTR4_WORD0_LEASE_TIME         = MAKE_WORD0(10),
    FATTR4_WORD0_RDATTR_ERROR       = MAKE_WORD0(11),
    FATTR4_WORD0_FILEHANDLE         = MAKE_WORD0(19),
    FATTR4_WORD2_SUPPATTR_EXCLCREAT = MAKE_WORD2(75),

/*
 * Recommended Attributes
 */
    FATTR4_WORD0_ACL                = MAKE_WORD0(12),
    FATTR4_WORD0_ACLSUPPORT         = MAKE_WORD0(13),
    FATTR4_WORD0_ARCHIVE            = MAKE_WORD0(14),
    FATTR4_WORD0_CANSETTIME         = MAKE_WORD0(15),
    FATTR4_WORD0_CASE_INSENSITIVE   = MAKE_WORD0(16),
    FATTR4_WORD0_CASE_PRESERVING    = MAKE_WORD0(17),
    FATTR4_WORD0_CHOWN_RESTRICTED   = MAKE_WORD0(18),
    FATTR4_WORD0_FILEID             = MAKE_WORD0(20),
    FATTR4_WORD0_FILES_AVAIL        = MAKE_WORD0(21),
    FATTR4_WORD0_FILES_FREE         = MAKE_WORD0(22),
    FATTR4_WORD0_FILES_TOTAL        = MAKE_WORD0(23),
    FATTR4_WORD0_FS_LOCATIONS       = MAKE_WORD0(24),
    FATTR4_WORD0_HIDDEN             = MAKE_WORD0(25),
    FATTR4_WORD0_HOMOGENEOUS        = MAKE_WORD0(26),
    FATTR4_WORD0_MAXFILESIZE        = MAKE_WORD0(27),
    FATTR4_WORD0_MAXLINK            = MAKE_WORD0(28),
    FATTR4_WORD0_MAXNAME            = MAKE_WORD0(29),
    FATTR4_WORD0_MAXREAD            = MAKE_WORD0(30),
    FATTR4_WORD0_MAXWRITE           = MAKE_WORD0(31),
    FATTR4_WORD1_MIMETYPE           = MAKE_WORD1(32),
    FATTR4_WORD1_MODE               = MAKE_WORD1(33),
    FATTR4_WORD1_NO_TRUNC           = MAKE_WORD1(34),
    FATTR4_WORD1_NUMLINKS           = MAKE_WORD1(35),
    FATTR4_WORD1_OWNER              = MAKE_WORD1(36),
    FATTR4_WORD1_OWNER_GROUP        = MAKE_WORD1(37),
    FATTR4_WORD1_QUOTA_AVAIL_HARD   = MAKE_WORD1(38),
    FATTR4_WORD1_QUOTA_AVAIL_SOFT   = MAKE_WORD1(39),
    FATTR4_WORD1_QUOTA_USED         = MAKE_WORD1(40),
    FATTR4_WORD1_RAWDEV             = MAKE_WORD1(41),
    FATTR4_WORD1_SPACE_AVAIL        = MAKE_WORD1(42),
    FATTR4_WORD1_SPACE_FREE         = MAKE_WORD1(43),
    FATTR4_WORD1_SPACE_TOTAL        = MAKE_WORD1(44),
    FATTR4_WORD1_SPACE_USED         = MAKE_WORD1(45),
    FATTR4_WORD1_SYSTEM             = MAKE_WORD1(46),
    FATTR4_WORD1_TIME_ACCESS        = MAKE_WORD1(47),
    FATTR4_WORD1_TIME_ACCESS_SET    = MAKE_WORD1(48),
    FATTR4_WORD1_TIME_BACKUP        = MAKE_WORD1(49),
    FATTR4_WORD1_TIME_CREATE        = MAKE_WORD1(50),
    FATTR4_WORD1_TIME_DELTA         = MAKE_WORD1(51),
    FATTR4_WORD1_TIME_METADATA      = MAKE_WORD1(52),
    FATTR4_WORD1_TIME_MODIFY        = MAKE_WORD1(53),
    FATTR4_WORD1_TIME_MODIFY_SET    = MAKE_WORD1(54),
    FATTR4_WORD1_MOUNTED_ON_FILEID  = MAKE_WORD1(55),
    FATTR4_WORD1_DIR_NOTIF_DELAY    = MAKE_WORD1(56),
    FATTR4_WORD1_DIRENT_NOTIF_DELAY = MAKE_WORD1(57),
    FATTR4_WORD1_DACL               = MAKE_WORD1(58),
    FATTR4_WORD1_SACL               = MAKE_WORD1(59),
    FATTR4_WORD1_CHANGE_POLICY      = MAKE_WORD1(60),
    FATTR4_WORD1_FS_STATUS          = MAKE_WORD1(61),
    FATTR4_WORD1_FS_LAYOUT_TYPE     = MAKE_WORD1(62),
    FATTR4_WORD1_LAYOUT_HINT        = MAKE_WORD1(63),
    FATTR4_WORD2_LAYOUT_TYPE        = MAKE_WORD2(64),
    FATTR4_WORD2_LAYOUT_BLKSIZE     = MAKE_WORD2(65),
    FATTR4_WORD2_LAYOUT_ALIGNMENT   = MAKE_WORD2(66),
    FATTR4_WORD2_FS_LOCATIONS_INFO  = MAKE_WORD2(67),
    FATTR4_WORD2_MDSTHRESHOLD       = MAKE_WORD2(68),
    FATTR4_WORD2_RETENTION_GET      = MAKE_WORD2(69),
    FATTR4_WORD2_RETENTION_SET      = MAKE_WORD2(70),
    FATTR4_WORD2_RETENTEVT_GET      = MAKE_WORD2(71),
    FATTR4_WORD2_RETENTEVT_SET      = MAKE_WORD2(72),
    FATTR4_WORD2_RETENTION_HOLD     = MAKE_WORD2(73),
    FATTR4_WORD2_MODE_SET_MASKED    = MAKE_WORD2(74),
    FATTR4_WORD2_FS_CHARSET_CAP     = MAKE_WORD2(76),
};

/*
 * File types
 */
enum nfs_ftype4 {
    NF4REG          = 1,    /* Regular File */
    NF4DIR          = 2,    /* Directory */
    NF4BLK          = 3,    /* Special File - block device */
    NF4CHR          = 4,    /* Special File - character device */
    NF4LNK          = 5,    /* Symbolic Link */
    NF4SOCK         = 6,    /* Special File - socket */
    NF4FIFO         = 7,    /* Special File - fifo */
    NF4ATTRDIR      = 8,    /* Attribute Directory */
    NF4NAMEDATTR    = 9,    /* Named Attribute */

    NFS_FTYPE_MASK  = 0xF
};

#define CREATE_SESSION4_FLAG_PERSIST        0x00000001
#define CREATE_SESSION4_FLAG_CONN_BACK_CHAN 0x00000002
#define CREATE_SESSION4_FLAG_CONN_RDMA      0x00000004

/* ACLS aclsupport attribute values */
#define ACL4_SUPPORT_ALLOW_ACL  0x00000001
#define ACL4_SUPPORT_DENY_ACL   0x00000002
#define ACL4_SUPPORT_AUDIT_ACL  0x00000004
#define ACL4_SUPPORT_ALARM_ACL  0x00000008

/* ACLS acetype4 field constants */
#define ACE4_ACCESS_ALLOWED_ACE_TYPE      0x00000000
#define ACE4_ACCESS_DENIED_ACE_TYPE       0x00000001
#define ACE4_SYSTEM_AUDIT_ACE_TYPE        0x00000002
#define ACE4_SYSTEM_ALARM_ACE_TYPE        0x00000003

/* ACLS acemask4 field constants */
#define ACE4_READ_DATA            0x00000001
#define ACE4_LIST_DIRECTORY       0x00000001
#define ACE4_WRITE_DATA           0x00000002
#define ACE4_ADD_FILE             0x00000002
#define ACE4_APPEND_DATA          0x00000004
#define ACE4_ADD_SUBDIRECTORY     0x00000004
#define ACE4_READ_NAMED_ATTRS     0x00000008
#define ACE4_WRITE_NAMED_ATTRS    0x00000010
#define ACE4_EXECUTE              0x00000020
#define ACE4_DELETE_CHILD         0x00000040
#define ACE4_READ_ATTRIBUTES      0x00000080
#define ACE4_WRITE_ATTRIBUTES     0x00000100
#define ACE4_WRITE_RETENTION      0x00000200
#define ACE4_WRITE_RETENTION_HOLD 0x00000400

#define ACE4_DELETE               0x00010000
#define ACE4_READ_ACL             0x00020000
#define ACE4_WRITE_ACL            0x00040000
#define ACE4_WRITE_OWNER          0x00080000
#define ACE4_SYNCHRONIZE          0x00100000

#define ACE4_ALL_FILE ACE4_READ_DATA|ACE4_WRITE_DATA|ACE4_APPEND_DATA| \
        ACE4_READ_NAMED_ATTRS|ACE4_WRITE_NAMED_ATTRS|ACE4_EXECUTE| \
        ACE4_READ_ATTRIBUTES|ACE4_WRITE_ATTRIBUTES| \
        ACE4_DELETE|ACE4_READ_ACL|ACE4_WRITE_ACL|ACE4_WRITE_OWNER| \
        ACE4_SYNCHRONIZE
#define ACE4_ALL_DIR ACE4_READ_DATA|ACE4_WRITE_DATA|ACE4_APPEND_DATA| \
        ACE4_READ_NAMED_ATTRS|ACE4_WRITE_NAMED_ATTRS|ACE4_EXECUTE| \
        ACE4_DELETE_CHILD|ACE4_READ_ATTRIBUTES|ACE4_WRITE_ATTRIBUTES| \
        ACE4_DELETE|ACE4_READ_ACL|ACE4_WRITE_ACL|ACE4_WRITE_OWNER| \
        ACE4_SYNCHRONIZE

#define ACE4_GENERIC_READ ACE4_READ_DATA|ACE4_READ_NAMED_ATTRS| \
        ACE4_READ_ATTRIBUTES|ACE4_READ_ACL|ACE4_SYNCHRONIZE
#define ACE4_GENERIC_WRITE ACE4_WRITE_DATA|ACE4_WRITE_NAMED_ATTRS| \
        ACE4_WRITE_ATTRIBUTES|ACE4_READ_ACL|ACE4_SYNCHRONIZE
#define ACE4_GENERIC_EXECUTE ACE4_EXECUTE|ACE4_READ_ATTRIBUTES| \
        ACE4_READ_ACL|ACE4_SYNCHRONIZE
        


#define ACE4_FILE_ALL_ACCESS ACE4_READ_DATA|ACE4_LIST_DIRECTORY| \
        ACE4_WRITE_DATA|ACE4_ADD_FILE|ACE4_APPEND_DATA|ACE4_ADD_SUBDIRECTORY| \
        ACE4_READ_NAMED_ATTRS|ACE4_WRITE_NAMED_ATTRS|ACE4_EXECUTE| \
        ACE4_READ_ATTRIBUTES|ACE4_WRITE_ATTRIBUTES

/* ACLS aceflag4 field constants */
#define ACE4_FILE_INHERIT_ACE             0x00000001
#define ACE4_DIRECTORY_INHERIT_ACE        0x00000002
#define ACE4_NO_PROPAGATE_INHERIT_ACE     0x00000004
#define ACE4_INHERIT_ONLY_ACE             0x00000008
#define ACE4_SUCCESSFUL_ACCESS_ACE_FLAG   0x00000010
#define ACE4_FAILED_ACCESS_ACE_FLAG       0x00000020
#define ACE4_IDENTIFIER_GROUP             0x00000040
#define ACE4_INHERITED_ACE                0x00000080

/* ACLS well-defined WHOs */
#define ACE4_OWNER "OWNER@"
#define ACE4_GROUP "GROUP@"
#define ACE4_EVERYONE "EVERYONE@"
#define ACE4_INTERACTIVE "INTERACTIVE@"
#define ACE4_NETWORK "NETWORK@"
#define ACE4_DIALUP "DIALUP@"
#define ACE4_BATCH "BATCH@"
#define ACE4_ANONYMOUS "ANONYMOUS@"
#define ACE4_AUTHENTICATED "AUTHENTICATED@"
#define ACE4_SERVICE "SERVICE@"
#define ACE4_NOBODY "nobody"

/* ACLE nfsacl41 aclflag4 constants */
#define ACL4_AUTO_INHERIT         0x00000001
#define ACL4_PROTECTED            0x00000002
#define ACL4_DEFAULTED            0x00000004


#endif /* !__NFS41_NFS_CONST_H__ */
