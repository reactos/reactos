#define __WINE_CONFIG_H

/* Define to a function attribute for Microsoft hotpatch assembly prefix. */
#ifndef DECLSPEC_HOTPATCH
#if defined(_MSC_VER) || defined(__clang__)
/* FIXME: http://llvm.org/bugs/show_bug.cgi?id=20888 */
#define DECLSPEC_HOTPATCH
#else
#define DECLSPEC_HOTPATCH __attribute__((__ms_hook_prologue__))
#endif
#endif /* DECLSPEC_HOTPATCH */

/* Define to the file extension for executables. */
#define EXEEXT ".exe"

/* Define to 1 if you have the <alias.h> header file. */
/* #undef HAVE_ALIAS_H */

/* Define to 1 if you have the <alsa/asoundlib.h> header file. */
/*#undef HAVE_ALSA_ASOUNDLIB_H */

/* Define to 1 if you have the <AL/al.h> header file. */
/* #undef HAVE_AL_AL_H */

/* Define to 1 if you have the <ApplicationServices/ApplicationServices.h>
   header file. */
/* #undef HAVE_APPLICATIONSERVICES_APPLICATIONSERVICES_H */

/* Define to 1 if you have the <arpa/inet.h> header file. */
/* #undef HAVE_ARPA_INET_H */

/* Define to 1 if you have the <arpa/nameser.h> header file. */
/* #undef HAVE_ARPA_NAMESER_H */

/* Define to 1 if you have the `asctime_r' function. */
/* #undef HAVE_ASCTIME_R */

/* Define to 1 if you have the <asm/types.h> header file. */
/* #undef HAVE_ASM_TYPES_H */

/* Define to 1 if you have the <AudioToolbox/AudioConverter.h> header file. */
/* #undef HAVE_AUDIOTOOLBOX_AUDIOCONVERTER_H */

/* Define to 1 if you have the <AudioUnit/AudioComponent.h> header file. */
/* #undef HAVE_AUDIOUNIT_AUDIOCOMPONENT_H */

/* Define to 1 if you have the <AudioUnit/AudioUnit.h> header file. */
/* #undef HAVE_AUDIOUNIT_AUDIOUNIT_H */

/* Define to 1 if you have the <audio/audiolib.h> header file. */
/* #undef HAVE_AUDIO_AUDIOLIB_H */

/* Define to 1 if you have the <audio/soundlib.h> header file. */
/* #undef HAVE_AUDIO_SOUNDLIB_H */

/* Define to 1 if you have the `AUGraphAddNode' function. */
/* #undef HAVE_AUGRAPHADDNODE */

/* Define to 1 if you have the <capi20.h> header file. */
/* #undef HAVE_CAPI20_H */

/* Define to 1 if you have the <Carbon/Carbon.h> header file. */
/* #undef HAVE_CARBON_CARBON_H */

/* Define to 1 if you have the `chsize' function. */
#define HAVE_CHSIZE 1

/* Define to 1 if you have the <CL/cl.h> header file. */
/* #undef HAVE_CL_CL_H */

/* Define to 1 if you have the <CoreAudio/CoreAudio.h> header file. */
/* #undef HAVE_COREAUDIO_COREAUDIO_H */

/* Define to 1 if you have the <cups/cups.h> header file. */
/* #undef HAVE_CUPS_CUPS_H */

/* Define to 1 if you have the <curses.h> header file. */
/* #undef HAVE_CURSES_H */

/* Define if you have the daylight variable */
#define HAVE_DAYLIGHT 1

/* Define to 1 if you have the <dbus/dbus.h> header file. */
/* #undef HAVE_DBUS_DBUS_H */

/* Define to 1 if you have the <direct.h> header file. */
#define HAVE_DIRECT_H 1

/* Define to 1 if you have the <dirent.h> header file. */
#define HAVE_DIRENT_H 1

/* Define to 1 if you have the <DiskArbitration/DiskArbitration.h> header
   file. */
/* #undef HAVE_DISKARBITRATION_DISKARBITRATION_H */

/* Define to 1 if you have the `dladdr' function. */
/* #undef HAVE_DLADDR */

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the `dlopen' function. */
/* #undef HAVE_DLOPEN */

/* Define to 1 if you have the <elf.h> header file. */
/* #undef HAVE_ELF_H */

/* Define to 1 if you have the `epoll_create' function. */
/* #undef HAVE_EPOLL_CREATE */

/* Define to 1 if you have the `ffs' function. */
/* #undef HAVE_FFS */

/* Define to 1 if you have the `finite' function. */
#define HAVE_FINITE 1

/* Define to 1 if you have the <float.h> header file. */
#define HAVE_FLOAT_H 1

/* Define to 1 if you have the `fnmatch' function. */
/* #undef HAVE_FNMATCH */

/* Define to 1 if you have the <fnmatch.h> header file. */
/* #undef HAVE_FNMATCH_H */

/* Define to 1 if you have the <fontconfig/fontconfig.h> header file. */
/* #undef HAVE_FONTCONFIG_FONTCONFIG_H */

/* Define to 1 if you have the `fork' function. */
/* #undef HAVE_FORK */

/* Define to 1 if you have the `fpclass' function. */
#define HAVE_FPCLASS 1

/* Define if FreeType 2 is installed */
#define HAVE_FREETYPE 1

/* Define to 1 if you have the <freetype/freetype.h> header file. */
#define HAVE_FREETYPE_FREETYPE_H 1

/* Define to 1 if you have the <freetype/ftglyph.h> header file. */
#define HAVE_FREETYPE_FTGLYPH_H 1

/* Define to 1 if you have the <freetype/ftlcdfil.h> header file. */
#define HAVE_FREETYPE_FTLCDFIL_H 1

/* Define to 1 if you have the <freetype/ftmodapi.h> header file. */
#define HAVE_FREETYPE_FTMODAPI_H 1

/* Define to 1 if you have the <freetype/ftoutln.h> header file. */
#define HAVE_FREETYPE_FTOUTLN_H 1

/* Define to 1 if you have the <freetype/ftsnames.h> header file. */
#define HAVE_FREETYPE_FTSNAMES_H 1

/* Define if you have the <freetype/fttrigon.h> header file. */
#define HAVE_FREETYPE_FTTRIGON_H 1

/* Define to 1 if you have the <freetype/fttypes.h> header file. */
/*#undef HAVE_FREETYPE_FTTYPES_H*/

/* Define to 1 if you have the <freetype/ftwinfnt.h> header file. */
#define HAVE_FREETYPE_FTWINFNT_H 1

/* Define to 1 if you have the <freetype/internal/sfnt.h> header file. */
/* #undef HAVE_FREETYPE_INTERNAL_SFNT_H */

/* Define to 1 if you have the <freetype/ttnameid.h> header file. */
#define HAVE_FREETYPE_TTNAMEID_H 1

/* Define to 1 if you have the <freetype/tttables.h> header file. */
#define HAVE_FREETYPE_TTTABLES_H 1

/* Define to 1 if the system has the type `fsblkcnt_t'. */
/* #undef HAVE_FSBLKCNT_T */

/* Define to 1 if the system has the type `fsfilcnt_t'. */
/* #undef HAVE_FSFILCNT_T */

/* Define to 1 if you have the `fstatfs' function. */
/* #undef HAVE_FSTATFS */

/* Define to 1 if you have the `fstatvfs' function. */
/* #undef HAVE_FSTATVFS */

/* Define to 1 if you have the <ft2build.h> header file. */
#define HAVE_FT2BUILD_H 1

/* Define to 1 if you have the `ftruncate' function. */
#define HAVE_FTRUNCATE 1

/* Define to 1 if you have the `FT_Load_Sfnt_Table' function. */
/* #undef HAVE_FT_LOAD_SFNT_TABLE */

/* Define to 1 if the system has the type `FT_TrueTypeEngineType'. */
#define HAVE_FT_TRUETYPEENGINETYPE 1

/* Define to 1 if you have the `futimes' function. */
/* #undef HAVE_FUTIMES */

/* Define to 1 if you have the `futimesat' function. */
/* #undef HAVE_FUTIMESAT */

/* Define to 1 if you have the `getaddrinfo' function. */
#define HAVE_GETADDRINFO 1

/* Define to 1 if you have the `getattrlist' function. */
/* #undef HAVE_GETATTRLIST */

/* Define to 1 if you have the `getdirentries' function. */
/* #undef HAVE_GETDIRENTRIES */

/* Define to 1 if you have the `getnameinfo' function. */
/* #undef HAVE_GETNAMEINFO */

/* Define to 1 if you have the `getnetbyname' function. */
/* #undef HAVE_GETNETBYNAME */

/* Define to 1 if you have the <getopt.h> header file. */
#define HAVE_GETOPT_H 1

/* Define to 1 if you have the `getopt_long' function. */
#define HAVE_GETOPT_LONG 1

/* Define to 1 if you have the `getpagesize' function. */
#define HAVE_GETPAGESIZE 1

/* Define to 1 if you have the `getprotobyname' function. */
#define HAVE_GETPROTOBYNAME 1

/* Define to 1 if you have the `getprotobynumber' function. */
#define HAVE_GETPROTOBYNUMBER 1

/* Define to 1 if you have the `getpwuid' function. */
/* #undef HAVE_GETPWUID */

/* Define to 1 if you have the `getservbyport' function. */
#define HAVE_GETSERVBYPORT 1

/* Define to 1 if you have the <gettext-po.h> header file. */
/* #undef HAVE_GETTEXT_PO_H */

/* Define to 1 if you have the `gettimeofday' function. */
/* #undef HAVE_GETTIMEOFDAY */

/* Define to 1 if you have the `getuid' function. */
/* #undef HAVE_GETUID */

/* Define to 1 if you have the <GL/glu.h> header file. */
/* #undef HAVE_GL_GLU_H */

/* Define to 1 if you have the <GL/glx.h> header file. */
/* #undef HAVE_GL_GLX_H */

/* Define to 1 if you have the <GL/gl.h> header file. */
/* #undef HAVE_GL_GL_H */

/* Define if we have libgphoto2 development environment */
/*#undef HAVE_GPHOTO2*/

/* Define to 1 if you have the <grp.h> header file. */
/*#undef HAVE_GRP_H*/

/* Define to 1 if you have the <gsm/gsm.h> header file. */
/*#undef HAVE_GSM_GSM_H*/

/* Define to 1 if you have the <gsm.h> header file. */
/* #undef HAVE_GSM_H*/

/* Define to 1 if you have the <hal/libhal.h> header file. */
/* #undef HAVE_HAL_LIBHAL_H*/

/* Define to 1 if you have the <ieeefp.h> header file. */
/* #undef HAVE_IEEEFP_H */

/* Define to 1 if you have the <ifaddrs.h> header file. */
/* #undef HAVE_IFADDRS_H */

/* Define to 1 if you have the <inet/mib2.h> header file. */
/*#undef HAVE_INET_MIB2_H*/

/* Define to 1 if you have the `inet_network' function. */
/* #undef HAVE_INET_NETWORK */

/* Define to 1 if you have the `inet_ntop' function. */
#define HAVE_INET_NTOP 1

/* Define to 1 if you have the `inet_pton' function. */
/* #undef HAVE_INET_PTON */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the `IOHIDManagerCreate' function. */
/* #undef HAVE_IOHIDMANAGERCREATE */

/* Define to 1 if you have the <IOKit/hid/IOHIDLib.h> header file. */
/* #undef HAVE_IOKIT_HID_IOHIDLIB_H */

/* Define to 1 if you have the <IOKit/IOKitLib.h> header file. */
/* #undef HAVE_IOKIT_IOKITLIB_H */

/* Define to 1 if you have the <io.h> header file. */
#define HAVE_IO_H 1

/* Define to 1 if you have the `isfinite' function. */
/* #undef HAVE_ISFINITE */

/* Define to 1 if you have the `isinf' function. */
/* #undef HAVE_ISINF */

/* Define to 1 if you have the `isnan' function. */
/* #undef HAVE_ISNAN */

/* Define to 1 if you have the <jack/jack.h> header file. */
/* #undef HAVE_JACK_JACK_H */

/* Define to 1 if you have the <jpeglib.h> header file. */
#define HAVE_JPEGLIB_H 1

/* Define to 1 if you have the `kqueue' function. */
/* #undef HAVE_KQUEUE */

/* Define to 1 if you have the <kstat.h> header file. */
/* #undef HAVE_KSTAT_H */

/* Define to 1 if you have the <lber.h> header file. */
/* #undef HAVE_LBER_H */

/* Define if you have the LittleCMS development environment */
/* #undef HAVE_LCMS */

/* Define to 1 if you have the <lcms.h> header file. */
/* #undef HAVE_LCMS_H */

/* Define to 1 if you have the <lcms/lcms.h> header file. */
/* #undef HAVE_LCMS_LCMS_H */

/* Define if you have the OpenLDAP development environment */
/* #undef HAVE_LDAP */

/* Define to 1 if you have the `ldap_count_references' function. */
/* #undef HAVE_LDAP_COUNT_REFERENCES */

/* Define to 1 if you have the `ldap_first_reference' function. */
/* #undef HAVE_LDAP_FIRST_REFERENCE */

/* Define to 1 if you have the <ldap.h> header file. */
/* #undef HAVE_LDAP_H */

/* Define to 1 if you have the `ldap_next_reference' function. */
/* #undef HAVE_LDAP_NEXT_REFERENCE */

/* Define to 1 if you have the `ldap_parse_reference' function. */
/* #undef HAVE_LDAP_PARSE_REFERENCE */

/* Define to 1 if you have the `ldap_parse_sortresponse_control' function. */
/* #undef HAVE_LDAP_PARSE_SORTRESPONSE_CONTROL */

/* Define to 1 if you have the `ldap_parse_sort_control' function. */
/* #undef HAVE_LDAP_PARSE_SORT_CONTROL */

/* Define to 1 if you have the `ldap_parse_vlvresponse_control' function. */
/* #undef HAVE_LDAP_PARSE_VLVRESPONSE_CONTROL */

/* Define to 1 if you have the `ldap_parse_vlv_control' function. */
/* #undef HAVE_LDAP_PARSE_VLV_CONTROL */

/* Define if you have libaudioIO */
/* #undef HAVE_LIBAUDIOIO */

/* Define to 1 if you have the `gettextpo' library (-lgettextpo). */
/* #undef HAVE_LIBGETTEXTPO */

/* Define to 1 if you have the `i386' library (-li386). */
/* #undef HAVE_LIBI386 */

/* Define to 1 if you have the `kstat' library (-lkstat). */
/* #undef HAVE_LIBKSTAT */

/* Define to 1 if you have the `ossaudio' library (-lossaudio). */
/* #undef HAVE_LIBOSSAUDIO */

/* Define if you have the libxml2 library */
#define HAVE_LIBXML2 1

/* Define to 1 if you have the <libxml/parser.h> header file. */
#define HAVE_LIBXML_PARSER_H 1

/* Define if you have the X Shape extension */
/* #undef HAVE_LIBXSHAPE */

/* Define to 1 if you have the <libxslt/pattern.h> header file. */
#define HAVE_LIBXSLT_PATTERN_H 1

/* Define to 1 if you have the <libxslt/transform.h> header file. */
#define HAVE_LIBXSLT_TRANSFORM_H 1

/* Define if you have the X Shm extension */
/* #undef HAVE_LIBXXSHM */

/* Define to 1 if you have the <link.h> header file. */
/* #undef HAVE_LINK_H */

/* Define if <linux/joystick.h> defines the Linux 2.2 joystick API */
/* #undef HAVE_LINUX_22_JOYSTICK_API */

/* Define to 1 if you have the <linux/capi.h> header file. */
/* #undef HAVE_LINUX_CAPI_H */

/* Define to 1 if you have the <linux/cdrom.h> header file. */
/* #undef HAVE_LINUX_CDROM_H */

/* Define to 1 if you have the <linux/compiler.h> header file. */
/* #undef HAVE_LINUX_COMPILER_H */

/* Define if Linux-style gethostbyname_r and gethostbyaddr_r are available */
/* #undef HAVE_LINUX_GETHOSTBYNAME_R_6 */

/* Define to 1 if you have the <linux/hdreg.h> header file. */
/* #undef HAVE_LINUX_HDREG_H */

/* Define to 1 if you have the <linux/input.h> header file. */
/* #undef HAVE_LINUX_INPUT_H */

/* Define to 1 if you have the <linux/ioctl.h> header file. */
/* #undef HAVE_LINUX_IOCTL_H */

/* Define to 1 if you have the <linux/ipx.h> header file. */
/* #undef HAVE_LINUX_IPX_H */

/* Define to 1 if you have the <linux/irda.h> header file. */
/* #undef HAVE_LINUX_IRDA_H */

/* Define to 1 if you have the <linux/joystick.h> header file. */
/* #undef HAVE_LINUX_JOYSTICK_H */

/* Define to 1 if you have the <linux/major.h> header file. */
/* #undef HAVE_LINUX_MAJOR_H */

/* Define to 1 if you have the <linux/param.h> header file. */
/* #undef HAVE_LINUX_PARAM_H */

/* Define to 1 if you have the <linux/serial.h> header file. */
/* #undef HAVE_LINUX_SERIAL_H */

/* Define to 1 if you have the <linux/types.h> header file. */
/* #undef HAVE_LINUX_TYPES_H */

/* Define to 1 if you have the <linux/ucdrom.h> header file. */
/* #undef HAVE_LINUX_UCDROM_H */

/* Define to 1 if you have the <linux/videodev.h> header file. */
/* #undef HAVE_LINUX_VIDEODEV_H */

/* Define to 1 if the system has the type `long long'. */
#define HAVE_LONG_LONG 1

/* Define to 1 if you have the `lstat' function. */
/* #undef HAVE_LSTAT */

/* Define to 1 if you have the <machine/cpu.h> header file. */
/* #undef HAVE_MACHINE_CPU_H */

/* Define to 1 if you have the <machine/limits.h> header file. */
/* #undef HAVE_MACHINE_LIMITS_H */

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <machine/sysarch.h> header file. */
/* #undef HAVE_MACHINE_SYSARCH_H */

/* Define to 1 if you have the <mach/machine.h> header file. */
/* #undef HAVE_MACH_MACHINE_H */

/* Define to 1 if you have the <mach/mach.h> header file. */
/* #undef HAVE_MACH_MACH_H */

/* Define to 1 if you have the <mach-o/dyld_images.h> header file. */
/* #undef HAVE_MACH_O_DYLD_IMAGES_H */

/* Define to 1 if you have the <mach-o/nlist.h> header file. */
/* #undef HAVE_MACH_O_NLIST_H */

/* Define to 1 if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define to 1 if you have the <mntent.h> header file. */
/* #undef HAVE_MNTENT_H */

/* Define to 1 if the system has the type `mode_t'. */
#define HAVE_MODE_T 1

/* Define to 1 if you have the `mousemask' function. */
/* #undef HAVE_MOUSEMASK */

/* Define to 1 if you have the <mpg123.h> header file. */
#define HAVE_MPG123_H 1

/* Define if you have NAS including devel headers */
/* #undef HAVE_NAS */

/* Define to 1 if you have the <ncurses.h> header file. */
/* #undef HAVE_NCURSES_H */

/* Define to 1 if you have the <netdb.h> header file. */
/* #undef HAVE_NETDB_H */

/* Define to 1 if you have the <netinet/icmp_var.h> header file. */
/* #undef HAVE_NETINET_ICMP_VAR_H */

/* Define to 1 if you have the <netinet/if_ether.h> header file. */
/* #undef HAVE_NETINET_IF_ETHER_H */

/* Define to 1 if you have the <netinet/if_inarp.h> header file. */
/* #undef HAVE_NETINET_IF_INARP_H */

/* Define to 1 if you have the <netinet/in.h> header file. */
/* #undef HAVE_NETINET_IN_H */

/* Define to 1 if you have the <netinet/in_pcb.h> header file. */
/* #undef HAVE_NETINET_IN_PCB_H */

/* Define to 1 if you have the <netinet/in_systm.h> header file. */
/* #undef HAVE_NETINET_IN_SYSTM_H */

/* Define to 1 if you have the <netinet/ip.h> header file. */
/* #undef HAVE_NETINET_IP_H */

/* Define to 1 if you have the <netinet/ip_icmp.h> header file. */
/* #undef HAVE_NETINET_IP_ICMP_H */

/* Define to 1 if you have the <netinet/ip_var.h> header file. */
/* #undef HAVE_NETINET_IP_VAR_H */

/* Define to 1 if you have the <netinet/tcp_fsm.h> header file. */
/* #undef HAVE_NETINET_TCP_FSM_H */

/* Define to 1 if you have the <netinet/tcp.h> header file. */
/* #undef HAVE_NETINET_TCP_H */

/* Define to 1 if you have the <netinet/tcp_timer.h> header file. */
/* #undef HAVE_NETINET_TCP_TIMER_H */

/* Define to 1 if you have the <netinet/tcp_var.h> header file. */
/* #undef HAVE_NETINET_TCP_VAR_H */

/* Define to 1 if you have the <netinet/udp.h> header file. */
/* #undef HAVE_NETINET_UDP_H */

/* Define to 1 if you have the <netinet/udp_var.h> header file. */
/* #undef HAVE_NETINET_UDP_VAR_H */

/* Define to 1 if you have the <netipx/ipx.h> header file. */
/* #undef HAVE_NETIPX_IPX_H */

/* Define to 1 if you have the <net/if_arp.h> header file. */
/* #undef HAVE_NET_IF_ARP_H */

/* Define to 1 if you have the <net/if_dl.h> header file. */
/* #undef HAVE_NET_IF_DL_H */

/* Define to 1 if you have the <net/if.h> header file. */
/* #undef HAVE_NET_IF_H */

/* Define to 1 if you have the <net/if_types.h> header file. */
/* #undef HAVE_NET_IF_TYPES_H */

/* Define to 1 if you have the <net/route.h> header file. */
/* #undef HAVE_NET_ROUTE_H */

/* Define to 1 if `_msg_ptr' is a member of `ns_msg'. */
/* #undef HAVE_NS_MSG__MSG_PTR */

/* Define to 1 if the system has the type `off_t'. */
#define HAVE_OFF_T 1

/* Define if mkdir takes only one argument */
#define HAVE_ONE_ARG_MKDIR 1

/* Define to 1 if OpenAL is available */
/* #undef HAVE_OPENAL */

/* Define to 1 if you have the <OpenAL/al.h> header file. */
/*#undef HAVE_OPENAL_AL_H */

/* Define if OpenGL is present on the system */
/* #undef HAVE_OPENGL */

/* Define to 1 if you have the <openssl/err.h> header file. */
/* #define HAVE_OPENSSL_ERR_H 1 */

/* Define to 1 if you have the <openssl/ssl.h> header file. */
/* #undef HAVE_OPENSSL_SSL_H */

/* Define to 1 if the system has the type `oss_sysinfo'. */
/* #undef HAVE_OSS_SYSINFO */

/* Define to 1 if you have the `pclose' function. */
#define HAVE_PCLOSE 1

/* Define to 1 if the system has the type `pid_t'. */
#define HAVE_PID_T 1

/* Define to 1 if you have the `pipe2' function. */
/* #undef HAVE_PIPE2 */

/* Define to 1 if you have the <png.h> header file. */
#define HAVE_PNG_H 1

/* Define to 1 if libpng has the png_set_expand_gray_1_2_4_to_8 function. */
#define HAVE_PNG_SET_EXPAND_GRAY_1_2_4_TO_8 1

/* Define to 1 if you have the `poll' function. */
/* #undef HAVE_POLL */

/* Define to 1 if you have the <poll.h> header file. */
/* #undef HAVE_POLL_H */

/* Define to 1 if you have the `popen' function. */
#define HAVE_POPEN 1

/* Define to 1 if you have the `port_create' function. */
/* #undef HAVE_PORT_CREATE */

/* Define to 1 if you have the <port.h> header file. */
/* #undef HAVE_PORT_H */

/* Define if we can use ppdev.h for parallel port access */
/* #undef HAVE_PPDEV */

/* Define to 1 if you have the `prctl' function. */
/* #undef HAVE_PRCTL */

/* Define to 1 if you have the `pread' function. */
/* #undef HAVE_PREAD */

/* Define to 1 if you have the <process.h> header file. */
#define HAVE_PROCESS_H 1

/* Define to 1 if you have the `pthread_attr_get_np' function. */
/* #undef HAVE_PTHREAD_ATTR_GET_NP */

/* Define to 1 if you have the `pthread_getattr_np' function. */
/* #undef HAVE_PTHREAD_GETATTR_NP */

/* Define to 1 if you have the `pthread_get_stackaddr_np' function. */
/* #undef HAVE_PTHREAD_GET_STACKADDR_NP */

/* Define to 1 if you have the `pthread_get_stacksize_np' function. */
/* #undef HAVE_PTHREAD_GET_STACKSIZE_NP */

/* Define to 1 if you have the <pthread.h> header file. */
/* #undef HAVE_PTHREAD_H */

/* Define to 1 if you have the <pthread_np.h> header file. */
/* #undef HAVE_PTHREAD_NP_H */

/* Define to 1 if you have the <pwd.h> header file. */
/* #undef HAVE_PWD_H */

/* Define to 1 if you have the `pwrite' function. */
/* #undef HAVE_PWRITE */

/* Define to 1 if you have the <QuickTime/ImageCompression.h> header file. */
/* #undef HAVE_QUICKTIME_IMAGECOMPRESSION_H */

/* Define to 1 if you have the `readdir' function. */
/* #undef HAVE_READDIR */

/* Define to 1 if you have the `readlink' function. */
/* #undef HAVE_READLINK */

/* Define to 1 if you have the <regex.h> header file. */
#define HAVE_REGEX_H 1

/* Define to 1 if the system has the type `request_sense'. */
/* #undef HAVE_REQUEST_SENSE */

/* Define if you have the resolver library and header */
/* #undef HAVE_RESOLV */

/* Define to 1 if you have the <resolv.h> header file. */
/* #undef HAVE_RESOLV_H */

/* Define to 1 if you have the <sched.h> header file. */
/* #undef HAVE_SCHED_H */

/* Define to 1 if you have the `sched_setaffinity' function. */
/* #undef HAVE_SCHED_SETAFFINITY */

/* Define to 1 if you have the `sched_yield' function. */
/* #undef HAVE_SCHED_YIELD */

/* Define to 1 if `cmd' is a member of `scsireq_t'. */
/* #undef HAVE_SCSIREQ_T_CMD */

/* Define to 1 if you have the <scsi/scsi.h> header file. */
/* #undef HAVE_SCSI_SCSI_H */

/* Define to 1 if you have the <scsi/scsi_ioctl.h> header file. */
/* #undef HAVE_SCSI_SCSI_IOCTL_H */

/* Define to 1 if you have the <scsi/sg.h> header file. */
/* #undef HAVE_SCSI_SG_H */

/* Define to 1 if you have the <Security/Security.h> header file. */
/* #undef HAVE_SECURITY_SECURITY_H */

/* Define to 1 if you have the `select' function. */
#define HAVE_SELECT 1

/* Define to 1 if you have the `sendmsg' function. */
/* #undef HAVE_SENDMSG */

/* Define to 1 if you have the `setproctitle' function. */
/* #undef HAVE_SETPROCTITLE */

/* Define to 1 if you have the `setrlimit' function. */
/* #undef HAVE_SETRLIMIT */

/* Define to 1 if you have the `settimeofday' function. */
/* #undef HAVE_SETTIMEOFDAY */

/* Define to 1 if `interface_id' is member of `sg_io_hdr_t'. */
/* #undef HAVE_SG_IO_HDR_T_INTERFACE_ID */

/* Define if sigaddset is supported */
/* #undef HAVE_SIGADDSET */

/* Define to 1 if you have the `sigaltstack' function. */
/* #undef HAVE_SIGALTSTACK */

/* Define to 1 if `si_fd' is member of `siginfo_t'. */
/* #undef HAVE_SIGINFO_T_SI_FD */

/* Define to 1 if you have the `sigprocmask' function. */
/* #undef HAVE_SIGPROCMASK */

/* Define to 1 if the system has the type `sigset_t'. */
/* #undef HAVE_SIGSET_T */

/* Define to 1 if the system has the type `size_t'. */
#define HAVE_SIZE_T 1

/* Define to 1 if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define to 1 if you have the `socketpair' function. */
/* #undef HAVE_SOCKETPAIR */

/* Define to 1 if you have the <soundcard.h> header file. */
/* #undef HAVE_SOUNDCARD_H */

/* Define to 1 if you have the `spawnvp' function. */
#define HAVE_SPAWNVP 1

/* Define to 1 if the system has the type `ssize_t'. */
#define HAVE_SSIZE_T 1

/* Define to 1 if you have the `statfs' function. */
/* #undef HAVE_STATFS */

/* Define to 1 if you have the `statvfs' function. */
/* #undef HAVE_STATVFS */

/* Define to 1 if you have the <stdbool.h> header file. */
/* #undef HAVE_STDBOOL_H */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the `strcasecmp' function. */
#if !defined(_WIN32) && !defined(_WIN64)
#define HAVE_STRCASECMP 1
#endif

/* Define to 1 if you have the `strdup' function. */
/* #undef HAVE_STRDUP */

/* Define to 1 if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strncasecmp' function. */
#if !defined(_WIN32) && !defined(_WIN64)
#define HAVE_STRNCASECMP 1
#endif

/* Define to 1 if you have the <stropts.h> header file. */
/* #undef HAVE_STROPTS_H */

/* Define to 1 if you have the `strtold' function. */
/* #undef HAVE_STRTOLD */

/* Define to 1 if you have the `strtoll' function. */
/* #undef HAVE_STRTOLL */

/* Define to 1 if you have the `strtoull' function. */
/* #undef HAVE_STRTOULL */

/* Define to 1 if `direction' is a member of `struct ff_effect'. */
/* #undef HAVE_STRUCT_FF_EFFECT_DIRECTION */

/* Define to 1 if `icps_outhist' is a member of `struct icmpstat'. */
/* #undef HAVE_STRUCT_ICMPSTAT_ICPS_OUTHIST */

/* Define to 1 if `ifr_hwaddr' is a member of `struct ifreq'. */
/* #undef HAVE_STRUCT_IFREQ_IFR_HWADDR */

/* Define to 1 if `msg_accrights' is a member of `struct msghdr'. */
/* #undef HAVE_STRUCT_MSGHDR_MSG_ACCRIGHTS */

/* Define to 1 if `mt_blkno' is a member of `struct mtget'. */
/* #undef HAVE_STRUCT_MTGET_MT_BLKNO */

/* Define to 1 if `mt_blksiz' is a member of `struct mtget'. */
/* #undef HAVE_STRUCT_MTGET_MT_BLKSIZ */

/* Define to 1 if `mt_gstat' is a member of `struct mtget'. */
/* #undef HAVE_STRUCT_MTGET_MT_GSTAT */

/* Define to 1 if `name' is a member of `struct option'. */
#define HAVE_STRUCT_OPTION_NAME 1

/* Define to 1 if `sin6_scope_id' is a member of `struct sockaddr_in6'. */
/* #undef HAVE_STRUCT_SOCKADDR_IN6_SIN6_SCOPE_ID */

/* Define to 1 if `sa_len' is a member of `struct sockaddr'. */
/* #undef HAVE_STRUCT_SOCKADDR_SA_LEN */

/* Define to 1 if `sun_len' is a member of `struct sockaddr_un'. */
/* #undef HAVE_STRUCT_SOCKADDR_UN_SUN_LEN */

/* Define to 1 if `f_bavail' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_BAVAIL */

/* Define to 1 if `f_bfree' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_BFREE */

/* Define to 1 if `f_favail' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_FAVAIL */

/* Define to 1 if `f_ffree' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_FFREE */

/* Define to 1 if `f_frsize' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_FRSIZE */

/* Define to 1 if `f_namelen' is a member of `struct statfs'. */
/* #undef HAVE_STRUCT_STATFS_F_NAMELEN */

/* Define to 1 if `f_blocks' is a member of `struct statvfs'. */
/* #undef HAVE_STRUCT_STATVFS_F_BLOCKS */

/* Define to 1 if `st_atim' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_ATIM */

/* Define to 1 if `st_blocks' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BLOCKS */

/* Define to 1 if `st_ctim' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_CTIM */

/* Define to 1 if `st_mtim' is a member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_MTIM */

/* Define to 1 if the system has the type `struct xinpgen'. */
/* #undef HAVE_STRUCT_XINPGEN */

/* Define to 1 if you have the `symlink' function. */
/* #undef HAVE_SYMLINK */

/* Define to 1 if you have the <syscall.h> header file. */
/* #undef HAVE_SYSCALL_H */

/* Define to 1 if you have the <sys/asoundlib.h> header file. */
/* #undef HAVE_SYS_ASOUNDLIB_H */

/* Define to 1 if you have the <sys/cdio.h> header file. */
/* #undef HAVE_SYS_CDIO_H */

/* Define to 1 if you have the <sys/elf32.h> header file. */
/* #undef HAVE_SYS_ELF32_H */

/* Define to 1 if you have the <sys/epoll.h> header file. */
/* #undef HAVE_SYS_EPOLL_H */

/* Define to 1 if you have the <sys/errno.h> header file. */
/* #undef HAVE_SYS_ERRNO_H */

/* Define to 1 if you have the <sys/event.h> header file. */
/* #undef HAVE_SYS_EVENT_H */

/* Define to 1 if you have the <sys/exec_elf.h> header file. */
/* #undef HAVE_SYS_EXEC_ELF_H */

/* Define to 1 if you have the <sys/filio.h> header file. */
/* #undef HAVE_SYS_FILIO_H */

/* Define to 1 if you have the <sys/inotify.h> header file. */
/* #undef HAVE_SYS_INOTIFY_H */

/* Define to 1 if you have the <sys/ioctl.h> header file. */
/* #undef HAVE_SYS_IOCTL_H */

/* Define to 1 if you have the <sys/ipc.h> header file. */
/* #undef HAVE_SYS_IPC_H */

/* Define to 1 if you have the <sys/limits.h> header file. */
/* #undef HAVE_SYS_LIMITS_H */

/* Define to 1 if you have the <sys/link.h> header file. */
/* #undef HAVE_SYS_LINK_H */

/* Define to 1 if you have the <sys/mman.h> header file. */
/* #undef HAVE_SYS_MMAN_H */

/* Define to 1 if you have the <sys/modem.h> header file. */
/* #undef HAVE_SYS_MODEM_H */

/* Define to 1 if you have the <sys/mount.h> header file. */
/* #undef HAVE_SYS_MOUNT_H */

/* Define to 1 if you have the <sys/msg.h> header file. */
/* #undef HAVE_SYS_MSG_H */

/* Define to 1 if you have the <sys/mtio.h> header file. */
/* #undef HAVE_SYS_MTIO_H */

/* Define to 1 if you have the <sys/param.h> header file. */
#define HAVE_SYS_PARAM_H 1

/* Define to 1 if you have the <sys/poll.h> header file. */
/* #undef HAVE_SYS_POLL_H */

/* Define to 1 if you have the <sys/prctl.h> header file. */
/* #undef HAVE_SYS_PRCTL_H */

/* Define to 1 if you have the <sys/protosw.h> header file. */
/* #undef HAVE_SYS_PROTOSW_H */

/* Define to 1 if you have the <sys/ptrace.h> header file. */
/* #undef HAVE_SYS_PTRACE_H */

/* Define to 1 if you have the <sys/resource.h> header file. */
/* #undef HAVE_SYS_REG_H */

/* Define to 1 if you have the <sys/scsiio.h> header file. */
/* #undef HAVE_SYS_SCSIIO_H */

/* Define to 1 if you have the <sys/shm.h> header file. */
/* #undef HAVE_SYS_SHM_H */

/* Define to 1 if you have the <sys/signal.h> header file. */
/* #undef HAVE_SYS_SIGNAL_H */

/* Define to 1 if you have the <sys/socketvar.h> header file. */
/* #undef HAVE_SYS_SOCKETVAR_H */

/* Define to 1 if you have the <sys/socket.h> header file. */
/* #undef HAVE_SYS_SOCKET_H */

/* Define to 1 if you have the <sys/sockio.h> header file. */
/* #undef HAVE_SYS_SOCKIO_H */

/* Define to 1 if you have the <sys/soundcard.h> header file. */
/* #undef HAVE_SYS_SOUNDCARD_H */

/* Define to 1 if you have the <sys/statfs.h> header file. */
/* #undef HAVE_SYS_STATFS_H */

/* Define to 1 if you have the <sys/statvfs.h> header file. */
/* #undef HAVE_SYS_STATVFS_H */

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/strtio.h> header file. */
/* #undef HAVE_SYS_STRTIO_H */

/* Define to 1 if you have the <sys/syscall.h> header file. */
/* #undef HAVE_SYS_SYSCALL_H */

/* Define to 1 if you have the <sys/sysctl.h> header file. */
/* #undef HAVE_SYS_SYSCTL_H */

/* Define to 1 if you have the <sys/thr.h> header file. */
/* #undef HAVE_SYS_THR_H */

/* Define to 1 if you have the <sys/tihdr.h> header file. */
/* #undef HAVE_SYS_TIHDR_H */

/* Define to 1 if you have the <sys/timeout.h> header file. */
/* #undef HAVE_SYS_TIMEOUT_H */

/* Define to 1 if you have the <sys/times.h> header file. */
/* #undef HAVE_SYS_TIMES_H */

/* Define to 1 if you have the <sys/time.h> header file. */
#define HAVE_SYS_TIME_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <sys/uio.h> header file. */
/* #undef HAVE_SYS_UIO_H */

/* Define to 1 if you have the <sys/un.h> header file. */
/* #undef HAVE_SYS_UN_H */

/* Define to 1 if you have the <sys/user.h> header file. */
/* #undef HAVE_SYS_USER_H */

/* Define to 1 if you have the <sys/utsname.h> header file. */
/* #undef HAVE_SYS_UTSNAME_H */

/* Define to 1 if you have the <sys/vfs.h> header file. */
/* #undef HAVE_SYS_VFS_H */

/* Define to 1 if you have the <sys/vm86.h> header file. */
/* #undef HAVE_SYS_VM86_H */

/* Define to 1 if you have the <sys/wait.h> header file. */
/* #undef HAVE_SYS_WAIT_H */

/* Define to 1 if you have the `tcgetattr' function. */
/* #undef HAVE_TCGETATTR */

/* Define to 1 if you have the <termios.h> header file. */
/* #undef HAVE_TERMIOS_H */

/* Define to 1 if you have the `thr_kill2' function. */
/* #undef HAVE_THR_KILL2 */

/* Define to 1 if you have the <tiffio.h> header file. */
#define HAVE_TIFFIO_H 1

/* Define to 1 if you have the `timegm' function. */
/* #undef HAVE_TIMEGM */

/* Define if you have the timezone variable */
#define HAVE_TIMEZONE 1

/* Define to 1 if you have the <ucontext.h> header file. */
/* #undef HAVE_UCONTEXT_H */

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the `usleep' function. */
/* #undef HAVE_USLEEP */

/* Define to 1 if you have the <utime.h> header file. */
#define HAVE_UTIME_H 1

/* Define to 1 if you have the <valgrind/memcheck.h> header file. */
/* #undef HAVE_VALGRIND_MEMCHECK_H */

/* Define to 1 if you have the <valgrind/valgrind.h> header file. */
/* #undef HAVE_VALGRIND_VALGRIND_H */

/* Define to 1 if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define to 1 if you have the `wait4' function. */
/* #undef HAVE_WAIT4 */

/* Define to 1 if you have the `waitpid' function. */
/* #undef HAVE_WAITPID */

/* Define to 1 if you have the <X11/extensions/shape.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_SHAPE_H */

/* Define to 1 if you have the <X11/extensions/Xcomposite.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86DGA_H */

/* Define to 1 if you have the <X11/extensions/xf86vmode.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86VMODE_H */

/* Define to 1 if you have the <X11/extensions/xf86vmproto.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86VMPROTO_H */

/* Define to 1 if you have the <X11/extensions/Xinerama.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XINERAMA_H */

/* Define to 1 if you have the <X11/extensions/XInput.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XINPUT_H */

/* Define to 1 if you have the <X11/extensions/Xrandr.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XRANDR_H */

/* Define to 1 if you have the <X11/extensions/Xrender.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XRENDER_H */

/* Define to 1 if you have the <X11/extensions/XShm.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XSHM_H */

/* Define to 1 if you have the <X11/Xcursor/Xcursor.h> header file. */
/* #undef HAVE_X11_XCURSOR_XCURSOR_H */

/* Define to 1 if you have the <X11/XKBlib.h> header file. */
/* #undef HAVE_X11_XKBLIB_H */

/* Define to 1 if you have the <X11/Xlib.h> header file. */
/* #undef HAVE_X11_XLIB_H */

/* Define to 1 if you have the <X11/Xutil.h> header file. */
/* #undef HAVE_X11_XUTIL_H */

/* Define to 1 if `callback' is a member of `XICCallback'. */
/* #undef HAVE_XICCALLBACK_CALLBACK */

/* Define if you have the XKB extension */
/* #undef HAVE_XKB */

/* Define if libxml2 has the xmlNewDocPI function */
#define HAVE_XMLNEWDOCPI 1

/* Define if libxml2 has the xmlReadMemory function */
#define HAVE_XMLREADMEMORY 1

/* Define if Xrender has the XRenderSetPictureTransform function */
/* #undef HAVE_XRENDERSETPICTURETRANSFORM */

/* Define to 1 if you have the `z' library (-lz). */
#define HAVE_ZLIB 1

/* Define to 1 if you have the <zlib.h> header file. */
#define HAVE_ZLIB_H 1

/* Define to 1 if you have the `_finite' function. */
#define HAVE__FINITE 1

/* Define to 1 if you have the `_isnan' function. */
#define HAVE__ISNAN 1

/* Define to 1 if you have the `_pclose' function. */
#define HAVE__PCLOSE 1

/* Define to 1 if you have the `_popen' function. */
#define HAVE__POPEN 1

/* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF 1

/* Define to 1 if you have the `_spawnvp' function. */
#define HAVE__SPAWNVP 1

/* Define to 1 if you have the `_strdup' function. */
#define HAVE__STRDUP 1

/* Define to 1 if you have the `_stricmp' function. */
#if defined(_WIN32) || defined(_WIN64)
#define HAVE__STRICMP 1
#endif

/* Define to 1 if you have the `_strnicmp' function. */
#if defined(_WIN32) || defined(_WIN64)
#define HAVE__STRNICMP 1
#endif

/* Define to 1 if you have the `_strtoi64' function. */
#define HAVE__STRTOI64 1

/* Define to 1 if you have the `_strtoui64' function. */
#define  HAVE__STRTOUI64 1

/* Define to 1 if you have the `_vsnprintf' function. */
#define HAVE__VSNPRINTF 1

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "ros-dev@reactos.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "ReactOS"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING PACKAGE_NAME " " PACKAGE_VERSION

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "ReactOS"

/* Define to the home page for this package. */
#define PACKAGE_URL "http://www.reactos.org/"

/* Define to the version of this package. */
#define PACKAGE_VERSION KERNEL_VERSION_STR

/* Define to the soname of the libcapi20 library. */
/* #undef SONAME_LIBCAPI20 */

/* Define to the soname of the libcrypto library. */
/* #undef SONAME_LIBCRYPTO */

/* Define to the soname of the libcups library. */
/* #undef SONAME_LIBCUPS */

/* Define to the soname of the libcurses library. */
/* #undef SONAME_LIBCURSES */

/* Define to the soname of the libfontconfig library. */
/* #undef SONAME_LIBFONTCONFIG */

/* Define to the soname of the libfreetype library. */
/* #undef SONAME_LIBFREETYPE */

/* Define to the soname of the libGL library. */
/* #undef SONAME_LIBGL */

/* Define to the soname of the libGLU library. */
/* #undef SONAME_LIBGLU */

/* Define to the soname of the libgnutls library. */
/* #undef SONAME_LIBGNUTLS */

/* Define to the soname of the libgsm library. */
/* #undef SONAME_LIBGSM */

/* Define to the soname of the libhal library. */
/* #undef SONAME_LIBHAL */

/* Define to the soname of the libjack library. */
/* #undef SONAME_LIBJACK */

/* Define to the soname of the libjpeg library. */
#define SONAME_LIBJPEG "libjpeg"

/* Define to the soname of the libmbedtls library. */
#define SONAME_LIBMBEDTLS L"mbedtls"

/* Define to the soname of the libncurses library. */
/* #undef SONAME_LIBNCURSES */

/* Define to the soname of the libodbc library. */
/* #undef SONAME_LIBODBC */

/* Define to the soname of the libopenal library. */
/* #undef SONAME_LIBOPENAL */

/* Define to the soname of the libpng library. */
#define SONAME_LIBPNG "libpng"

/* Define to the soname of the libsane library. */
/* #undef SONAME_LIBSANE */

/* Define to the soname of the libssl library. */
/* #undef SONAME_LIBSSL */

/* Define to the soname of the libtiff library. */
#define SONAME_LIBTIFF "libtiff"

/* Define to the soname of the libtxc_dxtn library. */
#define SONAME_LIBTXC_DXTN "dxtn"

/* Define to the soname of the libv4l1 library. */
/* #undef SONAME_LIBV4L1 */

/* Define to the soname of the libX11 library. */
/* #undef SONAME_LIBX11 */

/* Define to the soname of the libXcomposite library. */
/* #undef SONAME_LIBXCOMPOSITE */

/* Define to the soname of the libXcursor library. */
/* #undef SONAME_LIBXCURSOR */

/* Define to the soname of the libXext library. */
/* #undef SONAME_LIBXEXT */

/* Define to the soname of the libXi library. */
/* #undef SONAME_LIBXI */

/* Define to the soname of the libXinerama library. */
/* #undef SONAME_LIBXINERAMA */

/* Define to the soname of the libXrandr library. */
/* #undef SONAME_LIBXRANDR */

/* Define to the soname of the libXrender library. */
/* #undef SONAME_LIBXRENDER */

/* Define to the soname of the libxslt library. */
#define SONAME_LIBXSLT "libxslt"

/* Define to the soname of the libXxf86vm library. */
/* #undef SONAME_LIBXXF86VM */

/* Define to 1 if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to 1 if the X Window System is missing or not being used. */
#define X_DISPLAY_MISSING 1

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to a macro to output a .cfi assembly pseudo-op */
#define __ASM_CFI(str) str

/* Define to a macro to define an assembly function */
#ifndef _MSC_VER
#ifndef NO_UNDERSCORE_PREFIX
#define __ASM_DEFINE_FUNC(name,suffix,code) asm(".text\n\t.align 4\n\t.globl _" #name suffix "\n\t.def _" #name suffix "; .scl 2; .type 32; .endef\n_" #name suffix ":\n\t.cfi_startproc\n\t" code "\n\t.cfi_endproc");
#else
#define __ASM_DEFINE_FUNC(name,suffix,code) asm(".text\n\t.align 4\n\t.globl " #name suffix "\n\t.def " #name suffix "; .scl 2; .type 32; .endef\n" #name suffix ":\n\t.cfi_startproc\n\t" code "\n\t.cfi_endproc");
#endif
#else
#define __ASM_DEFINE_FUNC(name,suffix,code)
#endif

/* Define to a macro to generate an assembly function directive */
#define __ASM_FUNC(name) ".def " __ASM_NAME(name) "; .scl 2; .type 32; .endef"

/* Define to a macro to generate an assembly function with C calling
   convention */
#define __ASM_GLOBAL_FUNC(name,code) __ASM_DEFINE_FUNC(name,"",code)

/* Define to a macro to generate an assembly name from a C symbol */
#ifndef NO_UNDERSCORE_PREFIX
#define __ASM_NAME(name) "_" name
#else
#define __ASM_NAME(name) name
#endif

/* Define to a macro to generate an stdcall suffix */
#define __ASM_STDCALL(args) "@" #args

/* Define to a macro to generate an assembly function with stdcall calling
   convention */
#ifndef _MSC_VER
#define __ASM_STDCALL_FUNC(name,args,code) __ASM_DEFINE_FUNC(name,__ASM_STDCALL(args),code)
#endif

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `__inline__' or `__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

#if defined(_MSC_VER)
#define inline __inline
#endif
