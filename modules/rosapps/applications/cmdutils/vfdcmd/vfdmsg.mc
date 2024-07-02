;/*
;   vfdmsg.h
;
;   Virtual Floppy Drive for Windows
;   Driver control program (console version)
;   Message definition
;
;   Copyright (C) 2003-2005 Ken Kato
;*/
;
;#ifndef _VFDMSG_H_
;#define _VFDMSG_H_
;
;/*
; __REACTOS__:
; - Removed Japanese language.
; + Added a second blank line between some entries.
;*/

MessageIdTypedef=DWORD
LanguageNames=(English=0x409:MSG0409)

;//==============================================
;// Generic error messages
;//==============================================
;

MessageId=
SymbolicName=MSG_WRONG_PLATFORM
Language=English
Virtual Floppy Drive does not run on Windows 95/98/Me.
.


MessageId=
SymbolicName=MSG_TOO_MANY_ARGS
Language=English
Too many command line parameters.
.


MessageId=
SymbolicName=MSG_UNKNOWN_COMMAND
Language=English
Command '%1!s!' is unknown.
.


MessageId=
SymbolicName=MSG_AMBIGUOUS_COMMAND
Language=English
Command '%1!s!' is ambiguous.
.


MessageId=
SymbolicName=MSG_UNKNOWN_OPTION
Language=English
Option '%1!s!' is unknown.
.


MessageId=
SymbolicName=MSG_DUPLICATE_ARGS
Language=English
Parameter %1!s! is used more than once.
.


;//==============================================
;// Command result message
;//==============================================
;

MessageId=
SymbolicName=MSG_INSTALL_OK
Language=English
Installed the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_INSTALL_NG
Language=English
Failed to install the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_CONFIG_OK
Language=English
Configured the Virtual Floppy driver start method.
.


MessageId=
SymbolicName=MSG_CONFIG_NG
Language=English
Failed to configure the Virtual Floppy driver start method.
.


MessageId=
SymbolicName=MSG_REMOVE_OK
Language=English
Uninstalled the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_REMOVE_NG
Language=English
Failed to uninstall the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_REMOVE_PENDING
Language=English
The Virtual Floppy driver is going to be removed on the next system start up.
You may need to restart the system before installing the driver again.
.


MessageId=
SymbolicName=MSG_START_OK
Language=English
Started the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_START_NG
Language=English
Failed to start the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_STOP_OK
Language=English
Stopped the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_STOP_NG
Language=English
Failed to stop the Virtual Floppy driver.
.


MessageId=
SymbolicName=MSG_STOP_PENDING
Language=English
Stop operation has succeeded, but something
is preventing the driver from actually stopping.
You may need to reboot the system before restarting the driver.
.


MessageId=
SymbolicName=MSG_GET_SHELLEXT_NG
Language=English
Failed to get the shell extension status.
.


MessageId=
SymbolicName=MSG_SET_SHELLEXT_NG
Language=English
Failed to set the shell extension status.
.


MessageId=
SymbolicName=MSG_SHELLEXT_ENABLED
Language=English
Shell extension is enabled.
.


MessageId=
SymbolicName=MSG_SHELLEXT_DISABLED
Language=English
Shell extension is disabled.
.


MessageId=
SymbolicName=MSG_OPEN_NG
Language=English
Failed to open the image '%1!s!'.
.


MessageId=
SymbolicName=MSG_CLOSE_OK
Language=English
Closed the image on the drive %1!c!.
.


MessageId=
SymbolicName=MSG_CLOSE_NG
Language=English
Failed to close the image on the drive %1!c!.
.


MessageId=
SymbolicName=MSG_SAVE_OK
Language=English
Saved the image on the drive %1!c! into '%2!s!'.
.


MessageId=
SymbolicName=MSG_SAVE_NG
Language=English
Failed to save the image on the drive %1!c! into '%2!s!'.
.


MessageId=
SymbolicName=MSG_PROTECT_NG
Language=English
Failed to set write protect state on the drive %1!c!.
.


MessageId=
SymbolicName=MSG_FORMAT_OK
Language=English
Format complete.
.


MessageId=
SymbolicName=MSG_FORMAT_NG
Language=English
Failed to format the drive %1!c!.
.


MessageId=
SymbolicName=MSG_LINK_NG
Language=English
Failed to assign '%2!c!' to the drive %1!lu!.
.


MessageId=
SymbolicName=MSG_UNLINK_NG
Language=English
Failed to unlink the letter from the drive %1!lu!.
.


;//==============================================
;// Supplemental result message
;//==============================================
;

MessageId=
SymbolicName=MSG_GET_STAT_NG
Language=English
Failed to get the driver status.
.


MessageId=
SymbolicName=MSG_GET_CONFIG_NG
Language=English
Failed to get the driver configuration.
.


MessageId=
SymbolicName=MSG_GET_VERSION_NG
Language=English
Failed to get the driver version.
.


MessageId=
SymbolicName=MSG_WRONG_DRIVER
Language=English
A wrong driver is installed.
.


MessageId=
SymbolicName=MSG_QUERY_UPDATE
Language=English
Update now (y / n) ? %0
.


MessageId=
SymbolicName=MSG_GET_MEDIA_NG
Language=English
Failed to get the current media status.
.


MessageId=
SymbolicName=MSG_GET_FILE_NG
Language=English
Failed to get the image information.
.


MessageId=
SymbolicName=MSG_GET_LINK_NG
Language=English
Failed to get the current drive letter.
.


MessageId=
SymbolicName=MSG_LINK_FULL
Language=English
No drive letter is available.
.


MessageId=
SymbolicName=MSG_ACCESS_NG
Language=English
Failed to access the drive %1!c!.
.


MessageId=
SymbolicName=MSG_DRIVER_EXISTS
Language=English
The Virtual Floppy driver is already installed.
.


MessageId=
SymbolicName=MSG_NOT_INSTALLED
Language=English
The Virtual Floppy driver is not installed.
.


MessageId=
SymbolicName=MSG_ALREADY_RUNNING
Language=English
The Virtual Floppy driver is already running.
.


MessageId=
SymbolicName=MSG_NOT_STARTED
Language=English
The Virtual Floppy driver is not running.
.


MessageId=
SymbolicName=MSG_TARGET_NOTICE
Language=English
Using the default drive (%1!c!).
.


MessageId=
SymbolicName=MSG_CREATE_NOTICE
Language=English
Creating a new image file.
.


MessageId=
SymbolicName=MSG_CREATE_CONFIRM
Language=English
Create a new image file (Y:yes / N:no) ? %0
.


MessageId=
SymbolicName=MSG_OVERWRITE_NOTICE
Language=English
Overwriting the existing file.
.


MessageId=
SymbolicName=MSG_OVERWRITE_CONFIRM
Language=English
Overwrite the existing file (Y:yes / N:no) ? %0
.


MessageId=
SymbolicName=MSG_CREATE_NG
Language=English
Failed to create the new image file '%1!s!'.
.


MessageId=
SymbolicName=MSG_FILE_CREATED
Language=English
Created a new image file.
.


MessageId=
SymbolicName=MSG_RAM_MEDIA_UNKNOWN
Language=English
A size is not specified for a new RAM image.
.


MessageId=
SymbolicName=MSG_FILE_MEDIA_UNKNOWN
Language=English
A size is not specified for a new image file.
.


MessageId=
SymbolicName=MSG_CREATE144_NOTICE
Language=English
Creating a 1.44MB image.
.


MessageId=
SymbolicName=MSG_CREATE144_CONFIRM
Language=English
Create a 1.44MB image (Y:yes / N:no) ? %0
.


MessageId=
SymbolicName=MSG_IMAGE_TOO_SMALL
Language=English
The image is too small.
.


MessageId=
SymbolicName=MSG_NO_MATCHING_MEDIA
Language=English
The image size (%1!lu! bytes) does not match any supported media.
.


MessageId=
SymbolicName=MSG_MEDIATYPE_NOTICE
Language=English
Opening as a %1!s! media (%2!lu! bytes).
.


MessageId=
SymbolicName=MSG_MEDIATYPE_SUGGEST
Language=English
The largest possible media is %1!s! (%2!lu! bytes).
.


MessageId=
SymbolicName=MSG_MEDIATYPE_CONFIRM
Language=English
Open as this media type (Y:yes / N:no) ? %0
.


MessageId=
SymbolicName=MSG_RAM_MODE_NOTICE
Language=English
Opening the image in RAM mode.
.


MessageId=
SymbolicName=MSG_RAM_MODE_ONLY
Language=English
This file must be opened in RAM mode.
.


MessageId=
SymbolicName=MSG_RAM_MODE_CONFIRM
Language=English
Open in RAM mode (Y:yes / N:no) ? %0
.


MessageId=
SymbolicName=MSG_DEFAULT_PROTECT
Language=English
The media will be write protected by default.
.


MessageId=
SymbolicName=MSG_DRIVE_BUSY
Language=English
An image is already opened.
.


MessageId=
SymbolicName=MSG_TARGET_REQUIRED
Language=English
Specify a target file to save.
.


MessageId=
SymbolicName=MSG_TARGET_UP_TO_DATE
Language=English
The image file is up to date.
.


MessageId=
SymbolicName=MSG_OVERWRITE_PROMPT
Language=English
Overwrite the existing file
(O: just overwrite / T: overwrite & truncate / C: cancel) ? %0
.


MessageId=
SymbolicName=MSG_TARGET_IS_ZIP
Language=English
Cannot overwrite a ZIP compressed file.
.


MessageId=
SymbolicName=MSG_SAVE_FORCE
Language=English
The save operation is forced to continue.
.


MessageId=
SymbolicName=MSG_SAVE_QUIT
Language=English
The save operation is aborted.
.


MessageId=
SymbolicName=MSG_FORMAT_FORCE
Language=English
The format operation is forced to continue.
.


MessageId=
SymbolicName=MSG_FORMAT_QUIT
Language=English
The format operation is aborted.
.


MessageId=
SymbolicName=MSG_MEDIA_MODIFIED
Language=English
RAM disk data on the drive %1!c!: is modified.
.


MessageId=
SymbolicName=MSG_CLOSE_FORCE
Language=English
The close operation is forced to continue.
.


MessageId=
SymbolicName=MSG_CLOSE_QUIT
Language=English
The close operation is aborted.
.


MessageId=
SymbolicName=MSG_CLOSE_CONFIRM
Language=English
Close the image anyway (Y:yes / N:no) ? %0
.


MessageId=
SymbolicName=MSG_RETRY_FORCE_CANCEL
Language=English
R:retry / F:force / C:cancel ? %0
.


MessageId=
SymbolicName=MSG_RETRY_CANCEL
Language=English
R:retry / C:cancel ? %0
.


MessageId=
SymbolicName=MSG_LOCK_NG
Language=English
Failed to lock the drive %1!c!.  Some programs may be using the drive.
.


MessageId=
SymbolicName=MSG_STOP_FORCE
Language=English
Failed to close the all drives.  The operation is forced to continue.
.


MessageId=
SymbolicName=MSG_STOP_QUIT
Language=English
Failed to close the all drives.  The operation is aborted.
.


MessageId=
SymbolicName=MSG_STOP_WARN
Language=English
Failed to close the all drives.  The driver may not be able to unload
properly.  Continue the stop operation?
.


MessageId=
SymbolicName=MSG_REMOVE_FORCE
Language=English
Failed to stop the driver.  The operation is forced to continue;
.


MessageId=
SymbolicName=MSG_REMOVE_QUIT
Language=English
Failed to stop the driver.  The operation is aborted.
.


MessageId=
SymbolicName=MSG_REMOVE_WARN
Language=English
Failed to stop the driver.  The driver may not be removed completely
until the system is restarted.  Continue the operation?
.


MessageId=
SymbolicName=MSG_UNKNOWN_LONG
Language=English
Unknown (0x%1!08x!)
.


MessageId=
SymbolicName=MSG_DRIVER_FILE
Language=English
Driver     : %1!s!
.


MessageId=
SymbolicName=MSG_DRIVER_VERSION
Language=English
Version    : %1!d!.%2!d! %3!s!
.


MessageId=
SymbolicName=MSG_START_TYPE
Language=English
Start Type : %0
.


MessageId=
SymbolicName=MSG_START_AUTO
Language=English
AUTO
.


MessageId=
SymbolicName=MSG_START_BOOT
Language=English
BOOT
.


MessageId=
SymbolicName=MSG_START_DEMAND
Language=English
DEMAND
.


MessageId=
SymbolicName=MSG_START_DISABLED
Language=English
DISABLED
.


MessageId=
SymbolicName=MSG_START_SYSTEM
Language=English
SYSTEM
.


MessageId=
SymbolicName=MSG_DRIVER_STATUS
Language=English
Status     : %0
.


MessageId=
SymbolicName=MSG_STATUS_STOPPED
Language=English
STOPPED
.


MessageId=
SymbolicName=MSG_STATUS_START_P
Language=English
START_PENDING
.


MessageId=
SymbolicName=MSG_STATUS_STOP_P
Language=English
STOP_PENDING
.


MessageId=
SymbolicName=MSG_STATUS_RUNNING
Language=English
RUNNING
.


MessageId=
SymbolicName=MSG_STATUS_CONT_P
Language=English
CONTINUE_PENDING
.


MessageId=
SymbolicName=MSG_STATUS_PAUSE_P
Language=English
PAUSE_PENDING
.


MessageId=
SymbolicName=MSG_STATUS_PAUSED
Language=English
PAUSED
.


MessageId=
SymbolicName=MSG_DRIVE_LETTER
Language=English
Drive %1!lu!    : %0
.


MessageId=
SymbolicName=MSG_PERSISTENT
Language=English
%1!c! (Persistent) %0
.


MessageId=
SymbolicName=MSG_EPHEMERAL
Language=English
%1!c! (Ephemeral) %0
.


MessageId=
SymbolicName=MSG_IMAGE_NONE
Language=English
Image      : <none>
.


MessageId=
SymbolicName=MSG_IMAGE_NAME
Language=English
Image      : %1!s!
.


MessageId=
SymbolicName=MSG_FILE_DESC
Language=English
Description: %1!s!
.


MessageId=
SymbolicName=MSG_DISKTYPE_FILE
Language=English
Type       : FILE
.


MessageId=
SymbolicName=MSG_DISKTYPE_RAM_CLEAN
Language=English
Type       : RAM (not modified)
.


MessageId=
SymbolicName=MSG_DISKTYPE_RAM_DIRTY
Language=English
Type       : RAM (modified)
.


MessageId=
SymbolicName=MSG_MEDIA_TYPE
Language=English
Media      : %1!s!
.


MessageId=
SymbolicName=MSG_MEDIA_WRITABLE
Language=English
Access     : Writable
.


MessageId=
SymbolicName=MSG_MEDIA_PROTECTED
Language=English
Access     : Write Protected
.


;//
;// Help message text
;//
MessageId=
SymbolicName=MSG_HINT_INSTALL
Language=English
SYNTAX: %1!s!INSTALL [driver] [/AUTO | /A]
Try '%1!s!HELP INSTALL' for more information.
.


MessageId=
SymbolicName=MSG_HINT_REMOVE
Language=English
SYNTAX: %1!s!REMOVE [/FORCE | /F | /QUIT | /Q]
Try '%1!s!HELP REMOVE' for more information.
.


MessageId=
SymbolicName=MSG_HINT_CONFIG
Language=English
SYNTAX: %1!s!CONFIG {/AUTO | /A | /MANUAL | /M}
Try '%1!s!HELP CONFIG' for more information.
.


MessageId=
SymbolicName=MSG_HINT_START
Language=English
SYNTAX: %1!s!START
Try '%1!s!HELP START' for more information.
.


MessageId=
SymbolicName=MSG_HINT_STOP
Language=English
SYNTAX: %1!s!STOP [/FORCE | /F | /QUIT | /Q]
Try '%1!s!HELP STOP' for more information.
.


MessageId=
SymbolicName=MSG_HINT_SHELL
Language=English
SYNTAX: %1!s!SHELL [/ON | /OFF]
Try '%1!s!HELP SHELL' for more information.
.


MessageId=
SymbolicName=MSG_HINT_OPEN
Language=English
SYNTAX: %1!s!OPEN [drive:] [file] [/NEW] [/RAM] [/P | /W]
            [ /160 | /180 | /320 | /360 | /640 | /720 | /820 | /120 | /1.20
              | /144 | /1.44 | /168 | /1.68 | /172 | /1.72 | /288 | /2.88 ]
            [ /5 | /525 | /5.25 ] [/F | /FORCE | /Q | QUIT]
Try '%1!s!HELP OPEN' for more information.
.


MessageId=
SymbolicName=MSG_HINT_CLOSE
Language=English
SYNTAX: %1!s!CLOSE [drive:] [/FORCE | /F | /QUIT | /Q]
Try '%1!s!HELP CLOSE' for more information.
.


MessageId=
SymbolicName=MSG_HINT_SAVE
Language=English
SYNTAX: %1!s!SAVE [drive:] [file] [/OVER | /O | /TRUNC | /T]
                     [/FORCE | /F | /QUIT | /Q]
Try '%1!s!HELP SAVE' for more information.
.


MessageId=
SymbolicName=MSG_HINT_PROTECT
Language=English
SYNTAX: %1!s!PROTECT [drive:] [/ON | /OFF]
Try '%1!s!HELP PROTECT' for more information.
.


MessageId=
SymbolicName=MSG_HINT_FORMAT
Language=English
SYNTAX: %1!s!FORMAT [drive:] [/FORCE | /F | /QUIT | /Q]
Try '%1!s!HELP FORMAT' for more information.
.


MessageId=
SymbolicName=MSG_HINT_LINK
Language=English
SYNTAX: %1!s!LINK [number] [letter] [/L]
Try '%1!s!HELP LINK' for more information.
.


MessageId=
SymbolicName=MSG_HINT_ULINK
Language=English
SYNTAX: %1!s!ULINK [drive]
Try '%1!s!HELP ULINK' for more information.
.


MessageId=
SymbolicName=MSG_HINT_STATUS
Language=English
SYNTAX: %1!s!STATUS
Try '%1!s!HELP STATUS' for more information.
.


MessageId=
SymbolicName=MSG_HINT_VERSION
Language=English
SYNTAX: %1!s!VERSION
Print version information.
.


MessageId=
SymbolicName=MSG_HELP_GENERAL
Language=English
Usage:
  %1!s![command [options...]]

Commands:
  INSTALL   Install the Virtual Floppy driver.
  REMOVE    Uninstall the Virtual Floppy driver.
  CONFIG    Configure the Virtual Floppy driver.
  START     Start the Virtual Floppy driver.
  STOP      Stop the Virtual Floppy driver.
  SHELL     Enable/disable the shell extension.
  OPEN      Open a Virtual Floppy image.
  CLOSE     Close a Virtual Floppy image.
  SAVE      Save the current image into a file.
  PROTECT   Enable/disable drive write protect.
  FORMAT    Format the current Virtual Floppy media.
  LINK      Assign a drive letter to a Virtual Floppy drive.
  ULINK     Remove a drive letter from a Virtual Floppy drive.
  STATUS    Print the current status.
  HELP | ?  Print usage help.
  VERSION   Print version information

If a command is not specified, the interactive console is started.
Type '%1!s!HELP CONSOLE' for more information about the interactive
console.

All commands and options are case insensitive.

Shorter command name can be used as long as the command can be
distinguished uniquely: I for INSTALL, REM for REMOVE, etc. are
accepted, but ST is invalid because it is ambiguous.  You have
to type as much as STAR, STO or STAT in order to distinguish them.

'%1!s!command {/? | /h}' shows a brief hint about each command.
.


MessageId=
SymbolicName=MSG_HELP_INSTALL
Language=English
Install the Virtual Floppy driver.

SYNTAX:
  %1!s!INSTALL [driver] [/AUTO | /A]

OPTIONS:
  driver    Specifies the path to the Virtual Floppy driver file.
            Default is VFD.SYS in the same directory as the VFD
            console program (Note: *NOT* current directory).

  /AUTO     Configures the driver to start at the system startup.
  /A        (Note: *NOT* to start the driver after installation.)
            By default the driver has to be started manually.

Administrator rights are required to install a devide driver.

Device drivers cannot be installed from network drives.
Make sure to place VFD.SYS on a local drive.

It is advised to install the driver with the /AUTO option if the
Virtual Floppy drive is going to be used by users other than
Administrators and Power Users, who don't have enough rights to
start device drivers.
.


MessageId=
SymbolicName=MSG_HELP_CONFIG
Language=English
Configure the Virtual Floppy driver start method.

SYNTAX:
  %1!s!CONFIG {/AUTO | /A | /MANUAL | /M}

OPTIONS:
  /AUTO     Configures the driver to start at the system startup.
  /A

  /MANUAL   Configures the driver to start on demand.
  /M

The change takes effect the next system start up.
Administrator rights are required to configure a devide driver.
.


MessageId=
SymbolicName=MSG_HELP_REMOVE
Language=English
Uninstall the Virtual Floppy driver.

SYNTAX:
  %1!s!REMOVE [/FORCE | /F | /QUIT | /Q]

OPTIONS:
  /FORCE    Suppress prompting and forces the remove operation when
  /F        the driver cannot be stopped.

  /QUIT     Suppress prompting and quits the remove operation when
  /Q        the driver cannot be stopped.

Closes all images and stops the driver if necessary, then removes the
Virtual Floppy driver entries from the system registry.
This command does not delete the driver file from the local disk.

There are cases, due to the condition of the system, when
uninstallation does not complete immediately and restarting of the
system is required.  In such cases you may not be able to install the
Virtual Floppy driver again until the system is restarted and
uninstallation process is complete.

Administrator rights are required to uninstall a device driver.
.


MessageId=
SymbolicName=MSG_HELP_START
Language=English
Start the Virtual Floppy driver.

SYNTAX:
  %1!s!START

OPTIONS:
  NONE

If the driver is not installed, this command attempts to install it
with thedefault options.

At least Power User rights are required to start a device driver.
.


MessageId=
SymbolicName=MSG_HELP_STOP
Language=English
Stop the Virtual Floppy driver

SYNTAX:
  %1!s!STOP [/FORCE | /F | /QUIT | /Q]

OPTIONS:
  /FORCE    Suppress prompting and forces the stop operation when any
  /F        of the drives are in use and cannot be closed.

  /QUIT     Suppress prompting and quits the stop operation when any
  /Q        of the drives are in use and cannot be closed.

This command closes all images before stopping the driver.
An image cannot be closed if the virtual drive is used by any other
programs.  Forcing the stop operation with a drive in use may leave
the driver in stop pending state.  In such cases the driver cannot be
restarted until all programs stop using the drive and the driver is
properly unloaded.

At least Power User rights are required to stop a device driver.
.


MessageId=
SymbolicName=MSG_HELP_SHELL
Language=English
Enable / disable the Virtual Floppy drive shell extension.

SYNTAX:
  %1!s!SHELL [/ON | /OFF]

OPTIONS:
  /ON       Enables the shell extension.

  /OFF      Disables the shell extension.

If an option is not specified, this command prints the current state
of the shell extension.
.


;// __REACTOS__: s/read only/read-only/.
MessageId=
SymbolicName=MSG_HELP_OPEN
Language=English
Open a Virtual Floppy image.

SYNTAX:
  %1!s!OPEN [drive:] [file] [/NEW] [/RAM] [/P | /W]
        [/size] [/media] [/F | /FORCE | /Q | /QUIT]

OPTIONS:
  drive:    Specifies a target Virtual Floppy drive, either by a drive
            number or a drive letter, such as "0:", "1:", "B:", "X:".
            The trailing ':' is required.
            The drive 0 is assumed if not specified.

  file      Specifies a Virtual Floppy image file to open.
            An empty RAM disk is created if not specified.

  /NEW      Creates a new image file.
            Ignored if a file is not specified.

  /RAM      RAM mode - mounts an on-memory copy of the image, instead
            of directly mounting the image file.
            Changes made to the virtual media are lost when the image
            is closed, unless the image is explicitly saved to a file
            with the 'SAVE' command.
            Ignored if a file is not specified.

  /P        Opens the image as a write protected media.
            Write protection state can be chenged later with the
            'PROTECT' command.

  /W        Opens the image as a writable media.
            Write protection state can be chenged later with the
            'PROTECT' command.

  /size     Specifies a media size.  Acceptable options are:

              /160 (160KB)    /820 (820KB)
              /180 (180KB)    /120 or /1.20 (1.20MB)
              /320 (320KB)    /144 or /1.44 (1.44MB)
              /360 (360KB)    /168 or /1.68 (1.68MB DMF)
              /640 (640KB)    /172 or /1.72 (1.72MB DMF)
              /720 (720KB)    /288 or /2.88 (2.88MB)

  /5        Specifies a 5.25 inch media.  Takes effect only with
  /525      640KB, 720KB and 1.2MB media and otherwise ignored.
  /5.25     160KB, 180KB, 320KB and 360KB media are always 5.25".
            820KB, 1.44MB, 1.68MB, 1.72MB and 2.88MB media are always
            3.5".

  /FORCE    Suppress prompring on minor conflicts and/or omission of
  /F        necessary parameters and continues the operation as best
            as possible, employing default values if necessary.
            See below for details.

  /QUIT     Suppress prompring on minor conflicts and/or omission of
  /Q        necessary parameters and quits the operation on the first
            such occasion.
            See below for details.

If the target drive does not have a drive letter, this command also
assigns a local drive letter (see '%1!s!HELP LINK') using the first
available letter.

Read-only files, NTFS encrypted/compressed files and ZIP compressed
image files (such as WinImage IMZ file) cannot be mounted directly
and must be opened in RAM mode.

Without a size option, size of a virtual media is decided from the
actual image size.  With an explicit size option you can mount a
file as a smaller media, in such cases surplus data at the end of
the image is ignored.
A virtual media size cannot exceed the actual image size.

The /F and /Q options affect the behavior of the OPEN command in
many ways:

  When the target file does not exist and the /NEW option
  is not present

    (none) ask user whether to create the target
      /F   create the target without asking
      /Q   abort the operation without asking

  The target file exists and the /NEW option is present

    (none) ask user whether to overwrite the existing file
      /F   overwrite the file without asking
      /Q   abort the operation without asking

  The target file cannot be mounted directly and the /RAM
  option is not present

    (none) ask user whether to open in RAM mode
      /F   open in RAM mode without asking
      /Q   abort the operation without asking

  A size option is not present for creating a new image

    (none) ask user whether to create a 1.44MB (default) image
      /F   create a 1.44MB image without asking
      /Q   abort the operation without asking

  A size option is not present and the target file size is
  not an exact match for any of supported media

    (none) ask user whether to mount as a largest media to fit
           in the actual image
      /F   mount as a largest media to fit in the actual image
           without asking
      /Q   abort the operation without asking
.


MessageId=
SymbolicName=MSG_HELP_CLOSE
Language=English
Close a Virtual Floppy image.

SYNTAX:
  %1!s!CLOSE [drive:] [/FORCE | /F | /QUIT | /Q]

OPTIONS:
  drive:    Specifies a target Virtual Floppy drive, either by a drive
            number or a drive letter, such as "0:", "1:", "B:", "X:".
            The trailing ':' is optional.
            "*" stands for both drives.
            The drive 0 is used if not specified.

  /FORCE    Suppress prompting and forces the close operation when RAM
  /F        disk data is modified or the drive is in use.
            Forcing with the drive in use will work only on Windows
            2000 and later (not on NT).

  /QUIT     Suppress prompting and quits the close operation when RAM
  /Q        disk data is modified or the drive is in use.

If neither /Q nor /F is specified, the user has to choose whether to
retry, force, or quit.

Unlike the previous versions of the VFD, this command does *NOT*
remove the drive letter of the target drive.
.


MessageId=
SymbolicName=MSG_HELP_SAVE
Language=English
Save the current image data into a file.

SYNTAX:
  %1!s!SAVE [drive:] [file] [/O | /OVER | /T | /TRUNC]
        [/FORCE | /F | /QUIT | /Q]

OPTIONS:
  drive:    Specifies a target Virtual Floppy drive, either by a drive
            number or a drive letter, such as "0:", "1:", "B:", "X:".
            The trailing ':' is required.
            The drive 0 is used if not specified.

  file      Specifies a file name to save data.
            If not specified, the current image file name is used.
            Required if the current image is a pure RAM disk.

  /OVER     Overwrite the file if the target file exists.
  /O        If the existing file is larger than the current image,
            file size is not changed and the surplus data at the end
            of the file is left unchanged.
            If the target is the current image file, this is the
            default behavior of this command.
            Ignored if the target does not exist.

  /TRUNC    Overwrite the file if the target file exists.
  /T        If the existing file is larger than the current image,
            the file is truncated to the image size and the surplus
            data at the end of the file is discarded.
            Ignored if the target does not exist.

  /FORCE    Suppress prompting when the target volume can not be
  /F        locked and forces the operation without locking.

  /QUIT     Suppress prompting when the target volume can not be
  /Q        locked and quits the operation.

If the target is the current image file, the file is always
overwritten without a question and the /O option is not necessary.
Otherwise this command fails if the target file exists and neither
/O or /T is present.

If the existing file is smaller than the current image, the file
is always expanded to the current image size either with /O or /T.

This program NEVER overwrites a ZIP compressed file regardless of /O
or /T option, or even if it is the current image file.
The SAVE command always fails if the target is a ZIP compressed file.
.


;// __REACTOS__: s/read only/read-only/.
MessageId=
SymbolicName=MSG_HELP_PROTECT
Language=English
Enable / disable drive write protect.

SYNTAX:
  %1!s!PROTECT [drive:] [/ON | /OFF]

OPTIONS:
  drive:    Specifies a target Virtual Floppy drive, either by a drive
            number or a drive letter, such as "0:", "1:", "B:", "X:".
            The trailing ':' is optional.
            The drive 0 is used if not specified.

  /ON       Enables the drive write protect - the drive becomes read-only.

  /OFF      Disables the drive write protect - the drive becomes writable.

If an option is not specified, this command prints the current write
protect state of the drive.

After write protection is disabled with this command, Windows may not
notice the change immediately and claim that the media is still write
protected.  Refreshing the Explorer or retrying the faild operation
will fix that.
.


MessageId=
SymbolicName=MSG_HELP_FORMAT
Language=English
Format a Virtual Floppy media with FAT.

SYNTAX:
  %1!s!FORMAT [drive:] [/FORCE | /F | /QUIT | /Q]

OPTIONS:
  drive:    Specifies a target Virtual Floppy drive, either by a drive
            number or a drive letter, such as "0:", "1:", "B:", "X:".
            The trailing ':' is optional.
            The drive 0 is used if not specified.

  /FORCE    Suppress prompting when the target volume can not be
  /F        locked and forces the operation without locking.

  /QUIT     Suppress prompting when the target volume can not be
  /Q        locked and quits the operation.
.


MessageId=
SymbolicName=MSG_HELP_LINK
Language=English
Assign a drive letter to a Virtual Floppy drive.

SYNTAX:
  %1!s!LINK [number] [letter] [/L]

OPTIONS:
  number    Specifies a target drive number.
            If not specified, drive 0 is used.
            "*" stands for both drives.

  letter    Spesifies a drive letter to assign.
            If not specified, the first available letter is used.
            If the target is both drives, letters for each drives can
            be specified like "BF" (B for 0, F for 1).

  /L        Assign an ephemeral / local drive letter.
            The default (without this option) is persistent / global.

Persistent / global drive letters are reclaimed each time the driver
starts.
On Windows 2000 SP2 and later they are not deleted on user logoff.
On Terminal Servers they are globaly visible to all users on the
system.

Ephemeral / local drive letters are not reclaimed on driver start up.
On Windows 2000 SP2 and later they are deleted on user logoff.
On Terminal Servers, they are visible only to the current user and
each user can assign different drive letter to the same drive.
.


MessageId=
SymbolicName=MSG_HELP_ULINK
Language=English
Remove a drive letter from a Virtual Floppy drive.

SYNTAX:
  %1!s!ULINK [drive]

OPTIONS:
  drive     Specifies a target Virtual Floppy drive, either by a drive
            number or a drive letter.
            If not specified, drive 0 is used.
            "*" stands for both drives.

Drive letters can be removed even if the drive is being used.
Some applications such as Windows Explorer detects it and acts
accordingly, for example closes folder windows for the drive.
.


MessageId=
SymbolicName=MSG_HELP_STATUS
Language=English
Print the current status.

SYNTAX:
  %1!s!STATUS

OPTIONS:
  NONE

This command prints the following information:

  Driver file path
  Driver version
  Driver start type
  Driver running state

  Shell extension status

  Drive letter
  Image name
  Image description (file type, size, file attributes, etc.)
  Disk type (RAM or FILE)
  Write protection
.


MessageId=
SymbolicName=MSG_HELP_HELP
Language=English
Print the VFD console help.

SYNTAX:
  %1!s!HELP [command | topic]

OPTIONS:
  command   Specifies one of the following commands

              INSTALL REMOVE  CONFIG  START   STOP
              SHELL   OPEN    CLOSE   SAVE    PROTECT
              FORMAT  LINK    ULINK   STATUS  HELP
              VERSION

  topic     Specifies one of the following topics

              CONSOLE

If an option is not specified, the general help is printed.
.


MessageId=
SymbolicName=MSG_CONSOLE_HINT
Language=English

    ********** the VFD interactive console **********

you can use the following commands in addition to regular VFD commands:

    ATTRIB  CD      CHDIR   <drive>:
    DIR     EXIT    QUIT    BYE
    .(period) + Windows command

Type '? CONSOLE' or 'HELP CONSOLE' for more information

.


MessageId=
SymbolicName=MSG_HELP_CONSOLE
Language=English
In the interactive console, you can use the following commands in
addition to regular VFD commands:

  CD | CHDIR
            Displays the name of or changes the current directory.
            Similar to the Windows CD/CHDIR command.

  <drive>:  Change the current directory to the root of the specified
            drive.  Similar to the Windows drive change command.

  DIR       Executes the Windows DIR command.
            All options for the Windows DIR command are available.

  ATTRIB    Executes the Windows ATTRIB command.
            All options for the Windows ATTRIB command are available.

  EXIT | QUIT | BYE | <Ctrl+C>
            Quits the VFD interactive console.

A command typed with a leading '.'(period) is executed by the Windows
command processor (cmd.exe).

  e.g.) .FORMAT [options ...]
          Executes the Windows format.exe.  All options are passed to
          the format.exe.

        FORMAT [options ...]
          Executes the VFD 'FORMAT' command.

To execute an external command with spaces in its name, put the
'.' (period) outside the quoteation.

  e.g.) ."C:\Program Files\My App\My Program.exe" [options ...]

DIR and ATTRIB Windows commands are recognized without a period,
for they are used very frequently.

Commands to affect current directory and environment variable have
effects only inside the Windows command processor.  You can execute
them but they have no effect on the VFD console.

  .CD .CHDIR .<drive>: to change the current directory
  .PUSHD .POPD
  .PATH to change the search path
  .PROMPT
  .SET to change the value of an environment variable
.


MessageId=
SymbolicName=MSG_PAGER_PROMPT
Language=English
Press any key to continue ('Q' or <Ctrl+C> to quit) ...%0
.


;
;#endif // _VFDMSG_H_
