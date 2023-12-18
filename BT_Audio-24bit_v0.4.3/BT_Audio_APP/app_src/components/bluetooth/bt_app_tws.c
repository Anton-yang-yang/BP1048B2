/**
 **************************************************************************************
 * @file    bluetooth_tws.c
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



#include "type.h"
#include "debug.h"
#include "clk.h"
#include "stdlib.h"
#include "bt_manager.h"
#include "bt_interface.h"
#include "bt_tws_api.h"
#include "mcu_circular_buf.h"
#include "app_config.h"
#include "bt_play_mode.h"
#include "main_task.h"
#include "audio_core_api.h"
#include "ctrlvars.h"
#include "bt_avrcp_api.h"
#include "dac_interface.h"
#include "adc_interface.h"
#include "sbcenc_api.h"
#include "uarts.h"
#include "dma.h"
#include "i2s.h"
#include "audio_adc.h"
#include "i2s_interface.h"
#include "irqn.h"
#include "audio_vol.h"
#include "bt_app_ddb_info.h"
#include "remind_sound.h"
#include "bt_app_connect.h"
#include "bt_app_tws_connect.h"
#include "bt_app_sniff.h"
#include "bt_app_tws.h"
#include "tws_mode.h"
#include "main_task.h"
#include "rtos_api.h"
#include "media_play_mode.h"
#include "bt_stack_service.h"
#include "remind_sound.h"
#include "audio_core_service.h"
#include "bt_app_common.h"

#ifdef BT_TWS_SUPPORT
//tws cmd
typedef enum
{
	CMD_TWS_RESET = 0,
	CMD_TWS_SNIFF,
	CMD_TWS_REPAIR,
	CMD_TWS_VOL,
	CMD_TWS_MUTE,
	CMD_TWS_HFP_STATE,
	CMD_TWS_A2DP_STATE,
	CMD_TWS_EQ,
	CMD_TWS_MUSIC_TREB_BASS,
	CMD_TWS_REMIND,
	CMD_TWS_KEY_MSG,
	CMD_TWS_REMINDCLEAR_MSG,
	CMD_TWS_MAX,
}TWS_CMD_TYPE;

const char * const TWS_CommonCmds[CMD_TWS_MAX] =
{
	"CMD:TWS_RESET",
	"CMD:TWS_SNIFF",
	"CMD:TWS_REPAIR",
	"CMD:TWS_VOL",
	"CMD:MUTE",
	"CMD:HFP_STATE",
	"CMD:A2DP_STATE",
	"CMD:TWS_EQ",
	"CMD:TWS_MUSIC_TREB_BASS",
	"CMD:REMIND",
	"CMD:TWS_KEY_MSG",
	"CMD:TWS_REMINDCLEAR_MSG",
};

typedef struct
{
	TWS_CMD_TYPE type;
	uint8_t		 para_len;
	uint8_t	*	 para_buf;
}TwsCmdPara;

#include "dac.h"
BT_TWS_PLAY tws_play = BT_TWS_PLAY_IDLE;
uint32_t tws_delay = BT_TWS_DELAY_48;//96;	//Tony20221109 for delay

static uint32_t error_count = 0;
#define TWS_CONNECT_MUTE		30
#define TWS_DISCONNECT_MUTE		50
TIMER TwsMuteTimer;
#ifdef BT_TWS_SUPPORT
TIMER TwsDacOnTimer;
#endif
extern void AudioMusicVol(uint8_t musicVol);
extern void TwsSlaveFifoMuteTimeSet(void);
extern void TwsSlaveFifoUnmuteSet(void);
extern void mv_lmp_send(uint32_t cmd1,uint32_t cmd2,uint32_t cmd3);
extern void tws_device_close(void);

TWS_CMD_TYPE GetTwsRecCmd(uint8_t * cmd_string ,uint8_t *para_offset)
{
	uint8_t i;
	uint8_t len;
	for(i = 0;i < CMD_TWS_MAX; i++)
	{
		len = strlen(TWS_CommonCmds[i]);
		if(memcmp(cmd_string, TWS_CommonCmds[i],len) == 0)
		{
			*para_offset = len + 1;
			return i;
		}
	}
	return CMD_TWS_MAX;
}

void TwsSendCmd(TwsCmdPara *para)
{
	bool    ret;
	uint8_t *p;
	uint8_t len;
	uint8_t i;
	static uint8_t temp[30]; //常驻，TWS发送会用到这个buf

	if(para == NULL ||
	   para->type >= CMD_TWS_MAX ||
	   btManager.twsState < BT_TWS_STATE_CONNECTED )
		return;

	ret = GIE_STATE_GET();
	p  	= (uint8_t *)TWS_CommonCmds[para->type];
	len = strlen(TWS_CommonCmds[para->type]) + 1; //拷贝字符串结束符 '\0'

	GIE_DISABLE();

	memcpy(temp,p,len);
	for(i=0; i < para->para_len; i++)
	{
		temp[len++] = para->para_buf[i];
	}

	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_master_send(temp, len);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		tws_slave_send(temp, len);
	}

	if(ret)
	{
		GIE_ENABLE();
	}
}

/**************************************************************************
 *  tws cmd send
 *************************************************************************/
void tws_vol_send(uint16_t Vol,bool MuteFlag)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(!sys_parameter.TwsVolSyncEnable)
		return;

	buf[0] 			= Vol;
	buf[1] 			= MuteFlag;

	para.type 		= CMD_TWS_VOL;
	para.para_len   = 2;
	para.para_buf   = buf;
	TwsSendCmd(&para);
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_vol_mute:%u %u\n",mainAppCt.MusicVolume,MuteFlag);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_vol_mute:%u %u\n",mainAppCt.MusicVolume,MuteFlag);
	}
}

void tws_send_key_msg(uint16_t key_data)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	buf[0] = (uint8_t)(key_data>>8);
	buf[1] = (uint8_t)(key_data&0xff);
	
	para.type 		= CMD_TWS_KEY_MSG;
	para.para_len   = 2;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("send key msg: 0x%x\n", key_data);
}

void tws_slave_tws_reset_send(void)
{
	TwsCmdPara para;
	if(btManager.twsRole != BT_TWS_SLAVE)
		return;

	para.type 		= CMD_TWS_RESET;
	para.para_len   = 0;
	para.para_buf   = NULL;
	TwsSendCmd(&para);

	APP_DBG("slave send cmd_reset!!!\n");
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_vol_send(uint16_t Vol,bool MuteFlag)
{
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_vol_send(Vol,MuteFlag);
	}
}

/**************************************************************************
 *  
 *************************************************************************/
#if (BT_HFP_SUPPORT == ENABLE)
void tws_master_hfp_send(void)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;
	
	buf[0] = (uint8_t)GetHfpState(BtCurIndex_Get());

	para.type 		= CMD_TWS_HFP_STATE;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("master send cmd_hfp_state:%d\n", buf[0]);
}
#endif

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_a2dp_send(void)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;

	buf[0] = (uint8_t)GetBtPlayState();

	para.type 		= CMD_TWS_A2DP_STATE;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("master send cmd_a2dp_state:%d\n", buf[0]);
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_mute_send(bool MuteFlag)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;

	buf[0] = MuteFlag;

	para.type 		= CMD_TWS_MUTE;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);
//	vTaskDelay(CFG_CMD_DELAY);
	APP_DBG("master send cmd_dac_mute: %u\n",MuteFlag);

}


void tws_master_remindclear_send(bool isclear)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	if(btManager.twsRole == BT_TWS_SLAVE)
		return;

	buf[0] = isclear;

	para.type 		= CMD_TWS_REMINDCLEAR_MSG;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);
//	vTaskDelay(CFG_CMD_DELAY);
	APP_DBG("master send tts send: %d\n",isclear);

}

/**************************************************************************
 *  
 *************************************************************************/
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN	 
void tws_eq_mode_send(uint16_t eq_mode)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	buf[0] = eq_mode;

	para.type 		= CMD_TWS_EQ;
	para.para_len   = 1;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	if(btManager.twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_eq_mode:%u\n",eq_mode);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_eq_mode:%u\n",eq_mode);
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_eq_mode_send(uint16_t eq_mode)
{
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_eq_mode_send(eq_mode);
	}
}
#endif

/**************************************************************************
 *  
 *************************************************************************/
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
void tws_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol)
{
	TwsCmdPara para;
	uint8_t	   buf[2];

	buf[0] = bass_vol;
	buf[1] = treb_vol;

	para.type 		= CMD_TWS_MUSIC_TREB_BASS;
	para.para_len   = 2;
	para.para_buf   = buf;
	TwsSendCmd(&para);
	
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		APP_DBG("master send cmd_music_bass_treb:%u %u\n",bass_vol,treb_vol);
	}
	else if(btManager.twsRole == BT_TWS_SLAVE)
	{
		APP_DBG("slave send cmd_music_bass_treb:%u %u\n",bass_vol,treb_vol);
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void tws_master_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol)
{
	if(btManager.twsRole == BT_TWS_MASTER)
	{
		tws_music_bass_treb_send(bass_vol,treb_vol);
	}
}
#endif

#ifdef CFG_FUNC_REMIND_SOUND_EN
void tws_master_remind_sound_send(char * remind,uint32_t IsBlock)
{
	TwsCmdPara para;
	uint8_t	   buf[10];

	if(btManager.twsRole != BT_TWS_MASTER)
		return;

	buf[0] = IsBlock;
	memcpy(&buf[1],remind,8);

	para.type 		= CMD_TWS_REMIND;
	para.para_len   = 9;
	para.para_buf   = buf;
	TwsSendCmd(&para);

	APP_DBG("tws_master_remind_sound_send: %s\n",remind);
}
#endif
/**************************************************************************
 *  
 *************************************************************************/
void tws_peer_repair(void)
{
	TwsCmdPara para;

	para.type 		= CMD_TWS_REPAIR;
	para.para_len   = 0;
	para.para_buf   = NULL;
	TwsSendCmd(&para);

	APP_DBG("send cmd_repair\n");
}

/**************************************************************************
 * tws master callback
 *************************************************************************/
void BtTws_Master_Connected(BT_TWS_CALLBACK_PARAMS * param)
{
	APP_DBG("TWS_MASTER_CONNECTED:\n");
	{
		MessageContext		msgSend;
		msgSend.msgId		= MSG_BT_TWS_MASTER_CONNECTED;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	memcpy(btManager.btTwsDeviceAddr, param->params.bd_addr, param->paramsLen);
	APP_DBG("master connect: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
		btManager.btTwsDeviceAddr[0],
		btManager.btTwsDeviceAddr[1],
		btManager.btTwsDeviceAddr[2],
		btManager.btTwsDeviceAddr[3],
		btManager.btTwsDeviceAddr[4],
		btManager.btTwsDeviceAddr[5]
		);

	//改善延时的代码
	{
		extern void set_slaver_cmd(uint8_t* addr);
		set_slaver_cmd(param->params.bd_addr);
	}

	btManager.twsState = BT_TWS_STATE_CONNECTED;
	btManager.twsRole = BT_TWS_MASTER;
	BtDdb_UpgradeTwsInfor(btManager.btTwsDeviceAddr);
	btManager.twsFlag = 1;
	btManager.btReconnectedFlag = 1;

	if(btManager.btConStateProtectCnt)
		btManager.btConStateProtectCnt=0;

	BtReconnectTwsStop();

//蓝牙可见性状态更新
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsExitPairingMode();

	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);
	
#elif ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
	BtTwsExitSimplePairingMode();
#else //CFG_TWS_PEER_SLAVE/CFG_TWS_PEER_MASTER
	BtTwsExitPeerPairingMode();

	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if(Bt_sniff_sniff_start_state_get())
	{
		//再次使从进入sniff
		//printf("send sniff cmd to slave again\n");
		BTSetRemDevIntoSniffMode(btManager.btTwsDeviceAddr);
		return;
	}
	else
	{
		tws_master_active_cmd();
	}
#endif
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	DisableAdvertising();
#endif

	tws_master_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());

#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	tws_master_eq_mode_send(mainAppCt.EqMode);
#endif

#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	tws_master_music_bass_treb_send(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTws_Master_Disconnected(BT_TWS_CALLBACK_PARAMS * param)
{
	btManager.twsMode=0;
	APP_DBG("TWS_MASTER_DISCONNECT:\n");
	btManager.twsState = BT_TWS_STATE_NONE;

//蓝牙可见性配置
	BtStackServiceMsgSend(MSG_BTSTACK_ACCESS_MODE_SET);

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER)
	if(Bt_sniff_sniff_start_state_get() == 0)
		ble_advertisement_data_update();
	else
	{
		gBtTwsSniffLinkLoss = 1;
	}
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT);
#endif

	BtReconnectTwsStop();

	//TWS组网,发现对方LINKKEY不符合预期断开;则清除自身的TWS信息,再次发起组网;
	if(gBtTwsAppCt->btTwsPairingStart)
	{
		memset(&btManager.btLinkDeviceInfo[0], 0, sizeof(BT_LINK_DEVICE_INFO));
		BtTwsPairingTimeout();
	}
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTwsRecvCmdData(BT_TWS_CALLBACK_PARAMS * param)
{
	uint8_t para_offset;

	if(param == NULL)
		return;
	if(param->role == BT_TWS_SLAVE && (param->params.twsData)[0] == 'd')
		return;

	switch(GetTwsRecCmd(param->params.twsData,&para_offset))
	{
	default:
		break;
	case CMD_TWS_RESET:
		APP_DBG("%s:cmd_reset\n\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master");
#ifdef TWS_CODE_BACKUP
		g_tws_need_init = 1;
#endif
		break;
	case CMD_TWS_SNIFF:
#ifdef BT_SNIFF_ENABLE
		APP_DBG("%s:cmd_gotodeepsleep\n\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master");
		send_sniff_msg();
#endif
		break;
	case CMD_TWS_REPAIR:
		APP_DBG("%s:cmd_repair\n\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master");
		BtTwsPeerEnterPairingMode();
		break;
	case CMD_TWS_VOL:
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t MuteFlag 	  = param->params.twsData[para_offset + 1];
			mainAppCt.MusicVolume = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("%s rcv cmd_vol_mute:%u %u\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master",
												   mainAppCt.MusicVolume, MuteFlag);
			if(MuteFlag)
			{
				if(IsAudioPlayerMute() == FALSE)
				{
					HardWareMuteOrUnMute();
				}
				if(param->role == BT_TWS_SLAVE)
				{
					TwsSlaveFifoMuteTimeSet();
				}
			}
			else
			{
				if(param->role == BT_TWS_SLAVE)
				{
					TwsSlaveFifoUnmuteSet();
					AudioMusicVol(mainAppCt.MusicVolume);
					SystemVolSet();
					break;
				}
	#if (BT_AVRCP_VOLUME_SYNC == ENABLE)
				//add volume sync(bluetooth play mode)
				if(GetSystemMode() == ModeBtAudioPlay)
				{
					MessageContext		msgSend;

					extern void SetBtSyncVolume(uint8_t volume);
					SetBtSyncVolume(mainAppCt.MusicVolume);

					msgSend.msgId		= MSG_BT_PLAY_VOLUME_SET;
					MessageSend(GetSysModeMsgHandle(), &msgSend);
				}
	#endif
				if(IsAudioPlayerMute() == TRUE)
				{
					HardWareMuteOrUnMute();
				}
				AudioMusicVolSet(mainAppCt.MusicVolume);
				SystemVolSet();
	#ifdef TWS_CODE_BACKUP
	#ifdef CFG_RES_AUDIO_DAC0_EN
				AudioDAC_DigitalMute(DAC0, FALSE, FALSE);
	#endif

	#ifdef CFG_RES_AUDIO_DACX_EN
				AudioDAC_DigitalMute(DAC1, FALSE, FALSE);
	#endif
	#endif
	#ifdef CFG_FUNC_REMIND_SOUND_EN
				{
					extern void master_remind_sound_mute_release(void);
					master_remind_sound_mute_release();
				}
	#endif
				APP_DBG("\n--Unmute\n");
			}
		}
		break;
	case CMD_TWS_MUTE:
		if(param->role == BT_TWS_SLAVE)
		{
			uint8_t MuteFlag 	  = param->params.twsData[para_offset];
			if(MuteFlag)
			{
				if(!IsAudioPlayerMute())
				{
					HardWareMuteOrUnMute();
				}
				TwsSlaveFifoMuteTimeSet();
			}
			else
			{
				TwsSlaveFifoUnmuteSet();
			}
		}
		break;
	case CMD_TWS_REMINDCLEAR_MSG:
		if(param->role == BT_TWS_SLAVE)
		{
			uint8_t isclear 	  = param->params.twsData[para_offset];
			if(isclear)
			{
				RemindSoundClearSlavePlay();
			}
			else
			{
				//nothing to do
			}
		}
		break;
#if (BT_HFP_SUPPORT == ENABLE)
	case CMD_TWS_HFP_STATE:
		if(param->role == BT_TWS_SLAVE)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t val = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("slave rcv cmd_hfp_state = %d\n", val);
		}
		break;
#endif
	case CMD_TWS_A2DP_STATE:
		if(param->role == BT_TWS_SLAVE)
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			uint8_t val = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("slave rcv cmd_a2dp_state = %d\n", val);
		}
		break;
#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
	case CMD_TWS_EQ:
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			mainAppCt.EqMode = param->params.twsData[para_offset];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("%s rcv cmd_eq_mode = %u\n", (param->role == BT_TWS_SLAVE)?"Slave":"Master",mainAppCt.EqMode);
		#ifndef CFG_FUNC_EQMODE_FADIN_FADOUT_EN
			EqModeSet(mainAppCt.EqMode);
		#endif
		}
		break;
#endif
#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
	case CMD_TWS_MUSIC_TREB_BASS:
		{
			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			mainAppCt.MusicBassStep = param->params.twsData[para_offset];
			mainAppCt.MusicTrebStep = param->params.twsData[para_offset + 1];
			if(ret)
			{
				GIE_ENABLE();
			}
			APP_DBG("%s rcv cmd_music_bass_treb = %u %u\n",	(param->role == BT_TWS_SLAVE)?"Slave":"Master",
															mainAppCt.MusicBassStep,mainAppCt.MusicTrebStep);
			MusicBassTrebAjust(mainAppCt.MusicBassStep, mainAppCt.MusicTrebStep);
		}
		break;
#endif
#ifdef CFG_FUNC_REMIND_SOUND_EN
	case CMD_TWS_REMIND:
		{
			uint8_t buf[10];

			bool ret = GIE_STATE_GET();
			GIE_DISABLE();
			memcpy(&buf[0],&param->params.twsData[para_offset],9);
			buf[9] = 0;
			if(ret)
			{
				GIE_ENABLE();
			}

			APP_DBG("%s rcv cmd_remind_sound = %x %s\n", (param->role == BT_TWS_SLAVE)?"Slave":"Master",
														 buf[0],&buf[1]);
			RemindSoundServiceItemRequest((char *)(&buf[1]),(uint32_t)buf[0]);
		}
		break;
#endif
	case CMD_TWS_KEY_MSG:
		{
			MessageContext msgSend;

			bool ret = GIE_STATE_GET();
			GIE_DISABLE();

			msgSend.msgId = param->params.twsData[para_offset];
			msgSend.msgId <<= 8;
			msgSend.msgId |= param->params.twsData[para_offset + 1];

			if(ret)
			{
				GIE_ENABLE();
			}

			DBG("%s rev key msg: 0x%x\n",(param->role == BT_TWS_SLAVE)?"Slave":"Master",msgSend.msgId);
			if(GetSystemMode() == ModeBtHfPlay && param->role == BT_TWS_MASTER)
			{
				if(msgSend.msgId != MSG_POWERDOWN) //通话下响应从机关机命令
				break;
			}
			MessageSend(GetMainMessageHandle(), &msgSend);
		}
		break;
	}
}

/**************************************************************************
 * tws slave callback
 *************************************************************************/
void BtTws_Slave_Connected(BT_TWS_CALLBACK_PARAMS * param)
{
	APP_DBG("TWS_SLAVE_CONNECTED:\n");
	{
		MessageContext		msgSend;
		msgSend.msgId		= MSG_BT_TWS_SLAVE_CONNECTED;
		MessageSend(GetMainMessageHandle(), &msgSend);
	}
	memcpy(btManager.btTwsDeviceAddr, param->params.bd_addr, param->paramsLen);
	APP_DBG("addr: 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x \n",
		btManager.btTwsDeviceAddr[0],
		btManager.btTwsDeviceAddr[1],
		btManager.btTwsDeviceAddr[2],
		btManager.btTwsDeviceAddr[3],
		btManager.btTwsDeviceAddr[4],
		btManager.btTwsDeviceAddr[5]
		);
	btManager.twsState = BT_TWS_STATE_CONNECTED;
	btManager.twsRole = BT_TWS_SLAVE;
	BtDdb_UpgradeTwsInfor(btManager.btTwsDeviceAddr);
	btManager.twsFlag = 1;
	btManager.btReconnectedFlag = 1;
	
	BtReconnectDevStop();
	BtReconnectTwsStop();
	
	if(btManager.btConStateProtectCnt)
		btManager.btConStateProtectCnt=0;
	
//蓝牙可见性配置
	//slave在tws组网成功后，进入不可被搜索不可被连接状态
	BtSetAccessMode_NoDisc_NoCon();
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	tws_slave_simple_pairing_end();
#ifdef BT_SNIFF_ENABLE
	if(Bt_sniff_sniff_start_state_get() && (gBtTwsSniffLinkLoss==0))
		Bt_sniff_sniff_stop();
#endif
#endif

#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM)
	BtTwsExitPairingMode();
#elif ((TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_MASTER))
	BtTwsExitSimplePairingMode();
#else //CFG_TWS_PEER_SLAVE/CFG_TWS_PEER_MASTER
	BtTwsExitPeerPairingMode();
#endif

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	TwsSlaveModeEnter();
#endif
}

/**************************************************************************
 *  
 *************************************************************************/
void BtTws_Slave_Disconnected(BT_TWS_CALLBACK_PARAMS * param)
{
	btManager.twsMode=0;
	APP_DBG("TWS_SLAVE_DISCONNECT:\n");
	btManager.twsState = BT_TWS_STATE_NONE;
	
#if (TWS_PAIRING_MODE == CFG_TWS_ROLE_SLAVE)
	if(Bt_sniff_sniff_start_state_get())
	{
		//BleScanParamConfig_Sniff();
		BtTwsConnectApi();
		gBtTwsSniffLinkLoss = 1;
	}
	else if(!btManager.twsSbSlaveDisable)
	{
		tws_slave_simple_pairing_ready();
	}
#elif ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))//||(TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	if(tws_reconnect_flag_get())
	{
		extern void BtTwsLinkLoss(void);
		APP_DBG("tws link loss, reconnect...\n");
		BtTwsLinkLoss();
	}
	
	BtStackServiceMsgSend(MSG_BTSTACK_RECONNECT_REMOTE_PAGE_TIMEOUT);
#elif (TWS_PAIRING_MODE == CFG_TWS_PEER_SLAVE)
	TwsSlaveModeExit();
#endif

#ifdef CFG_AUTO_ENTER_TWS_SLAVE_MODE
	TwsSlaveModeExit();
#endif

	BtReconnectTwsStop();

}

/**************************************************************************
 *  tws audio
 *************************************************************************/
bool tws_state_audiocore_assort(TWS_SYNC_STATE State, TWS_SYNC_STATE LastState)
{
	bool ret = FALSE;
	if(State == LastState )
	{
		if(IsTimeOut(&TwsMuteTimer))
		{
			ret = TRUE;
			if(State == TWS_CONNECT_JITTER && tws_get_role() == BT_TWS_MASTER)
			{
				if(btManager.twsState == BT_TWS_STATE_CONNECTED)
				{
					mv_lmp_send(TWS_CMD_DEVICE_OPEN, 0, 0);
					TimeOutSet(&TwsDacOnTimer, 200);
					btManager.TwsDacNeedTimeout = TRUE;
				}
				else
				{
					tws_device_open_isr();
					if(AudioCoreSinkIsInit(TWS_SINK_NUM))
					{
						AudioCoreSinkDepthChange(TWS_SINK_NUM, 2 * 512);
					}
					AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
				}
			}
		}
	}
	else
	{
		switch(State)
		{
			case TWS_CONNECT_JITTER:
				tws_device_close();//reset fifo;
			case TWS_DISCONNECT_JITTER:
				tws_master_vol_send(mainAppCt.MusicVolume, IsAudioPlayerMute());
				AudioCoreSourceMute(TWS_SOURCE_NUM, TRUE, TRUE);
				TimeOutSet(&TwsMuteTimer, TWS_CONNECT_MUTE);
				break;

			case TWS_HW_INIT:
				if(AudioCoreSinkIsInit(TWS_SINK_NUM))
				{
					AudioCoreSinkDepthChange(TWS_SINK_NUM, TWS_STATRT_PLAY_FRAM * 128);
				}
				AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
				break;

			case TWS_DISCONNECT:
				if(AudioCoreSinkIsInit(TWS_SINK_NUM))
				{
					AudioCoreSinkDepthChange(TWS_SINK_NUM, 2 * 512);
				}
				AudioCoreSourceUnmute(TWS_SOURCE_NUM, TRUE, TRUE);//for test
				break;

			case TWS_AUDIO:
#ifdef CFG_FUNC_REMIND_SOUND_EN
				if(tws_get_role() == BT_TWS_MASTER && SoftFlagGet(SoftFlagTwsRemind))
				{
					RemindSoundServiceItemRequest(SOUND_REMIND_LEFTCHAN, REMIND_PRIO_PARTNER);//仅主机播放，从机mute播放，local audio识别。
					RemindSoundServiceItemRequest(SOUND_REMIND_RIGHTCHA, REMIND_PRIO_PARTNER | REMIND_ATTR_MUTE);//mute主机播放，local audio识别。
				}
				SoftFlagDeregister(SoftFlagTwsRemind);
#endif
				break;

			default:
				break;
		}
	}
	return ret;
}

/**************************************************************************
 *  
 *************************************************************************/
uint32_t tws_audio_adjust(uint32_t m_len,uint32_t s_len)
{
	int tt = m_len-s_len;
	printf("%d\n",tt);
	if(abs(tt) > 150)
	{
		error_count++;
		if(error_count > 5)
		{
			tws_slave_tws_reset_send();
		}
	}
	else
	{
		error_count = 0;;
	}
	return 1;
}


bool is_tws_slave(void)
{
	if((GetBtManager()->twsRole == BT_TWS_SLAVE)&&(GetBtManager()->twsState == BT_TWS_STATE_CONNECTED))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

#else

bool tws_state_audiocore_assort(TWS_SYNC_STATE State, TWS_SYNC_STATE LastState)
{
	return 0;
}

bool is_tws_slave(void)
{
	return 0;
}

#endif

