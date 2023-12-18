/**
 *************************************************************************************
 * @file	compander.h
 * @brief	Compander that can perform dynamic range compression & expansion with 
 *          downward & upward option.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.2
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __COMPANDER_H__
#define __COMPANDER_H__

#include <stdint.h>


/** error code for compander */
typedef enum _COMPANDER_ERROR_CODE
{
	COMPANDER_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	COMPANDER_ERROR_NUM_CHANNELS_NOT_SUPPORTED,
	COMPANDER_ERROR_THRESHOLD_OUT_OF_RANGE,
	COMPANDER_ERROR_RATIO_OUT_OF_RANGE,
	COMPANDER_ERROR_UNKNOWN_TYPE,
	// No Error
	COMPANDER_ERROR_OK = 0
} COMPANDER_ERROR_CODE;


/** Compander context */
typedef struct _CompanderContext
{
	int32_t num_channels;
	int32_t sample_rate;	
	int32_t state;
	int32_t threshold;
	int32_t slope_below;
	int32_t slope_above;
	int32_t alpha_attack;
	int32_t alpha_release;
} CompanderContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize Compander module
 * @param ct Pointer to a CompanderContext object.
 * @param num_channels Number of channels. 
 * @param sample_rate Sample rate.
 * @param threshold The threshold determines the input level at which the compander will activate. 
 *        This happens when the signal is loud enough to cross this threshold level (upward expansion or downward compression) or 
 *        quiet enough to fall below it (downward expansion or upward compression).
 *        Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00dB ~ 0.00dB. For example, -2550 stands for -25.50dB threshold.
 * @param ratio_below  The output/input ratio in step of 0.01 when the input level is below the threshold.
 *        Range: 1 ~ 10000 to cover real ratio from 0.01 ~ 100.00.
 *        Assuming a hard knee characteristic and a steady-state input, the ratio is defined as R=(y−T)/(x−T)
 *        where R is the ratio. y is the output signal in dB. x is the input signal in dB. T is the threshold in dB.
 *        When R>1.00, i.e. ratio_below>100, the signals below threshold get quieter (downward expander).
 *        When R<1.00, i.e. ratio_below<100, the signals below threshold get louder (upward compressor).
 *        When R=1.00, i.e. ratio_below=100, the signals below threshold keep unchanged.
 * @param ratio_above  The input/output ratio in step of 0.01 when the input level is above the threshold.
 *        Range: 1 ~ 10000 to cover real ratio from 0.01 ~ 100.00.
 *        Assuming a hard knee characteristic and a steady-state input, the ratio is defined as R=(x−T)/(y−T)
 *        where R is the ratio. y is the output signal in dB. x is the input signal in dB. T is the threshold in dB.
 *        When R>1.00, i.e. ratio_above>100, the signals above threshold get quieter (downward compressor).
 *        When R<1.00, i.e. ratio_above<100, the signals above threshold get louder (upward expander).
 *        When R=1.00, i.e. ratio_above=100, the signals above threshold keep unchanged.
 * @param attack_time Amount of time (in millisecond) the compander will go from zero to full compression or from full to zero expansion.
 * @param release_time Amount of time (in millisecond) the compander will go from full to zero compression or from zero to full expansion.
 * @return error code. COMPANDER_ERROR_OK means successful, other codes indicate error.
 */
int32_t compander_init(CompanderContext *ct, int32_t num_channels, int32_t sample_rate, 
	int32_t threshold, int32_t ratio_below, int32_t ratio_above, int32_t attack_time, int32_t release_time);


/**
 * @brief Apply Dynamic Range Companding to a frame of PCM data.
 * @param ct Pointer to a CompanderContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param pregain Pregain applied to input before companding.
 *        Q4.12 format to represent value in range [0.000244140625, 8) to cover -72dB ~ 18dB.
 *        For example, 2052 represents x0.501 (-6dB), 2900 represents x0.708 (-3dB), 4096 represents x1.0 (0dB),  5786 represents x1.413 (+3dB), 16306 represents x3.981 (+12dB)
 * @return error code. COMPANDER_ERROR_OK means successful, other codes indicate error.
 */
int32_t compander_apply(CompanderContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t pregain);


/**
 * @brief Apply Dynamic Range Companding to a frame of PCM data (24-bit PCM in & out).
 * @param ct Pointer to a CompanderContext object.
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param pregain Pregain applied to input before companding.
 *        Q4.12 format to represent value in range [0.000244140625, 8) to cover -72dB ~ 18dB.
 *        For example, 2052 represents x0.501 (-6dB), 2900 represents x0.708 (-3dB), 4096 represents x1.0 (0dB),  5786 represents x1.413 (+3dB), 16306 represents x3.981 (+12dB)
 * @return error code. COMPANDER_ERROR_OK means successful, other codes indicate error.
 */
int32_t compander_apply24(CompanderContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int32_t pregain);


#ifdef __cplusplus
}
#endif //__cplusplus

#endif // __COMPANDER_H__
