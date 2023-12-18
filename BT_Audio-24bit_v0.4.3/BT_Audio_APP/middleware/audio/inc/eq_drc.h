/**
 *************************************************************************************
 * @file	eq_drc.h
 * @brief	Combo of Parametric EQ & DRC (Dynamic Range Compression).
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v1.1.0
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __EQ_DRC_H__
#define __EQ_DRC_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>
#include "eq.h"
#include "drc.h"


typedef struct _EQDRCContext
{	
	EQContext eq;
    DRCContext drc;
}EQDRCContext;


/**
 * @brief Initialize the combo of Parametric EQ & DRC module
 * @param eq_drc pointer to an EQDRCContext object
 * @param num_channels Number of channels. Only 1 or 2 is supported.
 * @param sample_rate sample rate.
 * @param filter_params EQ filter parameters.
 * @param filter_count filter count. Value range: 0 ~ MAX_EQ_FILTER_COUNT (defined in eq.h)
 * @param mode DRC mode as listed in DRC_MODE.
 * @param cf_type Crossover filter type as listed in DRC_CF_TYPE. Select DRC_CF_NONE if mode = DRC_MODE_FULLBAND, otherwise select a filter type other than DRC_CF_NONE.
 * @param q_l Q value for low-pass filter of the Q controlled crossover filters in Q6.10 format (e.g 717 stands for 0.70) if cf_type is DRC_CF_Q4. It is ignored otherwise.
 * @param q_h Q value for high-pass filter of the Q controlled crossover filters in Q6.10 format (e.g 717 stands for 0.70) if cf_type is DRC_CF_Q4. It is ignored otherwise.
 * @param fc[2] Crossover frequencies in Hz between subbands. Range: 20 ~ 20000.
 *        fc[0] Crossover frequency in Hz between band 1 & band 2. It is ignored when mode = DRC_MODE_FULLBAND.
 *        fc[1] Crossover frequency in Hz between band 2 & band 3. It is ignored when mode = DRC_MODE_FULLBAND, DRC_MODE_2BAND or DRC_MODE_2BAND_FULLBAND.
 *        Once fc[1] is in use, fc[1] should be higher than fc[0].
 * @param threshold[4] A compressor reduces the level of an audio signal if its amplitude exceeds a certain threshold. Set the threshold in step of 0.01dB. Allowable range: -9000 ~ 0 to cover -90.00dB ~ 0.00dB. For example, -2550 stands for -25.50dB threshold. 
 *        threshold[0] is for band 1, threshold[1] is for band 2, threshold[2] is for band 3, threshold[3] is for full band. 
 * @param ratio[4] The amount of gain reduction is determined by ratio: a ratio of 4:1 means that if input level is 4 dB over the threshold, the output signal level will be 1 dB over the threshold. The gain (level) has been reduced by 3 dB. 
 *        ratio[0] is for band 1, ratio[1] is for band 2, ratio[2] is for band 3, ratio[3] is for full band.
 * @param attack_tc[4] Amount of time (in millisecond) it will take for the gain to decrease to a set level. 
 *        attack_tc[0] is for band 1, attack_tc[1] is for band 2, attack_tc[2] is for band 3, attack_tc[3] is for full band.
 * @param release_tc[4] Amount of time (in millisecond) it will take for the gain to restore to 0dB.
 *        release_tc[0] is for band 1, release_tc[1] is for band 2, release_tc[2] is for band 3, release_tc[3] is for full band.
 * @return None
 * @note The following table gives the details on which values are used at each combination of DRC mode & cf_type.
 *	-----------------------------------------------------------------------------------------------------
 *	| mode                    | cf_type     | q_l | q_h | fc[0] | fc[1] | xx[0] | xx[1] | xx[2] | xx[3] |
 *	|---------------------------------------------------------------------------------------------------|
 *  | DRC_MODE_FULLBAND       | (ignored)   |          (ignored)        |       |       |       |   o   |
 *	|---------------------------------------------------------------------------------------------------|
 *	| DRC_MODE_2BAND          | DRC_CF_B1   |     |     |   o   |       |   o   |   o   |       |       |
 *	|                         | DRC_CF_LR2  |     |     |   o   |       |   o   |   o   |       |       |
 *	|                         | DRC_CF_LR4  |     |     |   o   |       |   o   |   o   |       |       |
 *	|                         | DRC_CF_Q4   |  o  |  o  |   o   |       |   o   |   o   |       |       |
 *  |---------------------------------------------------------------------------------------------------|
 *	| DRC_MODE_2BAND_FULLBAND | DRC_CF_B1   |     |     |   o   |       |   o   |   o   |       |   o   |
 *	|                         | DRC_CF_LR2  |     |     |   o   |       |   o   |   o   |       |   o   |
 *	|                         | DRC_CF_LR4  |     |     |   o   |       |   o   |   o   |       |   o   |
 *	|                         | DRC_CF_Q4   |  o  |  o  |   o   |       |   o   |   o   |       |   o   |
 *  |---------------------------------------------------------------------------------------------------|
 *	| DRC_MODE_3BAND          | DRC_CF_B1   |     |     |   o   |   o   |   o   |   o   |   o   |       |
 *	|                         | DRC_CF_LR2  |     |     |   o   |   o   |   o   |   o   |   o   |       |
 *	|                         | DRC_CF_LR4  |     |     |   o   |   o   |   o   |   o   |   o   |       |
 *	|                         | DRC_CF_Q4   |  o  |  o  |   o   |   o   |   o   |   o   |   o   |       |
 *  |---------------------------------------------------------------------------------------------------|
 *	| DRC_MODE_3BAND_FULLBAND | DRC_CF_B1   |     |     |   o   |   o   |   o   |   o   |   o   |   o   |
 *	|                         | DRC_CF_LR2  |     |     |   o   |   o   |   o   |   o   |   o   |   o   |
 *	|                         | DRC_CF_LR4  |     |     |   o   |   o   |   o   |   o   |   o   |   o   |
 *	|                         | DRC_CF_Q4   |  o  |  o  |   o   |   o   |   o   |   o   |   o   |   o   |
 *  -----------------------------------------------------------------------------------------------------
 *
 *  "xx[-]" above stands for threshold[-], ratio[-], attack_tc[-] and release_tc[-].
 */
void eq_drc_init(EQDRCContext* eq_drc, int32_t num_channels, int32_t sample_rate, 
                EQFilterParams* filter_params, uint32_t filter_count,
                DRC_MODE mode, DRC_CF_TYPE cf_type, uint16_t q_l, uint16_t q_h, 
	            int32_t fc[2], int32_t threshold[4], int32_t ratio[4], int32_t attack_tc[4], int32_t release_tc[4] );


/**
 * @brief Configure EQ filter parameters
 * @param eq_drc pointer to an EQDRCContext object
 * @param sample_rate sample rate.
 * @param filter_params EQ filter parameters
 * @param filter_count filter count. Value range: 0 ~ MAX_EQ_FILTER_COUNT (defined in eq.h)
 * @return NONE
 */
void eq_drc_configure_filters(EQDRCContext* eq_drc, uint16_t sample_rate, EQFilterParams* filter_params, uint32_t filter_count);


/**
 * @brief Apply EQ & DRC effect
 * @param eq_drc pointer to an EQDRCContext object
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n  Number of PCM samples to process.
 * @param pregain[4] Pregains applied to each subband (multi-band) and entire band (full band) before DRC processing.
 *        Q4.12 format to represent value in range [0.000244140625, 8) to cover -72dB ~ 18dB. 
 *        For example, 2052 represents x0.501 (-6dB), 2900 represents x0.708 (-3dB), 4096 represents x1.0 (0dB),  5786 represents x1.413 (+3dB), 16306 represents x3.981 (+12dB)
 *        The following table gives the details on which pregain values are used for each DRC mode.
 *			-------------------------------------------------------------------------------
 *			| mode                    | pregain[0] | pregain[1] | pregain[2] | pregain[3] |
 *			|-----------------------------------------------------------------------------|
 *			| DRC_MODE_FULLBAND       |            |            |            |     o      |
 *			| DRC_MODE_2BAND          |     o      |     o      |            |            |
 *			| DRC_MODE_2BAND_FULLBAND |     o      |     o      |            |     o      |
 *			| DRC_MODE_3BAND          |     o      |     o      |     o      |            |
 *			| DRC_MODE_3BAND_FULLBAND |     o      |     o      |     o      |     o      |
 *			-------------------------------------------------------------------------------
 * @return NONE
 */
void eq_drc_apply(EQDRCContext* eq_drc, int16_t *pcm_in, int16_t *pcm_out, int32_t n, int32_t pregain[4]);


/**
 * @brief Apply EQ & DRC effect (24-bit)
 * @param eq_drc pointer to an EQDRCContext object
 * @param pcm_in Address of the PCM input buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 * @param pcm_out Address of the PCM output buffer. The PCM layout for mono is like "M0,M1,M2,..." and for stereo "L0,R0,L1,R1,L2,R2,...".
 *        pcm_out can be the same as pcm_in. In this case, the PCM signals are changed in-place.
 * @param n  Number of PCM samples to process.
 * @param pregain[4] Pregains applied to each subband (multi-band) and entire band (full band) before DRC processing.
 *        Q4.12 format to represent value in range [0.000244140625, 8) to cover -72dB ~ 18dB.
 *        For example, 2052 represents x0.501 (-6dB), 2900 represents x0.708 (-3dB), 4096 represents x1.0 (0dB),  5786 represents x1.413 (+3dB), 16306 represents x3.981 (+12dB)
 *        The following table gives the details on which pregain values are used for each DRC mode.
 *			-------------------------------------------------------------------------------
 *			| mode                    | pregain[0] | pregain[1] | pregain[2] | pregain[3] |
 *			|-----------------------------------------------------------------------------|
 *			| DRC_MODE_FULLBAND       |            |            |            |     o      |
 *			| DRC_MODE_2BAND          |     o      |     o      |            |            |
 *			| DRC_MODE_2BAND_FULLBAND |     o      |     o      |            |     o      |
 *			| DRC_MODE_3BAND          |     o      |     o      |     o      |            |
 *			| DRC_MODE_3BAND_FULLBAND |     o      |     o      |     o      |     o      |
 *			-------------------------------------------------------------------------------
 * @return NONE
 */
void eq_drc_apply24(EQDRCContext* eq_drc, int32_t *pcm_in, int32_t *pcm_out, int32_t n, int32_t pregain[4]);


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__EQ_DRC_H__
