/**
 *************************************************************************************
 * @file	howling_suppressor_fine.h
 * @brief	Howling detection & suppression with fine resolution for mono signals
 *
 * @author	Zhao Ying (Alfred)
 * @version	V3.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef _HOWLING_SUPPRESSOR_FINE_H
#define _HOWLING_SUPPRESSOR_FINE_H

#include <stdint.h>

#define MAX_PAST_FRAMES 12
#define MAX_FILTERS 4

 /** error code for howling suppressor fine */
typedef enum _HOWLING_SUPPRESSOR_FINE_ERROR_CODE
{
	HOWLING_SUPPRESSOR_FINE_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	HOWLING_SUPPRESSOR_FINE_ERROR_ILLEGAL_Q,

	// No Error
	HOWLING_SUPPRESSOR_FINE_ERROR_OK = 0,					/**< no error              */
} HOWLING_SUPPRESSOR_FINE_ERROR_CODE;

typedef struct _HowlingFineContext
{	
	int32_t sample_rate;
	int32_t lowfreq_limit;
	int32_t highfreq_limit;
	int32_t ibuf;
	int32_t howling_detected;
	int32_t lpd[4];
	//int16_t pcm_win[512];
	int32_t pcm_win[512];
	int16_t howling_candidates[MAX_PAST_FRAMES *MAX_FILTERS];
	int32_t fc[MAX_FILTERS];
	int32_t fc_new[MAX_FILTERS];
	float   fchz[MAX_FILTERS];
	int32_t fba[MAX_FILTERS][3];
	int32_t d[MAX_FILTERS][4];
	int32_t dp[MAX_FILTERS][4];
	int16_t pcm_buf[512 * 2];
	int32_t x[512 * 2];
	uint8_t peaks[512];
} HowlingFineContext;



#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Initialize howling suppressor module
 * @param ct Pointer to a HowlingFineContext object. 
 * @param sample_rate Sample rate.
 * @return error code. HOWLING_SUPPRESSOR_FINE_ERROR_OK means successful, other codes indicate error.
 */
int32_t howling_suppressor_fine_init(HowlingFineContext *ct, int32_t sample_rate);


/**
 * @brief Apply howling detection & suppression to a frame of PCM data.
 * @param ct Pointer to a HowlingFineContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n Number of PCM samples to process. It should be an even number between 128 ~ 512 (both inclusive). It is recommended to keep n constant between frames although it can be varied. Usually, the smaller the value of n, the greater the amount of MCPS consumed.
 * @param q_min Minimum Q factor in Q6.10 format of the notch filters for howling suppression. Q factor is defined as the ratio of a filter's center frequency to its 3dB bandwidth. Range: 256 ~ 16384 to stand for 0.25 ~ 16
 * @param q_max Maximum Q factor in Q6.10 format of the notch filters for howling suppression. Q factor is defined as the ratio of a filter's center frequency to its 3dB bandwidth. Range: 256 ~ 16384 to stand for 0.25 ~ 16
 * @return none.
 * @note Input signals should be mono. 
 *       q_min should be less than or equal to q_max:
 *       - If q_min = q_max, the notch filter's quality factor is fixed at q_min or q_max.
 *       - If q_min < q_max, the notch filter's quality factor is auto adjusted between q_min and q_max.
 */
int32_t howling_suppressor_fine_apply(HowlingFineContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t q_min, int32_t q_max);


/**
 * @brief Apply howling detection & suppression to a frame of PCM data (24-bit).
 * @param ct Pointer to a HowlingFineContext object.
 * @param pcm_in PCM input buffer.
 * @param pcm_out PCM output buffer. pcm_out can be the same as pcm_in. In this case, the PCM data is changed in-place.
 * @param n Number of PCM samples to process. It should be an even number between 128 ~ 512 (both inclusive). It is recommended to keep n constant between frames although it can be varied. Usually, the smaller the value of n, the greater the amount of MCPS consumed.
 * @param q_min Minimum Q factor in Q6.10 format of the notch filters for howling suppression. Q factor is defined as the ratio of a filter's center frequency to its 3dB bandwidth. Range: 256 ~ 16384 to stand for 0.25 ~ 16
 * @param q_max Maximum Q factor in Q6.10 format of the notch filters for howling suppression. Q factor is defined as the ratio of a filter's center frequency to its 3dB bandwidth. Range: 256 ~ 16384 to stand for 0.25 ~ 16
 * @return none.
 * @note Input signals should be mono.
 *       q_min should be less than or equal to q_max:
 *       - If q_min = q_max, the notch filter's quality factor is fixed at q_min or q_max.
 *       - If q_min < q_max, the notch filter's quality factor is auto adjusted between q_min and q_max.
 */
int32_t howling_suppressor_fine_apply24(HowlingFineContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int32_t q_min, int32_t q_max);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _HOWLING_SUPPRESSOR_FINE_H
