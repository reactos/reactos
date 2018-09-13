/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    NTMasks.h

    This file contains the Access mask mappings for the Generic ACL Editor
    for NTFS.



    FILE HISTORY:
	JeffreyS    1-Sept-1996 Borrowed from shell\security\rshx\ntmasks.hxx

*/

#ifndef _NTMASKS_H_
#define _NTMASKS_H_

/* The following manifests are the permission bitfields that represent
 * each string above.  Note that for the special bits we could have used
 * the permission manifest directly (i.e., FILE_READ_DATA instead of
 * FILE_PERM_SPEC_READ_DATA), however the special bits can also contain
 * multiple flags, so this protects us in case we ever decide to combine
 * some manifests.
 *
 */

/* File Special Permissions
 */
#define FILE_PERM_SPEC_READ		 FILE_GENERIC_READ
#define FILE_PERM_SPEC_WRITE		 FILE_GENERIC_WRITE
#define FILE_PERM_SPEC_EXECUTE		 FILE_GENERIC_EXECUTE
#define FILE_PERM_SPEC_ALL		 FILE_ALL_ACCESS
#define FILE_PERM_SPEC_DELETE		 DELETE
#define FILE_PERM_SPEC_CHANGE_PERM	 WRITE_DAC
#define FILE_PERM_SPEC_CHANGE_OWNER	 WRITE_OWNER

/* File General Permissions
 */
#define FILE_PERM_GEN_NO_ACCESS 	 (0)
#define FILE_PERM_GEN_READ		 (FILE_GENERIC_READ	  |\
					  FILE_GENERIC_EXECUTE)
#define FILE_PERM_GEN_MODIFY		 (FILE_GENERIC_READ	  |\
					  FILE_GENERIC_EXECUTE |\
					  FILE_GENERIC_WRITE   |\
					  DELETE )
#define FILE_PERM_GEN_ALL		 (FILE_ALL_ACCESS)


/* Directory Special Permissions
 */
#define DIR_PERM_SPEC_READ		   FILE_GENERIC_READ
#define DIR_PERM_SPEC_WRITE		   FILE_GENERIC_WRITE
#define DIR_PERM_SPEC_EXECUTE		   FILE_GENERIC_EXECUTE
#define DIR_PERM_SPEC_ALL		   FILE_ALL_ACCESS
#define DIR_PERM_SPEC_DELETE		   DELETE
#define DIR_PERM_SPEC_CHANGE_PERM	   WRITE_DAC
#define DIR_PERM_SPEC_CHANGE_OWNER	   WRITE_OWNER

/* Directory General Permissions
 */
#define DIR_PERM_GEN_NO_ACCESS		   (0)
#define DIR_PERM_GEN_LIST		   (FILE_GENERIC_READ    |\
					    FILE_GENERIC_EXECUTE)
#define DIR_PERM_GEN_READ		   (FILE_GENERIC_READ    |\
					    FILE_GENERIC_EXECUTE)
#define DIR_PERM_GEN_DEPOSIT		   (FILE_GENERIC_WRITE   |\
					    FILE_GENERIC_EXECUTE)
#define DIR_PERM_GEN_PUBLISH		   (FILE_GENERIC_READ    |\
					    FILE_GENERIC_WRITE   |\
					    FILE_GENERIC_EXECUTE)
#define DIR_PERM_GEN_MODIFY		   (FILE_GENERIC_READ    |\
					    FILE_GENERIC_WRITE   |\
					    FILE_GENERIC_EXECUTE |\
					    DELETE	   )
#define DIR_PERM_GEN_ALL		   (FILE_ALL_ACCESS)

//
//  Audit access masks
//
//  Note that ACCESS_SYSTEM_SECURITY is ored with both Generic Read and
//  Generic Write.  Access to the SACL is a privilege and if you have that
//  privilege, then you can both read and write the SACL.
//

#define FILE_AUDIT_ALL                      (FILE_ALL_ACCESS      |\
                                             ACCESS_SYSTEM_SECURITY)
#define FILE_AUDIT_MODIFY		    (FILE_GENERIC_READ    |\
					     FILE_GENERIC_WRITE   |\
					     FILE_GENERIC_EXECUTE |\
					     DELETE	          |\
                                             ACCESS_SYSTEM_SECURITY)
#define FILE_AUDIT_READ                     (FILE_GENERIC_READ |\
                                             ACCESS_SYSTEM_SECURITY)
#define FILE_AUDIT_WRITE                    (FILE_GENERIC_WRITE |\
                                             ACCESS_SYSTEM_SECURITY)
#define FILE_AUDIT_EXECUTE		    FILE_GENERIC_EXECUTE
#define FILE_AUDIT_DELETE		    DELETE
#define FILE_AUDIT_CHANGE_PERM		    WRITE_DAC
#define FILE_AUDIT_CHANGE_OWNER 	    WRITE_OWNER

//#define DIR_AUDIT_READ                      (FILE_GENERIC_READ |\
//                                             ACCESS_SYSTEM_SECURITY)
//#define DIR_AUDIT_WRITE                     (FILE_GENERIC_WRITE |\
//                                            ACCESS_SYSTEM_SECURITY)
//#define DIR_AUDIT_EXECUTE                   FILE_GENERIC_EXECUTE
//#define DIR_AUDIT_DELETE                    DELETE
//#define DIR_AUDIT_CHANGE_PERM               WRITE_DAC
//#define DIR_AUDIT_CHANGE_OWNER              WRITE_OWNER


/* The valid access masks for NTFS
 */
#define NTFS_VALID_ACCESS_MASK		    (0xffffffff)

#endif //_NTMASKS_H_
