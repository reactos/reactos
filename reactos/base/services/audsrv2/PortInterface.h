#ifndef __PORTINTERFACE_H
#define __PORTINTERFACE_H
class PortInterfaceCallback 
	{
public:
	virtual void PortInterfaceOpenComplete(int * error) = 0;
	virtual void PortInterfaceBufferCopied(int * error, const char * buffer) = 0;
	virtual void PortInterfacePlayComplete(int * error) = 0;
	};

class CStreamSettings
{
	int samplespersec;
	int channels;
	int bitspersample;
	double freq;
	char fourcc[4];
}

class CPortInterface 
	{
public:
	IMPORT_C static CPortInterface* NewL(PortInterfaceCallback * aCallBack);
	~CPortInterface();
	virtual void SetAudioPropertiesL(int aSampleRate, int aChannels);
	virtual void Open(CStreamSettings * aSettings);
	virtual int MaxVolume();
	virtual int Volume();
	virtual void SetVolume(const int aNewVolume);
	virtual void WriteL(const char * aData);
	virtual void Stop();
	IMPORT_C void SetBalanceL(float aBalance = 0);
	IMPORT_C TInt GetBalanceL() const;
private:
	CStreamSettings * settings;
	};

#endif
