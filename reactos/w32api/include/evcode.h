#ifndef __EVCODE__
#define __EVCODE__

#define EC_SYSTEMBASE                0x00
#define EC_COMPLETE                  0x01
#define EC_USERABORT                 0x02
#define EC_ERRORABORT                0x03
#define EC_TIME                      0x04
#define EC_REPAINT                   0x05
#define EC_STREAM_ERROR_STOPPED      0x06
#define EC_STREAM_ERROR_STILLPLAYING 0x07
#define EC_ERROR_STILLPLAYING        0x08
#define EC_PALETTE_CHANGED           0x09
#define EC_VIDEO_SIZE_CHANGED        0x0A
#define EC_QUALITY_CHANGE            0x0B
#define EC_SHUTTING_DOWN             0x0C
#define EC_CLOCK_CHANGED             0x0D
#define EC_PAUSED                    0x0E
#define EC_OPENING_FILE	             0x10
#define EC_BUFFERING_DATA            0x11
#define EC_FULLSCREEN_LOST           0x12
#define EC_ACTIVATE                  0x13
#define EC_NEED_RESTART              0x14
#define EC_WINDOW_DESTROYED          0x15
#define EC_DISPLAY_CHANGED           0x16
#define EC_STARVATION                0x17
#define EC_OLE_EVENT                 0x18
#define EC_NOTIFY_WINDOW             0x19
#define EC_STREAM_CONTROL_STOPPED    0x1A
#define EC_STREAM_CONTROL_STARTED    0x1B
#define EC_END_OF_SEGMENT            0x1C
#define EC_SEGMENT_STARTED           0x1D
#define EC_LENGTH_CHANGED            0x1E
#define EC_DEVICE_LOST               0x1F
#define EC_STEP_COMPLETE             0x24
#define EC_SKIP_FRAMES               0x25

#define EC_TIMECODE_AVAILABLE	     0x30
#define EC_EXTDEVICE_MODE_CHANGE     0x31
#define EC_GRAPH_CHANGED             0x50
#define EC_CLOCK_UNSET               0x51 
#define EC_WMT_EVENT_BASE            0x0251
#define EC_WMT_INDEX_EVENT           EC_WMT_EVENT_BASE
#define EC_USER                      0x8000

#endif
