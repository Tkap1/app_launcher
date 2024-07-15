
global constexpr int c_num_channels = 2;
global constexpr int c_sample_rate = 44100;
global constexpr int c_max_concurrect_sounds = 16;

#pragma pack(push, 1)
struct s_riff_chunk
{
	u32 chunk_id;
	u32 chunk_size;
	u32 format;
};

struct s_fmt_chunk
{
	u32 sub_chunk1_id;
	u32 sub_chunk1_size;
	u16 audio_format;
	u16 num_channels;
	u32 sample_rate;
	u32 byte_rate;
	u16 block_align;
	u16 bits_per_sample;
};

struct s_data_chunk
{
	u32 sub_chunk2_id;
	u32 sub_chunk2_size;
};
#pragma pack(pop)

struct s_sound
{
	int sample_count;
	s16* samples;

	int get_size_in_bytes() { return c_num_channels * sizeof(s16) * sample_count; }
};

#pragma warning(push)
#pragma warning(disable : 4100) // @Note(tkap, 20/05/2024): Disable unused arguments
struct s_voice : IXAudio2VoiceCallback
{
	volatile int playing;
	IXAudio2SourceVoice* voice;
	void OnStreamEnd()
	{
		InterlockedExchange((LONG*)&playing, 0);
	}
	void OnVoiceProcessingPassEnd() { }
	void OnVoiceProcessingPassStart(UINT32 SamplesRequired) {    }
	void OnBufferEnd(void * pBufferContext)    { }
	void OnBufferStart(void * pBufferContext) {    }
	void OnLoopEnd(void * pBufferContext) {    }
	void OnVoiceError(void * pBufferContext, HRESULT Error) { }
};
#pragma warning(pop)

struct s_xaudio
{
	IXAudio2* xaudio;
	IXAudio2MasteringVoice* master_voice;
	s_carray<s_voice, c_max_concurrect_sounds> voice_arr;
	s_sound pop;
};

func s_sound load_wav(char* path, s_lin_arena* arena);