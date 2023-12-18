/**
 **************************************************************************************
 * @file    audio_vol.h
 * @brief   audio syetem vol set here
 *
 * @author  Sam
 * @version V1.0.0
 *
 * $Created: 2016-1-7 15:42:47$
 *
 * @copyright Shanghai Mountain View Silicon Technology Co.,Ltd. All rights reserved.
 **************************************************************************************
 */
#ifndef __AUDIO_VOL_H__
#define __AUDIO_VOL_H__

#ifdef __cplusplus
extern "C" {
#endif//__cplusplus

#include "app_config.h"

extern const uint16_t gSysVolArr[];
void AudioMusicVol(uint8_t musicVol);
bool IsAudioPlayerMute(void);
void AudioPlayerMenu(void);
void AudioPlayerMenuCheck(void);
void AudioMusicVolSet(uint8_t musicVol);
void AudioHfVolSet(uint8_t HfVol);

uint8_t AudioMusicVolGet(void);
void AudioMusicVolDown(void);
void AudioMusicVolUp(void);
void SystemVolUp(void);
void SystemVolDown(void);
void SystemVolSet(void);
void SystemVolSetChannel(int8_t SetChannel, uint8_t volume);
void EqModeSet(uint8_t EqMode);
void AudioEffectParamSync(void);
void CommonMsgProccess(uint16_t Msg);
void SetRecMusic(uint8_t if_para_use);

uint8_t BtAbsVolume2VolLevel(uint8_t absValue);
uint8_t BtLocalVolLevel2AbsVolme(uint8_t localValue);
void AudioMicVolDown(void);
void AudioMicVolUp(void);
void HardWareMuteOrUnMute(void);

#ifdef  CFG_APP_HDMIIN_MODE_EN
void HDMISourceMute(void);
void HDMISourceUnmute(void);
#endif

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif

