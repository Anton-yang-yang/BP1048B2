/**
 **************************************************************************************
 * @file    bluetooth_audio.c
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
 
#include "sbcenc_api.h"
#include "sbc_frame_decoder.h"

typedef struct {
	uint8_t nrof_blocks;
	uint8_t nrof_subbands;
	uint8_t mode;
	uint8_t bitpool;
}SBC_FRAME_INFO;

static const uint16_t freq_values[] =    { 16000, 32000, 44100, 48000 };
static const uint8_t block_values[] =    { 4, 8, 12, 16 };
static const uint8_t band_values[] =     { 4, 8 };

/*****************************************************************************************
* 计算帧长
****************************************************************************************/
uint16_t CalculateFramelen(SBC_FRAME_INFO *frame)
{
	uint16_t nbits = frame->nrof_blocks * frame->bitpool;
	uint16_t nrof_subbands = frame->nrof_subbands;
	uint16_t result = nbits;

    if (frame->mode == 3) {//SBC_JOINT_STEREO
        result += nrof_subbands + (8 * nrof_subbands);
    } else {
        if (frame->mode == 1) { result += nbits; }//SBC_DUAL_CHANNEL
        if (frame->mode == 0) { result += 4*nrof_subbands; } else { result += 8*nrof_subbands; }// SBC_MONO
    }
    return 4 + (result + 7) / 8;
}

/*****************************************************************************************
* 获取帧长的相关信息
****************************************************************************************/
int sbc_get_fram_infor(uint8_t*data,uint32_t *fram_size,uint32_t *frequency,uint32_t *data_out_fram_size)
{
	uint8_t d1;
	SBC_FRAME_INFO frame;
	memset(&frame,0,sizeof(SBC_FRAME_INFO));
	if(data[0] != 0x9C)
	{
		return 1;
	}
	d1 = data[1];
    frame.bitpool = data[2];///////////////////////////////
    *frequency = freq_values[((d1 & (0x80 | 0x40)) >> 6)];
	frame.nrof_blocks = block_values[((d1 & (0x20 | 0x10)) >> 4)];////////
	frame.mode = (d1 & (0x08 | 0x04)) >> 2;///////////////
	frame.nrof_subbands = band_values[(d1 & 0x01)];/////////////
	*data_out_fram_size = (uint32_t)frame.nrof_blocks*frame.nrof_subbands;
	
	//printf("data_out_fram_size = %d \n",frame.nrof_blocks*frame.nrof_subbands);
	
	*fram_size = CalculateFramelen(&frame);
	return 0;
}


/*****************************************************************************************
* sbc解码器初始化函数
****************************************************************************************/
int32_t sbc_decoder_init(SBCFrameDecoderContext *ct)
{
	return sbc_frame_decoder_initialize(ct);
}

/*****************************************************************************************
* sbc解码函数
****************************************************************************************/
int32_t sbc_decoder_apply(SBCFrameDecoderContext *ct,uint8_t *sbc_buf,uint8_t sbc_size,int16_t *pcm_buf)
{
	int32_t ret = sbc_frame_decoder_decode(ct, sbc_buf, sbc_size);
	if(ret == 0)
	{
		if(ct->num_channels == 1)
		{
			int i;
			int16_t *in,*out;
			in = ct->pcm;
			out = pcm_buf;
			for(i=0;i<ct->pcm_length;i++)
			{
				out[2*i+0] = in[i];
				out[2*i+1] = in[i];
			}
		}
		else
		{
			memcpy(pcm_buf,ct->pcm,ct->num_channels*ct->pcm_length*2);
		}
	}
	//else
	//{
		
	//}
	return ret;
}

/*****************************************************************************************
* sbc解码函数
****************************************************************************************/
int32_t sbc_decoder_apply_phone(SBCFrameDecoderContext *ct,uint8_t *sbc_buf,uint8_t sbc_size,int16_t *pcm_buf)
{
	int32_t ret = sbc_frame_decoder_decode(ct, sbc_buf, sbc_size);
	if(ret == 0)
	{
		//memcpy(pcm_buf,ct->pcm,ct->num_channels*256);
		if(ct->num_channels == 1)
		{
			int i;
			int16_t *in,*out;
			in = ct->pcm;
			out = pcm_buf;
			for(i=0;i<ct->pcm_length;i++)
			{
				out[2*i+0] = in[i];
				out[2*i+1] = in[i];
			}
		}
		else
		{
			memcpy(pcm_buf,ct->pcm,ct->num_channels*ct->pcm_length*2);
		}
	}
	//else
	//{
		
	//}
	return ret;
}

/*****************************************************************************************
* sbc编码器初始化函数
****************************************************************************************/
int32_t sbc_encoder_init(SBCEncoderContext *ct,int32_t num_channels,SBC_ENC_QUALITY quality)
{
	int32_t samples_per_frame;
	return sbc_encoder_initialize(ct,num_channels,44100,16,quality,&samples_per_frame);
}

/*****************************************************************************************
* sbc编码函数
****************************************************************************************/
int32_t sbc_encoder_aplly(SBCEncoderContext *ct,int16_t *in_pcm,uint8_t *out_sbc,uint32_t *length)
{
	return sbc_encoder_encode(ct,in_pcm,out_sbc,length);
}






