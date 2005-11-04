typedef struct {

	void *frontBuffer;
	void *backBuffer;
	void *currentBuffer;
	int currentPitch;

	int depthCpp;
	void *depthBuffer;
	int depthPitch;

} x11ScreenPrivate;
