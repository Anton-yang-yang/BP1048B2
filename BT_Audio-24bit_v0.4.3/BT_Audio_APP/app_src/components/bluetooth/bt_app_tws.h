/**
 *******************************************************************************
 * @file    bluetooth_tws.h
 * @author  Owen
 * @version V1.0.0
 * @date    19-Sep-2019
 * @brief   tws callback events and actions
 *******************************************************************************
 * @attention
 *
 * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
 * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
 * TIME. AS A RESULT, MVSILICON SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
 * INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
 * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
 * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 *
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */

/**
* @addtogroup Bluetooth
* @{
* @defgroup bt_a2dp_api bt_a2dp_api.h
* @{
*/

#ifndef __BLUETOOTH_TWS_H__
#define __BLUETOOTH_TWS_H__

#include "type.h"
#include "app_config.h"
#include "bt_tws_api.h"

typedef enum
{
	BT_TWS_PLAY_REINIT,
	BT_TWS_PLAY_START = 0x55,
	BT_TWS_PLAY_IDLE = 0xFFFFFFFF,
} BT_TWS_PLAY;
#ifdef TWS_CODE_BACKUP
extern uint32_t g_tws_need_init;
#endif
extern BT_TWS_PLAY tws_play;


/***Tws 缓冲帧数预设  应主从 传输波动***/
#define	BT_TWS_DELAY_DEINIT  	20
#define BT_TWS_DELAY_48			48//set by tony
#define	BT_TWS_DELAY_DEFAULT 	96
#define BT_TWS_DELAY_BTMODE		112
extern uint32_t tws_delay;

void tws_vol_send(uint16_t Vol,bool MuteFlag);
void tws_eq_mode_send(uint16_t eq_mode);
void tws_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol);
void tws_master_hfp_send(void);
void tws_master_a2dp_send(void);

void tws_master_mute_send(bool MuteFlag);
void tws_master_remindclear_send(bool isclear);

void tws_eq_mode_send(uint16_t eq_mode);

void tws_master_eq_mode_send(uint16_t eq_mode);

void tws_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol);

void tws_master_music_bass_treb_send(uint16_t bass_vol,uint16_t treb_vol);

void tws_peer_repair(void);

void BtTws_Master_Connected(BT_TWS_CALLBACK_PARAMS * param);

void BtTws_Master_Disconnected(BT_TWS_CALLBACK_PARAMS * param);

void BtTwsRecvCmdData(BT_TWS_CALLBACK_PARAMS * param);

void BtTws_Slave_Connected(BT_TWS_CALLBACK_PARAMS * param);

void BtTws_Slave_Disconnected(BT_TWS_CALLBACK_PARAMS * param);

uint32_t tws_audio_adjust(uint32_t m_len,uint32_t s_len);

void tws_send_key_msg(uint16_t key_data);
#ifdef BT_TWS_SUPPORT
extern TIMER TwsDacOnTimer;
#endif
#endif /*__BT_TWS_API_H__*/

/**
 * @}
 * @}
 */

