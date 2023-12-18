/**
*************************************************************************************
* @file	    overdrive.h
* @brief	Overdrive effect for mono signals
*
* @author	ZHAO Ying (Alfred)
* @version	v1.0.0
*
* &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
*************************************************************************************
*/

#ifndef __OVERDRIVE_H__
#define __OVERDRIVE_H__

#include <stdint.h>

#define OVERDRIVE_THRESHOLD_COMPRESSION_MIN 4096	// 1/8 in Q1.15
#define OVERDRIVE_THRESHOLD_COMPRESSION_MAX 10923	// 1/3 in Q1.15


/** error code for overdrive */
typedef enum _OVERDRIVE_ERROR_CODE
{
	OVERDRIVE_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	OVERDRIVE_ERROR_ILLEGAL_THRESHOLD_COMPRESSION,

	// No Error
	OVERDRIVE_ERROR_OK = 0,					/**< no error              */
} OVERDRIVE_ERROR_CODE;

/** overdrive context */
typedef struct _OverdriveContext
{
	//int32_t sample_rate;
	//int32_t num_channels;
	int32_t th;

} OverdriveContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
* @brief Initialize the overdrive module.
* @param ct Pointer to a OverdriveContext object.
* @param sample_rate Sample rate.
* @param threshold_compression Threshold after which compression starts. Range: OVERDRIVE_THRESHOLD_COMPRESSION_MIN ~ OVERDRIVE_THRESHOLD_COMPRESSION_MAX
* @return error code. OVERDRIVE_ERROR_OK means successful, other codes indicate error.
*/
int32_t overdrive_init(OverdriveContext *ct, int32_t sample_rate, int32_t threshold_compression);


/**
* @brief Apply overdrive effect to a frame of PCM data (mono only)
* @param ct Pointer to a OverdriveContext object.
* @param pcm_in Address of the PCM input.
* @param pcm_out Address of the PCM output.
*        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
* @param n Number of PCM samples to process.
* @return error code. OVERDRIVE_ERROR_OK means successful, other codes indicate error.
* @note Note that only mono signals are accepted.
*/
int32_t overdrive_apply(OverdriveContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __OVERDRIVE_H__
