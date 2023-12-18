/**
 *************************************************************************************
 * @file	howling_guard.h
 * @brief	Postprocessing after howling suppression as a final guard.
 *
 * @author	Zhao Ying (Alfred)
 * @version	V1.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef _HOWLING_GUARD_H
#define _HOWLING_GUARD_H

#include <stdint.h>
#include "noise_gate.h"


/** error code for howling suppressor fine */
typedef enum _HOWLING_GUARD_ERROR_CODE
{
	HOWLING_GUARD_ERROR_ILLEGAL_SATURATION_THRESHOLD = -256,
	HOWLING_GUARD_ERROR_ILLEGAL_HIGH_FREQ_THRESHOLD,
	HOWLING_GUARD_ERROR_ILLEGAL_HIGH_FREQ_ENERGY_RATIO_THRESHOLD,
	HOWLING_GUARD_ERROR_ILLEGAL_MAX_SATURATED_HIGH_FREQ_DURATION,
	HOWLING_GUARD_ERROR_ILLEGAL_MAX_SATURATED_DURATION,
	HOWLING_GUARD_ERROR_ILLEGAL_MUTE_PERIOD,
	HOWLING_GUARD_ERROR_ILLEGAL_NOISE_GATE_THRESHOLD,

	// No Error
	HOWLING_GUARD_ERROR_OK = 0,					/**< no error              */
} HOWLING_GUARD_ERROR_CODE;


typedef struct _HowlingGuardContext
{
	NoiseGateCT noisegate;
	int32_t sample_rate;
	int32_t saturation16;
	int32_t saturation24;
	int32_t high_freq_energy_ratio_threshold;
	int32_t max_saturated_high_freq_samples;
	int32_t max_saturated_samples;
	int32_t mute_samples;
	int32_t gh, a1, a2;
	int32_t d[4];
	int32_t saturated_high_freq_samples;
	int32_t saturated_samples;
	int32_t silence_samples;
	int32_t noisegate_on;	
	int32_t maxv;
	int32_t hpercent;
} HowlingGuardContext;

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize howling guard module
 * @param ct Pointer to a HowlingGuardContext object.
 * @param sample_rate Sample rate.
 * @param saturation_threshold Saturation threshold in 0.01dB above which the signals are considered saturated. Range: -600 ~ 0 for -6.00dB ~ 0.00dB. For example -50 for -0.50dB.
 * @param high_freq_threshold Frequencies above high_freq_threshold in Hz are considered as high frequencies. Range: 1000 ~ 4000 Hz.
 * @param high_freq_energy_ratio_threshold The frame is considered howling-like if the ratio of high frequency energy is above high_freq_energy_ratio_threshold. Range: 1 ~ 99 %
 * @param max_saturated_high_freq_duration When saturation with high ratio of high frequency energy occurs and continues for max_saturated_high_freq_duration time in millisecond, the mute operation will be executed. Range: 1 ~ 3000 ms
 * @param max_saturated_duration When saturation occurs and exceeds for max_saturated_duration time in millisecond, the mute operation will be executed. Range: 1 ~ 3000 ms
 * @param mute_period Period of mute operation in millisecond. Range: 2 ~ 3000 ms
 * @param noise_gate_threshold Threshold of noise gate in dB. When input PCM level goes up beyond this threshold, the mute operation will be stopped. Range: -90 ~ 0 dB
 * @return error code. HOWLING_GUARD_ERROR_OK means successful, other codes indicate error.
 * @note When either of the following two conditions occurs, the module will force the output to be 0, entering the mute state:
		1. saturation with high ratio of high frequency energy occurs and continues for max_saturated_high_freq_duration time
		2. saturation occurs and continues for max_saturated_duration time
		Condition 2 is a weak condition compared to Condition 1, thus usually max_saturated_duration is set to be greater than max_saturated_high_freq_duration unless Condtion 1 is intended to be bypassed.
		The mute operation will last for at least mute_period milliseconds until input PCM level goes up beyond noise_gate_threshold.
 */
int32_t howling_guard_init(HowlingGuardContext *ct, int32_t sample_rate, int32_t saturation_threshold,
		int32_t high_freq_threshold, int32_t high_freq_energy_ratio_threshold, int32_t max_saturated_high_freq_duration,
		int32_t max_saturated_duration, int32_t mute_period, int32_t noise_gate_threshold);


/**
 * @brief Apply howling guard to a frame of PCM data.
 * @param ct Pointer to a HowlingGuardContext object.
 * @param pcm_in Address of the PCM input. The PCM input should be mono only.
 * @param pcm_out Address of the PCM output. The PCM output is mono as well.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. HOWLING_GUARD_ERROR_OK means successful, other codes indicate error. 
 */
int32_t howling_guard_apply(HowlingGuardContext *ct, int16_t *inbuf, int16_t *outbuf, int32_t n);


/**
 * @brief Apply howling guard to a frame of PCM data (24-bit).
 * @param ct Pointer to a HowlingGuardContext object.
 * @param pcm_in Address of the PCM input. The PCM input should be mono only.
 * @param pcm_out Address of the PCM output. The PCM output is mono as well.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. HOWLING_GUARD_ERROR_OK means successful, other codes indicate error. 
 */
int32_t howling_guard_apply24(HowlingGuardContext *ct, int32_t *inbuf, int32_t *outbuf, int32_t n);
 

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _HOWLING_GUARD_H
