

func b8 init_audio(s_lin_arena* arena)
{
	arena->push();

	b8 result = false;
	HRESULT hr;

	hr = CoInitializeEx(null, COINIT_MULTITHREADED);
	b8 do_create = false;
	if(!FAILED(hr)) {
		do_create = true;
	}

	b8 do_mastering_voice = false;
	if(do_create) {
		hr = XAudio2Create(&xaudio.xaudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
		if(!FAILED(hr)) {
			do_mastering_voice = true;
		}
	}

	b8 do_load = false;
	if(do_mastering_voice) {
		hr = xaudio.xaudio->CreateMasteringVoice(&xaudio.master_voice);
		if(!FAILED(hr)) {
			do_load = true;
			xaudio.master_voice->SetVolume(0.1f);
		}
	}

	if(do_load) {
		WAVEFORMATEX wf = zero;
		wf.wFormatTag = WAVE_FORMAT_PCM;
		wf.nChannels = c_num_channels;
		wf.nSamplesPerSec = c_sample_rate;
		wf.wBitsPerSample = 16;
		wf.nBlockAlign = c_num_channels * (wf.wBitsPerSample / 8);
		wf.nAvgBytesPerSec = c_sample_rate * wf.nBlockAlign;

		xaudio.pop = load_wav("pop.wav", arena);

		result = true;

		for(int i = 0; i < c_max_concurrect_sounds; i++) {
			hr = xaudio.xaudio->CreateSourceVoice(&xaudio.voice_arr[i].voice, &wf, 0, XAUDIO2_DEFAULT_FREQ_RATIO, &xaudio.voice_arr[i]);
			if(FAILED(hr)) {
				result = false;
				break;
			}
			xaudio.voice_arr[i].voice->Start(0);
		}
	}

	arena->pop();

	return result;
}

func s_sound load_wav(char* path, s_lin_arena* arena)
{

	s_sound result = zero;
	u8* data = (u8*)read_file_quick(path, arena);
	if(!data) { return zero; }

	s_riff_chunk riff = *(s_riff_chunk*)data;
	data += sizeof(riff);
	s_fmt_chunk fmt = *(s_fmt_chunk*)data;
	assert(fmt.num_channels == c_num_channels);
	assert(fmt.sample_rate == c_sample_rate);
	data += sizeof(fmt);
	s_data_chunk data_chunk = *(s_data_chunk*)data;
	if(memcmp(&data_chunk.sub_chunk2_id, "LIST", 4) == 0) {
		int to_skip = *(int*)(data + 4) + 8;
		data += to_skip;
		data_chunk = *(s_data_chunk*)data;
	}
	assert(memcmp(&data_chunk.sub_chunk2_id, "data", 4) == 0);
	data += 8;

	result.sample_count = data_chunk.sub_chunk2_size / c_num_channels / sizeof(s16);
	result.samples = (s16*)malloc(c_num_channels * sizeof(s16) * result.sample_count);
	memcpy(result.samples, data, result.sample_count * c_num_channels * sizeof(s16));

	return result;
}

func void play_sound(s_sound sound)
{
	XAUDIO2_BUFFER buffer = zero;
	buffer.AudioBytes = sound.get_size_in_bytes();
	buffer.pAudioData = (BYTE*)sound.samples;
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	for(int i = 0; i < c_max_concurrect_sounds; i++) {
		s_voice* voice = &xaudio.voice_arr[i];
		int original = InterlockedCompareExchange((LONG*)&voice->playing, 1, 0);
		if(original == 0) {
			voice->voice->SubmitSourceBuffer(&buffer);
			break;
		}
	}
}