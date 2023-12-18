#include <string.h>
#include <nds32_intrinsic.h>
#include "math.h"
#include "debug.h"
#include "app_config.h"
#include "rtos_api.h"
#include "audio_effect_api.h"
#include "audio_effect_library.h"
#include "audio_core_api.h"
#include "main_task.h"
#include "audio_effect.h"
#include "tws_mode.h"
#include "remind_sound.h"
#include "ctrlvars.h"
#include "comm_param.h"
#include "bt_manager.h"
#include "mode_task.h"
#include "spi_flash.h"
#include "audio_effect_flash_param.h"
#include "audio_effect_user.h"

#ifdef CFG_FUNC_AUDIO_EFFECT_EN

#define 	EFFECT_PARAM_OFFSET			2//不要随意修改，Sam，2021/12/15

#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
extern uint8_t  EqMode_Addr;
extern uint32_t music_eq_mode_unit;
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
extern EffectComCt* AudioEffectParamCtAddr;
extern uint8_t  effect_sum;
extern uint16_t effect_list[AUDIO_EFFECT_SUM];
extern uint16_t effect_list_addr[AUDIO_EFFECT_SUM];
extern void* effect_list_param[AUDIO_EFFECT_SUM];
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
extern uint8_t effect_enable_list[AUDIO_EFFECT_SUM];
#endif
#endif

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
extern EffectPaCt EffectParamCt;
#endif

EffectNodeList  gEffectNodeList[AUDIO_EFFECT_GROUP_NUM];
uint8_t AudioModeIndex ;//当前模式下使用的音效位于音效组合列表中的编号

PCM_DATA_TYPE * pcm_buf_1;
PCM_DATA_TYPE * pcm_buf_2;
PCM_DATA_TYPE * pcm_buf_3;
PCM_DATA_TYPE * pcm_buf_4;
PCM_DATA_TYPE * pcm_buf_5;

typedef enum
{
	BTPlayGain=0,
	USBPlayGain,
	USBDeviceGain,
	SDPlayGain,
	I2SGian,
	OPTICALGian,
	COAXIALGian
}DigitalGain;

const uint32_t AudioModeDigitalGianTable[][2]=
{
	{ModeBtAudioPlay, 	 BTPlayGain},
	{ModeUDiskAudioPlay, USBPlayGain},
	{ModeUsbDevicePlay,	USBDeviceGain,},
	{ModeCardAudioPlay,  SDPlayGain},
	{ModeI2SInAudioPlay, I2SGian},
	{ModeOpticalAudioPlay, OPTICALGian},
	{ModeCoaxialAudioPlay, COAXIALGian},
};
//app通路数字预增益处理
void AudioAPPDigitalGianProcess(uint32_t AppMode)
{
	EffectNode*  pNode = NULL;
	uint32_t i;
	uint32_t CurMode = 0xFF;
	GainControlUnit *p;
	
	for(i=0; i<sizeof(AudioModeDigitalGianTable)/8; i++)
	{
		if(AppMode == AudioModeDigitalGianTable[i][0])
		{
			CurMode = AudioModeDigitalGianTable[i][1];
			DBG("CurMode = %ld\n", CurMode);
		}
	}
	if(CurMode != 0xFF)
	{
		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[0].EffectNode[i];//伴奏使用第0组音效列表
			if(i == CurMode)
			{
				pNode->Enable = TRUE;
				if(pNode->EffectUnit != NULL)
				{
					p = (GainControlUnit *)pNode->EffectUnit;
					p->enable = TRUE;
				}
			}
			else
			{
				pNode->Enable = FALSE;
			}
		}
	}
	else// 数字增益控制全部变灰色
	{
		for(i=0; i<AUDIO_EFFECT_NODE_NUM; i++)
		{
			pNode = &gEffectNodeList[0].EffectNode[i];//伴奏使用第0组音效列表
			pNode->Enable = FALSE;
		}
	}
}

void EffectPcmBufMalloc(uint32_t SampleLen)
{
#ifdef CFG_FUNC_MIC_KARAOKE_EN
	if(pcm_buf_1 == NULL){
		pcm_buf_1 = (PCM_DATA_TYPE *)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_1 == NULL){
		APP_DBG("pcm_buf_1 malloc err\n");
		return;
	}

	if(pcm_buf_2 == NULL){
		pcm_buf_2 = (PCM_DATA_TYPE *)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_2 == NULL){
		APP_DBG("pcm_buf_2 malloc err\n");
		return;
	}

	if(pcm_buf_3 == NULL){
		pcm_buf_3 = (PCM_DATA_TYPE *)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_3 == NULL){
		APP_DBG("pcm_buf_3 malloc err\n");
		return;
	}

    #ifdef CFG_FUNC_ECHO_DENOISE
//	if(EchoAudioBuf == NULL){
//		EchoAudioBuf = (int16_t*)osPortMallocFromEnd(SampleLen * 2 * 2);
//	}
//  if(EchoAudioBuf == NULL){
//		APP_DBG("EchoAudioBuf malloc err\n");
//	}else{
//		memset(EchoAudioBuf, 0, SampleLen * 2 * 2);
//	}
    #endif
#endif

#if (BT_HFP_SUPPORT == ENABLE)
	if(GetSystemMode() == ModeBtHfPlay)
	{
		if(pcm_buf_4 == NULL){
			pcm_buf_4 = (PCM_DATA_TYPE *)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2);
		}
		if(pcm_buf_4 == NULL){
			APP_DBG("pcm_buf_4 malloc err\n");
			return;
		}
		if(pcm_buf_5 == NULL){
			pcm_buf_5 = (PCM_DATA_TYPE *)osPortMallocFromEnd(SampleLen * sizeof(PCM_DATA_TYPE) * 2);
		}
		if(pcm_buf_5 == NULL){
			APP_DBG("pcm_buf_5 malloc err\n");
			return;
		}
	}
#endif
}

void EffectPcmBufRelease(void)
{
	if(pcm_buf_1 != NULL)
	{
		osPortFree(pcm_buf_1);
		pcm_buf_1 = NULL;
	}
	if(pcm_buf_2 != NULL)
	{
		osPortFree(pcm_buf_2);
		pcm_buf_2 = NULL;
	}
	if(pcm_buf_3 != NULL)
	{
		osPortFree(pcm_buf_3);
		pcm_buf_3 = NULL;
	}
	if(pcm_buf_4 != NULL)
	{
		osPortFree(pcm_buf_4);
		pcm_buf_4 = NULL;
	}
	if(pcm_buf_5 != NULL)
	{
		osPortFree(pcm_buf_5);
		pcm_buf_5 = NULL;
	}
    #ifdef CFG_FUNC_ECHO_DENOISE
//  if(EchoAudioBuf != NULL)
//	{
//		osPortFree(EchoAudioBuf);
//		EchoAudioBuf = NULL;
//	}
    #endif
}

void EffectPcmBufClear(uint32_t SampleLen)
{
	if(pcm_buf_1 != NULL)
	{
		memset(pcm_buf_1, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_2 != NULL)
	{
		memset(pcm_buf_2, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_3 != NULL)
	{
		memset(pcm_buf_3, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_4 != NULL)
	{
		memset(pcm_buf_4, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
	if(pcm_buf_5 != NULL)
	{
		memset(pcm_buf_5, 0, SampleLen * sizeof(PCM_DATA_TYPE) * 2);
	}
}

#define 	AudioFramSize	128//

const uint16_t AudioFrame[]={
	256,          //AUTO_TUNE = 0x00, 可以任意长度
	AudioFramSize,//DC_BLOCK,//未使用
	AudioFramSize,//DRC,
	AudioFramSize,//ECHO,
	AudioFramSize,//EQ,
	AudioFramSize,//EXPANDER,//5,Noise Sup
	AudioFramSize,//FREQ_SHIFTER,
	256,          //HOWLING_SUPPRESSOR,
	AudioFramSize,//NOISE_GATE,//未使用
	256,//9       //PITCH_SHIFTER,
	AudioFramSize,//REVERB,//10
	AudioFramSize,//SILENCE_DETECTOR,
	AudioFramSize,//THREE_D,
	AudioFramSize,//VIRTUAL_BASS,
	512,//14      //VOICE_CHANGER,128,256,512, 256 MCPS高
	AudioFramSize,//GAIN_CONTROL,//15
	AudioFramSize,//VOCAL_CUT,
	AudioFramSize,//PLATE_REVERB,
	256,          //REVERB_PRO,
	256,//19      //VOICE_CHANGER_PRO,
	AudioFramSize,//PHASE_CONTROL,//20
	256,          //VOCAL_REMOVE,
	512,          //PITCH_SHIFTER_PRO,
	AudioFramSize,//VIRTUAL_BASS_CLASSIC,
	AudioFramSize,//PCM_DELAY,
	AudioFramSize,//EXCITER,//25
	AudioFramSize,//CHORUS,
	AudioFramSize,//AUTO_WAH,
	AudioFramSize,//STEREO_WIDENER,
	AudioFramSize,//PINGPONG,
	AudioFramSize,//THREE_D_PLUS,//30
	AudioFramSize,//SINE_GENERATOR,
	AudioFramSize,//NOISE_Suppressor_Blue,
	AudioFramSize,//AEC = 100,
};

uint16_t AudioEffectFramSizeGet(void)
{
	uint16_t i,j;
	uint16_t Len = AudioFramSize;
	EffectNode*  pNode = NULL;
	for(i=0;i<AUDIO_EFFECT_GROUP_NUM;i++)
	{
		for(j=0;j<AUDIO_EFFECT_NODE_NUM;j++)
		{
			pNode = &gEffectNodeList[i].EffectNode[j];
			if(pNode->Enable == FALSE)
			{
				continue;
			}
			if(AudioFrame[pNode->EffectType] > Len)
			{
				Len = AudioFrame[pNode->EffectType];
				DBG("Len= %d, i=%d, j=%d\n", Len,i,j);
			}
		}
	}
	DBG("Len = %d\n", Len);
	return Len;
}

//音效模块初始化
__attribute__((optimize("Og")))
void AudioEffectsInit(void)
{
	uint32_t i, j;
	uint32_t ChannelNum;
	EffectNode*  pNode = NULL;
	uint32_t SampleRate;
	//音效处理music通路时
	SampleRate = AudioCoreMixSampleRateGet(AudioCoreSourceMixNetGet(APP_SOURCE_NUM));
	DBG("!!!sampleRate = %ld\n", SampleRate);

	for(i=0; i<AUDIO_EFFECT_GROUP_NUM; i++)
	{
		for(j=0; j<AUDIO_EFFECT_NODE_NUM; j++)
		{
			pNode = &gEffectNodeList[i].EffectNode[j];
			ChannelNum = gEffectNodeList[i].Channel;
			if(pNode->Enable == FALSE)
			{
				continue;
			}
			switch(pNode->EffectType)
			{
#if CFG_AUDIO_EFFECT_AUTO_TUNE_EN
			case AUTO_TUNE:
				AudioEffectAutoTuneInit((AutoTuneUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
			case DC_BLOCK:
				break;
#if CFG_AUDIO_EFFECT_DRC_EN
			case DRC://DRC
				AudioEffectDRCInit((DRCUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_ECHO_EN
			case ECHO:
				AudioEffectEchoInit((EchoUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_EQ_EN
			case EQ:
				AudioEffectEQInit((EQUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN
			case EXPANDER://5,Noise Suppressor
				AudioEffectExpanderInit((ExpanderUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN
			case FREQ_SHIFTER:
				AudioEffectFreqShifterInit((FreqShifterUnit *)pNode->EffectUnit);
				break;
#endif
#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN
			case HOWLING_SUPPRESSOR:
				AudioEffectHowlingSuppressorInit((HowlingUnit *)pNode->EffectUnit);
				break;
#endif
			case NOISE_GATE:
				break;
#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN
			case PITCH_SHIFTER:
				AudioEffectPitchShifterInit((PitchShifterUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_REVERB_EN
			case REVERB:
				AudioEffectReverbInit((ReverbUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN
			case SILENCE_DETECTOR://11
				AudioEffectSilenceDetectorInit((SilenceDetectorUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_3D_EN
			case THREE_D:
				AudioEffectThreeDInit((ThreeDUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_VB_EN
			case VIRTUAL_BASS://13,VB
				AudioEffectVBInit((VBUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_VOICE_CHANGER_EN
			case VOICE_CHANGER:
				AudioEffectVoiceChangerInit((VoiceChangerUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_GAIN_CONTROL_EN
			case GAIN_CONTROL://15
				AudioEffectPregainInit((GainControlUnit *)pNode->EffectUnit, ChannelNum);
				break;
#endif
#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
			case VOCAL_CUT:
				AudioEffectVocalCutInit((VocalCutUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_PLATE_REVERB_EN
			case PLATE_REVERB:
				AudioEffectPlateReverbInit((PlateReverbUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
			case REVERB_PRO:
				AudioEffectReverbProInit((ReverbProUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN
			case VOICE_CHANGER_PRO:
				AudioEffectVoiceChangerProInit((VoiceChangerProUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_PHASE_CONTROL_EN
			case PHASE_CONTROL://20
				AudioEffectPhaseInit((PhaseControlUnit *)pNode->EffectUnit,ChannelNum);
				break;
#endif
#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
			case VOCAL_REMOVE:
				AudioEffectVocalRemoveInit((VocalRemoveUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN
			case PITCH_SHIFTER_PRO:
				AudioEffectPitchShifterProInit((PitchShifterProUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_VB_CLASS_EN
			case VIRTUAL_BASS_CLASSIC://23
				AudioEffectVBClassInit((VBClassUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_PCM_DELAY_EN
			case PCM_DELAY:
				AudioEffectPcmDelayInit((PcmDelayUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_EXCITER_EN
			case EXCITER://25
				AudioEffectExciterInit((ExciterUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_CHORUS_EN
			case CHORUS:
				AudioEffectChorusInit((ChorusUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_AUTOWAH_EN
			case AUTO_WAH:
				AudioEffectAutoWahInit((AutoWahUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
			case STEREO_WIDENER:
				AudioEffectStereoWidenerInit((StereoWidenerUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_PINGPONG_EN
			case PINGPONG:
				AudioEffectPingPongInit((PingPongUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_3D_PLUS_EN
			case THREE_D_PLUS://30
				AudioEffectThreeDPlusInit((VBClassUnit *)pNode->EffectUnit,ChannelNum,SampleRate);
				break;
#endif
			case SINE_GENERATOR://不支持
				break;
#if CFG_AUDIO_EFFECT_NS_BLUE_EN
			case NOISE_Suppressor_Blue:
				AudioEffectNSBlueInit((NSBlueUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_FLANGER_EN
			case FLANGER:
				AudioEffectFlangerInit((NSBlueUnit *)pNode->EffectUnit, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN
			case FREQ_SHIFTER_FINE:
				AudioEffectFreqShifterFineInit((NSBlueUnit *)pNode->EffectUnit, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_OVERDRIVE_EN
			case OVERDRIVE:
				AudioEffectOverdriveInit((OverdriveUnit *)pNode->EffectUnit, SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_DISTORTION_EN
			case DISTORTION:
				AudioEffectDistortionInit((DistortionUnit *)pNode->EffectUnit, SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_EQDRC_EN
			case EQ_DRC:
				AudioEffectEQDRCInit((EQDRCUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_AEC_EN
			case AEC:
				AudioEffectAECInit((AECUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
			case DISTORTION_DS1:
				AudioEffectDistortionDS1Init((DistortionDS1Unit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_OVERDRIVE_POLY_EN
			case OVERDRIVE_POLY:
				AudioEffectOverdrivePolyInit((OverdrivePolyUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_COMPANDER_EN
			case COMPANDER:
				AudioEffectCompanderInit((CompanderUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if	CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
			case LOW_LEVEL_COMPRESSOR:
				AudioEffectLowLevelCompressorInit((LowLevelCompressorUnit *)pNode->EffectUnit, ChannelNum, SampleRate);
				break;
#endif
#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN
			case HOWLING_FINE:
				AudioEffectHowlingSuppressorFineInit((HowlingFineUnit *)pNode->EffectUnit,SampleRate);
				break;
#endif
			default:
				break;
			}
		}
	}
}

// 音效模块反初始化
__attribute__((optimize("Og")))
void AudioEffectsDeInit(void)
{
	uint32_t i, j;
	EffectNode*  pNode = NULL;

#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif
    APP_DBG("AudioEffectsDeInit \n");

    for(i=0; i<AUDIO_EFFECT_GROUP_NUM; i++)
	{
		for(j=0; j<AUDIO_EFFECT_NODE_NUM; j++)
		{
			pNode = &gEffectNodeList[i].EffectNode[j];
			switch(pNode->EffectType)
			{
			#if CFG_AUDIO_EFFECT_ECHO_EN
			case ECHO:
				{
				EchoUnit * pTemp = NULL;
				if(pNode->EffectUnit != NULL)
				{
					pTemp = pNode->EffectUnit;
					if(pTemp->buf != NULL)
					{
						osPortFree(pTemp->buf);
					}
				}
				}
				break;
			#endif
			#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
			case REVERB_PRO:
				{
				ReverbProUnit * pTemp = NULL;
				if(pNode->EffectUnit != NULL)
				{
					pTemp = pNode->EffectUnit;
					if(pTemp->ct != NULL)
					{
						osPortFree(pTemp->ct);
					}
				}
				}
				break;
			#endif
			#if CFG_AUDIO_EFFECT_PCM_DELAY_EN
			case PCM_DELAY:
				{
				PcmDelayUnit * pTemp = NULL;
				if(pNode->EffectUnit != NULL)
				{
					pTemp = pNode->EffectUnit;
					if(pTemp->s_buf != NULL)
					{
						osPortFree(pTemp->s_buf);
					}
				}
				}
				break;
			#endif
			#if CFG_AUDIO_EFFECT_PINGPONG_EN
			case PINGPONG:
				{
				PingPongUnit * pTemp = NULL;
				if(pNode->EffectUnit != NULL)
				{
					pTemp = pNode->EffectUnit;
					if(pTemp->s != NULL)
					{
						osPortFree(pTemp->s);
					}
				}
				}
				break;
			#endif
			default:
				break;
			}
			//if(pNode->Enable == TRUE)
			{
				//节点有效，则释放音效结构体资源
				if(pNode->EffectUnit != NULL)
				{
					osPortFree(pNode->EffectUnit);
					pNode->EffectUnit = NULL;
				}
				//pNode->Enable = FALSE;
			}
		}
	}
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
#endif
}

//音效节点遍历查找
__attribute__((optimize("Og")))
EffectNode* FindEffectNode(uint8_t index)
{
	uint32_t i;
	uint32_t j;
	EffectNode *node = NULL;
	for(i=0; i<AUDIO_EFFECT_GROUP_NUM; i++)
	{
		for(j=0; j<AUDIO_EFFECT_NODE_NUM; j++)
		{
			if(index == gEffectNodeList[i].EffectNode[j].Index)
			{
				node = &gEffectNodeList[i].EffectNode[j];
				return node;
			}
		}
	}
	return node;
}

__attribute__((optimize("Og")))
void AudioEffectNoteParamAssign(void* pParam, void* pstr, uint8_t effect_index, uint16_t Len, bool IsReload)
{
	memcpy(pParam, pstr, Len);
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	if(IsReload == FALSE)
	{
		memcpy(pParam, effect_list_param[effect_index], Len);
	}
#endif
}

__attribute__((optimize("Og")))
void AudioEffectNoteParamBackup(void* pstr, uint8_t effect_index, uint16_t Len, bool IsReload)
{
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	if(IsReload == TRUE)//备份数据，更新ram中的参数
	{
		memcpy(effect_list_param[effect_index], pstr, Len);
	}
#endif
}

//音效解析
__attribute__((optimize("Og")))
bool AudioEffectParsePackage(uint8_t* add, uint16_t PackageLen, uint8_t* CommParam, bool IsReload)
{
	uint8_t EffectName;//音效还是硬件配置
	uint8_t EffectIndex;
	uint8_t EffectWidth;
	uint8_t EffectType;
	uint8_t EffectFlag;
	uint8_t EffectGroup;
	uint8_t EffectNodeType;
	uint8_t EffectGroupBack = 0xFF;
	uint16_t sPackageLen;
	uint16_t Len = 0;
	uint8_t j = 0;
	uint8_t effect_index = 0;

	volatile EffectComCt* AudioEffectComCt;
	AudioEffectComCt = (EffectComCt*)CommParam;

	AudioEffectUserNodeInit();

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
	extern char TagBuf[32];
	extern uint8_t lenTag;
	//调音，sam
	uint8_t eff_addr;
	eff_addr = 0x81;
	effect_sum = 0;
	memset(effect_list, 0, sizeof(uint16_t)*AUDIO_EFFECT_SUM);
	memset(effect_list_addr, 0, sizeof(uint16_t)*AUDIO_EFFECT_SUM);

	gCtrlVars.AutoRefresh = 1;//音效重新加载均需要和PC通讯一次
	if(IsReload == TRUE)
	{
		#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
		extern void AudioEffectParaminit(uint8_t* CommParam);
		AudioEffectParaminit(CommParam);
		#endif
	}
#else
	uint8_t lenTag;
#endif

	EffectNode * pNode = NULL;

	if(IsReload == TRUE)
	{
		memset(gEffectNodeList, 0, sizeof(EffectNodeList) * AUDIO_EFFECT_GROUP_NUM);
	}

	sPackageLen = PackageLen;//需要计算，去除crc相关长度,后续完善
	DBG("!!Audio Effect Parse Package!!\n");
	//需要解析的数据crc16是否有异常

	while(Len < sPackageLen)
	{
		EffectName = add[Len];
		if(EffectName >= 0x81 && EffectName <= 0xFB)
		{
			EffectType = AudioEffectComCt[effect_index].AudioEffectType;//add[Len+1];
			EffectFlag = add[Len+1];

            if(AudioEffectComCt[effect_index].EffectName[1] == ':')
            {
            	EffectGroup = AudioEffectComCt[effect_index].EffectName[0] - 48;//add[Len+3];
            }
            else
            {
            	EffectGroup = (AudioEffectComCt[effect_index].EffectName[0] - 48) * 10;//add[Len+3];
            	EffectGroup += AudioEffectComCt[effect_index].EffectName[1] - 48;
            }

			EffectNodeType = AudioEffectComCt[effect_index].AudioNodeType;//add[Len+4];
			EffectIndex = AudioEffectComCt[effect_index].Index;
			EffectWidth = AudioEffectComCt[effect_index].EffectWidth;
			if(EffectGroup != EffectGroupBack)
			{
				j = 0;//导入音效，切换音效组，从0开始
				EffectGroupBack = EffectGroup;
				//一个Group的声音为立体声还是单声道，取这一组的首个节点
				gEffectNodeList[EffectGroup - 1].Channel = AudioEffectComCt[effect_index].channel;//add[Len+5];
			}

			//if(EffectFlag == TRUE)
			{
				pNode = &gEffectNodeList[EffectGroup - 1].EffectNode[j];
				gEffectNodeList[EffectGroup - 1].EffectNode[j].Index = effect_index;
				gEffectNodeList[EffectGroup - 1].EffectNode[j].NodeType = EffectNodeType;
				gEffectNodeList[EffectGroup - 1].EffectNode[j].Width = EffectWidth;
				j++;//index
				if(j>AUDIO_EFFECT_NODE_NUM)
				{
					DBG("effect Index Err\n");
				}
				if(IsReload == TRUE)
				{
					pNode->Enable = EffectFlag;
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
					effect_enable_list[effect_sum] = EffectFlag;//flash参数区需要备份使能信号
#endif
				}

	#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
				effect_list[effect_sum] = EffectType;
				effect_list_addr[effect_sum] = eff_addr++;
				effect_sum++;
	#endif
				switch(EffectType)
				{
				case AUTO_TUNE:
					{
#if CFG_AUDIO_EFFECT_AUTO_TUNE_EN
					AutoTuneUnit * pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(AutoTuneUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("mem err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectAutoTuneApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(AutoTuneParam), IsReload);

							//for user config
						#ifdef CFG_FUNC_MIC_AUTOTUNE_STEP_EN
							if(EffectIndex != 0xFF)//不为0xFF,也可以直接使用index编号为判断条件
							{
								mic_autotune_unit = pTemp;
							}
						#endif
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(AutoTuneParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(AutoTuneParam);
					}
					break;
				case DC_BLOCK:
					break;
				case DRC://2://DRC
					{
#if CFG_AUDIO_EFFECT_DRC_EN
					DRCUnit * pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(DRCUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("mem err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectDRCApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectDRCApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(DRCParam), IsReload);
						}
					}
					//备份参数
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(DRCParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(DRCParam);
					}
					break;
				case ECHO://注意看是否完善，echo有buf操作
					{
#if CFG_AUDIO_EFFECT_ECHO_EN
					EchoUnit * pTemp = NULL;
					uint32_t buf_size;
					uint32_t max_delay_samples;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(EchoUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("mem err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectEchoApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(EchoParam), IsReload);
							//malloc delay buf
							max_delay_samples = pTemp->param.max_delay * gCtrlVars.sample_rate/1000;
							buf_size = pTemp->param.high_quality_enable ? max_delay_samples*2 : ceilf(max_delay_samples/32) * 19+64;
							pTemp->buf = osPortMallocFromEnd(buf_size);
							if(pTemp->buf == NULL)
							{
								DBG("malloc err\n");
								osPortFree(pNode->EffectUnit);
								pNode->EffectUnit = NULL;
								//return FALSE;
								pNode->Enable = FALSE;
							}
						#ifdef CFG_FUNC_MIC_ECHO_STEP_EN
							if((EffectIndex != 0xFF) && (pTemp->buf != NULL))//不为0xFF,也可以直接使用index编号为判断条件
							{
								mic_echo_unit = pTemp;
								if((IsReload == TRUE)
								|| (gCtrlUserVars.max_echo_delay == 0)/*未被赋值过*/)
								{
									gCtrlUserVars.max_echo_delay = pTemp->param.max_delay;
									gCtrlUserVars.max_echo_depth = pTemp->param.attenuation;
								}
							}
						#endif
						}
					}
					//备份参数
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(EchoParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(EchoParam);
					}
					break;
				case EQ:
					{
#if CFG_AUDIO_EFFECT_EQ_EN
					EQUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(EQUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc eq unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectEQApply24;

							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectEQApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(EQParam), IsReload);

							//for user config
//						#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
//							if(EffectIndex == 0x8E)//不为0xFF,也可以直接使用index编号为判断条件
//							{
//								music_trebbass_eq_unit = pTemp;//Treb/bass
//							}
//						#endif
						#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN
							if(EffectIndex == 0x8F)//不为0xFF,也可以直接使用index编号为判断条件
							{
								music_mode_eq_unit = pTemp;//Treb/bass
							}
						#endif
						}
					}
					//备份参数
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(EQParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(EQParam);
					}
					break;
				case EXPANDER://5,Noise Suppressor
					{
#if CFG_AUDIO_EFFECT_NOISE_SUPPRESSOR_EN
					ExpanderUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ExpanderUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc expadner Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectExpanderApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectExpanderApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ExpanderParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ExpanderParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ExpanderParam);
					}
					break;
				case FREQ_SHIFTER:
					{
#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_EN
					FreqShifterUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(FreqShifterUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc FreqShifter Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectFreqShifterApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(FreqShifterParam), IsReload);
							if(pTemp->param.deltaf >= 8)
							{
								pTemp->param.deltaf = 7;
							}
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(FreqShifterParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(FreqShifterParam);
					}
					break;
				case HOWLING_SUPPRESSOR:
					{
#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_EN
					HowlingUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(HowlingUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc howling Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectHowlingSuppressorApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(HowlingParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(HowlingParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(HowlingParam);
					}
					break;
				case NOISE_GATE:
					break;
				case PITCH_SHIFTER:
					{
#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_EN
					PitchShifterUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(PitchShifterUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc PitchShifter Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPitchShifterApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PitchShifterParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PitchShifterParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(PitchShifterParam);
					}
					break;
				case REVERB:
					{
#if CFG_AUDIO_EFFECT_REVERB_EN
					ReverbUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ReverbUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Reverb Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectReverbApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ReverbParam), IsReload);
						#ifdef CFG_FUNC_MIC_REVERB_STEP_EN
							if(EffectIndex != 0xFF)//不为0xFF,也可以直接使用index编号为判断条件
							{
								mic_reverb_unit = pTemp;
								if((IsReload == TRUE)
								|| (gCtrlUserVars.max_reverb_roomsize == 0)/*未被赋值过*/)
								{
									gCtrlUserVars.max_reverb_wet_scale = pTemp->param.wet_scale;
									gCtrlUserVars.max_reverb_roomsize  = pTemp->param.roomsize_scale;
								}
							}
						#endif
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ReverbParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ReverbParam);
					}
					break;
				case SILENCE_DETECTOR://11
					{
#if CFG_AUDIO_EFFECT_SILENCE_DECTOR_EN
					SilenceDetectorUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(SilenceDetectorUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc SilenceDeterctor Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectSilenceDetectorApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectSilenceDetectorApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(SilenceDetectorParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(SilenceDetectorParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(SilenceDetectorParam);
					}
					break;
				case THREE_D:
					{
#if CFG_AUDIO_EFFECT_3D_EN
					ThreeDUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ThreeDUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc 3D Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectThreeDApply24;

							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectThreeDApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ThreeDParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ThreeDParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ThreeDParam);
					}
					break;
				case VIRTUAL_BASS://0x0D://VB
					{
#if CFG_AUDIO_EFFECT_VB_EN
					VBUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(VBUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc VB Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVBApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVBApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VBParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VBParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(VBParam);
					}
					break;
				case VOICE_CHANGER:
					{
#if CFG_AUDIO_EFFECT_VOICE_CHANGER_EN
					VoiceChangerUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(VoiceChangerUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc VoiceChanger Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVoiceChangerApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VoiceChangerParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VoiceChangerParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(VoiceChangerParam);
					}
					break;
				case GAIN_CONTROL://15
					{
#if CFG_AUDIO_EFFECT_GAIN_CONTROL_EN
					GainControlUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(GainControlUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc GainControl Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPregainApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPregainApply;
							}

							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(GainControlParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(GainControlParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(GainControlParam);
					}
					break;
				case VOCAL_CUT:
					{
#if CFG_AUDIO_EFFECT_VOCAL_CUT_EN
					VocalCutUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(VocalCutUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Vocal Cut Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVocalCutApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VocalCutParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VocalCutParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(VocalCutParam);
					}
					break;
				case PLATE_REVERB:
					{
#if CFG_AUDIO_EFFECT_PLATE_REVERB_EN
					PlateReverbUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(PlateReverbUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Plate Reverb Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPlateReverbApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PlateReverbParam), IsReload);
						#ifdef CFG_FUNC_MIC_PLATE_REVERB_STEP_EN
							if(EffectIndex != 0xFF)//不为0xFF,也可以直接使用index编号为判断条件
							{
								mic_platereverb_unit = pTemp;
								if((IsReload == TRUE)
								|| (gCtrlUserVars.max_plate_reverb_roomsize == 0)/*未被赋值过*/)
								{
									gCtrlUserVars.max_plate_reverb_roomsize  = pTemp->param.decay; //gCtrlVars.plate_reverb_unit.decay;
									gCtrlUserVars.max_plate_reverb_wetdrymix = pTemp->param.wetdrymix;//gCtrlVars.plate_reverb_unit.wetdrymix;
								}
							}
						#endif
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PlateReverbParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(PlateReverbParam);
					}
					break;
				case REVERB_PRO:
					{
#if CFG_AUDIO_EFFECT_REVERB_PRO_EN
					ReverbProUnit* pTemp = NULL;
					uint32_t len;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ReverbProUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc ReverbPro Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectReverbProApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ReverbProParam), IsReload);
							if(gCtrlVars.sample_rate == 32000)
							{
								len = MAX_REVERB_PRO_CONTEXT_SIZE_32000;
							}
							else if(gCtrlVars.sample_rate == 44100)
							{
								len = MAX_REVERB_PRO_CONTEXT_SIZE_44100;
							}
							else if(gCtrlVars.sample_rate == 48000)
							{
								len = MAX_REVERB_PRO_CONTEXT_SIZE_48000;
							}
							else
							{
								osPortFree(pNode->EffectUnit);
								//return FALSE;
								pNode->Enable = FALSE;
							}
							pTemp->ct = osPortMallocFromEnd(len);
							if(pTemp->ct == NULL)
							{
								osPortFree(pNode->EffectUnit);
								DBG("malloc ReverbPro Ct err\n");
								//return FALSE;
								pNode->Enable = FALSE;
							}
						#ifdef CFG_FUNC_MIC_REVERB_PRO_STEP_EN
							if((EffectIndex != 0xFF) && (pTemp->ct != NULL))//不为0xFF,也可以直接使用index编号为判断条件
							{
								mic_reverbpro_unit = pTemp;
								if((IsReload == TRUE)
								|| (gCtrlUserVars.max_reverb_pro_wet == 0)/*未被赋值过*/)
								{
									gCtrlUserVars.max_reverb_pro_wet   = pTemp->param.wet;
									gCtrlUserVars.max_reverb_pro_erwet = pTemp->param.erwet;
								}
							}
						#endif
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ReverbProParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ReverbProParam);
					}
					break;
				case VOICE_CHANGER_PRO:
					{
#if CFG_AUDIO_EFFECT_VOICE_CHANGER_PRO_EN
					VoiceChangerProUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(VoiceChangerProUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Voice Changer Pro Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVoiceChangerProApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VoiceChangerProParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VoiceChangerProParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(VoiceChangerProParam);
					}
					break;
				case PHASE_CONTROL://20
					{
#if CFG_AUDIO_EFFECT_PHASE_CONTROL_EN
					PhaseControlUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(PhaseControlUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Phase Control Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPhaseApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PhaseControlParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PhaseControlParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(PhaseControlParam);
					}
					break;
				case VOCAL_REMOVE:
					{
#if CFG_AUDIO_EFFECT_VOCAL_REMOVE_EN
					VocalRemoveUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(VocalRemoveUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVocalRemoveApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VocalRemoverParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VocalRemoverParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(VocalRemoverParam);
					}
					break;
				case PITCH_SHIFTER_PRO:
					{
#if CFG_AUDIO_EFFECT_PITCH_SHIFTER_PRO_EN
					PitchShifterProUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(PitchShifterProUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPitchShifterProApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PitchShifterProParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PitchShifterProParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(PitchShifterProParam);
					}
					break;
				case VIRTUAL_BASS_CLASSIC:
					{
#if CFG_AUDIO_EFFECT_VB_CLASS_EN
					VBClassUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(VBClassUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc VB Classic err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVBClassApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectVBClassApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VBClassicParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(VBClassicParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(VBClassicParam);
					}
					break;
				case PCM_DELAY:
					{
#if CFG_AUDIO_EFFECT_PCM_DELAY_EN
					PcmDelayUnit* pTemp = NULL;
					uint32_t buf_size;
					uint32_t max_delay_samples;
					uint32_t channel = AudioEffectComCt[effect_index].channel;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(PcmDelayUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc pcm delay err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPcmDelayApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPcmDelayApply;
							}
							
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PcmDelayParam), IsReload);
							//malloc delay buf
							max_delay_samples = pTemp->param.max_delay * gCtrlVars.sample_rate/1000;

#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								buf_size = pTemp->param.high_quality ? max_delay_samples*2*channel*2 : ceil(max_delay_samples/32)*19*channel+64;
							}
							else
#endif
							{	
								buf_size = pTemp->param.high_quality ? max_delay_samples*2*channel : ceil(max_delay_samples/32)*19*channel+64;
							}

							pTemp->s_buf = osPortMallocFromEnd(buf_size);
							if(pTemp->s_buf == NULL)
							{
								DBG("malloc err\n");
								osPortFree(pNode->EffectUnit);
								//return FALSE;
								pNode->Enable = FALSE;
							}
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PcmDelayParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(PcmDelayParam);
					}
					break;
				case EXCITER://25
					{
#if CFG_AUDIO_EFFECT_EXCITER_EN
					ExciterUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ExciterUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Exciter Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectExciterApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectExciterApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ExciterParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ExciterParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ExciterParam);
					}
					break;
				case CHORUS:
					{
#if CFG_AUDIO_EFFECT_CHORUS_EN
					ChorusUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ChorusUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Chorus Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectChorusApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ChorusParam), IsReload);
						#ifdef CFG_FUNC_MIC_CHORUS_STEP_EN
							if(EffectIndex != 0xFF)//不为0xFF,也可以直接使用index编号为判断条件
							{
								mic_chorus_unit = pTemp;
								if((IsReload == TRUE)
								|| (gCtrlUserVars.max_chorus_wet == 0)/*未被赋值过*/)
								{
									gCtrlUserVars.max_chorus_wet = pTemp->param.wet;
								}
							}
						#endif
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ChorusParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ChorusParam);
					}
					break;
				case AUTO_WAH:
					{
#if CFG_AUDIO_EFFECT_AUTOWAH_EN
					AutoWahUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(AutoWahUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc AutoWah Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectAutoWahApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(AutoWahParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(AutoWahParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(AutoWahParam);
					}
					break;
				case STEREO_WIDENER:
					{
#if CFG_AUDIO_EFFECT_STEREO_WIDEN_EN
					StereoWidenerUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(StereoWidenerUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Stereo widener Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectStereoWidenerApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectStereoWidenerApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(StereoWidenerParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(StereoWidenerParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(StereoWidenerParam);
					}
					break;
				case PINGPONG:
					{
#if CFG_AUDIO_EFFECT_PINGPONG_EN
					PingPongUnit* pTemp = NULL;
					uint32_t buf_size;
					uint32_t max_delay_samples;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(PingPongUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Pingpong Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectPingPongApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PingPongParam), IsReload);
							//malloc delay buf
							max_delay_samples = pTemp->param.max_delay * gCtrlVars.sample_rate/1000;
							buf_size = pTemp->param.high_quality_enable ? max_delay_samples*4 : ceilf(max_delay_samples/32.0f) * 38;
							pTemp->s = osPortMallocFromEnd(buf_size);
							if(pTemp->s == NULL)
							{
								DBG("malloc err\n");
								osPortFree(pNode->EffectUnit);
								//return FALSE;
								pNode->Enable = FALSE;
							}
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(PingPongParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(PingPongParam);
					}
					break;
				case THREE_D_PLUS://30
					{
#if CFG_AUDIO_EFFECT_3D_PLUS_EN
					ThreeDPlusUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(ThreeDPlusUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc 3DPlus Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectThreeDPlusApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ThreeDPlusParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(ThreeDPlusParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(ThreeDPlusParam);
					}
					break;
				case SINE_GENERATOR://不支持
					break;
				case NOISE_Suppressor_Blue:
					{
#if CFG_AUDIO_EFFECT_NS_BLUE_EN
					NSBlueUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						DBG("!!!!blue ns\n");
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(NSBlueUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc ns_blue err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectNSBlueApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(NSBlueParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(NSBlueParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(NSBlueParam);
					}
					break;
				case FLANGER:
					{
#if CFG_AUDIO_EFFECT_FLANGER_EN
					FlangerUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(FlangerUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc flanger err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectFlangerApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(FlangerParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(FlangerParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(FlangerParam);
					}
					break;
				case FREQ_SHIFTER_FINE:
					{
#if CFG_AUDIO_EFFECT_FREQ_SHIFTER_FINE_EN
					FreqShifterFineUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(FreqShifterFineUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc freq shifter fine err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectFreqShifterFineApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(FreqShifterFineParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(FreqShifterFineParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(FreqShifterFineParam);
					}
					break;
				case OVERDRIVE:
					{
#if CFG_AUDIO_EFFECT_OVERDRIVE_EN
					OverdriveUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(OverdriveUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("overdrive err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectOverdriveApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(OverdriveParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(OverdriveParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(OverdriveParam);
					}
					break;
				case DISTORTION:
					{
#if CFG_AUDIO_EFFECT_DISTORTION_EN
					DistortionUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(DistortionUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("distortion err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectDistortionApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(DistortionParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(DistortionParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(DistortionParam);
					}
					break;
				case EQ_DRC:
					{
#if CFG_AUDIO_EFFECT_EQDRC_EN
					EQDRCUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(EQDRCUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("eq drc err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectEQDRCApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectEQDRCApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(EQDRCParam), IsReload);
							//for user config
						#ifdef CFG_FUNC_MUSIC_TREB_BASS_EN
							if(EffectIndex == 0x8E)//不为0xFF,也可以直接使用index编号为判断条件
							{
								music_trebbass_eq_unit = pTemp;//Treb/bass
							}
						#endif
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(EQDRCParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(EQDRCParam);
					}
					break;
				case AEC:
					{
#if CFG_AUDIO_EFFECT_AEC_EN
						AECUnit* pTemp = NULL;
						if(pNode->Enable == TRUE)
						{
							pNode->EffectUnit = osPortMallocFromEnd(sizeof(AECUnit));
							if(pNode->EffectUnit == NULL)
							{
								DBG("malloc AEC Unit err\n");
								//return FALSE;
								pNode->Enable = FALSE;
							}
							else
							{
								pTemp = pNode->EffectUnit;
								pTemp->enable = 1;

								pNode->FuncAudioEffect = NULL;//特殊处理
								AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(AECParam), IsReload);
							}
						}
						AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(AECParam), IsReload);
#endif
						Len = Len + EFFECT_PARAM_OFFSET + sizeof(AECParam);
					}
					break;
				case DISTORTION_DS1:
					{
#if CFG_AUDIO_EFFECT_DISTORTION_DS1_EN
					DistortionDS1Unit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(DistortionDS1Unit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc DistortionDS1 Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectDistortionDS1Apply;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectDistortionDS1Apply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(DistortionDS1Param), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(DistortionDS1Param), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(DistortionDS1Param);
					}
					break;
				case OVERDRIVE_POLY:
					{
#if CFG_AUDIO_EFFECT_OVERDRIVE_POLY_EN
					OverdrivePolyUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(OverdrivePolyUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc OverdrivePoly Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectOverdrivePolyApply;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectOverdrivePolyApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(OverdrivePolyParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(OverdrivePolyParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(OverdrivePolyParam);
					}
					break;
				case COMPANDER:
					{
#if CFG_AUDIO_EFFECT_COMPANDER_EN
					CompanderUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(CompanderUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc Compadner Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectCompanderApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectCompanderApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(CompanderParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(CompanderParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(CompanderParam);
					}
					break;
				case LOW_LEVEL_COMPRESSOR:
					{
#if CFG_AUDIO_EFFECT_LOW_LEVEL_COMPRESSOR_EN
					LowLevelCompressorUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(LowLevelCompressorUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc LowLevelCompressor Uint err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;
#ifdef CFG_AUDIO_WIDTH_24BIT
							if(EffectWidth == 24)
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectLowLevelCompressorApply24;
							}
							else
#endif
							{
								pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectLowLevelCompressorApply;
							}
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(LLCompressorParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(LLCompressorParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(LLCompressorParam);
					}
					break;
				case HOWLING_FINE:
					{
#if CFG_AUDIO_EFFECT_HOWLING_SUPPRESSOR_FINE_EN
					HowlingFineUnit* pTemp = NULL;
					if(pNode->Enable == TRUE)
					{
						pNode->EffectUnit = osPortMallocFromEnd(sizeof(HowlingFineUnit));
						if(pNode->EffectUnit == NULL)
						{
							DBG("malloc howling Fine Unit err\n");
							//return FALSE;
							pNode->Enable = FALSE;
						}
						else
						{
							pTemp = pNode->EffectUnit;
							pTemp->enable = 1;

							pNode->FuncAudioEffect = (AudioEffectApplyFunc)AudioEffectHowlingSuppressorFineApply;
							AudioEffectNoteParamAssign(&pTemp->param, &add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(HowlingFineParam), IsReload);
						}
					}
					AudioEffectNoteParamBackup(&add[Len+EFFECT_PARAM_OFFSET], effect_index, sizeof(HowlingFineParam), IsReload);
#endif
					Len = Len + EFFECT_PARAM_OFFSET + sizeof(HowlingFineParam);
					}
					break;
				default:
					//DBG("other effect\n");
					//Len += 1;
					break;
				}
				pNode->EffectType = EffectType;
			}
			effect_index++;
		}
		else
		{
			switch(EffectName)
			{
			case 0x03://ADC0 PGA
				if(IsReload)
				{
					memcpy(&gCtrlVars.ADC0PGACt, &add[Len+1], sizeof(ADC0PGAContext));
				}
				Len = Len + 1 + sizeof(ADC0PGAContext);
				break;
			case 0x04://ADC0 DIGITAL
				if(IsReload)
				{
					memcpy(&gCtrlVars.ADC0DigitalCt, &add[Len+1], sizeof(ADC0DigitalContext));
				}
				Len = Len + 1 + sizeof(ADC0DigitalContext);
				break;
			case 0x06://ADC1 PGA
				if(IsReload)
				{
					memcpy(&gCtrlVars.ADC1PGACt, &add[Len+1], sizeof(ADC1PGAContext));
				}
				Len = Len + 1 + sizeof(ADC1PGAContext);
				break;
			case 0x07://ADC1 DIGITAL
				if(IsReload)
				{
					memcpy(&gCtrlVars.ADC1DigitalCt, &add[Len+1], sizeof(ADC1DigitalContext));
				}
				Len = Len + 1 + sizeof(ADC1DigitalContext);
				break;
			case 0x08://AGC1  ADC1
				if(IsReload)
				{
					memcpy(&gCtrlVars.ADC1AGCCt, &add[Len+1], sizeof(ADC1AGCContext));
				}
				Len = Len + 1 + sizeof(ADC1AGCContext);
				break;
			case 0x09://DAC0
				if(IsReload)
				{
					memcpy(&gCtrlVars.DAC0Ct, &add[Len+1], sizeof(DAC0Context));
				}
				Len = Len + 1 + sizeof(DAC0Context);
				break;
			case 0x0a://DAC1
				if(IsReload)
				{
					memcpy(&gCtrlVars.DAC1Ct, &add[Len+1], sizeof(DAC1Context));
				}
				Len = Len + 1 + sizeof(DAC1Context);
				break;
			case 0xFC://tag
				lenTag = add[Len+1];
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
				DBG("lenTag = %d\n", lenTag);
				memcpy(TagBuf, &add[Len+2], lenTag);
				//DBG("%s\n", TagBuf);
#endif
				Len = Len + lenTag + 1 + 1;

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
				{
					char* str = NULL;//"zhang";
					uint8_t Index = FineAudioEffectParamasIndex(mainAppCt.EffectMode);
#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
					if(Index < 10)//form flash
					{
						str = (char*)EFFECT_TAB_FLASH[Index].EffectNameStr;
					}
					else
#endif
					{
						str = (char*)EFFECT_TAB[Index - EFFECT_MODE_NORMAL].EffectNameStr;
					}
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
					//DBG("str = %d\n", strlen(str));
					memcpy(TagBuf, str, strlen(str));
#endif
					lenTag = strlen(str);
				}
#endif
				break;
			default:
				Len += 1;
				break;
			}
		}
	}
	return TRUE;
}

__attribute__((optimize("Og")))
void AudioEffectsLoadInit(bool IsReload, uint8_t mode)
{
	uint8_t Index;

	AudioEffectsDeInit();//导入音效前增加一个音效释放，防止用户不规范操作导致音效切换异常

#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexLock(AudioEffectMutex);
	}
#endif
	//查找音效组合是否有效
	Index = FineAudioEffectParamasIndex(mode);

	//更新音效初值
	if(Index == 0xFF)
	{
		DBG("audio effect list in invalid\n");
	#ifdef FUNC_OS_EN
		if(AudioEffectMutex != NULL)
		{
			osMutexUnlock(AudioEffectMutex);
		}
	#endif
		return;//
	}
	else
	{
#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
		uint8_t * pParamBuf = NULL;

		pParamBuf = (uint8_t *)osPortMalloc(EffectParamCt.Len);//test
		if(Index < 10)//form flash
		{
			if(SpiFlashRead(get_effect_data_addr()+EffectParamCt.Offset, pParamBuf, EffectParamCt.Len, 1) == FLASH_NONE_ERR)
			{
				//DBG("Index = %d, %s\n", Index, (uint8_t*)EFFECT_TAB_FLASH[Index].CommParam);
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
				AudioEffectParamCtAddr = (EffectComCt*)EFFECT_TAB_FLASH[Index].CommParam;
#endif
				if(!AudioEffectParsePackage(pParamBuf, EffectParamCt.Len, (uint8_t*)EFFECT_TAB_FLASH[Index].CommParam, IsReload))
				{
					APP_DBG("Effect param err\n");
				}
			}
			else
			{
				DBG("spi flash read err\n");
				return;//
			}

		}
		else
#endif
		{//hard code
#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			AudioEffectParamCtAddr = (EffectComCt*)EFFECT_TAB[Index-EFFECT_MODE_NORMAL].CommParam;
#endif
			if(!AudioEffectParsePackage((uint8_t*)EFFECT_TAB[Index-EFFECT_MODE_NORMAL].EffectParamas, EFFECT_TAB[Index-EFFECT_MODE_NORMAL].len, (uint8_t*)EFFECT_TAB[Index-EFFECT_MODE_NORMAL].CommParam, IsReload))
			{
				APP_DBG("Effect param err\n");
			}
		}
#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
		if(pParamBuf != NULL)
		{
			osPortFree(pParamBuf);
		}
#endif
	}

	AudioEffectsInit();
#ifdef FUNC_OS_EN
	if(AudioEffectMutex != NULL)
	{
		osMutexUnlock(AudioEffectMutex);
	}
#endif

	AudioCodecGainUpdata();//更新硬件的增益控制
	void AudioEffectParamSync(void);
	AudioEffectParamSync();
	DBG("effect init ok\n");
}
#endif

void du_efft_fadein_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch)
{
	int16_t* temp = (int16_t *)pcm_in;
	int32_t n = 0, w = 0, step = 32768/pcm_length;

	if(ch == 2)
	{
		for(n = 0;	n < pcm_length; n++)
		{
			temp[n * 2] = (int16_t)(((int32_t)temp[n * 2] * w + 16384) >> 15);
			temp[n * 2 + 1] = (int16_t)(((int32_t)temp[n * 2 + 1] * w + 16384) >> 15);
			w += step;
		}
	}
	else if(ch == 1)
	{
		for(n = 0;	n < pcm_length; n++)
		{
			temp[n] = (int16_t)(((int32_t)temp[n] * w + 16384) >> 15);
			w += step;
		}
	}
}

void du_efft_fadeout_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch)
{
	int16_t* temp = (int16_t *)pcm_in;
	int32_t n = 0, w = (32768/pcm_length)*(pcm_length-1), step = 32768/pcm_length;

	if(ch == 2)
	{
		for(n = 0; n < pcm_length; n++)
		{
			temp[n * 2] = (int16_t)(((int32_t)temp[n * 2] * w + 16384) >> 15);
			temp[n * 2 + 1] = (int16_t)(((int32_t)temp[n * 2 + 1] * w + 16384) >> 15);
			w -= step;
		}
	}
	else if(ch == 1)
	{
		for(n = 0; n < pcm_length; n++)
		{
			temp[n] = (int16_t)(((int32_t)temp[n] * w + 16384) >> 15);
			w -= step;
		}
	}
}

uint8_t FineAudioEffectParamasIndex(uint8_t mode)
{
	uint8_t i = 0;

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
	//int32_t effecthead = 0;
	int32_t effectDataFlashCnt = 0;
	int32_t effectDataMaxLen = 0;
#endif

	APP_DBG("FUNC_ID_EFFECT_MODE -> %d\n", mode);
	AudioModeIndex = 0xFF;//如果未找到合适的音效，则单前模式无音效

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
	if(mode < 10)//从下载的10组参数对应的flash存储空间中获取
	{
		DBG("flash effect = %lx\n", get_effect_data_addr());

		//判断地址有效，
		//更具传递Index 读取头部信息，做一个数据合法性检查

//		if(SpiFlashRead(get_effect_data_addr(),&effecthead,4,1) == FLASH_NONE_ERR)
//		{
//			DBG("effecthead = %lx\n", effecthead);
//		}
		if(SpiFlashRead(get_effect_data_addr()+4,(uint8_t*)&effectDataFlashCnt,4,1) == FLASH_NONE_ERR)
		{
			DBG("effectDataFlashCnt = %lx\n", effectDataFlashCnt);
		}
		if(SpiFlashRead(get_effect_data_addr()+4+4,(uint8_t*)&effectDataMaxLen,4,1) == FLASH_NONE_ERR)
		{
			DBG("effectDataMaxLen = %lx\n", effectDataMaxLen);
		}
		if(mode >= effectDataFlashCnt)
		{
			DBG("effect mode error\n");
			return 0xFF;
		}
		if(SpiFlashRead(get_effect_data_addr()+sizeof(EffectHeadPaCt)+mode*sizeof(EffectPaCt),(uint8_t*)&EffectParamCt,sizeof(EffectPaCt),1) == FLASH_NONE_ERR)
		{
			DBG("EffectParamCt.Flag = %d\n", EffectParamCt.Flag);
			DBG("EffectParamCt.Offset = %d\n", EffectParamCt.Offset);
			DBG("EffectParamCt.Len = %d\n", EffectParamCt.Len);
		}
		AudioModeIndex = mode;
	}
	else//从调音数组中获取（由调音工具导出,hard code）
	{
		APP_DBG("Get Audio Effect Paramas From Data.c!!!!!!\n");

		for(i=0; i<EFFECT_MODE_NUM_ACTIVCE; i++)
		{
			if(EFFECT_TAB[i].eff_mode == mode)
			{
				AudioModeIndex = EFFECT_MODE_NORMAL + i;//mode;
				APP_DBG("number:%d , eff_mode:%d , len:%d\n",i,EFFECT_TAB[i].eff_mode,EFFECT_TAB[i].len);
	            break;
			}
		}
	}
#else
	{
		APP_DBG("Get Audio Effect Paramas From Data.c!!!!!!\n");

		for(i=0; i<EFFECT_MODE_NUM_ACTIVCE; i++)
		{
			if(EFFECT_TAB[i].eff_mode == mode)
			{
				AudioModeIndex = EFFECT_MODE_NORMAL + i;//mode;
				APP_DBG("number:%d , eff_mode:%d , len:%d\n",i,EFFECT_TAB[i].eff_mode,EFFECT_TAB[i].len);
	            break;
			}
		}
	}
#endif

	return AudioModeIndex;
}

uint8_t GetAudioEffectParamasIndex(void)
{
	return AudioModeIndex;
}
