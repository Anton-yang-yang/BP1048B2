/**
 *************************************************************************************
 * @file	engine_sound.h
 * @brief	Simulation of engine sound
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __ENGINE_SOUND_H__
#define __ENGINE_SOUND_H__

#include <stdint.h>
#include "pitch_shifter.h"


/** error code for engine sound */
typedef enum _ES_ERROR_CODE
{
	ENGINESOUND_ERROR_ILLEGAL_RPM_MIN = -240,
	ENGINESOUND_ERROR_ILLEGAL_RPM_MAX,
	ENGINESOUND_ERROR_ILLEGAL_FRAME_SIZE,
	ENGINESOUND_ERROR_ILLEGAL_CALLBACK_FUNCTION,
	ENGINESOUND_ERROR_RPM_CHANGE_TOO_FAST,
	ENGINESOUND_ERROR_RPM_CHANGE_NOT_ALLOWED,

	// No Error
	ENGINESOUND_ERROR_OK = 0,					/**< no error              */
} ES_ERROR_CODE;


/**
 * @brief Callback function to read "n" PCM samples from sound source indexed by "sound_idx". The PCM samples are saved in buffer "pcm_buf"(mono)
 * @param sound_idx index of the engine sound clip to play
 *     -1: engine startup sound if required (played once only)
 *		0: engine sound in idle state (minimum RPM)
 *		1: engine sound at 1000 RPM
 *		2: engine sound at 2000 RPM
 *		...
 *		N: engine sound at N*1000 RPM (maximum RPM)
 * @return actual number of PCM samples read. For sound_idx from 0~N, n samples should always be satisfied (loop playback); 
 *         for sound_idx=-1, actual number of PCM samples read is returned (play once only) so that the caller knows the engine startup stage is done.
 */
typedef int32_t(*EngineSoundPCMReadCallback)(int32_t sound_idx, int32_t n, int16_t *pcm_buf);


/** Engine Sound Context */
typedef struct _EngineSoundContext
{
	//int32_t num_channels;							// number of channels
	int32_t sample_rate;							// sample rate
	int32_t w;
	EngineSoundPCMReadCallback cb;
	int32_t frame_state;
	int32_t sound1, sound2;
	PSContext ct1, ct2;
	PSContext *pct1, *pct2;
	int32_t rpm, rpmfi, rpma, rpmb, rpm_min, rpm_max;
	int16_t pcm1[MAX_W_SIZE / 2];
	int16_t pcm2[MAX_W_SIZE / 2];
} EngineSoundContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize the engine sound module
 * @param ct Pointer to an EngineSoundContext object.
 * @param sample_rate Sample rate.
 * @param rpm_min RPM (Round Per Minute) value when the engine is in idle state. 700 is a recommended value. Range: 500 <= rpm_min < 1000
 * @param rpm_max RPM (Round Per Minute) value when the engine is running at the maximum speed. e.g. 5000. This value should be in thousands, more than rpm_min and >= 1000.
 * @param n Number of samples to process in each loop. This value should be in range [128, MAX_W_SIZE/2] and affects both the CPU consumption and the simulation performance.
 * @param cb Callback function that is able to read n PCM samples from a specified sound source.
 * @param startup_sound_required Whether engine startup sound is required. 0:not required, 1:required. If engine startup sound is required, it is played once only at the initial stage.
 * @return error code. ENGINESOUND_ERROR_OK means successful, other codes indicate error. 
 * @note Upon successful initialization, the engine's RPM is set at rpm_min. The user is responsible to provide the sound at each thousand RPM level.
 * For example, if rpm_min is set at 700 and rpm_max at 5000, the callback function will try to read one from the 6 sound sources, i.e.
 * sounds of engine speed at 700, 1000, 2000, 3000, 4000 & 5000 RPM respectively. If engine startup sound is required(startup_sound_required=1), the callback
 * function will try to read engine startup sound(sound_idx=-1) at the initial stage (only once). Please notice that the sound of engine at idle state will 
 * be played right after the engine startup sound. So make sure these two sound clips can be smoothly played, otherwise abrupt noise or distortion can be
 * heard due to the discontinuity between two clips if occured. Except the engine startup sound, other sound clips will be played in loop. So make sure these 
 * sound clips can be continuously played from the end of clip end back to the start of the clip itself.
 */
int32_t engine_sound_init(EngineSoundContext *ct, int32_t sample_rate, int32_t rpm_min, int32_t rpm_max, int32_t n, EngineSoundPCMReadCallback cb, int32_t startup_sound_required);


/**
 * @brief Configure the engine sound module
 * @param ct Pointer to an EngineSoundContext object.
 * @param rpm Current RPM (Round Per Minute) of the engine to set. Range:[rpm_min, rpm_max]
 * @return error code. ENGINESOUND_ERROR_OK means successful, other codes indicate error.
 * @note Please notice that RPM is not allowed to change during engine startup stage.
 */
int32_t engine_sound_configure(EngineSoundContext *ct, int32_t rpm);


/**
 * @brief Apply simulation of engine sound at current RPM.
 * @param ct Pointer to an EngineSoundContext object.
 * @param pcm_out Address of the PCM output (mono).
 * @return error code. ENGINESOUND_ERROR_OK means successful, other codes indicate error.
 * @note At the initial stage, engine startup sound will be played once if startup_sound_required is set true and the sound of engine at idle state follows.
 * The sound of engine is played in loop at current RPM until it is changed by engine_sound_configure().
 */
int32_t engine_sound_apply(EngineSoundContext *ct, int16_t *pcm_out);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__ENGINE_SOUND_H__
