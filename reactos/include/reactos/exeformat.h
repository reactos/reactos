#ifndef REACTOS_EXEFORMAT_H_INCLUDED_
#define REACTOS_EXEFORMAT_H_INCLUDED_ 1

#define FACILITY_ROS_EXEFMT        (0x10)

/*
 * Returned by ExeFormat loaders to tell the caller the format isn't supported,
 * as opposed to STATUS_INVALID_IMAGE_FORMAT meaning the format is supported,
 * but the particular file is malformed
 */
#define STATUS_ROS_EXEFMT_UNKNOWN_FORMAT /* TODO */

/*
 * Returned by MmCreateSection to signal successful loading of an executable
 * image, saving the caller the effort of determining the executable's format
 * again
 */
#define FACILITY_ROS_EXEFMT_FORMAT      (0x11)
#define STATUS_ROS_EXEFMT_LOADED_FORMAT /* TODO */

/* non-standard format, ZwQuerySection required to retrieve the format tag */
#define EXEFMT_LOADED_EXTENDED (0x0000FFFF)

/* Windows PE/PE+ */
#define EXEFMT_LOADED_PE32     (0x00000000)
#define EXEFMT_LOADED_PE64     (0x00000001)

/* Wine ELF */
#define EXEFMT_LOADED_WINE32   (0x00000002)
#define EXEFMT_LOADED_WINE64   (0x00000003)

/* regular ELF */
#define EXEFMT_LOADED_ELF32    (0x00000004)
#define EXEFMT_LOADED_ELF64    (0x00000005)

#endif

/* EOF */
