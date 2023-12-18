/**
 *************************************************************************************
 * @file	blue_aec.h
 * @brief	Acoustic Echo Cancellation (AEC) routines for voice signals
 *
 * @author	ZHAO Ying (Alfred)
 * @version	V6.6.1
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 *
 *************************************************************************************
 */

#ifndef _BLUE_AEC_H
#define _BLUE_AEC_H

#include <stdint.h>


#define AEC_BLK_LEN 64		// block length in samples
#define AEC_FILTER_L 12		// filter length = AEC_BLK_LEN * AEC_FILTER_L


/** error code for blue noise suppression */
typedef enum _BLUEAEC_ERROR_CODE
{
	BLUEAEC_ERROR_ILLEGAL_ES_LEVEL = -256,

	// No Error
	BLUEAEC_ERROR_OK = 0,					/**< no error              */
} BLUEAEC_ERROR_CODE;

typedef struct _BlueAECContext
{
	// matrices
	int32_t wm[AEC_FILTER_L][AEC_BLK_LEN*2];
	int32_t um[AEC_FILTER_L][AEC_BLK_LEN*2];
	int32_t uwm[AEC_FILTER_L][AEC_BLK_LEN*2];

	// vectors
	int32_t umclz[AEC_FILTER_L];
	int32_t uv[AEC_BLK_LEN*2];
	int32_t yev[AEC_BLK_LEN*2];
	int32_t phiv[AEC_BLK_LEN*2];
	float pv[AEC_BLK_LEN+1];
	
	float yp[AEC_BLK_LEN + 1];
	float sy[AEC_BLK_LEN + 1];
	float sym[AEC_BLK_LEN + 1];

	int64_t dd[AEC_BLK_LEN+1];
	int64_t uu[AEC_BLK_LEN+1];
	int64_t ee[AEC_BLK_LEN+1];
	int64_t ud[AEC_BLK_LEN*2];
	int64_t ed[AEC_BLK_LEN*2];

	int16_t cud[AEC_BLK_LEN+1];
	int16_t ced[AEC_BLK_LEN+1];
	int16_t hed[AEC_BLK_LEN+1];		

	int16_t uin_prev[AEC_BLK_LEN];
	int16_t din_prev[AEC_BLK_LEN];
	int16_t eout1_prev[AEC_BLK_LEN];
	int16_t eout2_prev[AEC_BLK_LEN];
	
	// scalars
	int32_t curr_blk_idx;
	int32_t blk_counter;
	int32_t maxw_idx;
	int32_t diverge;
	int32_t supp_state;
	int32_t cud_localmin;
	int32_t hlocalmin;
	int32_t hmin;
	int32_t hnewmin;
	int32_t hminctr;
	int32_t ovrd, ovrd_sm, ovrd_sm_cur;
	int32_t es_level;
	float max_att_b_prev;
	int32_t initial_max_suppression_count;

} BlueAECContext;


#ifdef __cplusplus
extern "C" {
#endif//__cplusplus


/**
 * @brief  Initialization for AEC
 * @param  ct Pointer to the AEC context structure. The structure's memory should be allocated by the calling process.
 * @param  es_level Echo suppression level. (0, 1~5, 11~15)
 *             0: NLP is turned off with output from adaptive filter only.
 *           1~5: Echo suppression is favored with 1 for minimum suppression and 5 for maximum suppression of echo.
 *         11~15: Duplex performance is favored with 11 for minimum suppression and 15 for maximum suppression of echo. 
 * @return error code. BLUEAEC_ERROR_OK means successful, other codes indicate error.
 */
int32_t blue_aec_init(BlueAECContext* ct, int32_t es_level);


/**
 * @brief  AEC processing routine
 * @param  ct Pointer to the AEC context structure. The structure's memory should be allocated by the calling process.
 * @param  uin far-end voice block as reference. The size of uin is equal to AEC_BLK_LEN in samples.
 * @param  din near-end (local) voice block recorded from microphone. The size of din is equal to AEC_BLK_LEN in samples.
 * @param  eout AEC processed near-end (local) voice block to be sent to the far-end. The size of eout is equal to AEC_BLK_LEN in samples.
 * @return none
 */
void blue_aec_run(BlueAECContext* ct, int16_t uin[AEC_BLK_LEN], int16_t din[AEC_BLK_LEN], int16_t eout[AEC_BLK_LEN]);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif // _BLUE_AEC_H
