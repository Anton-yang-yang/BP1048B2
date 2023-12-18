#include <string.h>
#include "type.h"
#include "comm_param.h"
#include "spi_flash.h"
#include "audio_effect.h"
#include "audio_effect_api.h"
#include "audio_effect_flash_param.h"
#include "ctrlvars.h"
#include "flash_table.h"
#include "main_task.h"
#include "debug.h"

#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN

extern MainAppContext	mainAppCt;
EffectPaCt EffectParamCt;

#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
//uint8_t EffectParamFlashIndex = 0xFF;
uint8_t EffectParamFlashUpdataFlag = 0;
uint8_t EffectParamFlahBuf[1024*4] ={0};//定义4K buf，读取存储参数
#endif

#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
extern uint8_t  effect_sum;
extern uint16_t effect_list[AUDIO_EFFECT_SUM];
extern uint16_t effect_list_addr[AUDIO_EFFECT_SUM];
extern void* effect_list_param[AUDIO_EFFECT_SUM];
#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH
extern uint8_t effect_enable_list[AUDIO_EFFECT_SUM];
#endif
#endif

#ifdef CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH

const uint16_t EffectTypeParamCtLen[] =
{
	sizeof(AutoTuneParam),//AUTO_TUNE
	0,//DC_BLOCK,//未使用
	sizeof(DRCParam),//DRC,
	sizeof(EchoParam),//ECHO,
	sizeof(EQParam),//EQ,
	sizeof(ExpanderParam),//EXPANDER,//5,Noise Suppressor
	sizeof(FreqShifterParam),//FREQ_SHIFTER,
	sizeof(HowlingParam),//HOWLING_SUPPRESSOR,
	0,//NOISE_GATE,//未使用
	sizeof(PitchShifterParam),//PITCH_SHIFTER,
	sizeof(ReverbParam),//REVERB,//10
	sizeof(SilenceDetectorParam),//SILENCE_DETECTOR,
	sizeof(ThreeDParam),//THREE_D,
	sizeof(VBParam),//VIRTUAL_BASS,
	sizeof(VoiceChangerParam),//VOICE_CHANGER,
	sizeof(GainControlParam),//GAIN_CONTROL,//15
	sizeof(VocalCutParam),//VOCAL_CUT,
	sizeof(PlateReverbParam),//PLATE_REVERB,
	sizeof(ReverbProParam),//REVERB_PRO,
	sizeof(VoiceChangerProParam),//VOICE_CHANGER_PRO,
	sizeof(PhaseControlParam),//PHASE_CONTROL,//20
	sizeof(VocalRemoverParam),//VOCAL_REMOVE,
	sizeof(PitchShifterProParam),//PITCH_SHIFTER_PRO,
	sizeof(VBClassicParam),//VIRTUAL_BASS_CLASSIC,
	sizeof(PcmDelayParam),//PCM_DELAY,
	sizeof(ExciterParam),//EXCITER,//25
	sizeof(ChorusParam),//CHORUS,
	sizeof(AutoWahParam),//AUTO_WAH,
	sizeof(StereoWidenerParam),//STEREO_WIDENER,
	sizeof(PingPongParam),//PINGPONG,
	sizeof(ThreeDPlusParam),//THREE_D_PLUS,//30
	0,//SINE_GENERATOR,//未使用
	0,//NOISE_Suppressor_Blue,//未使用
	sizeof(FlangerParam),//FLANGER
	sizeof(FreqShifterFineParam),//FREQ_SHIFTER_FINE
	sizeof(OverdriveParam),//OVERDRIVE,//35
	sizeof(DistortionParam),//DISTORTION
	sizeof(EQDRCParam),//EQ_DRC
	//AEC = 100,////AEC,用户自定义音效，固定为100
	//注意，用户自定义音效，参数往后增加
};

const uint16_t EffectTypeParam100CtLen[] =
{
	sizeof(AECParam),//AEC = 100,////AEC,用户自定义音效，固定为100
	//注意，用户自定义音效，参数往后增加
};

void EffectParamFlashUpdata(void)
{
	int32_t effectDataFlashCnt = 0;
	int32_t effectDataMaxLen = 0;
	int32_t effectParamStartAdd = 0;
	int32_t effectParamEndAdd = 0;
	int32_t FlashAdd = 0;
	bool IsFlashSpansSectors = 0;

	uint8_t * pParamBuf = NULL;
	uint8_t Index = FineAudioEffectParamasIndex(mainAppCt.EffectMode);

	if(SpiFlashRead(get_effect_data_addr()+4,(uint8_t*)&effectDataFlashCnt,4,1) == FLASH_NONE_ERR)
	{
		DBG("effectDataFlashCnt = %lx\n", effectDataFlashCnt);
	}
	if(SpiFlashRead(get_effect_data_addr()+4+4,(uint8_t*)&effectDataMaxLen,4,1) == FLASH_NONE_ERR)
	{
		DBG("effectDataMaxLen = %lx\n", effectDataMaxLen);
	}

	if(Index >= effectDataFlashCnt)
	{
		DBG("effect index error\n");
		return;
	}

	if(SpiFlashRead(get_effect_data_addr()+sizeof(EffectHeadPaCt)+Index*sizeof(EffectPaCt),(uint8_t*)&EffectParamCt,sizeof(EffectPaCt),1) != FLASH_NONE_ERR)
	{
		DBG("flash read err\n");
		return;
	}
	DBG("EffectParamCt.Flag = %d\n", EffectParamCt.Flag);
	DBG("EffectParamCt.Offset = %d\n", EffectParamCt.Offset);
	DBG("EffectParamCt.Len = %d\n", EffectParamCt.Len);

	effectParamStartAdd = get_effect_data_addr() + EffectParamCt.Offset;
	effectParamEndAdd = get_effect_data_addr() + EffectParamCt.Offset + EffectParamCt.Len - 1;

	DBG("effectParamStartAdd = %lx\n", effectParamStartAdd);
	DBG("effectParamEndAdd = %lx\n", effectParamEndAdd);

	if(effectParamStartAdd / 4096 != effectParamEndAdd / 4096)
	{
		IsFlashSpansSectors = 1;
	}
	DBG("IsFlashSpansSectors = %d\n", IsFlashSpansSectors);

	pParamBuf = (uint8_t *)osPortMalloc(EffectParamCt.Len);//申请有效长度

	//组装数据
	{
		uint32_t i;
		uint8_t *pPoint = pParamBuf;
		//DBG("start add = %lx\n", pPoint);
		*pPoint = 0x03;
		pPoint++;
		memcpy(pPoint, (uint8_t*)&gCtrlVars.ADC0PGACt, sizeof(ADC0PGAContext));
		pPoint += sizeof(ADC0PGAContext);

		*pPoint = 0x04;
		pPoint++;
		memcpy(pPoint, (uint8_t*)&gCtrlVars.ADC0DigitalCt, sizeof(ADC0DigitalContext));
		pPoint += sizeof(ADC0DigitalContext);

		*pPoint = 0x06;
		pPoint++;
		memcpy(pPoint, &gCtrlVars.ADC1PGACt, sizeof(ADC1PGAContext));
		pPoint += sizeof(ADC1PGAContext);

		*pPoint = 0x07;
		pPoint++;
		memcpy(pPoint, &gCtrlVars.ADC1DigitalCt, sizeof(ADC1DigitalContext));
		pPoint += sizeof(ADC1DigitalContext);

		*pPoint = 0x08;
		pPoint++;
		memcpy(pPoint, &gCtrlVars.ADC1AGCCt, sizeof(ADC1AGCContext));
		pPoint += sizeof(ADC1AGCContext);

		*pPoint = 0x09;
		pPoint++;
		memcpy(pPoint, &gCtrlVars.DAC0Ct, sizeof(DAC0Context));
		pPoint += sizeof(DAC0Context);

		*pPoint = 0x0a;
		pPoint++;
		memcpy(pPoint, &gCtrlVars.DAC1Ct, sizeof(DAC1Context));
		pPoint += sizeof(DAC1Context);

		*pPoint = 0xFC;
		pPoint++;
		*pPoint = 0x05;
		pPoint++;
		*pPoint = 0xFF;
		pPoint++;
		*pPoint = 0x01;
		pPoint++;
		*pPoint = 0x02;
		pPoint++;
		*pPoint = 0x03;
		pPoint++;
		*pPoint = 0x04;
		pPoint++;

		for(i=0; i<effect_sum; i++)
		{
			*pPoint = effect_list_addr[i];
			pPoint++;
			*pPoint = effect_enable_list[i];
			pPoint++;

			//DBG("sizeof(effect_list[%d]) = %d, %d\n", i, effect_list_param[i], EffectTypeParamCtLen[effect_list[i]]);
			//DBG("%d\n", effect_list[i]);
			if(effect_list[i] < 100)
			{
				memcpy(pPoint, effect_list_param[i], EffectTypeParamCtLen[effect_list[i]]);
				pPoint += EffectTypeParamCtLen[effect_list[i]];
			}
			else
			{
				memcpy(pPoint, effect_list_param[i], EffectTypeParam100CtLen[effect_list[i] - AEC]);//100以后为自定义音效，和前面的音效定义编号不连续
				pPoint += EffectTypeParam100CtLen[effect_list[i] - AEC];
			}
		}
		//DBG("end add = %lx\n", pPoint);
	}
	//debug
//	{
//		uint32_t i;
//		uint8_t *pPoint = pParamBuf;
//		DBG("\n");
//		for(i=0; i<EffectParamCt.Len; i++)
//		{
//			if(i%16 == 0)
//			{
//				DBG("\n");
//			}
//			DBG("0x%02x,",*pPoint);
//			pPoint++;
//		}
//		DBG("\n");
//	}

	FlashAdd = effectParamStartAdd/4096*4096;
	DBG("FlashAdd = %ld\n", FlashAdd);
	if(SpiFlashRead(FlashAdd, EffectParamFlahBuf,4096,1) != FLASH_NONE_ERR)
	{
		DBG("flash read err\n");
		return;
	}

	if(IsFlashSpansSectors == 0)
	{
		SPI_FLASH_ERR_CODE ret=0;
		uint32_t Add = effectParamStartAdd % 4096;
		SpiFlashErase(SECTOR_ERASE, FlashAdd /4096 , 1);
		//写数据
		memcpy(&EffectParamFlahBuf[Add], pParamBuf, EffectParamCt.Len);
		ret = SpiFlashWrite(FlashAdd, EffectParamFlahBuf, 4096, 1);
		if(ret != FLASH_NONE_ERR)
		{
			APP_DBG("write error:%d\n", ret);
			if(pParamBuf != NULL)
			{
				osPortFree(pParamBuf);
			}
			return;
		}
	}
	else
	{
		uint8_t * pParamAddr = pParamBuf;
		SPI_FLASH_ERR_CODE ret=0;
		uint32_t Add = effectParamStartAdd % 4096;
		uint32_t Len = 4096 - (effectParamStartAdd % 4096);

		SpiFlashErase(SECTOR_ERASE, FlashAdd /4096 , 1);
		//写数据
		memcpy(&EffectParamFlahBuf[Add], pParamAddr, Len);
		ret = SpiFlashWrite(FlashAdd, EffectParamFlahBuf, 4096, 1);
		if(ret != FLASH_NONE_ERR)
		{
			APP_DBG("write error:%d\n", ret);
			if(pParamBuf != NULL)
			{
				osPortFree(pParamBuf);
			}
			return;
		}
		//读下一个扇区
		FlashAdd += 4096;//effectParamStartAdd/4096*4096;
		if(SpiFlashRead(FlashAdd, EffectParamFlahBuf,4096,1) != FLASH_NONE_ERR)
		{
			DBG("flash read err\n");
			return;
		}
		SpiFlashErase(SECTOR_ERASE, FlashAdd /4096 , 1);
		//写数据
		pParamAddr += Len;
		memcpy(&EffectParamFlahBuf[0], pParamAddr, EffectParamCt.Len - Len);
		ret = SpiFlashWrite(FlashAdd, EffectParamFlahBuf, 4096, 1);
		if(ret != FLASH_NONE_ERR)
		{
			APP_DBG("write error:%d\n", ret);
			if(pParamBuf != NULL)
			{
				osPortFree(pParamBuf);
			}
			return;
		}
	}

	if(pParamBuf != NULL)
	{
		osPortFree(pParamBuf);
	}
}
#endif

#endif

