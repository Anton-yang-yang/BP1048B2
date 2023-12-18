/**
 *************************************************************************************
 * @file	overdrive_poly.h
 * @brief	Overdrive effect for mono signals based on polynomial function
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __OVERDRIVE_POLY_H__
#define __OVERDRIVE_POLY_H__

#include <stdint.h>


/** error code for overdrive */
typedef enum _OVERDRIVE_POLY_ERROR_CODE
{
	OVERDRIVE_POLY_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	OVERDRIVE_POLY_ERROR_GAIN_OUT_OF_RANGE,
	OVERDRIVE_POLY_ERROR_ILLEGAL_OUTPUT_LEVEL,

	// No Error
	OVERDRIVE_POLY_ERROR_OK = 0,					/**< no error              */
} OVERDRIVE_POLY_ERROR_CODE;

/** overdrive context */
typedef struct _OverdrivePolyContext
{
	//int32_t sample_rate;
	//int32_t num_channels;
	float dx, dy;
	float g_scale[50];

} OverdrivePolyContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize the overdrive module based on polynomial funciton.
 * @param ct Pointer to a OverdrivePolyContext object.
 * @param sample_rate Sample rate.
 * @return error code. OVERDRIVE_POLY_ERROR_OK means successful, other codes indicate error.
 */
int32_t overdrive_poly_init(OverdrivePolyContext *ct, int32_t sample_rate);


/**
 * @brief Apply overdrive effect to a frame of PCM data (mono only)
 * @param ct Pointer to a OverdrivePolyContext object.
 * @param pcm_in Address of the PCM input.
 * @param pcm_out Address of the PCM output.
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param gain Gain applied in dB before overdrive. Range: 0 ~ 48 dB
 * @param out_level The output level of effect signals. Range: 0~100%.
 * @return error code. OVERDRIVE_POLY_ERROR_OK means successful, other codes indicate error.
 * @note Note that only mono signals are accepted.
 */
int32_t overdrive_poly_apply(OverdrivePolyContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t gain, int32_t out_level);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __OVERDRIVE_POLY_H__
