/**
 *************************************************************************************
 * @file	beat_tracker.h
 * @brief	Tempo estimation and beat tracking
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __BEAT_TRACKER_H__
#define __BEAT_TRACKER_H__

#include <stdint.h>

// Do not change the following macros !
#define BEATTRACKER_BUFFER_SIZE 1024
#define BEATTRACKER_HOP_SIZE 512
#define BEATTRACKER_SPEC_LEN 513		// =BEATTRACKER_BUFFER_SIZE/2+1
#define BEATTRACKER_ONSET_KEEP_SIZE 7	// 5(post)+1(pre)+1
#define BEATTRACKER_WINLEN 512
#define BEATTRACKER_STEP 128			// BEATTRACKER_WINLEN/4
#define BEATTRACKER_LAGLEN 128			// BEATTRACKER_WINLEN/4


 /** error code for beat tracker */
typedef enum _BEATTRACKER_ERROR_CODE
{
	BEATTRACKER_ERROR_NUMBER_OF_CHANNELS_NOT_SUPPORTED = -256,
	BEATTRACKER_ERROR_SAMPLE_RATE_NOT_SUPPORTED,
	BEATTRACKER_ERROR_SILENCE_THRESHOLD_OUT_OF_RANGE,

	// No Error
	BEATTRACKER_ERROR_OK = 0,					/**< no error              */
} BEATTRACKER_ERROR_CODE;

/** beat tracking structure */
typedef struct _BeatTrackerContext
{
	int32_t sample_rate;
	int32_t num_channels;
	int32_t blockpos;	
	int32_t timesig;
	int32_t counter;
	int32_t flagstep;
	int32_t outnum;
	uint32_t silence_th;
	float lastbeat;
	float rayparam;
	float gp;
	float rp1;
	float rp2;
	float bp;
	float g_var;
	uint16_t rwv[BEATTRACKER_LAGLEN];
	uint16_t gwv[BEATTRACKER_LAGLEN];
	uint16_t phwv[BEATTRACKER_LAGLEN * 2];
	uint16_t dfwv[BEATTRACKER_WINLEN];
	int16_t xin_prev[BEATTRACKER_BUFFER_SIZE- BEATTRACKER_HOP_SIZE];
	int32_t xv[BEATTRACKER_BUFFER_SIZE];
	int32_t grain_norm[BEATTRACKER_SPEC_LEN];
	int32_t onset_keep[BEATTRACKER_ONSET_KEEP_SIZE];
	int32_t dfframe[BEATTRACKER_WINLEN];
	int32_t dfrev[BEATTRACKER_WINLEN];
	int32_t phout[BEATTRACKER_WINLEN];
	int32_t acf[BEATTRACKER_WINLEN * 2];
	float output[BEATTRACKER_STEP];
	
} BeatTrackerContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Initialize the beat tracker module.
 * @param ct Pointer to a BeatTrackerContext object.
 * @param num_channels Number of channels. Set 1 for mono signals and 2 for stereo signals.
 * @param sample_rate Sample rate. Sample rate lower than 32000 Hz is not supported.
 * @param silence_threshold Silence threshold in dB below which the signals are considered silence and beat indication is switched off. Range: -90 ~ -30 dB.
 * @return error code. BEATTRACKER_ERROR_OK means successful, other codes indicate error.
 */
int32_t beat_tracker_init(BeatTrackerContext *ct, int32_t num_channels, int32_t sample_rate, int32_t silence_threshold);


/**
 * @brief Detect beat for current frame of PCM data.
 * @param ct Pointer to a BeatTrackerContext object.
 * @param pcm_in Input PCM data. The size of pcm_in is equal to BEATTRACKER_HOP_SIZE * no. of channels. For stereo signals, left & right values should be interleaved, i.e. L,R,L,R,...; for mono, only the first BEATTRACKER_HOP_SIZE values in buffer need to be filled.
 * @return whether current frame is a beat or not. 0: no, 1: yes.
 * @note This function calculates the characteristics of current frame and updates the tempo and beat locations every BEATTRACKER_STEP frames.
 *       Thus when this frame is an "update" frame, the CPU consumption will be much higher than usual frames.
 */
int32_t beat_tracker_detect(BeatTrackerContext *ct, int16_t pcm_in[BEATTRACKER_HOP_SIZE * 2]);


/**
 * @brief Obtain the latest BPM (Beat Per Minute) value.
 * @param ct Pointer to a BeatTrackerContext object.
 * @return the latest BPM (Beat Per Minute) value.
 * @note This function should be called after initialization. BPM value is usually kept the same until beat_tracker_detect() makes the change.
 */
float beat_tracker_get_bpm(BeatTrackerContext *ct);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__BEAT_TRACKER_H__
