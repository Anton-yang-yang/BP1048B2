/**
 *************************************************************************************
 * @file	blue_ns_dual.h
 * @brief	Noise Suppression for Dual Microphone Signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.4.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __BLUE_NS_DUAL_H__
#define __BLUE_NS_DUAL_H__

#include <stdint.h>

#define NSDUAL_MAX_STEP 256
#define NSDUAL_MAX_L 512
#define NSDUAL_M 512
#define NSDUAL_HALF_M 256	// NSDUAL_M/2
#define NSDUAL_MEDFILT_N 31


/** error code for blue noise suppression for dual mic.*/
typedef enum _BLUENSDUAL_ERROR_CODE
{
	BLUENSDUAL_ERROR_SAMPLE_RATE_NOT_SUPPORTED = -256,
	BLUENSDUAL_ERROR_ILLEGAL_SAMPLE_RATE,
	BLUENSDUAL_ERROR_ILLEGAL_STEP_SIZE,
	BLUENSDUAL_ERROR_ILLEGAL_NS_LEVEL,
	
	// No Error
	BLUENSDUAL_ERROR_OK = 0,					/**< no error              */
} BLUENSDUAL_ERROR_CODE;


typedef struct MedfiltNode {
	float value;
	size_t index; // Node index in the sorted table
	struct MedfiltNode* parent;
	struct MedfiltNode* sorted;
} MedfiltNode;

typedef struct MedfiltData {
	MedfiltNode *kernel; // Working filter memory
	MedfiltNode *oldest; // Reference to the oldest value in the kernel
	size_t length; // Number of nodes
} MedfiltData;

/** Noise Suppression (Dual Microphone) Structure */
typedef struct _BlueNSDualContext
{	
	int32_t sample_rate;
	int32_t step_size;
	int32_t F0I;
	int32_t F1I;
	int32_t x1[NSDUAL_M];
	int32_t x2[NSDUAL_M];
	int16_t xin1_prev[NSDUAL_MAX_STEP];
	int16_t xin2_prev[NSDUAL_MAX_STEP];
	int16_t xout_prev[NSDUAL_MAX_STEP];	
	int16_t WIN[NSDUAL_MAX_L];
	float pnn[NSDUAL_HALF_M+1];
	float px11[NSDUAL_HALF_M + 1];
	float px22[NSDUAL_HALF_M + 1];
	float px12[(NSDUAL_HALF_M + 1)*2];
	float g[NSDUAL_HALF_M + 1];
	float gs[NSDUAL_HALF_M + 1];	
	float xratio_vad;
	float alpha_attack;
	float alpha_release;
	MedfiltData data;
	MedfiltNode nodes[NSDUAL_MEDFILT_N];
} BlueNSDualContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

/**
 * @brief Initialize dual microphone noise suppression module.
 * @param ct Pointer to a BlueNSDualContext object.
 * @param sample_rate Sample rate.
 * @param step_size Step size in samples. Value range: 128 ~ 256. This is the number of samples to process in each loop. 160 is a recommended value. Usually the lower the step size, the lower the delay but with higher MCPS (Mega Cycles Per Second). 
 * @return error code. BLUENSDUAL_ERROR_OK means successful, other codes indicate error. 
 * @note Choice of step_size may affect the MCPS consumption as well as the noise suppression effect. 
 */
int32_t blue_ns_dual_init(BlueNSDualContext *ct, int32_t sample_rate, int32_t step_size);


/**
 * @brief Apply dual microphone noise suppression to a block of PCM data
 * @param ct Pointer to a BlueNSDualContext object.
 * @param xin1 Input PCM data from main microphone (near mouth). The size of xin1 is equal to step_size set in blue_ns_dual_init().
 * @param xin2 Input PCM data from secondary microphone (far from mouth). The size of xin2 is equal to step_size set in blue_ns_dual_init().
 * @param xout Output PCM data. The size of xout is equal to step_size set in blue_ns_dual_init(). 
 *        xout can be the same as xin1 or xin2. In this case, the PCM is changed in-place.
 * @param ns_level Noise suppression level. Valid range: 0 ~ 2. Use 0 to disable noise suppression while 2 to apply maximum suppression. 
 * @return error code. BLUENSDUAL_ERROR_OK means successful, other codes indicate error.
 */
int32_t blue_ns_dual_run(BlueNSDualContext *ct, int16_t *xin1, int16_t *xin2, int16_t *xout, int32_t ns_level);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__BLUE_NS_DUAL_H__
