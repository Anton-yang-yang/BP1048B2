/**
 *************************************************************************************
 * @file    audio_decoder_api.c
 * @brief   Audio decoder API interface.
 *
 * @author  Alfred Zhao, Aissen Li, Lean Xiong
 * @version @see AUDIO_DECODER_LIBRARY_VERSION
 * 
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#include <string.h>
#include "audio_decoder_api.h"

// DO NOT change the following macros as they are only the copies of the values used in the library compilation stage.
//#define A52_INPUT_BUFFER_CAPACITY 3840
#define A52_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_A52Context 16696
#define AAC_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_AACContext 25440
#define AIF_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_AIFContext 8232
#define AMR_INPUT_BUFFER_CAPACITY 256   // 256 is enough!
#define SIZEOF_AmrnbDecoderContext 5080
#define AMRWB_INPUT_BUFFER_CAPACITY 256 // 256 is enough!
#define SIZEOF_AMRWBDecoderContext 4804
#define APE_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_APEContext 22340
#define DTS_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_DTSContext 25152
#define FLAC_INPUT_BUFFER_CAPACITY 2048
#ifdef FLAC_HIGH_RESOLUTION
#define SIZEOF_FLACContext 38520
#else
#define SIZEOF_FLACContext 20088
#endif
#define MP3_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_MPXContext 16132
#define SBC_INPUT_BUFFER_CAPACITY 1028	// 513*2 and round up to 4's multiples
#define SIZEOF_SBCContext 3856
#define WAV_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_WAVContext 8224
#define WMA_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_WMAContext 25532
#define MP2_INPUT_BUFFER_CAPACITY 2560
#define SIZEOF_MP2DecoderContext 9188


#ifdef  USE_MP3_DECODER
int32_t mp3_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t mp3_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t mp3_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t mp3_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_WMA_DECODER
int32_t wma_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t wma_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t wma_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t wma_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_SBC_DECODER
int32_t sbc_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t sbc_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t sbc_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t sbc_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_WAV_DECODER
int32_t wav_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t wav_decoder_initializeHR(AudioDecoderContext *audio_decoder);
int32_t wav_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t wav_decoder_decodeHR(AudioDecoderContext *audio_decoder);
int32_t wav_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t wav_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_FLAC_DECODER
int32_t flac_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t flac_decoder_initializeHR(AudioDecoderContext *audio_decoder);
int32_t flac_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t flac_decoder_decodeHR(AudioDecoderContext *audio_decoder);
int32_t flac_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t flac_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_AAC_DECODER
int32_t aac_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t aac_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t aac_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t aac_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_AIF_DECODER
int32_t aif_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t aif_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t aif_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t aif_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_AMR_DECODER
int32_t amrnb_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t amrnb_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t amrnb_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t amrnb_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_APE_DECODER
int32_t ape_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t ape_decoder_initializeHR(AudioDecoderContext *audio_decoder);
int32_t ape_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t ape_decoder_decodeHR(AudioDecoderContext *audio_decoder);
int32_t ape_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t ape_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef USE_DTS_DECODER
int32_t dts_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t dts_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t dts_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t dts_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef USE_A52_DECODER
int32_t a52_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t a52_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t a52_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t a52_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef USE_AMRWB_DECODER
int32_t amrwb_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t amrwb_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t amrwb_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t amrwb_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

#ifdef  USE_MP2_DECODER
int32_t mp2_decoder_initialize(AudioDecoderContext *audio_decoder);
int32_t mp2_decoder_decode(AudioDecoderContext *audio_decoder);
int32_t mp2_decoder_can_continue(AudioDecoderContext *audio_decoder);
int32_t mp2_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);
#endif

static const uint8_t audio_decoder_lib_version[] = AUDIO_DECODER_LIBRARY_VERSION " build @ " __DATE__ " " __TIME__;
const uint8_t* audio_decoder_get_lib_version(void)
{
	return audio_decoder_lib_version;
}


static void get_buffer_context_size(int32_t decoder_type, uint32_t* buffer_capacity, uint32_t* frame_context_size)
{
	switch (decoder_type)
	{
	case MP3_DECODER:
		*buffer_capacity = MP3_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_MPXContext;
		break;
	case WMA_DECODER:
		*buffer_capacity = WMA_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_WMAContext;
		break;
	case SBC_DECODER:
	case MSBC_DECODER:
		*buffer_capacity = SBC_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_SBCContext;
		break;
	case WAV_DECODER:
		*buffer_capacity = WAV_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_WAVContext;
		break;
	case FLAC_DECODER:
		*buffer_capacity = FLAC_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_FLACContext;
		break;
	case AAC_DECODER:
		*buffer_capacity = AAC_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_AACContext;
		break;
	case AIF_DECODER:
		*buffer_capacity = AIF_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_AIFContext;
		break;
	case AMR_DECODER:
		*buffer_capacity = AMR_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_AmrnbDecoderContext;
		break;
	case APE_DECODER:
		*buffer_capacity = APE_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_APEContext;
		break;
	case DTS_DECODER:
		*buffer_capacity = DTS_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_DTSContext;
		break;
	case A52_DECODER:
		*buffer_capacity = A52_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_A52Context;
		break;
	case AMRWB_DECODER:
		*buffer_capacity = AMRWB_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_AMRWBDecoderContext;
		break;
	case MP2_DECODER:
		*buffer_capacity = MP2_INPUT_BUFFER_CAPACITY;
		*frame_context_size = SIZEOF_MP2DecoderContext;
		break;
	default:
		*buffer_capacity = 0;
		*frame_context_size = 0;
	}
}

uint32_t audio_decoder_get_memory_size(int32_t decoder_type)
{
	uint32_t buffer_capacity, frame_context_size;

	get_buffer_context_size(decoder_type, &buffer_capacity, &frame_context_size);
	return (sizeof(AudioDecoderContext) + buffer_capacity + frame_context_size + 3) & 0xFFFFFFFC;
}


// Declare here to avoid compiler warning.
extern void buffer_context_init(BufferContext *bc, void *io_handle, int32_t io_type, uint8_t *buffer, int32_t buffer_capcity, int32_t buffer_type);

int32_t audio_decoder_initialize(uint8_t *ram_addr, void *io_handle, int32_t io_type, int32_t decoder_type)
{    
    uint32_t buffer_capacity;
    uint32_t frame_context_size;
	AudioDecoderContext* audio_decoder = (AudioDecoderContext*)ram_addr;
    
    if(ram_addr == NULL)
        return RT_FAILURE;	

    memset(audio_decoder, 0, sizeof(AudioDecoderContext));

	get_buffer_context_size(decoder_type, &buffer_capacity, &frame_context_size);

    switch(decoder_type)
    {
#ifdef  USE_MP3_DECODER
    case MP3_DECODER:
        audio_decoder->initialize   = mp3_decoder_initialize;
        audio_decoder->decode       = mp3_decoder_decode;
        audio_decoder->can_continue = mp3_decoder_can_continue;
        audio_decoder->seek         = mp3_decoder_seek;
        break;
#endif
    
#ifdef  USE_WMA_DECODER
    case WMA_DECODER:
        audio_decoder->initialize   = wma_decoder_initialize;
        audio_decoder->decode       = wma_decoder_decode;
        audio_decoder->can_continue = wma_decoder_can_continue;
        audio_decoder->seek         = wma_decoder_seek;
        break;
#endif
        
#ifdef  USE_SBC_DECODER
    case SBC_DECODER:
    case MSBC_DECODER:
        audio_decoder->initialize   = sbc_decoder_initialize;
        audio_decoder->decode       = sbc_decoder_decode;
        audio_decoder->can_continue = sbc_decoder_can_continue;
        audio_decoder->seek         = sbc_decoder_seek;
        break;
#endif
        
#ifdef  USE_WAV_DECODER
    case WAV_DECODER:
#ifdef WAV_HIGH_RESOLUTION
		audio_decoder->initialize = wav_decoder_initializeHR;
		audio_decoder->decode = wav_decoder_decodeHR;
#else
        audio_decoder->initialize   = wav_decoder_initialize;
        audio_decoder->decode       = wav_decoder_decode;
#endif
        audio_decoder->can_continue = wav_decoder_can_continue;
        audio_decoder->seek         = wav_decoder_seek;
        break;
#endif
    
#ifdef  USE_FLAC_DECODER
    case FLAC_DECODER:
#ifdef FLAC_HIGH_RESOLUTION
		audio_decoder->initialize = flac_decoder_initializeHR;
		audio_decoder->decode = flac_decoder_decodeHR;
#else
        audio_decoder->initialize   = flac_decoder_initialize;
        audio_decoder->decode       = flac_decoder_decode;
#endif
        audio_decoder->can_continue = flac_decoder_can_continue;
        audio_decoder->seek         = flac_decoder_seek;
        break;
#endif

#ifdef  USE_AAC_DECODER
    case AAC_DECODER:
        audio_decoder->initialize   = aac_decoder_initialize;
        audio_decoder->decode       = aac_decoder_decode;
        audio_decoder->can_continue = aac_decoder_can_continue;
        audio_decoder->seek         = aac_decoder_seek;
        break;
#endif

#ifdef  USE_AIF_DECODER
    case AIF_DECODER:
        audio_decoder->initialize   = aif_decoder_initialize;
        audio_decoder->decode       = aif_decoder_decode;
        audio_decoder->can_continue = aif_decoder_can_continue;
        audio_decoder->seek         = aif_decoder_seek;
        break;
#endif

#ifdef  USE_AMR_DECODER
    case AMR_DECODER:
        audio_decoder->initialize   = amrnb_decoder_initialize;
        audio_decoder->decode       = amrnb_decoder_decode;
        audio_decoder->can_continue = amrnb_decoder_can_continue;
        audio_decoder->seek         = amrnb_decoder_seek;
        break;
#endif

#ifdef  USE_APE_DECODER
	case APE_DECODER:
#ifdef APE_HIGH_RESOLUTION
		audio_decoder->initialize = ape_decoder_initializeHR;
		audio_decoder->decode = ape_decoder_decodeHR;
#else
		audio_decoder->initialize = ape_decoder_initialize;
		audio_decoder->decode = ape_decoder_decode;
#endif
		audio_decoder->can_continue = ape_decoder_can_continue;
		audio_decoder->seek = ape_decoder_seek;
		break;
#endif

#ifdef  USE_DTS_DECODER
	case DTS_DECODER:
		audio_decoder->initialize = dts_decoder_initialize;
		audio_decoder->decode = dts_decoder_decode;
		audio_decoder->can_continue = dts_decoder_can_continue;
		audio_decoder->seek = dts_decoder_seek;
		break;
#endif

#ifdef  USE_A52_DECODER
	case A52_DECODER:
		audio_decoder->initialize = a52_decoder_initialize;
		audio_decoder->decode = a52_decoder_decode;
		audio_decoder->can_continue = a52_decoder_can_continue;
		audio_decoder->seek = a52_decoder_seek;
		break;
#endif

#ifdef  USE_AMRWB_DECODER
	case AMRWB_DECODER:
		audio_decoder->initialize = amrwb_decoder_initialize;
		audio_decoder->decode = amrwb_decoder_decode;
		audio_decoder->can_continue = amrwb_decoder_can_continue;
		audio_decoder->seek = amrwb_decoder_seek;
		break;
#endif

#ifdef  USE_MP2_DECODER
	case MP2_DECODER:
		audio_decoder->initialize = mp2_decoder_initialize;
		audio_decoder->decode = mp2_decoder_decode;
		audio_decoder->can_continue = mp2_decoder_can_continue;
		audio_decoder->seek = mp2_decoder_seek;
		break;
#endif

	default:
        return RT_FAILURE;
    }

    audio_decoder->decoder_type = decoder_type;
	audio_decoder->decoder_size = audio_decoder_get_memory_size(decoder_type);
    
    // Memory allocation & initialization of inbuf & frame
    audio_decoder->inbuf     = (uint8_t*)audio_decoder + sizeof(AudioDecoderContext);
    audio_decoder->frame     = (void*)(audio_decoder->inbuf + buffer_capacity);    
	memset(audio_decoder->frame, 0, frame_context_size);
    buffer_context_init(&(audio_decoder->bc), io_handle, io_type, audio_decoder->inbuf, buffer_capacity, BUFFER_FOR_INPUT);

    // Initialize song info (non-zero fields)
    audio_decoder->song_info.genre = 255;			// 255: "unknown genre" (assumed). 0: blue, so it can't be used as default.
    audio_decoder->song_info.pcm_bit_width = 16;	// 16 bits per sample by default

	return audio_decoder->initialize(audio_decoder);
}


int32_t audio_decoder_decode(AudioDecoderContext *audio_decoder)
{
    if(NULL == audio_decoder)
        return RT_FAILURE;
    
    return audio_decoder->decode(audio_decoder);
}


int32_t audio_decoder_can_continue(AudioDecoderContext *audio_decoder)
{
    if(NULL == audio_decoder)
        return RT_NO;
    
    return audio_decoder->can_continue(audio_decoder);
}


int32_t audio_decoder_get_error_code(AudioDecoderContext *audio_decoder)
{
    if(NULL == audio_decoder)
        return 0; // no error is found.
    
    return audio_decoder->error_code;	
}


int32_t audio_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time)
{
    if(NULL == audio_decoder)
        return RT_FAILURE;
    
    return audio_decoder->seek(audio_decoder, seek_time > audio_decoder->song_info.duration ? audio_decoder->song_info.duration : seek_time);
}


SongInfo* audio_decoder_get_song_info(AudioDecoderContext *audio_decoder)
{
    if(NULL == audio_decoder)
        return NULL;
    
    return &(audio_decoder->song_info);
}


DecoderType audio_decoder_guess_format(uint8_t s[10])
{
	if (s[0] == 0xFF && (s[1] & 0xE0) == 0xE0 && (s[1] & 0x06) != 0) // First 11 bits(Bit 0~10) are all '1' for .mp3, layer(Bit 13~14)!=00
		return MP3_DECODER;
	//else if ( // 30 26 B2 75 8E 66 CF 11 A6 D9 00 AA 00 62 CE 6C (ASF_TOP_HEADER_OBJECT_GUID) for .wma, .asf
	//	s[0] == 0x30 && s[1] == 0x26 && s[2] == 0xB2 && s[3] == 0x75 &&
	//	s[4] == 0x8E && s[5] == 0x66 && s[6] == 0xCF && s[7] == 0x11 &&
	//	s[8] == 0xA6 && s[9] == 0xD9 && s[10] == 0x00 && s[11] == 0xAA &&
	//	s[12] == 0x00 && s[13] == 0x62 && s[14] == 0xCE && s[15] == 0x6C)
	//	return WMA_DECODER;
	else if ( // 30 26 B2 75 8E 66 CF 11 A6 D9 00 AA 00 62 CE 6C (ASF_TOP_HEADER_OBJECT_GUID) for .wma, .asf
		s[0] == 0x30 && s[1] == 0x26 && s[2] == 0xB2 && s[3] == 0x75 &&
		s[4] == 0x8E && s[5] == 0x66 && s[6] == 0xCF && s[7] == 0x11 &&
		s[8] == 0xA6 && s[9] == 0xD9) // Check only the first 10 bytes to align with ID3 length, a little bit unsafe, but maybe still OK~
		return WMA_DECODER;
	else if (s[0] == 0x9C)
		return SBC_DECODER;
	else if (s[0] == 0xAD)
		return MSBC_DECODER;
	else if (s[0] == 0x52 && s[1] == 0x49 && s[2] == 0x46 && s[3] == 0x46)// 52 49 46 46 ("RIFF") for .wav
		return WAV_DECODER; // may be DTS_DECODER as well. Further check of the error code returned by audio_decoder_initialize() is required. If the error code is WAV_ERROR_UNSUPPORTED_DTS_FORMAT(=-245), DTS coding is confirmed.
	else if (s[0] == 0x66 && s[1] == 0x4C && s[2] == 0x61 && s[3] == 0x43) // 66 4C 61 43 ("fLaC") for .flac 
		return FLAC_DECODER;
	else if (s[4] == 0x66 && s[5] == 0x74 && s[6] == 0x79 && s[7] == 0x70) // ?? ?? ?? ?? 66 74 79 70 (ftyp) for .mp4, .m4a
		return AAC_DECODER;
	else if (s[0] == 0xFF && (s[1] & 0xF0) == 0xF0 && (s[1] & 0x06) == 0) // First 12 bits(Bit 0~11) are all '1' for .aac (ADTS), layer(Bit 13~14)==00
		return AAC_DECODER;
	else if (s[0] == 0x47 && s[1] == 0xFC && s[2] == 0x00) // 47 FC 00 for LATM used in bluetooth
		return AAC_DECODER;
	else if (s[0] == 0x41 && s[1] == 0x44 && s[2] == 0x49 && s[3] == 0x46) // 41 44 49 46 for .adif (ADIF, not popular)
		return AAC_DECODER;
	else if (s[0] == 0x46 && s[1] == 0x4F && s[2] == 0x52 && s[3] == 0x4D) // 46 4F 52 4D ("FORM") for .aif
		return AIF_DECODER;
	else if (s[0] == 0x23 && s[1] == 0x21 && s[2] == 0x41 && s[3] == 0x4D && s[4] == 0x52 && s[5] == 0x0A) // 23 21 41 4D 52 0A ("#!AMR.") for .amr
		return AMR_DECODER;
	else if (s[0] == 0x23 && s[1] == 0x21 && s[2] == 0x41 && s[3] == 0x4D && s[4] == 0x52 && s[5] == 0x2D && s[6] == 0x57 && s[7] == 0x42 && s[8] == 0x0A) // 23 21 41 4D 52 0A ("#!AMR-WB.") for .amr
		return AMRWB_DECODER;
	else if (s[0] == 0x4D && s[1] == 0x41 && s[2] == 0x43 && s[3] == 0x20) // 4D 41 43 20 ("MAC ") for .ape
		return APE_DECODER;
	else if (s[0] == 0x0B && s[1] == 0x77) // 0B 77 for .ac3
		return A52_DECODER;
	else if (s[0] == 0x49 && s[1] == 0x44 && s[2] == 0x33) // "ID3"
	{
		int32_t id3len = (uint32_t)s[6] * 0x200000 + (uint32_t)s[7] * 0x4000 + (uint32_t)s[8] * 0x80 + (uint32_t)s[9];
		id3len += 10; // + 10 bytes ID3 frame header itself
		return -id3len; // Negative value to indicate the ID3 length to skip.
	}

	return UNKNOWN_DECODER;
}