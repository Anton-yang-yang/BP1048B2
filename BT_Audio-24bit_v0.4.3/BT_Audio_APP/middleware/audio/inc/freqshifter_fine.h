/**
 *************************************************************************************
 * @file	freqshifter_fine.h
 * @brief	Frequency shifter with fine resolution for mono signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	V2.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef _FREQSHIFTER_FINE_H
#define _FREQSHIFTER_FINE_H

#include <stdint.h>

 /** error code for frequency shifter fine */
typedef enum _FREQSHIFTER_FINE_ERROR_CODE
{
	FREQSHIFTER_FINE_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,	
	FREQSHIFTER_FINE_ERROR_ILLEGAL_FREQUENCY_SHIFT,

	// No Error
	FREQSHIFTER_FINE_ERROR_OK = 0,					/**< no error              */
} FREQSHIFTER_FINE_ERROR_CODE;


typedef struct _FreqShifterFineContext
{
	int32_t d[4*4*2+1];		// delay buffer	
	int32_t sample_rate;	// sample rate
	int32_t deltaf;			// shift frequency in Hz
	int32_t deltaf_step;
	int32_t deltaf_count;
} FreqShifterFineContext;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
 * @brief Initialize frequency shifter module
 * @param ct Pointer to a FreqShifterFineContext object. 
 * @param sample_rate Sample rate.
 * @param deltaf Frequency shift in 0.01Hz. For example, -3 for -0.03Hz, 51 for 0.51Hz, 666 for 6.66Hz. Range: -10.00~10.00Hz in step of 0.01Hz
 * @return error code. FREQSHIFTER_FINE_ERROR_OK means successful, other codes indicate error.
 * @note Input signals should be always mono PCM values.
 */
int32_t freqshifter_fine_init(FreqShifterFineContext* ct, int32_t sample_rate, int32_t deltaf);


/**
 * @brief Apply frequency shifting to a frame of PCM data.
 * @param ct Pointer to a FreqShifterFineContext object.
 * @param pcm_in Address of the PCM input.
 * @param pcm_out Address of the PCM output.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process. 
 * @return none.
 * @note Input signals should be always mono PCM values.
 */
void freqshifter_fine_apply(FreqShifterFineContext* ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply frequency shifting to a frame of PCM data (24-bit).
 * @param ct Pointer to a FreqShifterFineContext object.
 * @param pcm_in Address of the PCM input.
 * @param pcm_out Address of the PCM output.
 *        pcm_out can be the same as pcm_in. In this case, the PCM is changed in-place.
 * @param n Number of PCM samples to process.
 * @return none.
 * @note Input signals should be always mono PCM values.
 */
void freqshifter_fine_apply24(FreqShifterFineContext* ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // _FREQSHIFTER_FINE_H

