/**
 **************************************************************************************
 * @file    remind_sound_service.c
 * @brief   
 *
 * @author  pi
 * @version V1.0.0
 *
 * $Created: 2018-2-27 13:06:47$
 *
 * @Copyright (C) 2018, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#include <string.h>
#include "app_config.h"
#include "rtos_api.h"
#include "app_message.h"
#include "type.h"
#include "spi_flash.h"
#include "debug.h"
#include "audio_utility.h"
#include "type.h"
#include "remind_sound_item.h"
#include "audio_core_api.h"
#include "audio_core_service.h"
#include "mcu_circular_buf.h"
#include "main_task.h"
#include "dac_interface.h"
#include "timeout.h"
#include "bt_manager.h"
#include "bt_hf_mode.h"
#include "tws_mode.h"
#include "bt_app_tws.h"
#include "remind_sound.h"
#include "flash_table.h"
#include "delay.h"
#ifdef CFG_FUNC_REMIND_SOUND_EN

#define CFG_DBUS_ACCESS_REMIND_SOUND_DATA  	//开启宏则使用DBUS从flash中获取提示音数据
#define CFG_PARAM_REMIND_LIST_MAX		15	//提示音阻塞播放最大个数。

#define	REMIND_DBG(format, ...)		//printf(format, ##__VA_ARGS__)

#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
#include "mp2.h"
#endif
#include "rom.h"

#pragma pack(1)
typedef struct _SongClipsHdr
{
	char sync[4];
	uint32_t crc;
	uint8_t cnt;
} SongClipsHdr;
#pragma pack()

#pragma pack(1)
typedef struct _SongClipsEntry
{
	uint8_t id[8];
	uint32_t offset;
	uint32_t size;
} SongClipsEntry;
#pragma pack()

#define		REMIND_SOUND_ID_LEN				sizeof(((SongClipsEntry *)0)->id)

#pragma pack(1)
typedef struct _REMIND_REQUEST
{
	uint16_t ItemRef;
	uint8_t Attr;
} REMIND_REQUEST;
#pragma pack()

#define		REMIND_REQUEST_LEN				sizeof(REMIND_REQUEST)
#define		REMIND_MUTE_LEN					1024

typedef struct _RemindSoundContext
{
	uint32_t 				ConstDataAddr;
	uint32_t				ConstDataSize;
	uint32_t 				ConstDataOffset;
	uint32_t 				FramSize;

	REMIND_REQUEST			Request[CFG_PARAM_REMIND_LIST_MAX];//RemindBlockBuf[REMIND_SOUND_ID_BUF_LEN];//提示音队列
	uint8_t					EmptyIndex;//非0时，播Request[0]
	REMIND_ITEM_STATE		ItemState;
	bool					RequestUpdate;
	uint32_t				Samples;
	TIMER					Timer;

	uint8_t 				player_init;
	bool					MuteAppFlag;
	bool					Disable;
	bool 					NeedUnmute;
	bool                    NeedSync;
}RemindSoundContext;


#define REMIND_FLASH_MAX_NUM		255 //flash提示音区配置决定
#define REMIND_FLASH_HDR_SIZE		0x1000 //提示音条目信息区大小
#define REMIND_FLASH_READ_TIMEOUT 	100
#define REMIND_FLASH_ADDR(n) 		(REMIND_FLASH_STORE_BASE + sizeof(SongClipsEntry) * n + sizeof(SongClipsHdr))//flash提示音区配置决定
#define REMIND_ID(Addr)				((Addr - REMIND_FLASH_STORE_BASE - sizeof(SongClipsHdr)) / (sizeof(SongClipsEntry)))
#define REMIND_FLASH_FLAG_STR		("MVUB")

static RemindSoundContext		RemindSoundCt;

osMutexId RemindMutex = NULL;

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	#define REMIND_DECODER_IN_FIFO_SIZE		2*1024

	static uint8_t read_flash_buf[512];
	static MemHandle RemindDecoderInMemHandle;	
	bool RemindMp3DataRead(void);
	extern uint16_t RemindDecoderPcmDataGet(void * pcmData,uint16_t sampleLen);// call by audio core one by one
	extern uint16_t RemindDecoderPcmDataLenGet(void);	
#else
	static	struct _Mp2DecodeContext
	{
		MPADecodeContext  	dec_cnt;
		int16_t 			dec_fifo[15*128];//3840字节
		uint32_t 			dec_last_len;
		uint8_t 			dec_buf[626];
	}Mp2Decode;

	extern bool decode_header(uint32_t header);
	extern bool MP2_decode_frame(void* PcmData, uint8_t* Mp2Data);
	extern void MP2_decode_init(void* MemAddr);
#endif

uint32_t get_remind_state(void)
{

return RemindSoundCt.player_init;

}

void RemindSoundAudioPlayEnd(void);
void Reset_Remind_Buffer(void)
{
	RemindSoundAudioPlayEnd();
	memset(&Mp2Decode,0,sizeof(struct _Mp2DecodeContext));
	//printf("ppppppppppppppppppppppppppppppppppppppppppppppppppppppppp = %d\n",sizeof(struct _Mp2DecodeContext));
}

static uint16_t RemindSountItemFind(uint8_t *RemindItem)
{
	uint16_t j;
	SongClipsEntry SongClips;

	//查找对应的ConstDataId
	for(j = 0; j < SOUND_REMIND_TOTAL; j++)
	{
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
		memcpy((uint8_t *)&SongClips, (void *)REMIND_FLASH_ADDR(j), sizeof(SongClipsEntry));
#else
		if(FLASH_NONE_ERR != SpiFlashRead(REMIND_FLASH_ADDR(j), (uint8_t *)&SongClips, sizeof(SongClipsEntry), REMIND_FLASH_READ_TIMEOUT))
		{
			return SOUND_REMIND_TOTAL;
		}
#endif
		if(memcmp(&SongClips.id,RemindItem, sizeof(SongClips.id)) == 0)//找到
		{
//			RemindSoundCt.ConstDataOffset = 0;
//			RemindSoundCt.ConstDataAddr = SongClips.offset + REMIND_FLASH_STORE_BASE; //工具制作提示音bin 使用相对地址
//			RemindSoundCt.ConstDataSize = SongClips.size;
			REMIND_DBG("Find Remind Ref:%d\n", j);
			return j;
		}
	}
	return SOUND_REMIND_TOTAL;
}

//根据flash驱动设计，最大支持255条提示音。
static bool	RemindSoundReadItemInfo(uint16_t ItemRef)
{
	uint16_t j;
	SongClipsEntry SongClips;

	if(ItemRef >= SOUND_REMIND_TOTAL)
		return FALSE;

#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
	memcpy((uint8_t *)&SongClips, (uint8_t *)(REMIND_FLASH_ADDR(ItemRef)), sizeof(SongClipsEntry));
#else
	if(FLASH_NONE_ERR != SpiFlashRead(REMIND_FLASH_ADDR(ItemRef), (uint8_t *)&SongClips, sizeof(SongClipsEntry), REMIND_FLASH_READ_TIMEOUT))
	{
		return FALSE;
	}
#endif

	RemindSoundCt.ConstDataOffset = 0;
	RemindSoundCt.ConstDataAddr = SongClips.offset + REMIND_FLASH_STORE_BASE; //工具制作提示音bin 使用相对地址
	RemindSoundCt.ConstDataSize = SongClips.size;

	APP_DBG("[REMIND]: play :   ");
	for(j=0;j<sizeof(SongClips.id);j++)
		APP_DBG("%c",SongClips.id[j]);
	APP_DBG("\n");
#ifdef BT_TWS_SUPPORT
#ifdef TWS_CODE_BACKUP
	if(memcmp(RemindItem,SOUND_REMIND_LEFTCHAN,REMIND_SOUND_ID_LEN) == 0)
	{
		SoftFlagRegister(SoftFlagTwsSlaveRemind);
		gTwsCfg.IsRemindSyncEn = 0;
	}
	else if(memcmp(RemindItem,SOUND_REMIND_RIGHTCHA,REMIND_SOUND_ID_LEN) == 0)
	{
		gTwsCfg.IsRemindSyncEn = 0;
	}
	else
	{
		gTwsCfg.IsRemindSyncEn = 1;
	}
#endif

#endif			
		return TRUE;

	return FALSE;
}

//提示音条目和数据区完整性校验，影响开机速度。
bool sound_clips_all_crc(void)
{
	SongClipsHdr *hdr;
	SongClipsEntry *ptr;
	uint16_t crc=0, i, j, CrcRead;
	uint32_t FlashAddr, all_len = 0;
	uint8_t *data_ptr = NULL;
	bool ret = TRUE;
	FlashAddr = REMIND_FLASH_STORE_BASE;
	data_ptr = (uint8_t *)osPortMalloc(REMIND_FLASH_HDR_SIZE);
	if(data_ptr == NULL)
	{
		return FALSE;
	}
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
	memcpy(data_ptr, (void *)FlashAddr, REMIND_FLASH_HDR_SIZE);
#else
	if(SpiFlashRead(FlashAddr,data_ptr,REMIND_FLASH_HDR_SIZE,REMIND_FLASH_READ_TIMEOUT) != FLASH_NONE_ERR)
	{
		REMIND_DBG("read const data error!\r\n");
		ret = FALSE;
	}
	else
#endif
	{
		ptr = (SongClipsEntry*)(data_ptr + sizeof(SongClipsHdr));
		hdr = (SongClipsHdr *)(data_ptr);
		if(strncmp(hdr->sync, REMIND_FLASH_FLAG_STR, 4) || !hdr->cnt)
		{
			REMIND_DBG("sync not found or no Item\n");
			ret = FALSE;
		}
		else
		{
			for(i = 0; i < hdr->cnt; i++)
			{
				all_len += ptr[i].size;
				for(j = 0; j < REMIND_SOUND_ID_LEN; j++)
				{
					REMIND_DBG("%c", ((uint8_t *)&ptr[i].id)[j]);
				}
				REMIND_DBG("/");
			}
			REMIND_DBG("\nALL clips size = %d\n", (int)all_len);
//			if(REMIND_FLASH_STORE_BASE + REMIND_FLASH_HDR_SIZE + all_len >= REMIND_FLASH_STORE_OVERFLOW)
//			{
//				REMIND_DBG("Remind flash const data overflow.\n");
//				ret = FALSE;
//			}
			CrcRead = hdr->crc;
			crc = ROM_CRC16((char *)data_ptr, 4, crc);
			crc = ROM_CRC16((char *)data_ptr + 8, REMIND_FLASH_HDR_SIZE - 8, crc);
			FlashAddr += REMIND_FLASH_HDR_SIZE;
			while(all_len && ret)
			{
				if(all_len > REMIND_FLASH_HDR_SIZE)
				{
					i = REMIND_FLASH_HDR_SIZE;
				}
				else
				{
					i = all_len;
				}
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
				memcpy(data_ptr, (void *)FlashAddr, i);
#else
				if(SpiFlashRead(FlashAddr,data_ptr,i,REMIND_FLASH_READ_TIMEOUT) != FLASH_NONE_ERR)
				{
					REMIND_DBG("read const data error!\r\n");
					ret = FALSE;
				}
				else
#endif
				{
					crc = ROM_CRC16((char *)data_ptr, i, crc);
					FlashAddr += i;
					all_len -= i;
				}
			}
			if(crc == CrcRead)
			{
				REMIND_DBG("Crc = %04X\n", crc);
			}
			else
			{
				REMIND_DBG("Crc error: %04X != %04X\n", crc, CrcRead);
				ret = FALSE;
			}
		}
	}
	osPortFree(data_ptr);
	return ret;
}

#ifndef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
static void mp2_play_init(void)
{
	MP2_decode_init(&Mp2Decode.dec_cnt);
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
	memcpy(Mp2Decode.dec_buf, (void *)RemindSoundCt.ConstDataAddr, 4);
#else
	if(FLASH_NONE_ERR != SpiFlashRead(RemindSoundCt.ConstDataAddr, Mp2Decode.dec_buf, 4, 1))
	{
		return;
	}
#endif
	//读取4个字节确定帧长
	if(!decode_header((Mp2Decode.dec_buf[0] << 24) | (Mp2Decode.dec_buf[1] << 16) | (Mp2Decode.dec_buf[2] << 8) | Mp2Decode.dec_buf[3]))
	{
		REMIND_DBG("decode_header error!\n");
		return;
	}
	RemindSoundCt.FramSize = Mp2Decode.dec_cnt.frame_size;
	Mp2Decode.dec_last_len = 0;
//	REMIND_DBG("sample ramte:%u\n",Mp2Decode.dec_cnt.sample_rate);
//	REMIND_DBG("fram size   :%u\n",Mp2Decode.dec_cnt.frame_size);
//	REMIND_DBG("bit rate    :%u\n",Mp2Decode.dec_cnt.bit_rate);
//	REMIND_DBG("channels    :%u\n",Mp2Decode.dec_cnt.nb_channels);
	AudioCoreSourceChange(REMIND_SOURCE_NUM, Mp2Decode.dec_cnt.nb_channels, Mp2Decode.dec_cnt.sample_rate);
}
#endif

void RemindSoundAudioPlayEnd(void)
{
//	AudioCoreSourceDisable(REMIND_SOURCE_NUM);
//	RemindSoundCt.IsBlock = FALSE;
	RemindSoundCt.player_init = MP2_DECODE_IDLE;
	//REMIND_DBG("remind audio close!\n");
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	DecoderStop(DECODER_REMIND_CHANNEL);
#endif
}

#ifdef BT_TWS_SUPPORT
static struct
{
	uint32_t gSysCurTick;
	bool	 mute_flag;
}remind_mute_timeout;
extern uint32_t gSysTick;

void master_remind_sound_mute_release(void)
{
	remind_mute_timeout.mute_flag	= FALSE;
	remind_mute_timeout.gSysCurTick = gSysTick;
}
#endif

void SendRemindSoundEndMsg(void)
{
	MessageContext		msgSend;
	msgSend.msgId = MSG_REMIND_PLAY_END;
	MessageSend(GetMainMessageHandle(), &msgSend);
}

//提示音播放结束通知其他task
void RemindSoundPlayEndNotify(void)
{
	//通话时,连上蓝牙,播放连接提示音结束后,再进入通话模式
	SoftFlagDeregister(SoftFlagWaitBtRemindEnd);
	if(SoftFlagGet(SoftFlagDelayEnterBtHf))
	{
		MessageContext		msgSend;
		SoftFlagDeregister(SoftFlagDelayEnterBtHf);

		msgSend.msgId = MSG_DEVICE_SERVICE_ENTER_BTHF_MODE;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}	
#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
	if(SoftFlagGet(SoftFlagTwsSlaveRemind))
	{
		extern void BtStackServiceMsgSend(uint16_t msgId);
		if(GetBtManager()->twsRole == BT_TWS_MASTER)
		{
			BtStackServiceMsgSend(MSG_REMIND_SOUND_PLAY_REQUEST);
			//主机发送从机链接成功提示音请求，主机mute
			if(IsAudioPlayerMute() == FALSE)
			{
				HardWareMuteOrUnMute();
			}

			remind_mute_timeout.mute_flag 	= TRUE;
			remind_mute_timeout.gSysCurTick	= gSysTick;
		}
		SoftFlagDeregister(SoftFlagTwsSlaveRemind);
	}
	//从机播放链接成功提示音完成，给主机解mute	
	if(gTwsCfg.IsRemindSyncEn == 0 && GetBtManager()->twsRole == BT_TWS_SLAVE)
	{
		extern void tws_vol_send(uint16_t Vol,bool MuteFlag);
		tws_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());	
	}
#endif	

	if(RemindSoundCt.MuteAppFlag && (!RemindSoundWaitingPlay()))
	{	
		RemindSoundCt.MuteAppFlag = FALSE;	
		//AudioCoreSourceUnmute(MIC_SOURCE_NUM,TRUE,TRUE);
		AudioCoreSourceUnmute(APP_SOURCE_NUM,TRUE,TRUE);
	}
	
	SendRemindSoundEndMsg();
}

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
void RemindSoundAudioDecoderStop(void)
{
	RemindSoundAudioPlayEnd();
	REMIND_DBG("remind play end!\n");
	RemindSoundPlayEndNotify();
}
#endif

void RemindMp2Decode(void)
{
	uint8_t *p = (uint8_t*)Mp2Decode.dec_fifo;

	if(RemindSoundCt.player_init == MP2_DECODE_HEADER)
	{
		REMIND_DBG("remind play start!\n");
		mp2_play_init();
		RemindSoundCt.player_init = MP2_DECODE_FRAME;
		RemindSoundCt.NeedUnmute = TRUE;
		RemindSoundCt.Samples = 0;
	}
	if(RemindSoundCt.player_init == MP2_DECODE_FRAME)
	{
		if(Mp2Decode.dec_last_len < CFG_PARA_SAMPLES_PER_FRAME)
		{
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
			memcpy(Mp2Decode.dec_buf,(void *)RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, 4);
#else
			SpiFlashRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, Mp2Decode.dec_buf, 4, 1);
#endif
			if(!decode_header((Mp2Decode.dec_buf[0] << 24) | (Mp2Decode.dec_buf[1] << 16) | (Mp2Decode.dec_buf[2] << 8) | Mp2Decode.dec_buf[3]))
			{
				REMIND_DBG("decode_header error!\n");
				RemindSoundAudioPlayEnd();
				RemindSoundCt.NeedUnmute = TRUE;			
				//SendRemindSoundEndMsg();
				RemindSoundPlayEndNotify();
				return;
			}
			RemindSoundCt.FramSize = Mp2Decode.dec_cnt.frame_size;
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
			memcpy(Mp2Decode.dec_buf,(void *)RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, RemindSoundCt.FramSize);
#else
			SpiFlashRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, Mp2Decode.dec_buf, RemindSoundCt.FramSize, 1);
#endif
			
			//REMIND_DBG("RemindSoundCt.FramSize:%u %08X\n",RemindSoundCt.FramSize,RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset);
			RemindSoundCt.ConstDataOffset += RemindSoundCt.FramSize;
//			DBG("Remind %d @ %d\n", RemindSoundCt.ConstDataOffset, RemindSoundCt.ConstDataSize);
			if(RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize)
			{
				REMIND_DBG("Remind end\n");
				RemindSoundCt.player_init = MP2_DECODE_END;
				//SendRemindSoundEndMsg();
				RemindSoundPlayEndNotify();
			}
//			REMIND_DBG("%02X,%02X,%02X,%02X\n",Mp2Decode.dec_buf[0],Mp2Decode.dec_buf[1],Mp2Decode.dec_buf[2],Mp2Decode.dec_buf[3]);
			if(MP2_decode_frame(p+Mp2Decode.dec_last_len*2,Mp2Decode.dec_buf) == FALSE)
			{
				REMIND_DBG("MP2_decode_frame error!\n");
				RemindSoundAudioPlayEnd();
				RemindSoundCt.NeedUnmute = TRUE;				
				//SendRemindSoundEndMsg();
				RemindSoundPlayEndNotify();
				return;
			}
			Mp2Decode.dec_last_len += 1152;
		}
	}
}

uint16_t RemindDataLenGet(void)
{
	static uint16_t delay_cnt = 0;
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	uint16_t len ;//= RemindDecoderPcmDataLenGet();

	if(RemindSoundCt.player_init == MP2_DECODE_HEADER)
	{
		delay_cnt = 0;
		if(RemindMp3DataRead())	
			RemindSoundCt.player_init = MP2_DECODE_FRAME;
	}
	if(RemindSoundCt.player_init != MP2_DECODE_IDLE)
		RemindDecodeProcess();//  decode step 4
		
	len = RemindDecoderPcmDataLenGet();
	if(len == 0 && RemindSoundCt.player_init == MP2_DECODE_FRAME && mv_msize(&RemindDecoderInMemHandle) == 0)
	{
		if(++delay_cnt > 100) //延时结束
		{
			RemindSoundAudioDecoderStop();
			delay_cnt = 0;
		}
	}
	return len;
#else	
	if(RemindSoundCt.player_init == MP2_DECODE_HEADER)
	{
		RemindSoundCt.NeedUnmute = FALSE;
		delay_cnt = 0;
	}
	RemindMp2Decode();

	if(RemindSoundCt.NeedUnmute)
	{
		RemindSoundCt.NeedUnmute = FALSE;
		if(IsAudioPlayerMute() == TRUE)
		{
			HardWareMuteOrUnMute();
		}
	}

	if(RemindSoundCt.player_init == MP2_DECODE_END && Mp2Decode.dec_last_len < CFG_PARA_SAMPLES_PER_FRAME)
	{
		++delay_cnt;
#ifdef TWS_CODE_BACKUP//BT_TWS_SUPPORT
		if((GetBtManager()->twsState == BT_TWS_STATE_CONNECTED && delay_cnt > 300)	||
			(GetBtManager()->twsState != BT_TWS_STATE_CONNECTED && delay_cnt > 100)	)
#else		
		if(delay_cnt > 100) //延时结束		
#endif		
		{
			RemindSoundAudioPlayEnd();
			REMIND_DBG("remind play end!\n");
			RemindSoundPlayEndNotify();	
			delay_cnt = 0;
		}
	}
	return Mp2Decode.dec_last_len;
#endif
}


uint16_t RemindDataGet(void* Buf, uint16_t Samples)
{

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	return RemindDecoderPcmDataGet(Buf,Samples);
#else
	uint8_t *p = (uint8_t*)Mp2Decode.dec_fifo;

	if(Mp2Decode.dec_last_len >= Samples)
	{
		memcpy(Buf,p,Samples*2);
		RemindSoundCt.Samples += Samples;
		Mp2Decode.dec_last_len = Mp2Decode.dec_last_len-Samples;
		memcpy(p,p+Samples*2,Mp2Decode.dec_last_len*2);
		return Samples;
	}
	return 0;
#endif	
}

void RemindSoundItemRequestDisable(void)
{
	RemindSoundCt.Disable = TRUE;
}

bool RemindSoundServiceItemRequest(char *SoundItem, uint32_t play_attribute)
{
	uint8_t i;
	uint16_t ItemRef = SOUND_REMIND_TOTAL;

	if(SoftFlagGet(SoftFlagNoRemind) || RemindSoundCt.Disable == TRUE)
		return FALSE;
	if(SoundItem == NULL)//strlen(SoundItem) != REMIND_SOUND_ID_LEN ||
		return FALSE;
	if(RemindSoundCt.EmptyIndex == CFG_PARAM_REMIND_LIST_MAX)
	{
		REMIND_DBG("REMIND_SOUND_ID_BUF is full!\n");
		return FALSE;
	}
	ItemRef = RemindSountItemFind((uint8_t *)SoundItem);
	if(ItemRef >= SOUND_REMIND_TOTAL)
		return FALSE;

	RemindSoundCt.RequestUpdate = TRUE;
	osMutexLock(RemindMutex);

	for(i = RemindSoundCt.EmptyIndex; i > 1; i--)//if index==0 no move
	{
		if((RemindSoundCt.Request[i - 1].Attr & REMIND_PRIO_MASK) < (play_attribute & REMIND_PRIO_MASK))
		{
			memcpy(&RemindSoundCt.Request[i], &RemindSoundCt.Request[i - 1], REMIND_REQUEST_LEN);
		}
		else
		{
			break;
		}
	}

	RemindSoundCt.Request[i].ItemRef = ItemRef;
	RemindSoundCt.Request[i].Attr = play_attribute;// & REMIND_PRIO_MASK;
	RemindSoundCt.EmptyIndex++;
	REMIND_DBG("i:%d Addr:%x Attr:%x\n", i, ItemRef, play_attribute);
	if(RemindSoundCt.EmptyIndex > 1
			&& (RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0
			&& (RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_NORMAL)
	{//当前播放 REMIND_PRIO_NORMAL时 先停播。
		//需检测同步。
		RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
		tws_local_audio_stop(RemindSoundCt.Request[0].ItemRef);
	}

	osMutexUnlock(RemindMutex);

//	if(play_attribute & REMIND_SOUND_NEED_HOLD_PLAY)
//	{
//		if(RemindSoundCt.player_init != MP2_DECODE_IDLE
//			&& RemindSoundCt.player_init != MP2_WAIT_FOR_DECODE
//			&& !RemindSoundCt.IsBlock)
//			RemindSoundCt.IsBlock = TRUE;
//	}


//	if(RemindSoundCt.player_init == MP2_DECODE_IDLE)
//		RemindSoundCt.player_init = MP2_WAIT_FOR_DECODE;
//	if(play_attribute & REMIND_SOUND_NEED_MUTE_APP_SOURCE)
//	{
//		RemindSoundCt.MuteAppFlag = TRUE;
//		//AudioCoreSourceMute(MIC_SOURCE_NUM,TRUE,TRUE);
//		AudioCoreSourceMute(APP_SOURCE_NUM,TRUE,TRUE);
//	}
	return TRUE;
}

bool RemindSoundSyncRequest(uint16_t ItemRef, TWS_AUDIO_CMD CMD)
{
	uint8_t i;
	uint8_t play_attribute = REMIND_PRIO_PARTNER;

	if(SoftFlagGet(SoftFlagNoRemind) || RemindSoundCt.Disable == TRUE || ItemRef >= SOUND_REMIND_TOTAL)
		return FALSE;

	if(RemindSoundCt.EmptyIndex == CFG_PARAM_REMIND_LIST_MAX && CMD != TWS_CMD_LOCAL_STOP)
	{
		REMIND_DBG("REMIND_SOUND_ID_BUF is full!\n");
		return FALSE;
	}

	if(CMD == TWS_CMD_LOCAL_STOP)
	{
//		for(i = 0; i < RemindSoundCt.EmptyIndex; i++)
//		{
//			if(ItemAddr == RemindSoundCt.Request[i].Remind
//					&& (RemindSoundCt.Request[i].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_PARTNER
//					&& (RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR) == 0)
//			{
				RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
//				break;
//			}
//		}
	}
	else
	{
		if((CMD == TWS_CMD_LOCAL_MASTER && tws_get_role() == BT_TWS_SLAVE)
				|| (CMD == TWS_CMD_LOCAL_SLAVE && tws_get_role() == BT_TWS_MASTER))
		{
			play_attribute |= REMIND_ATTR_MUTE;
		}
		RemindSoundCt.RequestUpdate = TRUE;
		osMutexLock(RemindMutex);
		for(i = RemindSoundCt.EmptyIndex; i > 1; i--)//if index==0 no move
		{
			if((RemindSoundCt.Request[i - 1].Attr & REMIND_PRIO_MASK) < (play_attribute & REMIND_PRIO_MASK))
			{
				memcpy(&RemindSoundCt.Request[i], &RemindSoundCt.Request[i - 1], REMIND_REQUEST_LEN);
			}
			else
			{
				break;
			}
		}
		RemindSoundCt.Request[i].ItemRef = ItemRef;
		RemindSoundCt.Request[i].Attr = play_attribute;// & REMIND_PRIO_MASK;
		RemindSoundCt.EmptyIndex++;
		if(RemindSoundCt.EmptyIndex > 1 && ((RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_NORMAL))
		{//当前播放 REMIND_PRIO_NORMAL时 先停播。
			//需检测同步。
			RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
		}
		osMutexUnlock(RemindMutex);
	}


	return TRUE;
}

void RemindSoundSyncEnable(void)
{
	AudioCoreSourceEnable(REMIND_SOURCE_NUM);
}

//bool RemindSoundItemClearWaitting(uint8_t Prio)
//{
//	if(DisablePrio < Attr)
//}


//有提示音在队列或者buf中等待播放
bool RemindSoundWaitingPlay(void)
{
//	if(RemindSoundCt.RequestRemind[0] || MCUCircular_GetDataLen(&RemindSoundCt.RemindBlockCircular))
//		return 1;
	return 0;
}

void RemindRequestsRecast(void)
{
	uint8_t i, Index = 0;

	if(RemindSoundCt.ItemState != REMIND_ITEM_IDLE || RemindSoundCt.EmptyIndex == 0)
	{
		return ;
	}

	osMutexLock(RemindMutex);

	for(i = 0; i < RemindSoundCt.EmptyIndex; i++)
	{
		REMIND_DBG("Remind Item:%d %d", i, RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR);

		if((RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR) == 0)
		{
			if(Index == 0)
			{
				memcpy(&RemindSoundCt.Request[Index++], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);

			}
			else if((RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_NORMAL)
			{//play last REMIND_PRIO_NORMAL
				memcpy(&RemindSoundCt.Request[0], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);
			}
			else if((RemindSoundCt.Request[i].Attr & REMIND_PRIO_MASK) > (RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK))
			{//冗余检测,先由RemindSoundServiceItemRequest排序
				memcpy(&RemindSoundCt.Request[0], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);
			}
			else
			{
				memcpy(&RemindSoundCt.Request[Index++], &RemindSoundCt.Request[i], REMIND_REQUEST_LEN);
			}
		}
	}
	RemindSoundCt.EmptyIndex = Index;
	RemindSoundCt.RequestUpdate = FALSE;
	osMutexUnlock(RemindMutex);

}

bool RemindSoundRun(SysModeState ModeState)
{
	int i;
	extern void tws_local_audio_stopped(uint16_t ItemRef);

	if(RemindSoundCt.EmptyIndex == 0 && RemindSoundCt.ItemState == REMIND_ITEM_IDLE)
	{
		RemindSoundCt.NeedSync = FALSE;
		return FALSE;
	}
	else if(ModeState == ModeStateDeinit
			&& RemindSoundCt.ItemState == REMIND_ITEM_IDLE)
	{//切模式期间提示音 sys播放或其他打断处理完毕
		return FALSE;
	}

	if(ModeState == ModeStateDeinit)
	{
		if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0)
		{
			tws_local_audio_stop(RemindSoundCt.Request[0].ItemRef);
			RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
		}
		osMutexLock(RemindMutex);
		for(i = 1; i < RemindSoundCt.EmptyIndex; i++)
		{
			if((RemindSoundCt.Request[i].Attr & REMIND_ATTR_CLEAR) == 0
				&& ((RemindSoundCt.Request[i].Attr & REMIND_PRIO_MASK) != REMIND_PRIO_SYS))
			{
				RemindSoundCt.Request[i].Attr |= REMIND_ATTR_CLEAR;
			}
		}
#ifdef BT_TWS_SUPPORT
		if(!RemindSoundCt.NeedSync)
		{
			RemindSoundCt.NeedSync = TRUE;
			tws_master_remindclear_send(TRUE);
		}
#endif
		osMutexUnlock(RemindMutex);
	}
	else if(ModeState == ModeStateInit)
	{

		if(RemindSoundCt.ItemState != REMIND_ITEM_IDLE)
		{
			RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
			RemindSoundCt.RequestUpdate = TRUE;
			RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
			tws_local_audio_stopped(RemindSoundCt.Request[0].ItemRef);
		}
		RemindRequestsRecast();
		return TRUE;
	}

	switch(RemindSoundCt.ItemState)
	{
		case REMIND_ITEM_IDLE:
			if(RemindSoundCt.EmptyIndex && ModeState == ModeStateRunning)
			{
				if(!RemindSoundCt.RequestUpdate && (RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) == 0)
				{
					if(RemindSoundReadItemInfo(RemindSoundCt.Request[0].ItemRef))
					{
						RemindSoundCt.ItemState = REMIND_ITEM_PREPARE;
						RemindSoundCt.player_init = MP2_DECODE_HEADER;

						if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_MUTE) == 0 && ((RemindSoundCt.Request[0].Attr & REMIND_PRIO_MASK) == REMIND_PRIO_PARTNER))
						{
							tws_local_audio_set(RemindSoundCt.Request[0].ItemRef, TWS_CMD_LOCAL_MASTER);
						}
						else
						{
							tws_local_audio_set(RemindSoundCt.Request[0].ItemRef, TWS_CMD_LOCAL_SYNC);
						}
						break;
					}
					else
					{
						RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
						RemindSoundCt.RequestUpdate = TRUE;
					}
				}
				RemindRequestsRecast();
			}
			break;

		case REMIND_ITEM_PREPARE:
			if(!tws_local_audio_wait())
			{
				AudioCoreSourceEnable(REMIND_SOURCE_NUM);
				RemindSoundCt.ItemState = REMIND_ITEM_PLAY;
			}
			else if(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR
					&&	!AudioCoreSourceIsEnable(REMIND_SOURCE_NUM))
			{//等待开播时直接结束
					RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
					RemindSoundCt.RequestUpdate = TRUE;
					AudioCoreSourceDisable(REMIND_SOURCE_NUM);
					tws_local_audio_stopped(RemindSoundCt.Request[0].ItemRef);
			}
		case REMIND_ITEM_PLAY:
			if(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR
					&& RemindSoundCt.FramSize > 0
					&& RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize * 2 < RemindSoundCt.ConstDataSize)
			{
//				RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
//				AudioCoreSourceDisable(REMIND_SOURCE_NUM);
//				RemindSoundCt.RequestUpdate = TRUE;
				RemindSoundCt.ConstDataSize = RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize * 2;
			}
			else if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_MUTE) == 0
					&& RemindSoundCt.FramSize > 0
					&& RemindSoundCt.ConstDataOffset >= RemindSoundCt.FramSize * 2)
			{
				AudioCoreSourceUnmute(REMIND_SOURCE_NUM, TRUE, TRUE);
				DBG("Unmute Remind\n");
				RemindSoundCt.ItemState = REMIND_ITEM_UNMUTE;
			}
			else if(RemindSoundCt.ConstDataOffset + RemindSoundCt.FramSize - 1 >= RemindSoundCt.ConstDataSize
					&& (RemindSoundCt.Request[0].Attr & REMIND_ATTR_MUTE))
//					&& Mp2Decode.dec_last_len < CFG_PARA_SAMPLES_PER_FRAME))//(RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) ||
			{
				RemindSoundCt.ItemState = REMIND_ITEM_MUTE;
			}
			break;

		case REMIND_ITEM_UNMUTE:
			if((RemindSoundCt.Request[0].Attr & REMIND_ATTR_CLEAR) || (RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize))
			{
				AudioCoreSourceMute(REMIND_SOURCE_NUM, TRUE, TRUE);
				DBG("Mute Remind\n");
				RemindSoundCt.ItemState = REMIND_ITEM_MUTE;

			}
			break;

		case REMIND_ITEM_MUTE:
			if(Mp2Decode.dec_last_len < CFG_PARA_SAMPLES_PER_FRAME)
			{
				RemindSoundCt.Request[0].Attr |= REMIND_ATTR_CLEAR;
				RemindSoundCt.ItemState = REMIND_ITEM_IDLE;
				RemindSoundCt.RequestUpdate = TRUE;
				AudioCoreSourceDisable(REMIND_SOURCE_NUM);

				tws_local_audio_stopped(RemindSoundCt.Request[0].ItemRef);
			}
			break;
	}

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY	
	if(GetDecoderState(DECODER_REMIND_CHANNEL) != DecoderStateNone)
		DecoderStop(DECODER_REMIND_CHANNEL);
	if(RemindMp3DataRead()) 
		RemindSoundCt.player_init = MP2_DECODE_FRAME;
	DecoderInit(&RemindDecoderInMemHandle,DECODER_REMIND_CHANNEL, (int32_t)IO_TYPE_MEMORY, MP3_DECODER);
	DecoderPlay(DECODER_REMIND_CHANNEL);
	REMIND_DBG("remind play start!\n");
#endif	

	return TRUE;
}

void RemindSoundInit(void)
{
#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
	memset(&RemindDecoderInMemHandle, 0, sizeof(RemindDecoderInMemHandle));
#else
	memset(&Mp2Decode,0,sizeof(Mp2Decode));
#endif
	memset(&RemindSoundCt,0,sizeof(RemindSoundCt));

	if(!sound_clips_all_crc())
	{
		SoftFlagRegister(SoftFlagNoRemind);
	}
//#ifdef BT_TWS_SUPPORT
//	master_remind_sound_mute_release();
//#endif
}

uint32_t SoundRemindItemGet(void)
{
	if(RemindSoundCt.EmptyIndex)
	{
		return RemindSoundCt.Request[0].ItemRef;
	}
	return 0;
}


void RemindSoundClearPlay(void)
{
	RemindSoundAudioPlayEnd();
	memset(&RemindSoundCt,0,sizeof(RemindSoundCt));

}

void RemindSoundClearSlavePlay(void)
{
	memset(&RemindSoundCt,0,sizeof(RemindSoundCt));
	memset(&Mp2Decode,0,sizeof(struct _Mp2DecodeContext));
}

uint8_t RemindSoundIsPlay(void)
{
//#ifdef BT_TWS_SUPPORT
//	//tws从机正在播放提示音 主机需要等待从机提示音播放完成以后才能同步
//	if(remind_mute_timeout.mute_flag && GetBtManager()->twsState == BT_TWS_STATE_CONNECTED)
//		return 3;
//#endif
//	if(RemindSoundCt.player_init == MP2_DECODE_IDLE)
//		return 0;
//	else if(RemindSoundCt.player_init == MP2_WAIT_FOR_DECODE)
//		return 1;
//	else
//		return 2;
	return RemindSoundCt.EmptyIndex;
}

uint8_t RemindSoundIsBlock(void)
{
//	if(MCUCircular_GetDataLen(&RemindSoundCt.RemindBlockCircular) >= REMIND_SOUND_ID_MIX_FLAG_LEN)
//		return 1;
	return 0;
}

bool RemindSoundIsMix(void)
{
	return RemindSoundCt.EmptyIndex && (RemindSoundCt.Request[0].Attr & REMIND_ATTR_NEED_MIX);
}

#ifdef CFG_REMIND_SOUND_DECODING_USE_LIBRARY
int32_t RemindMp3DecoderInit(void)
{
	RemindDecoderInMemHandle.addr = osPortMalloc(REMIND_DECODER_IN_FIFO_SIZE);
	if(RemindDecoderInMemHandle.addr == NULL)
		return -1;
	
	memset(RemindDecoderInMemHandle.addr, 0, REMIND_DECODER_IN_FIFO_SIZE);
	RemindDecoderInMemHandle.mem_capacity = REMIND_DECODER_IN_FIFO_SIZE;
	RemindDecoderInMemHandle.mem_len = 0;
	RemindDecoderInMemHandle.p = 0;

	return DecoderServiceInit(GetSysModeMsgHandle(),DECODER_REMIND_CHANNEL, DECODER_BUF_SIZE_MP3, DECODER_FIFO_SIZE_FOR_MP3);// decode step1
}

int32_t RemindMp3DecoderDeinit(void)
{
	DecoderServiceDeinit(DECODER_REMIND_CHANNEL);

	if(RemindDecoderInMemHandle.addr)
	{
		osPortFree(RemindDecoderInMemHandle.addr);
	}

	RemindDecoderInMemHandle.addr = NULL;
	RemindDecoderInMemHandle.mem_capacity = 0;
	RemindDecoderInMemHandle.mem_len = 0;
	RemindDecoderInMemHandle.p = 0;
	
	return 0;
}

bool RemindMp3DataRead(void)
{
	if(mv_mremain(&RemindDecoderInMemHandle) > sizeof(read_flash_buf))
	{
		uint32_t len = RemindSoundCt.ConstDataSize - RemindSoundCt.ConstDataOffset;
		
		if(len > sizeof(read_flash_buf))
			len = sizeof(read_flash_buf);
#ifdef CFG_DBUS_ACCESS_REMIND_SOUND_DATA
		memcpy(read_flash_buf,(void *)RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, len);
#else
		SpiFlashRead(RemindSoundCt.ConstDataAddr+RemindSoundCt.ConstDataOffset, read_flash_buf, len, 1);
#endif		
		RemindSoundCt.ConstDataOffset += len;
		mv_mwrite(read_flash_buf, len, 1, &RemindDecoderInMemHandle);
		if(RemindSoundCt.ConstDataOffset >= RemindSoundCt.ConstDataSize)
		{
			return 1;
		}
	}
	return 0;
}
#endif

#else
uint32_t SoundRemindItemGet(void)
{
	return 0;
}

bool RemindSoundSyncRequest(uint16_t ItemRef, TWS_AUDIO_CMD CMD)
{
	return FALSE;
}

void RemindSoundSyncEnable(void)
{

}
#endif


