/**
 *************************************************************************************
 * @file	mixer.h
 * @brief	Mixers for channels and sources
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __MIXER_H__
#define __MIXER_H__

#include <stdint.h>
#include "mvintrinsics.h"


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

// -------------------- Channel Mixer --------------------

/**
 * @brief Downmix 2 channels into 1 channel
 * @param pcm_in Address of the stereo PCM input. The PCM layout: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the mono PCM output. The PCM layout: M0,M1,M2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.		
 * @param n Number of PCM samples to process.
 * @return none. 
 */
__STATIC_INLINE void downmix_2to1_apply(int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
	for (int i = 0; i < n; i++)
		pcm_out[i] = ((int32_t)pcm_in[i * 2] + (int32_t)pcm_in[i * 2 + 1] + 1) >> 1; // No saturation is required here. Don't waste MCPS.
}

/**
 * @brief Downmix 2 channels into 1 channel (24-bit)
 * @param pcm_in Address of the stereo PCM input. The PCM layout: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the mono PCM output. The PCM layout: M0,M1,M2,...
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n Number of PCM samples to process.
 * @return none.
 */
__STATIC_INLINE void downmix_2to1_apply24(int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
	for (int i = 0; i < n; i++)
		pcm_out[i] = (pcm_in[i * 2] + pcm_in[i * 2 + 1] + 1) >> 1; // No saturation is required here. Don't waste MCPS.
}


/**
 * @brief Upmix 1 channel to 2 channels
 * @param pcm_in Address of the mono PCM input. The PCM layout: M0,M1,M2,...
 * @param pcm_out Address of the stereo PCM output. The PCM layout: L0,R0,L1,R1,L2,R2,... 
 *        pcm_out CANNOT be the same as pcm_in.
 * @param n Number of PCM samples to process.
 * @return none.
 */
__STATIC_INLINE void upmix_1to2_apply(int16_t *pcm_in, int16_t *pcm_out, int32_t n)
{
	for (int i = 0; i < n; i++)
	{
		int16_t t = pcm_in[i];
		pcm_out[i * 2] = t;
		pcm_out[i * 2 + 1] = t;
	}
}

/**
 * @brief Upmix 1 channel to 2 channels (24-bit)
 * @param pcm_in Address of the mono PCM input. The PCM layout: M0,M1,M2,...
 * @param pcm_out Address of the stereo PCM output. The PCM layout: L0,R0,L1,R1,L2,R2,...
 *        pcm_out CANNOT be the same as pcm_in.
 * @param n Number of PCM samples to process.
 * @return none.
 */
__STATIC_INLINE void upmix_1to2_apply24(int32_t *pcm_in, int32_t *pcm_out, int32_t n)
{
	for (int i = 0; i < n; i++)
	{
		int32_t t = pcm_in[i];
		pcm_out[i * 2] = t;
		pcm_out[i * 2 + 1] = t;
	}
}


// -------------------- Source Mixer --------------------

/**
 * @brief Mix 2 sources into 1
 * @param pcm_in1 Address of the PCM input 1. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_in2 Address of the PCM input 2. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in1 or pcm_in2. In this case, the PCM signals are changed in-place.
 * @param num_channels number of channels. Both 1 and 2 channels are supported.
 * @param n Number of PCM samples to process.
 * @return none.
 * @note Input 1 & Input 2 should have the same number of channels.
 */
__STATIC_INLINE void mix_2sources_apply(int16_t *pcm_in1, int16_t *pcm_in2, int16_t *pcm_out, int32_t num_channels, int32_t n)
{
	if (num_channels == 1)
	{
		for (int i = 0; i < n; i++)
		{
			int32_t t = (int32_t)pcm_in1[i] + (int32_t)pcm_in2[i];
			pcm_out[i] = CLIP_16BIT(t);
		}
	}
	else // num_channels == 2 is assumed
	{
		int k = 0;
		for (int i = 0; i < n; i++)
		{
			int32_t t;			
			t = (int32_t)pcm_in1[k] + (int32_t)pcm_in2[k];
			pcm_out[k++] = CLIP_16BIT(t);
			t = (int32_t)pcm_in1[k] + (int32_t)pcm_in2[k];
			pcm_out[k++] = CLIP_16BIT(t);
		}
	}
}


/**
 * @brief Mix 2 sources into 1 (24-bit)
 * @param pcm_in1 Address of the PCM input 1. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_in2 Address of the PCM input 2. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 * @param pcm_out Address of the PCM output. The PCM layout for mono: M0,M1,M2,...; for stereo: L0,R0,L1,R1,L2,R2,...
 *        pcm_out can be the same as pcm_in1 or pcm_in2. In this case, the PCM signals are changed in-place.
 * @param num_channels number of channels. Both 1 and 2 channels are supported.
 * @param n Number of PCM samples to process.
 * @return none.
 */
__STATIC_INLINE void mix_2sources_apply24(int32_t *pcm_in1, int32_t *pcm_in2, int32_t *pcm_out, int32_t num_channels, int32_t n)
{
	if (num_channels == 1)
	{
		for (int i = 0; i < n; i++)
		{
			int32_t t = pcm_in1[i] + pcm_in2[i];
			pcm_out[i] = CLIP_24BIT(t);
		}
	}
	else // num_channels == 2 is assumed
	{
		int k = 0;
		for (int i = 0; i < n; i++)
		{
			int32_t t;
			t = pcm_in1[k] + pcm_in2[k];
			pcm_out[k++] = CLIP_24BIT(t);
			t = pcm_in1[k] + pcm_in2[k];
			pcm_out[k++] = CLIP_24BIT(t);
		}
	}
}


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__MIXER_H__
