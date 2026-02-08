#define SOFTPLG_MAGIC '$SOF'

#ifdef __REACTOS__
// Downgrade unsupported NT6.2+ features.
#define NonPagedPoolNx NonPagedPool
#define NonPagedPoolNxCacheAligned NonPagedPoolCacheAligned
#endif

typedef struct _SOF_TPLG {
	UINT32 magic;
	UINT32 length;
	char speaker_tplg[32];
	char hp_tplg[32];
	char dmic_tplg[32];
} SOF_TPLG, *PSOF_TPLG;

NTSTATUS GetSOFTplg(WDFDEVICE FxDevice, SOF_TPLG *sofTplg);
