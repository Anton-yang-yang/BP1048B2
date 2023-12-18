/**
 *************************************************************************************
 * @file	echo.h
 * @brief	Echo effect
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __ECHO_H__
#define __ECHO_H__

#include <stdint.h>

/** error code for echo effect */
typedef enum _ECHO_ERROR_CODE
{
    ECHO_ERROR_CHANNEL_NUMBER_NOT_SUPPORTED = -256,
	ECHO_ERROR_SAMPLE_RATE_NOT_SUPPORTED,
	ECHO_ERROR_ILLEGAL_MAX_DELAY,
	ECHO_ERROR_ILLEGAL_BUFFER_POINTER,
	ECHO_ERROR_DELAY_TOO_LARGE,	
	ECHO_ERROR_DELAY_NOT_POSITIVE,
	ECHO_ERROR_ILLEGAL_DRY,
	ECHO_ERROR_ILLEGAL_WET,
	ECHO_ERROR_ILLEGAL_QUALITY_MODE,

	// No Error
	ECHO_ERROR_OK = 0,					/**< no error */
} ECHO_ERROR_CODE;


/** echo context */
typedef struct _EchoContext
{
	int32_t num_channels;		// number of channels
	int32_t lpb, lpa;			// filter coefficients
	int32_t lpd[2];				// filter delays
	int32_t max_delay_samples;	// maximum delay in samples	
	int32_t prev_delay_samples;	// previous delay samples	
	int32_t p;					// next position for overwriting
	int32_t quality_mode;		// quality mode

	int16_t in_block[32];		// input block
	int16_t delay_block[32];	// delay block
	int16_t delay_block1[32];	// delay block 1	
	int16_t prevsample;			// previous samples
	int16_t prevsample1;
	int8_t  index;				// encoding index
	int8_t  index1;
	int8_t  index2;
	int8_t  index3;
	uint8_t *s;					// delay buffer
} EchoContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize echo effect module
 * @param ct Pointer to an EchoContext object.
 * @param num_channels Number of channels.
 * @param sample_rate Sample rate.
 * @param fc Cutoff frequency of the low-pass filter in Hz. Set 0 to disable the use of the low-pass filter in echo effect. Note that this value should not exceed half of the sample rate, i.e. Nyquist frequency.
 * @param max_delay_samples Maximum delay in samples. This number should be positive. For example if you'd like to have maximum 500ms delay at 44.1kHz sample rate, the max_delay_samples = delay time*sample rate = 500*44.1 = 22050.
 * @param quality_mode Quality mode of echo. 0: compressed, 1: uncompressed 16 bit PCM, 2: uncompressed 24-bit PCM. 
 *        for 16-bit PCM I/O, choose either 0 or 1. for 24-bit PCM I/O, choose either 0 or 2.
 * @param s Delay buffer pointer. This buffer should be allocated by the caller and its capacity depends on both "quality_mode" and "max_delay_samples".
 *        If quality_mode is set 2, the buffer capacity = "max_delay_samples*4" in bytes. For example if max_delay_samples = 22050, then the buffer capacity should be 88200 bytes (22050*4=88200)
 *        If quality_mode is set 1, the buffer capacity = "max_delay_samples*2" in bytes. For example if max_delay_samples = 22050, then the buffer capacity should be 44100 bytes (22050*2=44100)
 *        If quality_mode is set 0, the buffer capacity = "ceil(max_delay_samples/32)*19" in bytes. For example if max_delay_samples = 22050, then the buffer capacity should be 13110 bytes (ceil(22050/32)*19=690*19=13110)
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 */
int32_t echo_init(EchoContext *ct, int32_t num_channels, int32_t sample_rate, int32_t fc, int32_t max_delay_samples, int32_t quality_mode, uint8_t *s);


/**
 * @brief Apply echo effect to a frame of PCM data.
 * @param ct Pointer to a EchoContext object.
 * @param pcm_in Address of the PCM input. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param attenuation attenuation coefficient. Q1.15 format to represent value in range from 0 to 1. For example, 8192 represents 0.25 as the attenuation coefficient.
 * @param delay_samples Delay in samples. Range: 1 ~ max_delay_samples.
 * @param dry The level of dry(direct) signals in the output. Range: 0% ~ 100%.
 * @param wet The level of wet(effect) signals in the output. Range: 0% ~ 100%.
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 */
int32_t echo_apply(EchoContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int16_t attenuation, int32_t delay_samples, int32_t dry, int32_t wet);


/**
 * @brief Apply echo effect to a frame of PCM data (24-bit)
 * @param ct Pointer to a EchoContext object.
 * @param pcm_in Address of the PCM input. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The data layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @param attenuation attenuation coefficient. Q1.15 format to represent value in range from 0 to 1. For example, 8192 represents 0.25 as the attenuation coefficient.
 * @param delay_samples Delay in samples. Range: 1 ~ max_delay_samples.
 * @param dry The level of dry(direct) signals in the output. Range: 0% ~ 100%.
 * @param wet The level of wet(effect) signals in the output. Range: 0% ~ 100%.
 * @return error code. ECHO_ERROR_OK means successful, other codes indicate error.
 */
int32_t echo_apply24(EchoContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int16_t attenuation, int32_t delay_samples, int32_t dry, int32_t wet);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__ECHO_H__
