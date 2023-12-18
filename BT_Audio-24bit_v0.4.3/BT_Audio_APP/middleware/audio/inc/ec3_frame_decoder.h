/*************************************************************************************/
/**
 * @file	ec3_frame_decoder.h
 * @brief	EC3 Frame Decoder API.
 *
 * @author	Zhao Ying (Alfred)
 * @version	V1.0.0
 *
 * &copy; Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 */
 /*************************************************************************************/

#ifndef __EC3_FRAME_DECODER_H__
#define __EC3_FRAME_DECODER_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include <stdint.h>


/** EC3 frame decoding context */
typedef struct _EC3FrameDecoderContext {

	int32_t sample_rate;			/**< sampling rate*/
	uint32_t num_channels;			/**< no. of channels*/
	int16_t *pcm;					/**< PCM buffer*/
	int32_t pcm_length;				/**< PCM length in samples per channel in frame */
	int32_t num_blocks;				/**< no. of blocks in frame (6 for AC-3; 1,2,3,6 for EC-3)  */
	int32_t iblock;					/**< index of current block in processing  */
	int32_t bit_rate;
	int32_t downmix_mode;
	int32_t frame_size;		

	uint8_t priv_data[86576];
} EC3FrameDecoderContext;


 /**
  * @brief  Initialize the EC3 frame decoder.
  * @param[in] ct  Pointer to the EC3 frame decoder context.
  * @param[in] downmix_mode  Downmix mode. 0: downmix off, 1: downmix on
  * If downmix_mode=0, the original number of channels, e.g. 5.1 (6 channels) is available in non-interleaved format as follows,
  *   L1,L2,...,L256,R1,R2,...,R256,C1,C2,...,C256,LFE1,LFE2,...,LFE256,Lsd1,Lsd2,...,Lsd256,Rsd1,Rsd2,...,Rsd256
  *   L:Left, R:Right, C:Center, LFE:Low-Frequency Effects, Lsd:Left surround direct, Rsd:Right surround direct
  * If downmix_mode=1, the original channels are downmixed to 
  *   either stereo output in interleaved format if the original number of channels >= 2 as follows:
  *   L1,R1,L2,R2,L3,R3,...,L256,R256
  *   or mono output if the original number of channels == 1 as follows:
  *   M1,M2,M3,...,M256
  * @return Initialization result. 0 means OK, other codes indicate error.
  */
int32_t	ec3_frame_decoder_initialize(EC3FrameDecoderContext *ec3_decoder, int32_t downmix_mode);


/**
 * @brief  Decode an EC3 frame block by block.
 * @param[in] ct  Pointer to the EC3 frame decoder context.
 * @param[in] data Data buffer that contains the encoded EC3 frame.
 * @param[in] size Length of the encoded EC3 frame in bytes.
 * @return Decoding result. 0 means OK, other codes indicate error.
 * @note Once the EC3 frame is successfully decoded, the sampling rate, number of channels, PCM buffer address, PCM length are 
 *  represented by sample_rate, num_channels, pcm and pcm_length in the ec3_decoder structure respectively.
 *  The data layout in PCM buffer follows downmix_mode set in ec3_frame_decoder_initialize() as described above.
 *  Each call to this function usually gives 256 PCM samples per block as shown in pcm_length in the ec3_decoder structure. 
 *  The total number of blocks in the frame is indicated by num_blocks after the first call to decoding a new frame.
 *  Please call this API num_blocks times to obtain all the blocks of PCM samples in the frame. Range of num_blocks: 1~6.
 */
int32_t ec3_frame_decoder_decode_block(EC3FrameDecoderContext *ec3_decoder, uint8_t *data, int32_t size);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__EC3_FRAME_DECODER_H__
