/**
 **************************************************************************************
 * @file    media_audio_play.h
 * @brief   uDisk and TF Card 
 *
 * @author  kk
 * @version V1.0.0
 *
 * $Created: 2017-3-17 13:06:47$
 *
 * @Copyright (C) 2016, Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __MEDIA_AUDIO_PLAY_H__
#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define __MEDIA_AUDIO_PLAY_H__
#define MEDIA_PLAY_DECODER_SOURCE_NUM			APP_SOURCE_NUM
bool MediaPlayInit(void);
void MediaPlayRun(uint16_t msgId);
bool MediaPlayDeinit(void);
#ifdef __cplusplus
}
#endif // __cplusplus

#endif /*__MEDIA_AUDIO_PLAY_H__*/
