#ifndef _INCLUDE_SMEXT_I_VOICETOOLS_H_
#define _INCLUDE_SMEXT_I_VOICETOOLS_H_

#include <IShareSys.h>

#define SMINTERFACE_VOICETOOLS_NAME "IVoiceTools"
#define SMINTERFACE_VOICETOOLS_VERSION 1

class IVoiceTools : public SourceMod::SMInterface
{
public:
	virtual const char *GetInterfaceName() = 0;
	virtual unsigned int GetInterfaceVersion() = 0;
public:
	virtual bool SendVoiceDataToClient(int sender, int client, const char *data, int size) = 0;
	virtual int Compress(const char *data, int frameSize, char *compressed, int maxCompressedSize) = 0;
	virtual int Decompress(const char *data, int compressedBytes, char *uncompresssed, int maxUncompressedBytes) = 0;
};

#endif /* _INCLUDE_SMEXT_I_VOICETOOLS_H_ */