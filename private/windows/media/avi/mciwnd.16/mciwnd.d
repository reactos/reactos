/*
     modified July 23, 1993 HFF Marked messages used in macros as internal. 
                            These messages are documented with equivalent 
                            macro. 
*/

/*----------------------------------------------------------------------------*\
 *
 *  MCIWnd
 *
 *    the MCIWnd window class is a window class for controling MCI devices
 *    MCI devices include, wave files, midi files, AVI Video, cd audio,
 *    vcr, video disc, and others..
 *
 *    to learn more about MCI and mci command sets see the
 *    "Microsoft Multimedia Programmers's guide" in the Win31 SDK
 *
 *    the easiest use of the MCIWnd class is like so:
 *
 *          hwnd = MCIWndCreate(hwndParent, hInstance, 0, "smag.wav");
 *          ...
 *          MCIWndPlay(hwnd);
 *          MCIWndStop(hwnd);
 *          MCIWndPause(hwnd);
 *          ....
 *          MCIWndDestroy(hwnd);
 *
 *    this will create a window with a play/pause, stop and a playbar
 *    and start the wave file playing.
 *
 *    mciwnd.h defines macros for all the most common MCI commands, but
 *    any string command can be used if needed.  Note unlike the
 *    mciSendString() API, no alias or file name needs to be specifed.
 *
 *          MCIWndSendString(hwnd, "set hue to bright");
 *
 *
 *----------------------------------------------------------------------------*/

/***********************************************************************
* @doc EXTERNAL 
*
* @api BOOL | MCIWndRegisterClass | This function registers the MCI 
*      window class MCIWND_WINDOW_CLASS. 
*
* @parm HINSTANCE | hinstance | Specifies a handle to the device instance.
*
* @rdesc Returns zero if the class is registered successfully.
*
* @comm Use <f CreateWindow> to create the window used for 
*       playing the MCI device. Use <f  MCIWndCreate> to 
*       simultaneously register the window class and 
*       create a window.
*
* @xref MCIWndCreate
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api HWND | MCIWndCreate | This function creates an MCIWnd window for 
*      using MCI services. 
*
* @parm HWND | hwndParent | Specifies a handle to the parent window.
*
* @parm HINSTANCE | hInstance | Specifies the window instance.
*
* @parm  DWORD | dwStyle | Specifies any flags for the window style. 
*	If you do not specify any of the WS_ styles, your window will
*	use WS_CHILD, WS_BORDER, and WS_VISIBLE if you specify a non-NULL
*	parent window, and WS_OVERLAPPEDWINDOW and WS_VISIBLE otherwise.
*	Thus, to create an invisible child window, specify WS_CHILD to
*	prevent the default flags from being used.
* 
*       The following styles are defined in addition to 
*        those defined for <f CreateWindow>.
*
*    @flag MCIWNDF_NOAUTOSIZEWINDOW | Inhibits the changing of 
*          the window size when the image size changes. By default, 
*          the window size changes automatically to reflect changes in 
*          image size. The window also automatically resizes when a 
*          new file opens or closes.
*
*    @flag MCIWNDF_NOAUTOSIZEMOVIE | Prevents stretching the 
*          image when window size changes. For MCI devices 
*          that display video in a window, MCIWnd normally 
*          resizes the image to fill the window.  
*
*    @flag MCIWNDF_NOPLAYBAR | Inhibits the display of the playbar. 
*          By default, a playbar is displayed. The default controls on 
*          on the playbar include the PLAY, STOP, RECORD, 
*          and MENU buttons as well as a trackbar to set the 
*          media position.
*
*       Holding a shift key down while pressing the PLAY button plays 
*       the media backwards (if supported). For windowed devices, 
*       holding the control key down while pressing the PLAY 
*       button plays the device in FULLSCREEN mode if supported.
*
*    @flag MCIWNDF_NOMENU | Inhibits the display of a menu button 
*          on the playbar and inhibits display of the popup menu 
*          normally available when the right mouse button 
*          is pressed when the cursor is over the window.
*
*     @flag MCIWNDF_RECORD | Displays a record button on the playbar 
*           or menu, and adds a new file command to the menu. 
*           By default, only nondestructive 
*           commands are available on the playbar and menu.
*
*     @flag MCIWNDF_NOERRORDLG | Inhibits display of an error dialog 
*           box for MCI errors. Applications can use <f MCIWndGetError>
*           to determine the most recent error.
*
*     @flag MCIWNDF_NOTIFYMODE | Has MCIWnd notify the parent 
*           window when the mode of the MCI device changes.
*           Whenever the mode changes (for example, 
*           from stop to play) the parent window receives a 
*           <m MCIWNDM_NOTIFYMODE> message. The <p lParam> of the 
*           message contains the new state of the device (for 
*           example, MCI_MODE_STOP). The application can get 
*           the text string of the new mode with the <f MCIWndGetMode>.
*
*     @flag MCIWNDF_NOTIFYPOS | Has MCIWnd notify the parent window 
*           when the position of the media changes. MCIWnd sends 
*           the <m MCIWNDM_NOTIFYPOS> message to inform the parent 
*           window of changes. The <p lParam> of the message 
*           contains the new position in the media.
*
*     @flag MCIWNDF_NOTIFYMEDIA | Has MCIWnd notify the parent window 
*           when the media changes. Whenever a file is opened or 
*           closed, or a new device is loaded, MCIWnd sends the 
*           <m MCIWNDM_NOTIFYMEDIA> message. The <p lParam> of 
*           the message contains a pointer to the new filename.
*
*     @flag MCIWNDF_NOTIFYSIZE | Has MCIWnd notify the parent window 
*           when the size of the window changes.
*
*     @flag MCIWNDF_NOTIFYERROR | Has MCIWnd notify the parent window 
*           when an error occurs.
*
*    @flag MCIWNDF_NOTIFYALL | Notifies parent window of all changes.
*
*    @flag MCIWNDF_SHOWNAME | Displays the name of the open 
*          MCI device element in the window caption.
*
*    @flag MCIWNDF_SHOWPOS | Displays the position of the MCI device 
*          in the window caption.
*
*    @flag MCIWNDF_SHOWMODE | Displays the mode of the MCI device 
*          in the window caption.
*
*    @flag MCIWNDF_SHOWALL | Displays all window features.
*
* @parm  LPSTR | szFile | Specifies a zero-terminated string indicating 
*        the name of an MCI device or device element to open.
*
* @rdesc Returns a handle to an MCIWnd window if successful. 
*        Otherwise it returns zero.
*
* @comm Use the window handle returned by this function for the 
*       window handle in the MCIWnd macros. If your application 
*       uses this macro, it does not need to use <f MCIWndRegisterClass>.
*    
*       For non-windowed devices, applications need a window to 
*       display the toolbar and trackbar. Applications that 
*       access non-windowed devices (such as a waveform audio device), 
*       and do not display any controls, can leave the window invisible.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_???? | This message is a placeholder for 
*      flags until I find out which message they apply to.
*
* @parm  LPARAM | wFlags | Specifies applicable flags for the open.
*
*
*     @flag MCIWND_START | Indicates the start of the media.
*
*     @flag MCIWND_END | Indicates the end of the media.
*
*     @flag MCIWND_CURRENT | Indicates the current position.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SENDSTRING | This MCIWnd message sends a string command 
*      to an MCI device.
*
* @parm WPARAM | WParam1 | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies a zero-terminated string to 
*       send to the MCI device.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm When sending a string with this message, omit the 
*       alias used when a string is sent with <f mciSendString>.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETPOSITION | This MCIWnd message has an MCI 
*      device return the current position of the device element.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the current position.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETSTART | This MCIWnd message has an MCI 
*      device return the starting position of the device element.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the starting position of the device element.
*
* @comm Typically the return value is zero but some devices 
*       use a non-zero starting position for the device 
*       element.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETLENGTH | This MCIWnd message has an MCI 
*      device return the length of its media.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the length of the media.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETEND | This MCIWnd message has an MCI 
*      device return the ending position of the media. 
*      (The ending position corresponds to the starting 
*      position plus the length.)
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the ending position of the media.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETMODE | This MCIWnd message has an MCI 
*      device return its mode.
*
* @parm WPARAM | wParam | Specifies the size of the data buffer.
*
* @parm LPARAM | lParam | Specifies a far pointer to the 
*       application-supplied buffer used to return the mode.
*
* @rdesc Returns an integer corresponding to the MCI mode.
*
* @comm If the zero-terminated string describing the mode is 
*       longer than the buffer, it is truncated.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETZOOM | This MCIWnd message has an MCI device stretch 
*      a video image.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies an integer indicating 
*       the zoom factor as a percentage of the original 
*       image. Specifying 100 displays the image at its 
*       authored sized. Specifying 200 displays the image 
*       at twice normal size. Specifying 50 displays the image at 
*       half normal size.
*
* @rdesc None.
*
* @xref MCIWNDM_GETZOOM
*
***********************************************************************/
  
/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETZOOM | This MCIWnd message has an MCI device return 
*      the zoom factor last set with <f MCIWNDM_SETZOOM>.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns an integer corresponding to the zoom factor. A value 
*        of 100 indicates the image is not zoomed. A value of 200 
*        indicates the image is twice normal size. A value of 50 
*        indicates the image is half normal size.
*
* @xref MCIWNDM_SETZOOM
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_EJECT | This MCIWnd message has an MCI 
*      device eject its media.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns false if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETVOLUME | This MCIWnd message has an MCI 
*      device set its volume level.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies an integer indicating the volume level
*       Specify 1000 for the normal volume level. Specify larger 
*       values to increase the volume level and smaller 
*       values to decrease the volume level.
*
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @xref  MCIWNDM_GETVOLUME
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETVOLUME | This MCIWnd message has an MCI 
*      device return its volume level.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the volume level.
*
* @xref  MCIWNDM_SETVOLUME
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETSPEED | This MCIWnd message has an MCI 
*      device set its playback speed.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies an integer indicating 
*       the playback speed. Specify 1000 
*       for the normal speed. Specify larger values for faster speeds 
*       and smaller values for slower speeds.
**
* @rdesc Returns false if successful. Otherwise it returns 
*        an MCI error code.
*
* @xref  MCIWNDM_GETSPEED
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETSPEED | This MCIWnd message has an MCI 
*      device return its playback speed.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the playback speed.
*
* @xref  MCIWNDM_SetSpeed
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETTIMEFORMAT | This MCIWnd message has an MCI 
*      device set its time format.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies a far pointer to a data buffer 
*       containing the time format.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm Application can specify formats other than frames and 
*       milliseconds if they are supported by the MCI device. 
*       Non-continuous formats, such as tracks and 
*       SMPTE can cause the scroll bar to behave erratically. 
*       For these time formats, applications might change the 
*       window style by using MCIWNDF_NOPLAYBAR to turn off the 
*       playbar.
*
* @xref  MCIWNDM_GETTIMEFORMAT
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETTIMEFORMAT | This MCIWnd message has an MCI 
*      device return its time format.
*
* @parm WPARAM | wParam | Specifies the size of the data buffer.
*
* @parm LPARAM | lParam | Specifies a far pointer to the 
*       application-supplied data buffer used to return the time format.
*
* @rdesc Returns an integer corresponding to the MCI time format.
*
* @comm If the zero-terminated string describing the time format
*       longer than the buffer, MCIWnd truncates it.
*
* @xref  MCIWNDM_SETTIMEFORMAT
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_PLAYFROM | This MCIWnd message specifies the 
*      starting position for playing and sends the MCI_PLAY 
*      message to an MCI device to begin playing.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies the starting position for the sequence.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This message starts the device playing. Play continues until it is 
*       stopped or the device reaches the end of the sequence.
*
* @xref MCIWNDM_PLAYTO
*
***********************************************************************/

/***********************************************************************
* @doc INTERNAL 
*
* @msg MCIWNDM_PLAYTO | This MCIWnd message specifies the ending position 
*      of a playback sequence and sends the MCI_PLAY message to an 
*      MCI device to begin play.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies the ending position for the sequence.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This message starts play from the current position. Play stops 
*       when the sequence reaches the specified position (or the end 
*       of the sequence if the specified position is beyond the end 
*       of the sequence).
*
* @xref MCIWNDM_PLAYTO
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETFILENAME | This MCIWnd message has an MCI 
*      device return the filename of its device element.
*
* @parm WPARAM | wParam | Specifies the size of the data buffer.
*
* @parm LPARAM | lParam | Specifies a far pointer to the 
*       application-supplied data buffer used 
*       to return the filename.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm If the zero-terminated string containing the filename is 
*       longer than the buffer, it is truncated.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETDEVICE | This MCIWnd message has an MCI 
*      device return its device type.
*
* @parm WPARAM | wParam | Specifies the size of the data buffer.
*
* @parm LPARAM | lParam | Specifies a far pointer to the 
*       application-supplied data buffer used 
*       to return the device type.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm If the zero-terminated string describing the device type is 
*       longer than the buffer, MCIWnd truncates it.
*
************************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETPALETTE | This MCIWnd message has an MCI 
*      device return a handle to its palette.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the handle to the palette.
*
* @xref  MCIWNDM_SETPALETTE
*
***********************************************************************/

/***********************************************************************
* @doc INTERNAL 
*
* @msg MCIWNDM_SETPALETTE | This MCIWnd message sends a palette handle 
*      to an MCI device.
*
* @parm WPARAM | wParam | Specifies the handle to the palette.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @xref  MCIWNDM_GETPALETTE
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETERROR | This message has MCIWnd return 
*      the last MCI error associated with its window.
*
* @parm WPARAM | wParam | Specifies the size of the error data buffer.
*
* @parm LPARAM | lParam | Specifies a far pointer to the 
*       application-supplied data buffer used to return the error.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an integer corresponding to the MCI error code.
*
* @comm If the zero-terminated string describing the error is 
*       longer than the buffer, it is truncated.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETACTIVETIMER | This message sets the 
*       period used by MCIWnd to update the scrollbar, 
*       position information displayed in the window caption, 
*       and send notification messages to the parent window 
*       when the MCIWnd is the active window.
*
* @parm WPARAM | wParam | Specifies the integer number of milliseconds 
*       between updates.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc None.
*
* @xref  MCIWNDM_SETINACTIVETIMER MCIWNDM_SETTIMERS
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETINACTIVETIMER | This message sets the 
*       period used by MCIWnd to update the scrollbar, 
*       position information displayed in the window caption, 
*       and send notification messages to the parent window 
*       when the MCIWnd is the inactive window.
*
* @parm WPARAM | wParam | Specifies the integer number of milliseconds 
*       between updates.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc None.
*
* @xref  MCIWNDM_SETACTIVETIMER MCIWNDM_SETTIMERS
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETTIMERS | This message sets the 
*       periods used by MCIWnd to update the scrollbar, 
*       position information displayed in the window caption, 
*       and send notification messages to the parent window.
*
* @parm WPARAM | wParam | Specifies the integer number of milliseconds 
*       between updates when the window is the active window.
*
* @parm LPARAM | lParam | Specifies the integer number of milliseconds 
*       between updates when the window is the inactive window.
*
* @rdesc None.
*
* @xref  MCIWNDM_SETACTIVETIMER MCIWNDM_SETINACTIVETIMER
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETACTIVETIMER | This message returns 
*      the timer period used when the MCIWnd window is the 
*      active window.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the update period in milliseconds. 
*        The default is 500 milliseconds.
*
* @xref  MCIWNDM_SETACTIVETIMER
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETINACTIVETIMER | This message returns 
*      the timer period used when the MCIWnd window is the 
*      inactive window.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the update period in milliseconds. 
*        The default is 2000 milliseconds.
*
* @xref  MCIWNDM_SETINACTIVETIMER
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETSTYLES | This MCIWnd message returns 
*      the flags used for specifying the styles used 
*      for an MCIWnd window.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the flags specifying the styles used for 
*        the window.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_CHANGESTYLES | This MCIWnd message 
*      changes the styles used for the window.
*
* @parm WPARAM | wParam | Specifies a mask for <p lParam>. This 
*       mask is the bitwise or of all styles that will be updated.
*
* @parm LPARAM | lParam | Specifies the styles used for the window. 
*       See <f MCIWndCreate> for the available styles.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETALIAS | This MCIWnd message returns the alias 
*      used to open and MCI device or device element.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns the device alias.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_NEW | This MCIWnd message has an MCI device create a new 
*      file.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Specifies a far pointer to a buffer 
*       containing the name of the MCI device used to create 
*       the file.
*
* @rdesc Returns false if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_SETREPEAT | This MCIWnd message has an MCI device 
*      repeat a sequence.
*
* @parm WPARAM | wParam | Specifies the number of times to repeat a sequence.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_GETREPEAT | This MCIWnd message has an MCI device 
*      return the number times it will repeat a sequence.
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_REALIZE | This MCIWnd message has an MCI device 
*      realize its palette.
*
* @parm WPARAM | wParam | Specifies TRUE if the window is a 
*       background window.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_VALIDATEMEDIA | This message has MCIWnd synchronize 
*      its position information to the new media when it has changed. 
*
* @parm WPARAM | wParam | Not used; set to zero.
*
* @parm LPARAM | lParam | Not used; set to zero.
*
* @rdesc None.
*
* @comm If  your application changes the time format of a 
*       device without using MCIWnd, the starting and ending 
*       position of the media, as well as the trackbar continue to 
*       use the old format. Use the <f MCIWndValidateMedia> 
*       macro or the <m MCIWNDM_VALIDATEMEDIA> message to update 
*       these values. Normally, you should not 
*       need to use this message. 
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @msg MCIWNDM_NOTIFYMODE | MCIWND sends this message to 
*      the parent window of an application to notify 
*      it that device mode has changed.
*
* @parm WPARAM | WParam | Not used.
*
* @parm LPARAM | LParam | Specifies an integer corresponding to 
*       the MCI mode.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @msg MCIWNDM_NOTIFYPOS | MCIWND sends this message to 
*      the parent window of an application to notify it 
*      that the window position has changed.
*
* @parm WPARAM | WParam | Not used.
*
* @parm LPARAM | LParam | Specifies the new position.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @msg MCIWNDM_NOTIFYSIZE | MCIWND sends this message to 
*      the parent window of an application to notify it 
*      that the window size has changed.
*
* @parm HWND | WParam | Specifies a handle to the MCIWnd window that 
*       has changed size.
*
* @parm LPARAM | LParam | Not used; set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @msg MCIWNDM_NOTIFYMEDIA | MCIWND sends this message to 
*      the parent window of an application to notify it 
*      that the media has changed.
*
* @parm WPARAM | WParam | Not used.
*
* @parm LPARAM | LParam | Specifies a far pointer to 
*       a zero-terminated string containing the new filename.
*       If the media is closing, it specifies a null string.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @msg MCIWNDM_NOTIFYCLOSE | MCIWND sends this message to 
*      the parent window of an application to notify it 
*      that the window is closing.
*
* @parm HWND | WParam | Specifies the window handle.
*
* @parm LPARAM | LParam | Not used; set to zero.
*
***********************************************************************/

/***********************************************************************
*
* @doc EXTERNAL
*
* @msg MCIWNDM_NOTIFYERROR | MCIWND sends this message to 
*      the parent window of an application to send it an error from MCI.
*
* @parm HWND | WParam | Specifies the window handle.
*
* @parm LPARAM | LParam | Specifies a far pointer to a zero-terminated 
*       string indicating the error.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndOpen | This macro has MCIWnd open an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LPSTR | szFile | Specifies a zero-terminated string for the file name. 
*       Specifying -1 for this parameter has the MCI device display 
*       a dialog box used to specify the filename.
*
* @parm  WORD | wFlags | Specifies applicable flags for the open. 
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCI_OPEN message. 
*        The <p wParam> parameter for the message is set to <p wFlags>, 
*        and the <p lParam> parameter for the message is set to 
*        <p szFile>.
*
* @xref MCIWndClose MCIWndOpenDialog
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndOpenDialog | This macro 
*      has MCIWnd display a dialog box for specifying the filename 
*      MCIWnd uses to open an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCI_OPEN message. 
*        The <p wParam> parameter for the message is set to -1, 
*        and the <p lParam> parameter for the message is set to zero.
*
* @xref MCIWndClose MCIWndOpen
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndNew | This MCIWnd macro has an MCI device create a new 
*      file.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LPARAM | lp | Points to the name of the device 
*       used to create the new file.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCIWNDM_NEW message. 
*        The <p wParam> parameter for the message is set to zero, 
*        and the <p lParam> parameter is set to <p lp>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndClose | This macro has MCIWnd close an 
*      MCI device or device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero.
*
* @comm Use this macro to close an MCI device 
*       without destroying the MCIWnd window. 
*       Use <f MCIWndDestroy> to simultaneously close 
*       the MCI device and destroy the MCIWnd window.
*
*       If your application will immediately open a new MCI device in 
*       the MCIWnd window, use <f MCIWndOpen> to simultaneously 
*       close the current MCI device and open a new one.
*
* @comm  This macro is defined with the MCI_CLOSE message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndOpen MCIWndDestroy
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api VOID | MCIWndDestroy | This MCIWnd macro closes an MCI device 
*      or device element and destroys the MCIWnd window.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @comm  This macro is defined with the WM_CLOSE message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndOpen MCIWndClose
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api VOID | MCIWndSetZoom | This MCIWnd macro has an MCI device stretch 
*      a video image.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm int | iZoom | Specifies an integer indicating the zoom factor as 
*       a percentage of the original image. Specifying 100 displays the 
*       image at its authored sized. Specifying 200 displays the image 
*       at twice normal size. Specifying 50 displays the image at 
*       half normal size.
*
* @comm This macro is defined with the MCIWNDM_SETZOOM message. 
*       The <p wParam> parameter for the message is set to zero,
*       and the <p lParam> parameter has the same 
*       definition as <p iZoom>.
*
* @xref MCIWndGetZoom
*
***********************************************************************/
  
/***********************************************************************
* @doc EXTERNAL 
*
* @api UINT | MCIWndGetZoom | This MCIWnd macro has an MCI device return 
*      the zoom factor it is using.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns an integer corresponding to the zoom factor. A value 
*        of 100 indicates the image is not zoomed. A value 
*        of 200 indicates the image is zoomed to twice its 
*        original size. A value of 50 indicates the image 
*        is reduced to half its original size.
*
* @comm  This macro is defined with the MCIWNDM_GETZOOM message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndSetZoom
*
***********************************************************************/
  
/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSendString | This MCIWnd macro sends a string command 
*      to an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPSTR | sz | Specifies a string to send to the MCI device.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm When sending a string with this macro, omit the 
*       alias used when the string is sent with <f mciSendString>.
*
*       This macro is defined with the MCIWNDM_SENDSTRING message. 
*       The <p wParam> parameter for the message is set to zero,
*       and the <p lParam> parameter has the same 
*       definition as <p sz>.
*
***********************************************************************/


/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndReturnString | This MCIWnd macro obains 
*      the string information returned by an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPARAM | lp | Specifies a far pointer to the application-supplied 
*       buffer used to return the zero-terminated string.
*
* @parm int | iLen | Specifies the size of the data buffer.
*
* @rdesc Returns an integer corresponding to the MCI string.
*
* @comm If the zero-terminated string is 
*       longer than the buffer, it is truncated.
*
*       This macro is defined with the MCIWNDM_RETURNSTRING message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iLen>, and the <p lParam> parameter has the same 
*       definition as <p lp>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSeek | This MCIWnd macro sends the MCI_SEEK message 
*      to an MCI device to seek to a specified position.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LONG | lPos | Specifies the seek position.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm The units for the seek position depend on the current time format.
*
*       This macro is defined with the MCI_SEEK message. 
*       The <p wParam> parameter for the message is set to zero, 
*       and the <p lParam> parameter has the same 
*       definition as <p lPos>.
*
* @xref MCIWndEnd MCIWndHome
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndHome | This MCIWnd macro sends the MCI_SEEK message 
*      to an MCI device to seek to the start of the media.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCIWND_START message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndEnd MCIWndSeek
*
**********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndEnd | This MCIWnd macro has 
*      an MCI device seek to the end of the media.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCIWND_END message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndHome MCIWndSeek
*
**********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPlay | This MCIWnd macro has the MCI device 
*      start playing.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts playing from the current position.
*
* @comm  This macro is defined with the MCI_PLAY message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndPlayReverse MCIWndPlayFrom MCIWndPlayTo MCIWndPlayFromTo
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPlayFrom | This MCIWnd macro specifies the 
*      starting position for playing and has the MCI device 
*      start playing.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LONG | lPos | Specifies the starting position for the sequence.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts play from the specified position. 
*       Playing continues until it is 
*       stopped or play reaches the end of the sequence.
*
*      This macro is defined with the MCIWNDM_PLAYFROM message. 
*      The <p wParam> parameter for the message is set to zero, 
*      and the <p lParam> parameter has the same 
*      definition as <p lPos>.
*
* @xref MCIWndPlay MCIWndPlayReverse MCIWndPlayTo MCIWndPlayFromTo
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPlayFromTo | This MCIWnd macro specifies the 
*      starting and ending positions for playing a sequence and 
*      has the MCI device start playing.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LONG | lStart | Specifies the starting position for the sequence.
*
* @parm LONG | lEnd | Specifies the ending position for the sequence.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro seeks to the starting position then starts playing. 
*       Play stops when the sequence reaches the specified position 
*       (or the end of the sequence if the specified position is 
*       beyond the end of the sequence).
*
*       This macro is defined with the MCIWndSeek and MCIWndPlayTo 
*       macros. 
*
* @xref MCIWndPlay  MCIWndPlayFrom MCIWndPlayReverse MCIWndPlayTo 
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPlayTo | This MCIWnd macro specifies the ending position 
*      for playing and has the MCI device start playing.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LONG | lPos | Specifies the ending position for the sequence.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts play from the current position. Play stops 
*       when the sequence reaches the specified position (or the end 
*       of the sequence if the specified position is beyond the end 
*       of the sequence).
*
*       This macro is defined with the MCWINDM_PLAYTO message. 
*       The <p wParam> parameter is set to zero for the message,
*       and the <p lParam> parameter has the same 
*       definition as <p lPos>.
*
* @xref MCIWndPlay MCIWndPlayFrom MCIWndPlayTo MCIWndPlayFromTo
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPlayReverse | This MCIWnd macro 
*      has the MCI device start playing the sequence in reverse.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts play from the current position. Play stops 
*       when the sequence reaches the begining of the file or it is 
*       stopped.
*
* @comm  This macro is defined with the MCI_PLAYREVERSE message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndPlay MCIWndPlayFrom MCIWndPlayTo MCIWndPlayFromTo
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndStop | This MCIWnd macro has and MCI device
*      stop playing or recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCI_STOP message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPause | This MCIWnd macro has an 
*      MCI device pause playing or recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCI_PAUSE message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL
*
* @api LONG | MCIWndResume | This MCIWnd macro 
*      has an MCI device continue playing or recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCI_RESUME message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
*
* @doc EXTERNAL
*
* @api LRESULT | MCIWndCanRecord | This MCIWnd macro determines if an MCI device 
*      supports recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device supports recording. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCIWNDM_CAN_RECORD message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
************************************************************************/

/***********************************************************************
* 
* @doc EXTERNAL
*
* @api LRESULT | MCIWndCanSave | This macro determines if an MCI device 
*      can save data.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device supports saving data. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCIWNDM_CAN_SAVE message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
*
***********************************************************************/

/***********************************************************************
*
* @doc EXTERNAL
*
* @api LRESULT | MCIWndCanPlay | This macro determines if an MCI device 
*      supports playing a sequence. 
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device supports playing data. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCIWNDM_CAN_PLAY message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* 
* @doc EXTERNAL
*
* @api LRESULT | MCIWndCanEject | This macro determines if an MCI device 
*      can eject its media.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device can eject its media. Otherwise it returns 
*        an MCI error code.
*
*
* @comm  This macro is defined with the MCIWNDM_CAN_EJECT message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* 
* @doc EXTERNAL
*
* @api LRESULT | MCIWndCanConfig | This macro determines if an MCI device 
*      supports configuration.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device supports configuration. Otherwise it returns 
*        an MCI error code.
*
*
* @comm  This macro is defined with the MCIWNDM_CAN_CONFIG message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LRESULT | MCIWndCanVolume | This macro determines if an MCI device 
*      supports volume changes.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device supports changing it volume level. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
*
* @doc EXTERNAL
*
* @api LRESULT | MCIWndCanWindow | This macro determines if an MCI device 
*      supports window commands.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the device supports windowing operations. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndRecord | This MCIWnd macro has 
*      an MCI device start recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts recording from the current position.
*
* @comm  This macro is defined with the MCI_RECORD message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref  MCIWndStop MCIWndPause
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LONG | MCIWndRecordFrom | This macro specifies the starting position 
*      for recording and sends the MCI_RECORD message 
*      to an MCI device to start recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LONG | lStart | Specifies the starting position for recording.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts record from the specified position. 
*        Recording continues until it is 
*       stopped or record reaches the end of the record file.
*
* @xref MCIWndRecord MCIWndRecordTo MCIWndRecordFromTo MCIWndRecordPreview
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LONG | MCIWndRecordFromTo | This macro specifies the starting and ending 
*      positions for recording a sequence and sends the MCI_RECORD message 
*      to an MCI device to start recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LONG | lStart | Specifies the starting position for recording.
*
* @parm LONG | lEnd | Specifies the ending position for recording.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro seeks to the starting position then starts recording. 
*       Recording stops when the sequence reaches the specified position 
*       (or the end of the sequence if the specified position is 
*       beyond the end of the sequence).
*
* @xref MCIWndRecord  MCIWndRecordFrom MCIWndRecordTo MCIWndRecordPreview 
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LONG | MCIWndRecordTo | This macro specifies the ending position 
*      for recording and sends the MCI_RECORD message 
*      to an MCI device to start recording.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LONG | lPos | Specifies the ending position for recording.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro starts recording from the current position. Record stops 
*       when the sequence reaches the specified position (or the end 
*       of the sequence if the specified position is beyond the end 
*       of the sequence).
*
* @xref MCIWndRecord MCIWndRecordFrom MCIWndRecordFromTo MCIWndRecordPreview 
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LRESULT | MCIWndRecordPreview | This macro previews the input 
*      used for recording. 
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro does not initiate recording.
*
* @xref  MCIWndRecordFrom MCIWndRecordTo MCIWndRecordFromTo MCIWndRecordPreview
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSave | This MCIWnd macro has an
*      MCI device save its data.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPSTR | szFile | Specifies a zero-terminated string containing 
*       the name and path of the destination file. Specifying -1 for 
*       this parameter has the device display its dialog box for 
*       specifying the filename.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
*       This macro is defined with the MCI_SAVEE message. 
*       The <p wParam> parameter for the message is set to zero, 
*       and the <p lParam> parameter has the same 
*       definition as <p szFile>.
*
* @xref  MCIWndSaveDialog
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSaveDialog | This MCIWnd macro displays 
*      a dialog box for specifying the filename used to save data 
*      and has the MCI device save the data.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
*       This macro is defined with the MCIWNDM_SAVE message. 
*       The <p wParam> parameter for the message is set to zero, 
*       and the <p lParam> parameter is set to -1.
*
* @xref  MCIWndSave
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LRESULT | MCIWndSetFormat | This macro has an MCI 
*      device set its format data.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm HWND | lp | Specifies a far pointer to the format data buffer.
*
* @parm int | cb | Specifies the size of the format data buffer.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @xref  MCIWndGetFormat MCIWndFormatDialog
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LRESULT | MCIWndGetFormat | This macro has an MCI 
*      device return its format data.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPARAM | lp | Specifies a far pointer to the format data buffer.
*
* @parm int | cb | Specifies the size of the format data buffer.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @xref  MCIWndSetFormat MCIWndFormatDialog
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LRESULT | MCIWndFormatDialog | This macro has an MCI 
*      device display a dialog box for setting its format data.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
* @xref  MCIWndSetFormat MCIWndGetFormat
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndEject | This MCIWnd macro has an MCI 
*      device eject its media.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm  This macro is defined with the MCWINDM_EJECT message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api UINT | MCIWndGetDeviceID | This MCIWnd macro has an MCI 
*      device return its device ID.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the device ID.
*
* @comm  This macro is defined with the MCIWNDM_GETDEVICEID message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api UINT | MCIWndGetAlias | This MCIWnd macro returns the 
*      alias used to open an MCI device or device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the device alias.
*
* @comm  This macro is defined with the MCIWNDM_GETALIAS message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetMode | This MCIWnd macro has an MCI 
*      device return its mode.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPARAM | lp | Specifies a far pointer to the application-supplied 
*       buffer used to return the mode.
*
* @parm int | iLen | Specifies the size of the data buffer.
*
* @rdesc Returns an integer corresponding to the MCI constant 
*        defining the mode.
*
* @comm If the zero-terminated string describing the mode is 
*       longer than the buffer, it is truncated.
*
*       This macro is defined with the MCIWNDM_GETMODE message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iLen>, and the <p lParam> parameter has the same 
*       definition as <p lp>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetPosition | This MCIWnd macro 
*      returns the current position of the MCI device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the current position.
*
* @comm The units for the position value depend on the current time format.
*
* @comm  This macro is defined with the MCIWNDM_GETPOSITION message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetPositionString | This MCIWnd macro 
*      returns the current position of the MCI device element 
*      in a string format.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPARAM | lp | Specifies a far pointer to the application-supplied 
*       buffer used to return the zero-terminated string.
*
* @parm int | iLen | Specifies the size of the data buffer.
*
* @rdesc Returns an integer corresponding to the position.
*
* @comm If the device supports tracks, the position information 
*       is returned in the TT:MM:SS:FF format where TT corresponds 
*       to tracks, MM and SS corresponds to minutes and seconds, and
*       FF corresponds to frames. 
*
*       If the zero-terminated string is 
*       longer than the buffer, it is truncated.
*
*       This macro is defined with the MCIWNDM_GETPOSITION message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iLen>, and the <p lParam> parameter has the same 
*       definition as <p lp>.
*
*
* @comm The units for the position value depend on the current time format.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetStart | This MCIWnd macro returns 
*      the starting position of an MCI device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the starting position of the device element.
*
* @comm Typically, the return value is zero but some devices 
*       use a non-zero starting position for the device 
*       element. Seeking to this position sets the device 
*       to the start of the media.
*
*       This macro is defined with the MCIWNDM_GETSTART message. 
*       The <p wParam> and <p lParam> parameters for the message 
*       are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetLength | This MCIWnd macro returns 
*      the length of an MCI device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the length of the device element.
*
* @comm This value plus the value returned for <f MCIWndGetStart> 
*       equals the end of the media.
*
* @comm  This macro is defined with the MCIWNDM_GETLENGTH message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetEnd | This MCIWnd macro returns
*      the ending position of an MCI device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the ending position.
*
* @comm  This macro is defined with the MCIWNDM_GETEND message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndStep | This MCIWnd macro steps 
*      an MCI device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPARAM | n | Specifies the number of frames or 
*       milliseconds to step. Negative 
*       values step the device element in reverse.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm The units for the step value depend on the current time format.
*
* @comm  This macro is defined with the MCI_STEP message. 
*        The <p wParam> parameter is set to zero and the <p lParam> 
*        has the same definition as <p n>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSetVolume | This MCIWnd macro sets the 
*      volume level of an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm int | iVol | Specifies the volume level. Specify 1000 
*       for the normal volume level. Specify larger values to increase 
*       the volume level and smaller values to decrease the volume level.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro is defined with the MCIWNDM_SETVOLUME message. 
*       The <p wParam> parameter for the message is set to zero,
*       and the <p lParam> parameter has the same 
*       definition as <p iVol>.
*
* @xref  MCIWndGetVolume
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetVolume | This MCIWnd macro returns 
*      volume level of an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the volume level.
*
* @comm  This macro is defined with the MCIWNDM_GETVOLUME message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref  MCIWndSetVolume
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSetSpeed | This MCIWnd macro sets 
*      the playback speed of an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm int | iSpeed | Specifies the playback speed. Specify 1000 
*       for the normal speed. Specify larger values for faster speeds 
*       and smaller values for slower speeds.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro is defined with the MCIWNDM_SETSPEED message. 
*       The <p wParam> parameter for the message is set to zero,
*       and the <p lParam> parameter has the same 
*       definition as <p iSpeed>.
*
* @xref  MCIWndGetSpeed
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetSpeed | This MCIWnd macro returns the 
*      playback speed of an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns the playback speed.
*
* @comm  This macro is defined with the MCIWNDM_GETSPEED message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref  MCIWndSetSpeed
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSetTimeFormat | This MCIWnd macro 
*      specifies time fomat used by an MCI  device.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPSTR | lp | Specifies a far pointer to a data buffer 
*       containing the zero-terminated string indicating the time format.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm Application can specify formats other than frames and 
*       milliseconds if they are supported by the MCI device. 
*       Non-continuous formats, such as tracks and 
*       SMPTE can cause the scroll bar to behave erratically. 
*       For these time formats, applications might change the 
*       window style by using MCIWNDF_NOPLAYBAR to turn off the 
*       playbar.
*
*       This macro is defined with the MCIWNDM_SETTIMEFORMAT message. 
*       The <p wParam> parameter for the message is set to zero, 
*       and the <p lParam> parameter has the same 
*       definition as <p lp>.
*
* @xref  MCIWndGetTimeFormat MCIWndUseFrames MCIWndUseTime
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndUseFrames | This MCIWnd macro has an MCI 
*      device set its time format to frames.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
*       This macro is defined with the MCIWNDM_SETTIMEFORMAT message. 
*       The <p wParam> parameter for the message is set to zero, 
*       and the <p lParam> parameter is set to "frames".
*
* @xref  MCIWndSetTimeFormat 
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndUseTime | This MCIWnd macro has an MCI 
*      device set its time format to milliseconds.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
*       This macro is defined with the MCIWNDM_SETTIMEFORMAT message. 
*       The <p wParam> parameter for the message is set to zero, 
*       and the <p lParam> parameter is set to "ms".
*
* @xref  MCIWndSetTimeFormat 
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetTimeFormat | This macro returns the time 
*      format of an MCI device in an application-supplied buffer.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LPARAM | lp | Specifies a far pointer to the data buffer used 
*       to return the zero-terminated string indicating time format.
*
* @parm int | iLen | Specifies the size of the data buffer.
*
* @rdesc Returns an integer corresponding to the MCI constant 
*        defining the time format.
*
* @comm If the time format string is longer than the return buffer, 
*       MCIWnd truncates the string.
*
*      This macro is defined with the MCIWNDM_GETTIMEFORMAT message. 
*      The <p wParam> parameter for the message has the same 
*      definition as <p iLen>, and the <p lParam> parameter has the same 
*      definition as <p lp>.
*
* @xref  MCIWndSetTimeFormat
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api VOID | MCIWndSetActiveTimer | This MCIWnd macro sets the 
*      period used by MCIWnd to update the scrollbar, 
*      position information displayed in the window caption, 
*      and send notification messages to the parent window 
*      when the MCIWnd window is the active window.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm int | iActive | Specifies an integer timer period in milliseconds.
*
* @comm This macro is defined with the MCIWNDM_SETACTIVETIMER message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iACTIVE>, and the <p lParam> parameter 
*       is set to zero.
*
* @xref  MCIWndSetInactiveTimer MCIWndSetTimers
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api VOID | MCIWndSetInactiveTimer | This MCIWnd macro sets 
*      the period used by MCIWnd to update the scrollbar, 
*      position information displayed in the window caption, 
*      and send notification messages to the parent window 
*      when the MCIWnd window is the inactive window.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm int | iInactive | Specifies an integer timer period in milliseconds.
*
* @comm This macro is defined with the MCIWNDM_SETINACTIVETIMER message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iInactive>, and the <p lParam> parameter 
*       is set to zero.
*
* @xref  MCIWndSetActiveTimer MCIWndSetTimers
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api VOID | MCIWndSetTimers | This MCIWnd macro sets the time 
*      periods used by MCIWnd to update the scrollbar, position 
*      information displayed in the window caption, 
*      and send notification messages to the parent window.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm int | iActive | Specifies an integer timer period used 
*       by MCIWnd when it is the active window. This value 
*       is limited to 16 bits.
*
* @parm int | iInactive | Specifies an integer timer period used 
*       by MCIWnd when it is the inactive window. This value 
*       is limited to 16 bits.
*
*       This macro is defined with the MCIWNDM_SETTIMERS message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iActive>, and the <p lParam> parameter has the same 
*       definition as <p iInactive>.
*
* @xref  MCIWndSetActiveTimer MCIWndSetInactiveTimer
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api UINT | MCIWndGetActiveTimer | This MCIWnd macro returns 
*      the timer period used when the MCIWnd window is 
*      the active window.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns the update period in milliseconds. 
*        The default is 500 milliseconds.
*
* @comm  This macro is defined with the MCIWNDM_GETACTIVETIMER message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref  MCIWndSetActiveTimer MCIWndSetTimers MCIWndGetInactiveTimer
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api UINT | MCIWndGetInactiveTimer | This MCIWnd macro returns 
*      the timer period used when the MCIWnd window is 
*      the inactive window.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns the timer period in milliseconds. 
*        The default is 2000 milliseconds.
*
* @comm  This macro is defined with the MCIWNDM_GETINACTIVERTIMER message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref  MCIWndSetInactiveTimer MCIWndSetTimers MCIWndGetActiveTimer
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetError | This MCIWnd macro returns the last MCI 
*      error encountered.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LPARAM | lp | Specifies a far pointer to an application-supplied 
*       buffer used to return the error string.
*
* @parm int | iLen | Specifies the size of the error data buffer.
*
* @rdesc Returns the integer error code.
*
*@comm If <p lp> is a valid pointer, a zero-terminated string 
*      corresponding to the error is returned in its buffer. If 
*      the error string is longer than the buffer, MCIWnd truncates it.
*
*      This macro is defined with the MCIWNDM_GETERROR message. 
*      The <p wParam> parameter for the message has the same 
*      definition as <p iLen>, and the <p lParam> parameter has the same 
*      definition as <p lp>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api HPALETTE | MCIWndGetPalette | This MCIWnd macro returns 
*      a handle to the palette used by an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @rdesc Returns the handle to the palette.
*
* @comm  This macro is defined with the MCIWNDM_GETPALETTE message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref  MCIWndSetPalette
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndSetPalette | This MCIWnd macro sends a palette handle 
*      to an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm HPALETTE | hpal | Specifies the handle to the palette.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm This macro is defined with the MCIWNDM_SETPALETTE message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p hpal>, and the <p lParam> parameter 
*       is set to zero.
*
* @xref  MCIWndGetPalette
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetFileName | This MCIWnd macro returns 
*      the filename used by an MCI device element.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPARAM | lp | Specifies a far pointer to the 
*       application-supplied data buffer used 
*       to return the filename.
*
* @parm int | iLen | Specifies the size of the data buffer.
*
* @rdesc Returns zero if successful. Otherwise it returns one.
*
* @comm If the zero-terminated string containing the filename is 
*       longer than the buffer, MCIWnd truncates it.
*
*       This macro is defined with the MCIWNDM_GETFILENAME message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p iLen>, and the <p lParam> parameter has the same 
*       definition as <p lp>.
*
***********************************************************************/


/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetDevice | This MCIWnd macro returns the 
*      name of the currently opened MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm LPARAM | lp | Specifies a far pointer to the application-supplied 
*       data buffer used to return the device name.
*
* @parm int | len | Specifies the size of the data buffer.
*
* @rdesc Returns zero if successful. Otherwise it returns 42.
*
* @comm If the zero-terminated string containing the device name is 
*       longer than the buffer, MCIWnd truncates it.
*
*       This macro is defined with the MCIWNDM_GETDEVICE message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p len>, and the <p lParam> parameter has the same 
*       definition as <p lp>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api UINT | MCIWndGetStyles | This MCIWnd macro returns 
*      the flags specifying the styles used 
*      for its window.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns the flags specifying the styles used for 
*        the window. This value is the bitwise or of the MCIWNDF flags.
*
* @comm  This macro is defined with the MCIWNDM_GETSTYLES message. 
*        The <p wParam> and <p lParam> parameters for the message 
*        are set to zero.
*
* @xref MCIWndChangeStyles MCIWndCreate
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndChangeStyles | This MCIWnd macro changes 
*      the styles used for its window.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm int | mask | Specifies a mask for <p value>. This 
*       mask is the bitwise or of all styles that will be 
*       permitted to change.
*
* @parm int | value | Specifies the styles used for the window.
*       See <f MCIWndCreate> for the available styles.
*
* @rdesc Returns zero.
*
* @comm  This macro is defined with the MCIWNDM_CHANGESTYLES message. 
*        The <p wParam> parameter for the message has the same 
*        definition as <p mask>, and the <p lParam> parameter has the same 
*        definition as <p value>.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api VOID | MCIWndValidateMedia | This macro has MCIWnd synchronize 
*      its position information to the new media when it has changed. 
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @comm If your application changes the time format of a 
*       device without using MCIWnd, the starting and ending 
*       position of the media, as well as the trackbar continue to 
*       use the old format. Use the <f MCIWndValidateMedia> 
*       macro to update these values. Normally, you should not 
*       need to use this macro. 
*
*      This macro is defined with the MCIWNDM_VALIDATEMEDIA message. 
*      The <p wParam> and <p lParam> parameters for the message 
*      are set to zero.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api LONG | MCIWndGetRepeat | This macro obtains the number 
*      of times the MCI device will repeat a sequence.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns an integer indicating the number of times 
*        the MCI device will repeat a sequence.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndRealize | This MCIWnd macro has an MCI 
*      device realize its pallette.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm BOOL | fBkgnd | Set to TRUE to if the window is 
*       a background application.
*
* @rdesc Returns zero if successful. Otherwise it returns 
*        an MCI error code.
*
* @comm Call this macro from the WM_PALETTECHANGED 
*       and WM_QUERYNEWPALETTE message handlers instead of using 
*       <f RealizePalette>.  This MCIWnd macro uses 
*       the palette of the MCI device and calls <f RealizePalette>. 
*       If realizing the palette is the only purpose of your 
*       WM_PALETTECHANGED and WM_QUERYNEWPALETTE message handlers, 
*       just pass the WM_PALETTECHANGED and WM_QUERYNEWPALETTE messages 
*       on to the MCI window and it will automatically realize the palette.
*
*       This macro is defined with the MCIWNDM_REALIZE message. 
*       The <p wParam> parameter for the message has the same 
*       definition as <p fBkgnd>, and the <p lParam> parameter 
*       is set to zero.
*
***********************************************************************/

/***********************************************************************
*
* @doc INTERNAL
*
* @api VOID | MCIWndActivate | This macro has an MCI 
*      device activate its window.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm int | f | Specifies applicable flags.
*
* @rdesc Returns TRUE if successful. Otherwise it returns 
*        an MCI error code.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPutSource | This MCIWnd macro specifies the 
*      source rectangle for an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm RECT | prc | Specifies a rectangle indicating image 
*       source.
*
* @rdesc Returns zero if successful. Otherwise it returns an error.
*
* @comm  This macro is defined with the MCIWNDM_PUT_SOURCE message. 
*        The <p wParam> parameter is set to zero and the <p lParam> 
*        has the same definition as <p prc>.
*
* @xref MCIWndGetSource
*
***********************************************************************/
  
/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetSource | This MCIWnd macro has an MCI device return 
*      the rectangle used for the source of the image.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPRECT | prc | Specifies a pointer to a <t RECT> data type 
*       used to return the source rectangle.
*
* @rdesc Returns zero if successful. Otherwise it returns an error.
*
* @comm  This macro is defined with the MCIWNDM_GET_SOURCE message. 
*        The <p wParam> parameter is set to zero and the <p lParam> 
*        has the same definition as <p prc>.
*
* @xref MCIWndPutSource
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndPutDest | This MCIWnd macro specifies the 
*      destination rectangle for an MCI device.
*
* @parm HWND | hwnd | Specifies the handle to the MCIWnd window.
*
* @parm RECT | prc | Specifies a rectangle indicating image 
*       destination.
*
* @rdesc Returns zero if successful. Otherwise it returns an error.
*
* @comm  This macro is defined with the MCIWNDM_PUT_DEST message. 
*        The <p wParam> parameter is set to zero and the <p lParam> 
*        has the same definition as <p prc>.
*
* @xref MCIWndGetDest
*
***********************************************************************/
  
/***********************************************************************
* @doc EXTERNAL 
*
* @api LONG | MCIWndGetDest | This MCIWnd macro has an MCI device return 
*      the rectangle used for the image destination.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm LPRECT | prc | Specifies a pointer to a <t RECT> data 
*       type used to return the destination rectangle.
*
* @rdesc Returns zero if successful. Otherwise it returns an error.
*
* @comm  This macro is defined with the MCIWNDM_GET_DEST message. 
*        The <p wParam> parameter is set to zero and the <p lParam> 
*        has the same definition as <p prc>.
*
* @xref MCIWndPutDest
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api void | MCIWndSetRepeat | This MCIWnd macro has an MCIWnd 
*      repeat a play sequence subsequently specified with an 
*      MCIWndPlay macro.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @rdesc Returns TRUE if the flag is set.
*
* @comm This macro is defined with the MCIWNDM_SETREPEAT message. 
*       The <p wParam> and <p lParam> parameters are set to zero.
*
***********************************************************************/

/***********************************************************************
* @doc EXTERNAL 
*
* @api BOOL | MCIWndGetRepeat | This MCIWnd macro returns the status 
*      of the MCIWnd repeat flag.
*
* @parm HWND | hwnd | Specifies the handle to the MCI window.
*
* @parm BOOL | f | Set to TRUE to if MCIWnd is to continuously repeat 
*       a sequence.
*
* @comm This macro is defined with the MCIWNDM_SETREPEAT message. 
*       The <p wParam> parameter for the message is set to zero,
*       and the <p lParam> parameter has the same definition 
*       as <p f>.
*
***********************************************************************/

 
