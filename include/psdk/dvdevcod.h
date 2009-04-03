
#ifndef __DVDEVCOD__
#define __DVDEVCOD__

#define EC_DVDBASE                        0x0100
#define EC_DVD_DOMAIN_CHANGE              (EC_DVDBASE + 0x01)
#define EC_DVD_TITLE_CHANGE               (EC_DVDBASE + 0x02)
#define EC_DVD_CHAPTER_START              (EC_DVDBASE + 0x03)
#define EC_DVD_AUDIO_STREAM_CHANGE        (EC_DVDBASE + 0x04)
#define EC_DVD_SUBPICTURE_STREAM_CHANGE   (EC_DVDBASE + 0x05)
#define EC_DVD_ANGLE_CHANGE               (EC_DVDBASE + 0x06)
#define EC_DVD_BUTTON_CHANGE              (EC_DVDBASE + 0x07)
#define EC_DVD_VALID_UOPS_CHANGE          (EC_DVDBASE + 0x08)
#define EC_DVD_STILL_ON                   (EC_DVDBASE + 0x09)
#define EC_DVD_STILL_OFF                  (EC_DVDBASE + 0x0A)
#define EC_DVD_CURRENT_TIME               (EC_DVDBASE + 0x0B)
#define EC_DVD_ERROR                      (EC_DVDBASE + 0x0C)
#define EC_DVD_WARNING                    (EC_DVDBASE + 0x0D)
#define EC_DVD_CHAPTER_AUTOSTOP           (EC_DVDBASE + 0x0E)
#define EC_DVD_NO_FP_PGC                  (EC_DVDBASE + 0x0F)
#define EC_DVD_PLAYBACK_RATE_CHANGE       (EC_DVDBASE + 0x10)
#define EC_DVD_PARENTAL_LEVEL_CHANGE      (EC_DVDBASE + 0x11)
#define EC_DVD_PLAYBACK_STOPPED           (EC_DVDBASE + 0x12)
#define EC_DVD_ANGLES_AVAILABLE           (EC_DVDBASE + 0x13)
#define EC_DVD_PLAYPERIOD_AUTOSTOP        (EC_DVDBASE + 0x14)
#define EC_DVD_BUTTON_AUTO_ACTIVATED      (EC_DVDBASE + 0x15)
#define EC_DVD_CMD_START                  (EC_DVDBASE + 0x16)
#define EC_DVD_CMD_END                    (EC_DVDBASE + 0x17)
#define EC_DVD_DISC_EJECTED               (EC_DVDBASE + 0x18)
#define EC_DVD_DISC_INSERTED              (EC_DVDBASE + 0x19)
#define EC_DVD_CURRENT_HMSF_TIME          (EC_DVDBASE + 0x1A)
#define EC_DVD_KARAOKE_MODE               (EC_DVDBASE + 0x1B)


#ifndef EXCLUDE_DVDEVCODE_ENUMS
typedef enum _tagDVD_WARNING
{
        DVD_WARNING_InvalidDVD1_0Disc  =1,
        DVD_WARNING_FormatNotSupported =2,
        DVD_WARNING_IllegalNavCommand  =3,
        DVD_WARNING_Open               =4,
        DVD_WARNING_Seek               =5,
        DVD_WARNING_Read               =6
} DVD_WARNING;

typedef enum _tagDVD_ERROR
{
        DVD_ERROR_Unexpected                              =1,
        DVD_ERROR_CopyProtectFail                         =2,
        DVD_ERROR_InvalidDVD1_0Disc                       =3,
        DVD_ERROR_InvalidDiscRegion                       =4,
        DVD_ERROR_LowParentalLevel                        =5,
        DVD_ERROR_MacrovisionFail                         =6,
        DVD_ERROR_IncompatibleSystemAndDecoderRegions     =7,
        DVD_ERROR_IncompatibleDiscAndDecoderRegions       =8

} DVD_ERROR;

#endif
#endif
