#if !defined(SHFMT_OPT_FULL)

   #if defined (__cplusplus)
   extern "C" {
   #endif

   /*****************************************************************
   The SHFormatDrive API provides access to the Shell's format
   dialog box. This allows applications that want to format disks to bring
   up the same dialog box that the Shell uses for disk formatting.

   PARAMETERS
      hwnd    = The window handle of the window that will own the
                dialog. NOTE that hwnd == NULL does not cause this
                dialog to come up as a "top level application"
                window. This parameter should always be non-null,
                this dialog box is only designed to be the child of
                another window, not a stand-alone application.

      drive   = The 0 based (A: == 0) drive number of the drive
                to format.

      fmtID   = Currently must be set to SHFMT_ID_DEFAULT.

      options = There are currently only two option bits defined.

                   SHFMT_OPT_FULL
                   SHFMT_OPT_SYSONLY

                SHFMT_OPT_FULL specifies that the "Quick Format"
                setting should be cleared by default. If the user
                leaves the "Quick Format" setting cleared, then a
                full format will be applied (this is useful for
                users that detect "unformatted" disks and want
                to bring up the format dialog box).

                If options is set to zero (0), then the "Quick Format"
                setting is set by default. In addition, if the user leaves
                it set, a quick format is performed. Under Windows NT 4.0,
                this flag is ignored and the "Quick Format" box is always
                checked when the dialog box first appears. The user can
                still change it. This is by design.

                The SHFMT_OPT_SYSONLY initializes the dialog to
                default to just sys the disk.

                All other bits are reserved for future expansion
                and must be 0.

                Please note that this is a bit field and not a
                value, treat it accordingly.

      RETURN
         The return is either one of the SHFMT_* values, or if
         the returned DWORD value is not == to one of these
         values, then the return is the physical format ID of the
         last successful format. The LOWORD of this value can be
         passed on subsequent calls as the fmtID parameter to
         "format the same type you did last time".

   *****************************************************************/ 
   DWORD WINAPI SHFormatDrive(HWND hwnd,
                              UINT drive,
                              UINT fmtID,
                              UINT options);

   // 
   // Special value of fmtID which means "use the defaultformat"
   // 

   #define SHFMT_ID_DEFAULT   0xFFFF

   // 
   // Option bits for options parameter
   // 

   #define SHFMT_OPT_FULL     0x0001
   #define SHFMT_OPT_SYSONLY  0x0002

   // 
   // Special return values. PLEASE NOTE that these are DWORD values.
   // 

   #define SHFMT_ERROR     0xFFFFFFFFL    // Error on last format,
                                          // drive may be formatable
   #define SHFMT_CANCEL    0xFFFFFFFEL    // Last format wascanceled
   #define SHFMT_NOFORMAT  0xFFFFFFFDL    // Drive is not formatable

   #if defined (__cplusplus)
   }
   #endif
   #endif