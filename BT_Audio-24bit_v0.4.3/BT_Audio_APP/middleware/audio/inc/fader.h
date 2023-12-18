/**
 *************************************************************************************
 * @file	fader.h
 * @brief	Fade-in & fade-out effect
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __FADER_H__
#define __FADER_H__

#include <stdint.h>

/** error code for fader */
typedef enum _FADER_ERROR_CODE
{
	FADER_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
	FADER_ERROR_ILLEGAL_FADE_TYPE,
	FADER_ERROR_ILLEGAL_FADE_SAMPLES,

	// No Error
	FADER_ERROR_OK = 0,					/**< no error              */
} FADER_ERROR_CODE;


/** Fader context */
typedef struct _FaderContext
{
	int32_t num_channels;
	int32_t fade_type;
	int32_t fade_samples;
	int32_t fade_counter;
} FaderContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize fade module.
 * @param ct Pointer to a FaderContext object. 
 * @param num_channels number of channels. Both 1 and 2 channels are supported.
 * @param fade_type Fade type. 0:by-pass, 1:fade-in(linear), 2:fade-out(linear)
 * @param fade_samples. Fade-in or fade-out duration in samples to perform. If fade_type is by-pass, this parameter is ignored.
 * @return error code. FADER_ERROR_OK means successful, other codes indicate error.
 * @note You need to initialize the fader again in order to let the fader start a new fade-in or fade-out process.
 */
int32_t fader_init(FaderContext *ct, int32_t num_channels, int32_t fade_type, int32_t fade_samples);


/**
 * @brief Apply fade effect.
 * @param ct Pointer to a FaderContext object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. FADER_ERROR_OK means successful, other codes indicate error. 
 */
int32_t fader_apply(FaderContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


/**
 * @brief Apply fade effect (24-bit).
 * @param ct Pointer to a FaderContext object.
 * @param pcm_in Address of the PCM input. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @return error code. FADER_ERROR_OK means successful, other codes indicate error.
 */
int32_t fader_apply24(FaderContext *ct, int32_t *pcm_in, int32_t *pcm_out, int32_t n);


/**
 * @brief Check whether fader is done with the current fade-in or fade-out process.
 * @param ct Pointer to a FaderContext object.
 * @return 0: not done yet. 1: done.
 * @note If fade_type is by-pass, 1(done) is always returned. 
 */
int32_t fader_is_done(FaderContext *ct);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__FADER_H__
