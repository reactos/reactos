#ifndef __GENDRV_H__
#define __GENDRV_H__

#define GENDRV_MAX_EXT_DRVR  		2
#define GENDRV_DRVR_FLAG_IF_DRVR	0x01

#define GENDRV_MSG_ADDDEVICE		0x01
#define GENDRV_MSG_STOPDEVICE		0x02
#define GENDRV_MSG_DISCDEVICE		0x03

typedef struct _GENDRV_EXT_DRVR_ENTRY
{
	LIST_ENTRY				drvr_link;	// used for dynamic load/unload driver
	ULONG					drvr_key;	// used to find the driver loaded, the key
	                                    // is ( vendor_id << 16 )|( product_id ) or
										// ( class_id << 16 ) | ( sub_class_id << 8 ) | ( protocol_id ))
	PDRIVER_OBJECT			pext_drvr;	// the external driver count
	ULONG					ref_count;	// number of devices attached
	LIST_HEAD				dev_list;	// link the devices by deviceExtension->dev_obj_link

} GENDRV_EXT_DRVR_ENTRY, *PGENDRV_EXT_DRVR_ENTRY;

typedef struct _GENDRV_DRVR_EXTENSION
{
	FAST_MUTEX				drvr_ext_mutex;
	ULONG					ext_drvr_count; // loaded driver count
	LIST_HEAD				ext_drvr_list;
	GENDRV_EXT_DRVR_ENTRY	ext_drvr_array[ GENDRV_MAX_EXT_DRVR ];

}GENDRV_DRVR_EXTENSION, *PGENDRV_DRVR_EXTENSION;

typedef struct _GENDRV_IF_CTX
{
	UCHAR					if_idx;			// valid for if device
	PUSB_INTERFACE_DESC     pif_desc;

} GENDRV_IF_CTX, *PGENDRV_IF_IDX;

typedef struct _GENDRV_DEVICE_EXTENSION
{
	//this structure is the device extension for dev_obj
	//created for the device.
	DEVEXT_HEADER			dev_ext_hdr;
	ULONG					flags;
	LIST_ENTRY				dev_obj_link;	// this link is used by the driver object to track the existing dev_objs
	PGENDRV_EXT_DRVR_ENTRY	ext_drvr_entry;

	PDEVICE_OBJECT  		pdo;			// Our device object
	DEV_HANDLE				dev_handle;		// handle to the usb_dev under
	PUCHAR					desc_buf;
	UCHAR					dev_id;			// used to build symbolic link
	GENDRV_IF_CTX			if_ctx;

	KEVENT					sync_event;
	KSPIN_LOCK				dev_lock;

	PUSB_DRIVER				pdriver;
	PUSB_DEV_MANAGER		dev_mgr;

} GENDRV_DEVICE_EXTENSION, *PGENDRV_DEVICE_EXTENSION;

#endif
