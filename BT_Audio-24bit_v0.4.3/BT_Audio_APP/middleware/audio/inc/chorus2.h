/**
*************************************************************************************
* @file	    chorus2.h
* @brief	Chorus effect for mono signals with 2 oscillators
*
* @author	ZHAO Ying (Alfred)
* @version	v2.0.0
*
* &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
*************************************************************************************
*/

#ifndef __CHORUS2_H__
#define __CHORUS2_H__

#include <stdint.h>

#define MAX_CHORUS2_DELAY_SAMPLES (30*2*48+2) // maximum delay in samples 30ms*2@48kHz

/** error code for chorus */
typedef enum _CHORUS2_ERROR_CODE
{
	CHORUS2_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,	
	CHORUS2_ERROR_ILLEGAL_DELAY_LENGTH,
	CHORUS2_ERROR_ILLEGAL_MODULATION_DEPTH,
	CHORUS2_ERROR_ILLEGAL_MODULATION_RATE,
	CHORUS2_ERROR_ILLEGAL_DRY,
	CHORUS2_ERROR_ILLEGAL_WET,

	// No Error
	CHORUS2_ERROR_OK = 0,					/**< no error              */
} CHORUS2_ERROR_CODE;

/** chorus context */
typedef struct _Chorus2Context
{
	int32_t sample_rate;
	//int32_t num_channels;
	int32_t delay_length;
	int32_t delay_length_samples;
	int32_t dp;	
	int32_t mod1_depth;
	int32_t mod1_depth_now;
	int32_t mod1_depth_target;
	int32_t mod1_rate;
	int32_t mod1_rate_step;
	int32_t mod1_count;	
	int32_t mod2_depth;
	int32_t mod2_depth_now;
	int32_t mod2_depth_target;
	int32_t mod2_rate;
	int32_t mod2_rate_step;
	int32_t mod2_count;
	int16_t delay[MAX_CHORUS2_DELAY_SAMPLES];
} Chorus2Context;
#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
* @brief Initialize the chorus module.
* @param ct Pointer to a ChorusContext object.
* @param sample_rate Sample rate.
* @param delay_length Delay length in millisecond. Range: 1~30ms
* @return error code. CHORUS_ERROR_OK means successful, other codes indicate error.
*/
int32_t chorus2_init(Chorus2Context *ct, int32_t sample_rate, int32_t delay_length);


/**
* @brief Apply chorus effect to a frame of PCM data (mono only)
* @param ct Pointer to a ChorusContext object.
* @param pcm_in PCM input buffer.
* @param pcm_out PCM output buffer.
*        pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
* @param n Number of PCM samples to process.
* @param dry The level of dry(direct) signals in the output. Range: 0~100%.
* @param wet1 The level of wet(effect) signals from oscillator 1 in the output. Range: 0~100%.
* @param wet2 The level of wet(effect) signals from oscillator 2 in the output. Range: 0~100%.
* @param mod1_depth Modulation depth in % for oscillator 1. Range: 0~100%
* @param mod1_rate Modulation rate in 0.01Hz for oscillator 1. For example, 2 for 0.02Hz, 30 for 0.30Hz, 1000 for 10.00Hz. Range: 0.00~10.00Hz in step of 0.01Hz
* @param mod2_depth Modulation depth in % for oscillator 2. Range: 0~100%
* @param mod2_rate Modulation rate in 0.01Hz for oscillator 2. For example, 2 for 0.02Hz, 30 for 0.30Hz, 1000 for 10.00Hz. Range: 0.00~10.00Hz in step of 0.01Hz
* @return error code. CHORUS_ERROR_OK means successful, other codes indicate error.
* @note Note that only mono signals are accepted.
*/
int32_t chorus2_apply(Chorus2Context *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t dry, int32_t wet1, int32_t wet2, int32_t mod1_depth, int32_t mod1_rate, int32_t mod2_depth, int32_t mod2_rate);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // __CHORUS2_H__

