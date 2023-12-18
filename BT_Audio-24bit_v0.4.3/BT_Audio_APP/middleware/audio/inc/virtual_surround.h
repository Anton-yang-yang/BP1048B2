/**
 *************************************************************************************
 * @file	virtual_surround.h
 * @brief	Virtual surround effect for stereo headphones.
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __VIRTUAL_SURROUND_H__
#define __VIRTUAL_SURROUND_H__

#include <stdint.h>

 /** error code for virtual surround */
typedef enum _VIRTUAL_SURROUND_ERROR_CODE
{
	VIRTUAL_SURROUND_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	VIRTUAL_SURROUND_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED,
	// No Error
	VIRTUAL_SURROUND_ERROR_OK = 0,					/**< no error              */
} VIRTUAL_SURROUND_ERROR_CODE;


/** virtual surround context */
typedef struct _VirtualSurroundContext
{
	int32_t sample_rate;
	int32_t num_channels;
	const uint8_t *gd;
	const int16_t(*b)[2];
	int32_t a;
	int32_t d[2][8][34+1];
	int16_t dp[2][8];
	int16_t fdb[2][8];
	int32_t fda[2][8];
} VirtualSurroundContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Initialize virtual surround audio effect module
 * @param ct Pointer to a VirtualSurroundContext object.
 * @param num_channels number of input channels. Range: 2,4,6,8. The following channel layouts are assumed according to the number of channels:
 * -----------------------------------------------------
 * #channels    1    2    3    4    5    6    7    8
 * -----------------------------------------------------
 *    2         L    R
 *    4(3.1)    L    R    C    LFE
 *    6(5.1)    L    R    C    LFE  Ls   Rs
 *    8(7.1)    L    R    C    LFE  Ls   Rs   Lsd  Rsd
 * -----------------------------------------------------
 * @param sample_rate sample raten in Hz. Only 48000, 44100 and 32000 are supported.
 * @return error code. VIRTUAL_SURROUND_ERROR_OK means successful, other codes indicate error. 
 */
int32_t virtual_surround_init(VirtualSurroundContext *ct, int32_t num_channels, int32_t sample_rate);


/**
 * @brief Apply virtual surround audio effect to a frame of PCM data.
 * @param ct Pointer to a VirtualSurroundContext object.
 * @param pcm_in Address of the PCM input. The PCM layout must be interleaved according to the channel layout as described in virtual_surround_init().
 * @param pcm_out Address of the PCM output. The data layout is always stereo: L0,R0,L1,R1,L2,R2,...
          pcm_out CANNOT be the same as pcm_in!
 * @param n Number of PCM samples to process.
 * @return error code. VIRTUAL_SURROUND_ERROR_OK means successful, other codes indicate error.
 */
int32_t virtual_surround_apply(VirtualSurroundContext *ct, int16_t *pcm_in, int16_t *pcm_out, int32_t n);


#ifdef __cplusplus
}
#endif//__cplusplus
#endif // __VIRTUAL_SURROUND_H__
