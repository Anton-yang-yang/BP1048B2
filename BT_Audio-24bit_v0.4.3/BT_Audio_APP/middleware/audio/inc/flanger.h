/**
*************************************************************************************
* @file	    flanger.h
* @brief	Flanger effect for mono signals
*
* @author	ZHAO Ying (Alfred)
* @version	v1.0.1
*
* &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
*************************************************************************************
*/

#ifndef __FLANGER_H__
#define __FLANGER_H__

#include <stdint.h>

#define MAX_FLANGER_DELAY_SAMPLES (15*2*48+2) // maximum delay in samples 15ms*2@48kHz

/** error code for flanger */
typedef enum _FLANGER_ERROR_CODE
{
	FLANGER_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	FLANGER_ERROR_ILLEGAL_DELAY_LENGTH,
	FLANGER_ERROR_ILLEGAL_MODULATION_DEPTH,
	FLANGER_ERROR_ILLEGAL_MODULATION_RATE,
	FLANGER_ERROR_ILLEGAL_FEEDBACK,
	FLANGER_ERROR_ILLEGAL_DRY,
	FLANGER_ERROR_ILLEGAL_WET,

	// No Error
	FLANGER_ERROR_OK = 0,					/**< no error              */
} FLANGER_ERROR_CODE;

/** flanger context */
typedef struct _FlangerrContext
{
	int32_t sample_rate;
	//int32_t num_channels;
	//int32_t delay_length;
	int32_t delay_length_samples;
	int32_t dp;
	int32_t mod_count;
	int32_t mod_depth_samples;	
	int32_t mod_rate;
	int32_t mod_rate_step;	
	int16_t delay[MAX_FLANGER_DELAY_SAMPLES];
} FlangerContext;
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
* @brief Initialize the flanger module.
* @param ct Pointer to a FlangerContext object.
* @param sample_rate Sample rate.
* @param delay_length Delay length in millisecond. Range: 1~15ms
* @param mod_depth Modulation depth in millisecond, which should NOT be more than delay_length. Range: 0~delay_length
* @param mod_rate Modulation rate in 0.01Hz. For example, 2 for 0.02Hz, 30 for 0.30Hz, 1000 for 10.00Hz. Range: 0.00~10.00Hz in step of 0.01Hz
* @return error code. FLANGER_ERROR_OK means successful, other codes indicate error.
*/
int32_t flanger_init(FlangerContext *ct, int32_t sample_rate, int32_t delay_length, int32_t mod_depth, int32_t mod_rate);


/**
* @brief Apply flanger effect to a frame of PCM data (mono only)
* @param ct Pointer to a FlangerContext object.
* @param pcm_in PCM input buffer.
* @param pcm_out PCM output buffer.
*        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
* @param n Number of PCM samples to process.
* @param feedback Feedback percentage in use. Range: 0~100%
* @param dry The level of dry(direct) signals in the output. Range: 0%~100%.
* @param wet The level of wet(effect) signals in the output. Range: 0%~100%.
* @param mod_rate Modulation rate in 0.01Hz. For example, 2 for 0.02Hz, 30 for 0.30Hz, 1000 for 10.00Hz. Range: 0.00~10.00Hz in step of 0.01Hz
* @return error code. FLANGER_ERROR_OK means successful, other codes indicate error.
* @note Note that only mono signals are accepted.
*/
int32_t flanger_apply(FlangerContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t feedback, int32_t dry, int32_t wet, int32_t mod_rate);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __FLANGER_H__

