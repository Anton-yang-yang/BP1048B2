/**
 *************************************************************************************
 * @file	low_level_compressor.h
 * @brief	Upward compressor that increases gain for low level signals.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __LOW_LEVEL_COMPRESSOR_H__
#define __LOW_LEVEL_COMPRESSOR_H__

#include <stdint.h>


/** error code for low level compressor */
typedef enum _LOW_LEVEL_COMPRESSOR_ERROR_CODE
{
	LOW_LEVEL_COMPRESSOR_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	LOW_LEVEL_COMPRESSOR_ERROR_NUM_CHANNELS_NOT_SUPPORTED,
	LOW_LEVEL_COMPRESSOR_ERROR_THRESHOLD_OUT_OF_RANGE,
	LOW_LEVEL_COMPRESSOR_ERROR_GAIN_OUT_OF_RANGE,
	// No Error
	LOW_LEVEL_COMPRESSOR_ERROR_OK = 0
} LOW_LEVEL_COMPRESSOR_ERROR_CODE;


/** Low level compressor context */
typedef struct _LowLevelCompressorContext
{
	int32_t num_channels;
	int32_t sample_rate;	
	int32_t state;
	int32_t threshold;
	int32_t gain;
	int32_t slope;
	int32_t alpha_attack;
	int32_t alpha_release;
} LowLevelCompressorContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize low level compressor module
 * @param ct Pointer to a LowLevelCompressorContext object.
 * @param num_channels Number of channels. 
 * @param sample_rate Sample rate.
 * @param threshold The threshold determines the input level below which the compressor will activate. 
 *        Set the threshold in step of 0.01dB. Allowable range: -9600 ~ 0 to cover -96.00dB ~ 0.00dB. For example, -2550 stands for -25.50dB threshold.
 * @param gain maximum amount of gain makeup and compression applied to the signal below the threshold. Range: 0 ~ 4800 to cover 0.00 ~ 48.00dB.
 * @param attack_time Amount of time (in millisecond) the compressor reduces its gain from full to zero.
 * @param release_time Amount of time (in millisecond) the compressor increases its gain from zero to full.
 * @return error code. LOW_LEVEL_COMPRESSOR_ERROR_OK means successful, other codes indicate error.
 */
int32_t low_level_compressor_init(LowLevelCompressorContext *ct, int32_t num_channels, int32_t sample_rate,
	int32_t threshold, int32_t gain, int32_t attack_time, int32_t release_time);


/**
 * @brief Apply low level compression to a frame of PCM data.
 * @param ct Pointer to a LowLevelCompressorContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. LOW_LEVEL_COMPRESSOR_ERROR_OK means successful, other codes indicate error.
 */
int32_t low_level_compressor_apply(LowLevelCompressorContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply low level compression to a frame of PCM data (24-bit PCM in & out).
 * @param ct Pointer to a LowLevelCompressorContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. LOW_LEVEL_COMPRESSOR_ERROR_OK means successful, other codes indicate error.
 */
int32_t low_level_compressor_apply24(LowLevelCompressorContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __LOW_LEVEL_COMPRESSOR_H__
