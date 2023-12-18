/**
 *************************************************************************************
 * @file	pitch_detector.h
 * @brief	Pitch detector for mono signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.1.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __PITCH_DETECTOR_H__
#define __PITCH_DETECTOR_H__

#include <stdint.h>

/** error code for pitch detector */
typedef enum _PITCHDETECTOR_ERROR_CODE
{
    PITCHDETECTOR_ERROR_WINDOW_SIZE_TOO_SMALL = -256,
	PITCHDETECTOR_ERROR_WINDOW_SIZE_TOO_LARGE,
	PITCHDETECTOR_ERROR_SAMPLE_RATE_NOT_SUPPORTED,
	PITCHDETECTOR_ERROR_ILLEGAL_PITCH_MIN,
	PITCHDETECTOR_ERROR_ILLEGAL_PITCH_MAX,
	PITCHDETECTOR_ERROR_ILLEGAL_STEP_SIZE,
	PITCHDETECTOR_ERROR_ILLEGAL_CONFIDENCE_THRESHOLD,
	
	// No Error
	PITCHDETECTOR_ERROR_OK = 0,					/**< no error              */
} PITCHDETECTOR_ERROR_CODE;


/** Pitch Detector context */
typedef struct _PitchDetectorContext
{
	int32_t sample_rate;
	int32_t pitch_min;
	int32_t pitch_max;
	int32_t max_lag;
    int32_t min_lag;
	int32_t max_hit_prev;
	int32_t window_size;
	int32_t step_size;
	int32_t confidence_threshold;
	int16_t pcm_buf[3200];	// size related to window_size
	float   v[1600+1];		// size related to max_lag
	int32_t B[2000+1][2];	// size related to pitch_max
} PitchDetectorContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief Initialize Pitch Detector module
 * @param ct Pointer to a PitchDetectorContext object.
 * @param sample_rate Sample rate. It should be no more than 48000 Hz.
 * @param pitch_min Minimum pitch in Hz to detect. It should be no less than 30 Hz and no more than pitch_max.
 * @param pitch_max Maximum pitch in Hz to detect. It should be no more than 2000 Hz.
 * @param window_size Analysis window size in samples. It should be no less than 2*ceil(sample_rate/pitch_min) and no more than 3200.
 *        Usually larger window size makes the estimation of pitch more accurately but demands higher computation cost.
 * @param step_size Step size in samples. It should be no more than window_size.
 * @param confidence_threshold Confidence threshold above which a valid pitch is confirmed. Range: 5000~10000 for 50.00% ~ 100%
 * @return error code. PITCHDETECTOR_ERROR_OK means successful, other codes indicate error.
 */
int32_t pitch_detector_init(PitchDetectorContext *ct, int32_t sample_rate, int32_t pitch_min, int32_t pitch_max, int32_t window_size, int32_t step_size, int32_t confidence_threshold);


/**
 * @brief Apply pitch detecting.
 * @param ct Pointer to a PitchDetectorContext object.
 * @param pcm_in Address of the PCM input. PCM input should be mono signals only.
 * @param[out] confidence Confidence level at which the pitch detected is believed to be correct. Range: 0~10000 for 0.00% ~ 100.00%.
 *             Notice that this confidence level may be lower than the confidence_threshold set in the initialization function.
 * @return pitch in Hz. The returned value is an error code if it is negative or no pitch is found if it is 0 (e.g. white noise)
 * @note The number of PCM samples to process is equal to the step_size set in pitch_detector_init(...).
 */
int32_t pitch_detector_apply(PitchDetectorContext *ct, int16_t *pcm_in, int32_t *confidence);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__PITCH_DETECTOR_H__
