#pragma once

/* GDI objects from the handle table */
#define TAG_BRUSHOBJ    'OHRB' /* brush object */
#define TAG_CLIP        'PILC' /* clipping */
#define TAG_CLIPOBJ     'OPLC' /* clip object */
#define TAG_COLORMAP    'MLOC' /* color map */
#define TAG_DC          '  CD' /* DC */
#define TAG_DFSM        'msfD' /* Eng event allocation */
#define TAG_DIB         ' BID' /* dib */
#define TAG_DRIVER      'VRDG' /* video drivers */
#define TAG_GDIHNDTBLE  'HIDG' /* gdi handle table */
#define TAG_GSEM        'mesG' /* Gdi Semaphore */
#define TAG_STRING      ' RTS'
#define TAG_SURFOBJ     'OFRS' /* surface object */
#define TAG_PALETTE     '8alG'
#define TAG_RTLREGISTRY 'vrqR' /* RTL registry */
#define TAG_XLATEOBJ    'OALX' /* xlate object */
#define TAG_PENSTYLES   'ytsG' /* pen styles */
#define TAG_BRUSH       '@alG'
#define TAG_BITMAP      'PMTB' /* bitmap */

/* Official tags */
#define GDITAG_RBRUSH   'rbdG'
#define USERTAG_PROCESSINFO              'ipsU'
#define USERTAG_THREADINFO               'itsU'
