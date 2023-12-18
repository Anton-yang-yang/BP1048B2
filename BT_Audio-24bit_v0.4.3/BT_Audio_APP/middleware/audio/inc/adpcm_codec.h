/**
 *************************************************************************************
 * @file	adpcm_codec.h
 * @brief	ADPCM codec for compression of PCM data
 *
 * @author	ZHAO Ying (Alfred)
 * @version	v2.0.1
 *
 * &copy; Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 *************************************************************************************
 */

#ifndef __ADPCM_CODEC_H__
#define __ADPCM_CODEC_H__

#include <stdint.h>


/**
 * @brief Encode a frame of mono PCM data into an ADPCM block. 
 * @param prevsample Pointer to the last PCM sample in the previous block.
 * @param index Pointer to the current index.
 * @param sample Address of the PCM input. The PCM layout is like "M0,M1,M2,M3,..."
 * @param enc_buf The encoded ADPCM block.
 * @param nsamples The number of PCM samples to encode. This number should be even.
 * @return none.
 * @note The length of the encoded ADPCM "enc_buf" is 3+nsamples/2 bytes = 2 bytes(prevsample) + 1 byte(index) + nsamples/2 bytes(encoded data)
 */
void adpcm_encode_block_mono(int16_t *prevsample, int8_t *index, int16_t *sample, uint8_t *enc_buf, int32_t nsamples);


/**
 * @brief Decode an ADPCM block into a frame of mono PCM data.
 * @param enc_buf The encoded ADPCM block.
 * @param sample Address of the PCM output. The PCM layout is like "M0,M1,M2,M3,..." 
 * @param nsamples The number of PCM samples to decode. This number should be even.
 * @return none.
 * @note The length of the encoded ADPCM "enc_buf" is 3+nsamples/2 bytes = 2 bytes(prevsample) + 1 byte(index) + nsamples/2 bytes(encoded data)
 */
void adpcm_decode_block_mono(uint8_t *enc_buf, int16_t *sample, int32_t nsamples);


/**
 * @brief Encode a frame of stereo PCM data into an ADPCM block.
 * @param prevsample Pointer to the last pair of PCM samples (L,R) in the previous block.
 * @param index Pointer to the current pair of indices.
 * @param sample Address of the PCM input. The PCM layout is like "L,R,L,R,..."
 * @param enc_buf The encoded ADPCM block.
 * @param nsamples The number of pairs of PCM samples to encode. This number should be even.
 * @return none.
 * @note The length of the encoded ADPCM "enc_buf" is 6+nsamples bytes = 4 bytes(prevsample) + 2 bytes(index) + nsamples bytes(encoded data)
 */
void adpcm_encode_block_stereo(int16_t *prevsample, int8_t *index, int16_t *sample, uint8_t *enc_buf, int32_t nsamples);


/**
 * @brief Decode an ADPCM block into a frame of stereo PCM data.
 * @param enc_buf The encoded ADPCM block.
 * @param sample Address of the PCM output. The PCM layout is like "L,R,L,R,..."
 * @param nsamples The number of pairs of PCM samples to decode. This number should be even.
 * @return none.
 * @note The length of the encoded ADPCM "enc_buf" is 6+nsamples bytes = 4 bytes(prevsample) + 2 bytes(index) + nsamples bytes(encoded data)
 */
void adpcm_decode_block_stereo(uint8_t *enc_buf, int16_t *sample, int32_t nsamples);


// -------------------- for internal use only --------------------
void adpcm_encode_block32_mono(int16_t *prevsample, int8_t *index, int16_t *sample, uint8_t *enc_buf);
void adpcm_decode_block32_mono(uint8_t *enc_buf, int16_t *sample);
void adpcm_encode_block32_stereo(int16_t *prevsample, int8_t *index, int16_t *sample, uint8_t *enc_buf);
void adpcm_decode_block32_stereo(uint8_t *enc_buf, int16_t *sample);


#ifdef __cplusplus
}
#endif//__cplusplus

#endif//__ADPCM_CODEC_H__
