//bt_play_api.h
#include "type.h"
#include "mcu_circular_buf.h"
#include "resampler.h"
#include "resampler_polyphase.h"
#include "sbc_frame_decoder.h"

#ifndef __BT_PLAY_API_H__
#define __BT_PLAY_API_H__

#define SBC_FIFO_SIZE	8*1024
#define SBC_FIFO_LEVEL_HIGH 	(SBC_FIFO_SIZE / 10 * 7) //70% 开播

#define BT_AAC_LEVEL_HIGH					(12)//frame 计数
#define BT_AAC_LEVEL_LOW					(5)
#define BT_AAC_START_FRAME					((BT_AAC_LEVEL_LOW + BT_AAC_LEVEL_HIGH) / 2 + 1)

typedef struct _BT_A2DP_PLAYER
{
#ifdef BT_AUDIO_AAC_ENABLE
	uint8_t sbc_fifo[SBC_FIFO_SIZE];//缓存
	MemHandle MemHandle;
#else
	SBCFrameDecoderContext sbc_dec_handle;//sbc解码器
	MCU_CIRCULAR_CONTEXT sbc_fifo_cnt;
	uint8_t sbc_fifo[SBC_FIFO_SIZE];//sbc缓存
	uint8_t sbc_buffer[150/*119*4*/];//sbc解码使用，每次最大读取4个fram     ***逻辑保障 读sbc时 last_pcm_buf已空。可复用
	uint32_t sbc_bytes;
	uint32_t sample_rate;
	uint32_t decoder_out_sample;

	uint16_t last_pcm_len;
	int16_t last_pcm_buf[284];// 支持sbc和44.1K转48, 即(129/44.1)*48
	int16_t dec_out_pcm[480];
	
	int16_t dec_out_pcm_offset;
#endif
	//sbc相关参数初始化标志,未初始化前,不能收发数据
	uint32_t sbc_init_flag;
}BT_A2DP_PLAYER;


//extern BT_A2DP_PLAYER *a2dp_player;



/***********************************************************************************
 * 获取 SBC DATA 有效长度
 **********************************************************************************/
uint32_t GetValidSbcDataSize(void);
uint32_t GetValidFrameDataSize(void);

/***********************************************************************************
 * AVRCP连接成功后自动发送播放命令，播放音乐
 **********************************************************************************/
void BtAutoPlayMusic(void);

/***********************************************************************************
 * a2dp sbc decoder 初始化函数
 **********************************************************************************/
void a2dp_sbc_decoer_init(void);
/***********************************************************************************
 * a2dp sbc 接收数据保存处理函数
 **********************************************************************************/
void a2dp_sbc_save(uint8_t *p,uint32_t len);


#endif

