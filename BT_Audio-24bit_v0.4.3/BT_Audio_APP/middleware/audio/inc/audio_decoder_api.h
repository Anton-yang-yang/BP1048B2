/**
 *************************************************************************************
 * @file    audio_decoder_api.h
 * @brief   Audio decoder API interface (multi-thread safe)
 *
 * @author  Alfred Zhao, Aissen Li, Lean Xiong
 * @version @see AUDIO_DECODER_LIBRARY_VERSION
 * 
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __AUDIO_DECODER_API_H__
#define __AUDIO_DECODER_API_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "typedefine.h"
#include "song_info.h"
#include "audio_decoder_in_use.h" // User defined configuration file for audio decoders to be used.

#ifdef LOSSLESS_DECODER_HIGH_RESOLUTION
#define	APE_HIGH_RESOLUTION
#define	FLAC_HIGH_RESOLUTION
#define	WAV_HIGH_RESOLUTION
#endif

#define AUDIO_DECODER_LIBRARY_VERSION   "8.10.2"     /**< audio decoder library version string        */


/**
 * Audio Decoder Type Set 
 */
typedef enum _DecoderType
{
    UNKNOWN_DECODER              = 0,       /**< Unknown decoder */
    FLAC_DECODER ,                          /**< FLAC decoder */
	APE_DECODER,                            /**< APE  decoder */
	WAV_DECODER,							/**< WAV  decoder */	
    AIF_DECODER,                            /**< AIFF & AIFC decoder */
    AMR_DECODER,                            /**< AMR  decoder */
	AAC_DECODER,							/**< AAC  decoder */
	MP3_DECODER,                            /**< MP3  decoder */
    WMA_DECODER,                            /**< WAM  decoder */
    SBC_DECODER,                            /**< SBC  decoder */
    MSBC_DECODER,                           /**< mSBC decoder */
	DTS_DECODER,                            /**< DTS  decoder */
	A52_DECODER,                            /**< A52  decoder */
	AMRWB_DECODER,						    /**< AMR wideband decoder */
	MP2_DECODER,                            /**< MP2 only decoder */
}DecoderType;


/**
 * Audio Decoder Context
 */
typedef struct _AudioDecoderContext
{
    int32_t       decoder_type;	/**< Audio decoder type, value must be in @code DecoderType */
    uint32_t      decoder_size;	/**< Audio decoder size, equal to sizeof(AudioDecoderContext)+sizeof(input_buffer_capacity)+sizeof(frame_context_size) */
    int32_t       error_code;	/**< Audio decoder return error code */
    uint8_t       *inbuf;		/**< Input buffer pointer */
    void          *frame;		/**< Audio decoder state info context pointer,can be MP3Context, WMAContext, SBCContext, WAVContext, FLACContext etc */
    BufferContext bc;			/**< Buffer context */
    SongInfo      song_info;	/**< Song information context */
    
	int32_t      (*initialize)(struct _AudioDecoderContext *);				/**< Function pointer for initializing audio decoder           */
    int32_t      (*decode)(struct _AudioDecoderContext *);					/**< Function pointer for decoding one audio frame             */
    int32_t      (*can_continue)(struct _AudioDecoderContext *);			/**< Function pointer for checking whether the audio decoder can go on, if can, then decode, else abort audio decoder  */
    int32_t      (*seek)(struct _AudioDecoderContext *, uint32_t seek_time);/**< Function pointer for seeking to special time to play */
}AudioDecoderContext;


// -------------------- General audio decoder APIs --------------------

/**
 * @brief  Get audio decoder library version
 * @return audio decoder library version string
 */
const uint8_t* audio_decoder_get_lib_version(void);

/**
 * @brief  Get audio decoder's entire memory size in bytes.
 * @param  decoder_type audio decoder type, value must be in @code DecoderType
 * @return audio decoder's entire memory size in bytes.
 */
uint32_t audio_decoder_get_memory_size(int32_t decoder_type);

/**
 * @brief  Initialize audio decoder
 * @param  ram_addr     ram address
 * @param  io_handle    IO handle
 * @param  io_type      IOType pointer, value must be in @code IOType
 * @param  decoder_type audio decoder type, value must be in @code DecoderType
 * @return initialize result, RT_FAILURE, RT_SUCCESS
 */ 
int32_t audio_decoder_initialize(uint8_t *ram_addr, void *io_handle, int32_t io_type, int32_t decoder_type);

/**
 * @brief  Decode an audio frame
 * @return RT_FAILURE, RT_SUCCESS
 */ 
int32_t audio_decoder_decode(AudioDecoderContext *audio_decoder);

/**
 * @brief  Check whether the audio decoder can go on
 * @return check result, RT_NO, RT_YES
 */ 
int32_t audio_decoder_can_continue(AudioDecoderContext *audio_decoder);

/**
 * @brief  Get error code from AudioDecoderContext
 * @return error code
 */ 
int32_t audio_decoder_get_error_code(AudioDecoderContext *audio_decoder);

/**
 * @brief  Seek to file position according as the specified time. 
 * @param  seek_time   the specified time in millisecond(ms)
 * @return RT_SUCCESS, RT_FAILURE
*/
int32_t audio_decoder_seek(AudioDecoderContext *audio_decoder, uint32_t seek_time);

/**
 * @brief  Get song information.
 * @return SongInfo pointer
 */
SongInfo* audio_decoder_get_song_info(AudioDecoderContext *audio_decoder);


/**
 * @brief  Guess the audio coding format by checking the initial bytes in stream/file.
 * @param  s  Initial 10 bytes in stream or file.
 * @return decoder_type audio decoder type in @code DecoderType. 
 *         If no guess can be made, UNKNOWN_DECODER is returned.
 *         If ID3 tag is found, a negative value is returned. Its absolute value indicates the ID3 length to skip so that further check can be done.
 * @note If DTS format is used in a WAV file, it is usually difficult to distinguish it from the regular raw PCM format by checking only the initial bytes of the file.
 *       In this case, further check of the error code returned by audio_decoder_initialize() is required. If the error code is WAV_ERROR_UNSUPPORTED_DTS_FORMAT(=-245), DTS coding is confirmed.
 */
DecoderType audio_decoder_guess_format(uint8_t s[10]);

// -------------------- Specific audio decoder APIs --------------------

#ifdef  USE_MP3_DECODER
/**
 * @brief  Get MPEG version for MP2/3 songs
 * @return MPEG version value. 0:Unkown MPEG version, 1:MPEG-1, 2:MPEG-2, 3:MPEG-2.5
 */
int32_t mp3_decoder_get_mpeg_version(AudioDecoderContext *audio_decoder);

/**
 * @brief  Get ID3 version
 * @return ID3 version value. 0:No ID3 available, 1:Only ID3 v1 available, 2:Only ID3 v2 available, 3:Both ID3 v1 & v2 available
 */
int32_t mp3_decoder_get_id3_version(AudioDecoderContext *audio_decoder);
#endif


#ifdef  USE_APE_DECODER
/**
 * @brief  Get compression type of APE after initialization.
 * @return compression type. Possible values: 1000/2000/3000
 */
int32_t ape_decodere_get_compression_type(AudioDecoderContext *audio_decoder);
#endif


#ifdef  USE_A52_DECODER
/**
 * @brief  Set downmix mode for A52 decoder
 * @param  downmix_mode Downmix mode. 0:Off, 1:On
 * @return RT_SUCCESS, RT_FAILURE
 * @note If downmix is enabled(downmix_mode=1), channels more than 2 are always downmixed to mono or stereo in interleaved format, i.e. L1,R1,L2,R2,L3,R3,...
 *       If downmix is disabled(downmix_mode=0), channels more than 2 are always output as 5.1 (6 channels) in non-interleaved
 *          format as the following sequence in the output:
 *       LFE1,LFE2,LFE3,...,L1,L2,L3,...,C1,C2,C3,...,R1,R2,R3,...,Lsd1,Lsd2,Lsd3,...,Rsd1,Rsd2,Rsd3,...
 * LFE:Low-Frequency Effects, L:Left, C:Center, R:Right, Lsd:Left surround direct, Rsd:Right surround direct
 * Downmix is enabled by default in a52_decoder_initialize(). Disabling downmix can be done after initialization if required.
 * Please notice that the change of downmix mode can change the number of channels in song_info in audio_decoder as a result.
*/
int32_t a52_decoder_set_downmix_mode(AudioDecoderContext *audio_decoder, int32_t downmix_mode);
#endif

#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_DECODER_API_H__
