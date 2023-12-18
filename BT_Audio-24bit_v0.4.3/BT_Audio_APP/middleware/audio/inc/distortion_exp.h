/**
 *************************************************************************************
 * @file	distortion_exp.h
 * @brief	Distortion based on an exponential function for mono signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __DISTORTION_EXP_H__
#define __DISTORTION_EXP_H__

#include <stdint.h>


/** error code for distortion */
typedef enum _DISTORTION_EXP_ERROR_CODE
{
	DISTORTION_EXP_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	DISTORTION_EXP_ERROR_GAIN_OUT_OF_RANGE,
	DISTORTION_EXP_ERROR_ILLEGAL_DRY,
	DISTORTION_EXP_ERROR_ILLEGAL_WET,

	// No Error
	DISTORTION_EXP_ERROR_OK = 0,					/**< no error              */
} DISTORTION_EXP_ERROR_CODE;

/** overdrive context */
typedef struct _DistortionExpContext
{
	//int32_t sample_rate;
	//int32_t num_channels;
	int32_t gain_db;
	int32_t gain_sf;

} DistortionExpContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
* @brief Initialize the distortion module.
* @param ct Pointer to a DistortionExpContext object.
* @param sample_rate Sample rate.
* @param gain Gain applied in dB before distortion. Range: 0 ~ 48 dB
* @return error code. DISTORTION_EXP_ERROR_OK means successful, other codes indicate error.
*/
int32_t distortion_exp_init(DistortionExpContext *ct, int32_t sample_rate, int32_t gain);


/**
* @brief Apply distortion effect to a frame of PCM data (mono only)
* @param ct Pointer to a DistortionExpContext object.
* @param pcm_in Address of the PCM input.
* @param pcm_out Address of the PCM output.
*        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
* @param n Number of PCM samples to process.
* @param gain Gain applied in dB before distortion. Range: 0 ~ 48 dB
* @param dry The level of dry(direct) signals in the output. Range: 0~100%.
* @param wet The level of wet(effect) signals in the output. Range: 0~100%.
* @return error code. DISTORTION_EXP_ERROR_OK means successful, other codes indicate error.
* @note Note that only mono signals are accepted.
*/
int32_t distortion_exp_apply(DistortionExpContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t gain, int32_t dry, int32_t wet);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __DISTORTION_EXP_H__
