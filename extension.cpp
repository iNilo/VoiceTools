#include "extension.h"

#include "inetmessage.h"
#include "inetchannel.h"
#include "ivoicecodec.h"

#include "protobuf/netmessages.pb.h"

#include <cstdio>

#define QUALITY 3

VoiceTools g_Tools;

SMEXT_LINK(&g_Tools);

IVoiceCodec *voiceCodec = NULL;

class CNetMessagePB_VoiceData : public INetMessage, public CSVCMsg_VoiceData
{
	virtual void	SetNetChannel(INetChannel * netchan) {}
	virtual void	SetReliable(bool state) {}

	virtual bool	Process(void) { return false; }

	virtual	bool	ReadFromBuffer(bf_read &buffer) { return false;  }
	virtual	bool	WriteToBuffer(bf_write &buffer) {
		int bytesSize = this->ByteSize();
		char *buf = new char[bytesSize];
		if (!this->SerializeToArray(buf, bytesSize)) {
			delete buf;
			return false;
		}
		buffer.WriteVarInt32(this->GetType());
		buffer.WriteVarInt32(bytesSize);
		buffer.WriteBytes(buf, bytesSize);
		
		delete buf;
		return !buffer.IsOverflowed();
	}

	virtual bool	IsReliable(void) const {
		return false;
	}

	virtual int				GetType(void) const {
		return 15;
	}
	virtual int				GetGroup(void) const {
		return -1;
	}
	virtual const char		*GetName(void) const {
		return "";
	}
	virtual INetChannel		*GetNetChannel(void) const {
		return NULL;
	}
	virtual const char		*ToString(void) const {
		return "";
	}
	virtual size_t			GetSize() const {
		return this->ByteSize();
	}
};

HMODULE lib = NULL;

const char* VoiceTools::GetInterfaceName()
{
	return SMINTERFACE_VOICETOOLS_NAME;
}

unsigned int VoiceTools::GetInterfaceVersion()
{
	return SMINTERFACE_VOICETOOLS_VERSION;
}

int VoiceTools::Compress(const char *data, int frameSize, char *compressed, int maxCompressedSize)
{
	m_Mutex->Lock();
	int wroted = voiceCodec->Compress(data, frameSize, compressed, maxCompressedSize, true);
	m_Mutex->Unlock();
	return wroted;
}

int VoiceTools::Decompress(const char *data, int compressedBytes, char *uncompresssed, int maxUncompressedBytes)
{
	return voiceCodec->Decompress(data, compressedBytes, uncompresssed, maxUncompressedBytes);
}

// TODO: DATA RACE?
bool VoiceTools::SendVoiceDataToClient(int sender, int client, const char *data, int size)
{
	CNetMessagePB_VoiceData sample;
	
	sample.set_client(sender);
	sample.set_proximity(false);
	sample.set_xuid(0.0);
	sample.set_audible_mask(0);
	sample.set_caster(false);
	auto voice_data = sample.mutable_voice_data();
	voice_data->assign(data, size);

	auto edict = gamehelpers->EdictOfIndex(client);
	if (!edict || edict->IsFree()) {
		return false;
	}
	auto player = playerhelpers->GetGamePlayer(edict);
	if (!player || !player->IsInGame() || player->IsFakeClient()) {
		return false;
	}

	INetChannel *pNetChannel = static_cast<INetChannel *>(engine->GetPlayerNetInfo(client));
	if (!pNetChannel) {
		return false;
	}
	return pNetChannel->SendNetMsg(sample, false, true);
}

bool VoiceTools::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	char path[PLATFORM_MAX_PATH];
	g_pSM->BuildPath(Path_Game, path, sizeof(path), "../bin/vaudio_celt.dll");
	lib = LoadLibrary(path);
	if (!lib) {
		snprintf(error, maxlength, "LoadLibrary failed.");
		return false;
	}
	CreateInterfaceFn createCodecFn;
	createCodecFn = (CreateInterfaceFn)GetProcAddress(lib, "CreateInterface");
	if (!createCodecFn) {
		snprintf(error, maxlength, "GetProcAddress failed.");
		return false;
	}
	voiceCodec = (IVoiceCodec*)createCodecFn("vaudio_celt", NULL);
	if (!voiceCodec) {
		snprintf(error, maxlength, "Create Voice VoiceCodec error");
		return false;
	}
	if (!voiceCodec->Init(QUALITY)) {
		snprintf(error, maxlength, "Voice VoiceCodec Init error");
		return false;
	}

	sharesys->AddInterface(myself, this);
	m_Mutex = threader->MakeMutex();

	return true;
}

void VoiceTools::SDK_OnUnload()
{
	m_Mutex->DestroyThis();
	voiceCodec->Release();
	FreeLibrary(lib);
}
