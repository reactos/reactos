#define SOFTPLG_MAGIC '$SOF'

typedef struct _SOF_TPLG {
	UINT32 magic;
	UINT32 length;
	char speaker_tplg[32];
	char hp_tplg[32];
	char dmic_tplg[32];
} SOF_TPLG, *PSOF_TPLG;

NTSTATUS GetSOFTplg(WDFDEVICE FxDevice, SOF_TPLG *sofTplg);