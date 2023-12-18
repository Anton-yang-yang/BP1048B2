/**
 *************************************************************************************
 * @file	distortion_ds1.h
 * @brief	Distortion DS1 for mono signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.2
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __DISTORTION_DS1_H__
#define __DISTORTION_DS1_H__

#include <stdint.h>


/** error code for distortion ds1*/
typedef enum _DISTORTION_DS1_ERROR_CODE
{
	DISTORTION_DS1_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	DISTORTION_DS1_ERROR_ILLEGAL_DISTORTION_LEVEL,
	DISTORTION_DS1_ERROR_ILLEGAL_OUTPUT_LEVEL,

	// No Error
	DISTORTION_DS1_ERROR_OK = 0,					/**< no error              */
} DISTORTION_DS1_ERROR_CODE;

/** Distortion DS1 context */
typedef struct _DistortionDS1Context
{
	int32_t sample_rate;
	//int32_t num_channels;
	float T_amp, T_A1, T_A2;
	float O_B0, O_B1, O_B2, O_A1, O_A2;
	int32_t dTx[2];
	float dTy[2];
	float dO[4];

} DistortionDS1Context;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize the distortion ds1 module.
 * @param ct Pointer to a DistortionDS1Context object.
 * @param sample_rate Sample rate.
 * @param distortion_level Distortion level. Range: 0~100%.
 * @return error code. DISTORTION_DS1_ERROR_OK means successful, other codes indicate error.
 */
int32_t distortion_ds1_init(DistortionDS1Context *ct, int32_t sample_rate, int32_t distortion_level);


/**
* @brief Apply distortion ds1 effect to a frame of PCM data (mono only)
* @param ct Pointer to a DistortionDS1Context object.
* @param pcm_in Address of the PCM input.
* @param pcm_out Address of the PCM output.
*        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
* @param n Number of PCM samples to process.
* @param out_level The output level of effect signals. Range: 0~100%.
* @return error code. DISTORTION_DS1_ERROR_OK means successful, other codes indicate error.
* @note Note that only mono signals are accepted.
*/
int32_t distortion_ds1_apply(DistortionDS1Context *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n,  int32_t out_level);


/**
 * @brief Configure distortion ds1's distortion level.
 * @param distortion_level Distortion level. Range: 0~100%.
 * @return error code. DISTORTION_DS1_ERROR_OK means successful, other codes indicate error.
 * @note Do not call this function before distortion_ds1_init() is called.
 */
int32_t distortion_ds1_configure_distortion(DistortionDS1Context *ct, int32_t distortion_level);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __DISTORTION_DS1_H__
