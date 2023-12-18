/**
 *************************************************************************************
 * @file	sbcenc_api.h
 * @brief	SBC Encoder API interface.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.2.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __SBCENC_API_H__
#define __SBCENC_API_H__

#include <stdint.h>

// for mSBC mode (blocks = 15), MAX_SBC_FRAME_LEN = 57 bytes always.
#define MAX_SBC_FRAME_LEN 532 // 4+(4*8*2)/8+ceil(1*8+16*(32*8)/8), ref to "bitpool" & "frame_length" concepts in A2DP reference.

/**
 * Error code of SBC Encoder
 */
typedef enum _SBC_ENC_ERROR_CODE
{
	// The followings are unrecoverable errors, i.e. encoder can't continue
    SBC_ENC_ERROR_UNRECOVERABLE = -256,
	SBC_ENC_ERROR_NOT_SUPPORTED_SAMPLE_RATE,	/**< the specified sample rate is not supported */
	SBC_ENC_ERROR_NOT_SUPPORTED_CHANNELS,		/**< the specified number of channels is not supported */
	SBC_ENC_ERROR_NOT_SUPPORTED_BLOCKS,			/**< the specified number of blocks is not supported */
	SBC_ENC_ERROR_INCORRECT_MSBC_SETTINGS,		/**< incorrect settings are used for mSBC mode */
	SBC_ENC_ERROR_ILLEGAL_CHANNEL_MODE,			/**< the specified channel mode is illegal */
	SBC_ENC_ERROR_ILLEGAL_NUM_SUBBANDS,			/**< the specified number of subbands is illegal */
	SBC_ENC_ERROR_ILLEGAL_BITPOOL,				/**< the specified bitpool is illegal */
	SBC_ENC_ERROR_ILLEGAL_ALLOCATION_METHOD,	/**< the specified allocation method is illegal */

	// The followings are recoverable errors, i.e. encoder can continue
	SBC_ENC_ERROR_RECOVERABLE = -128,
	SBC_ENC_ERROR_BAD,							/**< not enough PCM samples for encoding */	

	// No Error
	SBC_ENC_ERROR_OK = 0,						/**< no error              */

}SBC_ENC_ERROR_CODE;


/**
 * SBC encoder quality selection
 */
typedef enum _SBC_ENC_QUALITY
{
	SBC_ENC_QUALITY_HIGH,
	SBC_ENC_QUALITY_MIDDLE,
}SBC_ENC_QUALITY;

/**
 * SBC encoder channel mode
 */
typedef enum _SBC_ENC_CHANNEL_MODE
{
	SBC_ENC_MODE_MONO = 0,
	SBC_ENC_MODE_DUAL_CHANNEL,
	SBC_ENC_MODE_STEREO,
	SBC_ENC_MODE_JOINT_STEREO,
}SBC_ENC_CHANNEL_MODE;

/**
 * SBC encoder allocation method
 */
typedef enum _SBC_ENC_ALLOCATION_METHOD
{
	SBC_ENC_AM_LOUDNESS = 0,
	SBC_ENC_AM_SNR,
}SBC_ENC_ALLOCATION_METHOD;


/** SBC Encoder Context */
typedef struct _SBCEncoderContext
{
	uint8_t priv_instance[4096]; // at least 4035 for encoder only, 5527 bytes for codec
	void *priv;
	void *priv_alloc_base;

	uint32_t flags;	
	uint8_t frequency;
	uint8_t blocks;
	uint8_t subbands;
	uint8_t mode;
	uint8_t allocation;
	uint8_t bitpool;
	uint8_t endian;

} SBCEncoderContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/*******************************************************************************
 * SBC encoder API functions prototype. 
 *******************************************************************************/ 
/**
 * @brief  Initialize the SBC encoder.
 * @param[in] ct  pointer to the SBC encoder context
 * @param[in] num_channels number of channels
 * @param[in] sample_rate sample rate. Choose from 16000, 32000, 44100 or 48000.
 * @param[in] blocks number of blocks per frame. Select from 4, 8, 12, 15, 16 only. If 15 is selected, mSBC encoding mode is implied.
 * @param[in] quality encoding quality. Select either SBC_ENC_QUALITY_HIGH (high bitrate) or SBC_ENC_QUALITY_MIDDLE (middle bitrate). This quality setting is ignored if mSBC encoding mode is used (blocks=15).
 * @param[out] samples_per_frame number of samples per frame per channel.
 *             For example if the returned samples per frame is 128, then for mono input, the required samples from input are 128 samples/frame; for 2 channels, the required samples from input are 256 samples(=128 * 2)/frame.
 * @return initialization result. SBC_ENC_ERROR_OK means OK, other codes indicate error.
 */ 
int32_t	sbc_encoder_initialize(SBCEncoderContext *ct, int32_t num_channels, int32_t sample_rate, int32_t blocks, SBC_ENC_QUALITY quality, int32_t *samples_per_frame);


/**
 * @brief  Initialize the SBC encoder(advanced mode).
 * @param[in] ct  pointer to the SBC encoder context
 * @param[in] channel_mode channel mode. For mono signals, choose SBC_ENC_MODE_MONO; for stereo signals, choose from SBC_ENC_MODE_DUAL_CHANNEL, SBC_ENC_MODE_STEREO or SBC_ENC_MODE_JOINT_STEREO.
 * @param[in] sample_rate sample rate. Choose from 16000, 32000, 44100 or 48000.
 * @param[in] blocks number of blocks per frame. Select from 4, 8, 12, 15, 16 only. If 15 is selected, mSBC encoding mode is implied.
 * @param[in] subbands number of subbands used. Select either 4 or 8.
 * @param[in] bitpool Value that controls the quality of encoding. Range: 2~250. Higher bitpool gives better quality but results in more frame length or bitrate.
 * Recommended settings for bitpool:
 * -------------------------------------------------------------------------------------
 *                                 |      Middle Quality     |      High Quality       |
 * -------------------------------------------------------------------------------------
 *                                 |    Mono    |  J.Stereo  |    Mono    |   J.Stereo |
 * -------------------------------------------------------------------------------------
 * Sampling frequency (kHz)        |  44.1  48  |  44.1  48  |  44.1  48  |   44.1  48 |
 * Bitpool value                   |  19    18  |  35    33  |  31    29  |   53    51 |
 * Resulting frame length (bytes)  |  46    44  |  83    79  |  70    66  |  119   115 |
 * Resulting bit rate (kb/s)       | 127   132  | 229   237  | 193   198  |  328   345 |
 * -------------------------------------------------------------------------------------
 * Other settings: Block length = 16, Allocation method = Loudness, Subbands = 8.  "J.Stereo" means Joint Stereo.
 *
 * for MONO and DUAL_CHANNEL channel modes:
 *     frame_length = 4 + (4 * subbands * channels ) / 8 + ceil(blocks * channels * bitpool / 8)
 * for STEREO and JOINT_STEREO channel modes:
 *     frame_length = 4 + (4 * subbands * channels ) / 8 + ceil((join * subbands + blocks * bitpool) / 8)
 * Bit rate = frame_length * 8 *sampling rate / (subbands * blocks)
 * @param[in] allocation Bit allocation method. Choose either SBC_ENC_AM_LOUDNESS or SBC_ENC_AM_SNR.
 * @return initialization result. SBC_ENC_ERROR_OK means OK, other codes indicate error.
 * @note (1) If blocks=15, mSBC encoding mode is implied and all other settings are fixed as follows:
 *           sample_rate=16000, blocks=15, subbands=8, channel_mode=SBC_ENC_MODE_MONO, allocation = SBC_ENC_AM_LOUDNESS, bitpool=26. If different values are set, they are ignored.
 *       (2) Number of samples per frame per channel = blocks x subbands. For example, if the input is mono, blocks=16, subbands=8, then the samples per frame per channel is 128.
 */
int32_t	sbc_encoder_initialize_advanced(SBCEncoderContext *ct, SBC_ENC_CHANNEL_MODE channel_mode, int32_t sample_rate, int32_t blocks, int32_t subbands, int32_t bitpool, SBC_ENC_ALLOCATION_METHOD allocation);


/**
 * @brief  Encode an SBC frame.
 * @param[in] ct  pointer to the SBC encoder context
 * @param[in] pcm one frame of PCM data. The data layout for mono, m0, m1, m2, ...; for stereo, l0, r0, l1, r1, ...
 *            The length of PCM samples should be (samples_per_frame*num_channels) as given in sbc_encoder_initialize() or (blocks*subbands*snum_channels) as in sbc_encoder_initialize_advanced().
 * @param[out] data  data buffer to receive the encoded SBC frame.
 * @param[out] length length of the encoded SBC frame in bytes. This value shall not exceed MAX_SBC_FRAME_LEN.
 * @return encoding result. SBC_ENC_ERROR_OK means OK, other codes indicate error.
 */
int32_t sbc_encoder_encode(SBCEncoderContext *ct, int16_t *pcm, uint8_t *data, uint32_t *length);

//int32_t sbc_encoder_finish(SBCEncoderContext *ct);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__SBCENC_API_H__
