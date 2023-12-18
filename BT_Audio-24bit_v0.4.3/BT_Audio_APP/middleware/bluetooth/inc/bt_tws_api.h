/**
 *******************************************************************************
 * @file    bt_tws_api.h
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

#include "type.h"

#ifndef __BT_TWS_API_H__
#define __BT_TWS_API_H__
#ifdef CFG_APP_CONFIG
#include "app_config.h"
typedef unsigned char  U8;
typedef	int BOOL;
#else
#include "xatypes.h"
#endif

//#define TWS_CODE_BACKUP //原有tws audio同步状态迁移代码屏蔽，此宏打开后恢复执行。
#define TWS_DBG(format, ...)		printf(format, ##__VA_ARGS__)

typedef enum{
	BT_STACK_EVENT_TWS_NONE = 0,

	BT_STACK_EVENT_TWS_CONNECTED,

	BT_STACK_EVENT_TWS_DISCONNECT,

	BT_STACK_EVENT_TWS_DATA_IND,

	//master event
	BT_STACK_EVENT_TWS_MASTER_CONNECTED,
	
	BT_STACK_EVENT_TWS_MASTER_DISCONNECT,
		
	BT_STACK_EVENT_TWS_MASTER_DATA_IND,

	BT_STACK_EVENT_TWS_ACL_DATA_LINK,
	
	BT_STACK_EVENT_TWS_ACL_DATA_UNLINK,
	
	//slave event
	BT_STACK_EVENT_TWS_SLAVE_CONNECTED,
	
	BT_STACK_EVENT_TWS_SLAVE_DISCONNECT,
		
	BT_STACK_EVENT_TWS_SLAVE_DATA_IND,

	BT_STACK_EVENT_TWS_SLAVE_STREAM_START,

	BT_STACK_EVENT_TWS_SLAVE_STREAM_STOP,

	BT_STACK_EVENT_TWS_MUTE,
	
	//当前HOST链路异常,延迟重新组网
	BT_STACK_EVENT_TWS_CONNECT_DELAY,
	//当前TWS链路存在,取消组网流程
	BT_STACK_EVENT_TWS_CONNECT_LINK_EXIST,
	
}BT_TWS_CALLBACK_EVENT;

typedef enum
{
	BT_TWS_MASTER = 0x00,
	BT_TWS_SLAVE,
	BT_TWS_UNKNOW = 0xff,
} BT_TWS_ROLE;

#ifndef TWS_PAIR_STATE
typedef enum
{
	TWS_PAIR_NONE = 0,
	TWS_PAIR_STOP,
	TWS_PAIR_START,			//延时2s再INQUIRY
	TWS_PAIR_WAITING,		//slave等待配对
	TWS_PAIR_INQUIRY,		//
	TWS_PAIR_PAIRING,
	TWS_PAIR_PAIRED,
	TWS_PAIR_STOPPING,

	//unused
	TWS_PAIR_BLE_START,
	TWS_PAIR_BLE_CONNECTING,
	TWS_PAIR_BLE_CONNECTED,
	TWS_PAIR_BLE_STOPPING,
} TWS_PAIR_STATE;
#endif

typedef struct _BT_TWS_CALLBACK_PARAMS
{
	BT_TWS_ROLE					role;
	uint16_t 					paramsLen;
	bool						status;
	uint16_t		 			errorCode;

	union
	{
		uint8_t					*bd_addr;
		uint8_t					*twsData;
	}params;
}BT_TWS_CALLBACK_PARAMS;

typedef void (*BTTwsCallbackFunc)(BT_TWS_CALLBACK_EVENT event, BT_TWS_CALLBACK_PARAMS * param);

void BtTwsCallback(BT_TWS_CALLBACK_EVENT event, BT_TWS_CALLBACK_PARAMS * param);

typedef struct _twsAppFeatures
{
	uint32_t			twsSimplePairingCfg;	//0=both sides;  1=one side;
	uint32_t			twsRoleCfg;		//0=random; 1=master; 2=slave
	BTTwsCallbackFunc	twsAppCallback;
}TwsAppFeatures;

//for bt tws link
typedef enum
{
	BT_TWS_STATE_NONE,
	BT_TWS_STATE_DISCONNECT,
	BT_TWS_STATE_CONNECTING,
	BT_TWS_STATE_WAITING_RS,//连接成功后,等待role switch
	BT_TWS_STATE_WAITING_CONFIRM,
	BT_TWS_STATE_SNIFF,//待核实
	BT_TWS_STATE_CONNECTED,
} BT_TWS_STATE;

//for tws lmp cmd 最大32个cmd
typedef enum{
	TWS_CMD_CONNECT,		//0
	TWS_CMD_DEVICE_OPEN,	//1
	TWS_CMD_HW_INIT,		//2
	TWS_CMD_BASE,			//3
	TWS_CMD_GETBASE,		//4
	TWS_CMD_BASE_LEN,		//5
	TWS_CMD_COMPENSATION,	//6
	TWS_CMD_SYNCED,			//7
	TWS_CMD_POWERDOWN,		//8
	TWS_CMD_ACTIVE,			//9
	TWS_CMD_TWS_SYNC_STATE,	//10

	TWS_CMD_LOCAL_MASTER,	//11
	TWS_CMD_LOCAL_SLAVE,	//12
	TWS_CMD_LOCAL_SYNC,		//13
	TWS_CMD_LOCAL_STOP,		//14
}TWS_AUDIO_CMD;

#define IS_CACHE(State) ((State >= TWS_HW_INIT) && (State <= TWS_IDLE))
//for tws audio link
typedef enum{
	TWS_DISCONNECT,
	TWS_CONNECT_JITTER,
	TWS_CONNECT,
/*********tws Audio fifo.data.net switch*******************/
	TWS_DISCONNECT_JITTER,
	TWS_HW_INIT,
	TWS_BASE_POINT,
	TWS_COMPENSATION,
	TWS_AUDIO,

	TWS_WAIT_SLAVE,
	TWS_IDLE,
	TWS_RE_INIT_CMD,
//	TWS_RE_INIT_CMD_DONE,
//	TWS_STOP_TRANSFER,
//	TWS_SLAVE_SNED_READY,
//	TWS_MASTER_SNED_READY,
//	TWS_MASTER_ACTIVE,
	TWS_WAIT_TRANSFER,
	TWS_CONNECT_CONFIRM,
	TWS_ACTIVE_DISCONNECTION,

	TWS_BASE_RESYNC_CMD,//命令,交互成功后切状态
}TWS_SYNC_STATE;

//audio_mode
#define TWS_M_LR__S_LR		0//主机:立体声,  从机:立体声
#define TWS_M_L__S_R		1//主机:L声道,  	  从机:R声道
#define TWS_M_R__S_L		2//主机:R声道,   从机:L声道
#define TWS_M_LR__S_MONO	3//主机:立体声,  从机:L+R混合

//TWS组网角色
typedef enum
{
	TWS_ROLE_RANDOM = 0,
	TWS_ROLE_MASTER,
	TWS_ROLE_SLAVE,
}TWS_PAIRING_ROLE;

#define TWS_SOURCE_FRAME0		(0x1000)
#define	TWS_ENCODE_FRAME0		(TWS_SOURCE_FRAME0 + 112)//编码起始帧号


/***************************************************************************
 *bt tws link state
 ****************************************************************************/
void tws_link_state_set(BT_TWS_STATE state);

BT_TWS_STATE tws_link_state_get(void);

void tws_active_disconnection(void);

void tws_active_disconnection_set(uint8_t val);

uint8_t tws_active_disconnection_get(void);

//主从之间由于交互异常而断开,需要上层在断开后进行回连操作
U8 tws_reconnect_flag_get(void);

//tws当前的角色,用于tws_sync中mv_lmp_send交互使用的role
uint8_t tws_cur_role_get(void);

/***************************************************************************
 * tws内存管理
 ****************************************************************************/
uint32_t tws_mem_size(uint32_t delay,uint8_t audio_mode);
uint32_t tws_mem_set(uint8_t*mem);

//bt task
/***************************************************************************
 * tws initializer
 ****************************************************************************/
BOOL twsInit(TwsAppFeatures *params);

//bt task
/***************************************************************************
 * tws 组网流程处理
 ****************************************************************************/
//peer-master/peer-slave/radom
void tws_start_pairing_radom(void);

void tws_stop_pairing_radom(void);

void tws_start_pairing(TWS_PAIRING_ROLE role);

/***************************************************************************
 * tws start simple pairing
 * role: slave
 ****************************************************************************/
//soundbar slave
void tws_slave_start_simple_pairing(void);

void tws_slave_stop_simple_pairing(void);

void tws_slave_simple_pairing_ready(void);

void tws_slave_simple_pairing_end(void);

/***************************************************************************
 * tws connect函数入口
 ****************************************************************************/
void tws_connect(U8 *addr);

/***************************************************************************
 * tws link disconnect
 ****************************************************************************/
void tws_link_disconnect(void);

uint8_t *Get_Peer_Addr(void);

//TWS命令收发接口,按照以上内容格式发送
void tws_master_send(uint8_t *buf,uint16_t len);

void tws_slave_send(uint8_t *buf,uint16_t len);

/***************************************************************************
 * tws avrcp处理 -- L2CAP
 ****************************************************************************/
void tws_slave_send_cmd_play_pause(void);

void tws_slave_send_cmd_next(void);

void tws_slave_send_cmd_prev(void);

void tws_slave_send_cmd_sniff(void);

/***************************************************************************
 * tws 连接成功内部状态同步初始化
 ****************************************************************************/
void tws_audio_init_sync(void);

/***************************************************************************
 * tws 命令交互 -- LMP
 ****************************************************************************/
void tws_connect_confirm_cmd(void);

void tws_sync_powerdown_cmd(void);

//master 调用
void tws_stop_transfer(void);

void tws_wait_transfer(void);

void tws_slave_send_ready(void);

void tws_master_send_ready(void);

void tws_master_active_cmd(void);

/*
 * func:CMD for tws reinit
 */
void tws_sync_reinit(void);


void tws_audio_connect_cmd(void);


/***************************************************************************
 * func:tws play set device, for interrupt
 ****************************************************************************/
void tws_device_sync_isr(uint8_t cmd);

void tws_audio_bt_state(BT_TWS_STATE event);

/*
 * func: get TWS audio link sync state
 * return :state
 */
uint8_t tws_audio_state_get(void);

/***************************************************************************
 * tws 内部状态循环处理函数
 ****************************************************************************/
void tws_audio_state_sync(void);

/*
 * p_m主机音频数据
 * len 音频数据的sample数
 * data_source 音频数据的通路 app或者提示音
 */
void tws_music_pcm_process(int16_t *p_m, uint32_t len, uint8_t data_source);

/*
 * func:获取TWS fifo 延时帧数设定 单位:128点
 * return :TWS fifo 延时帧数设定值
 */
uint32_t tws_get_delay(void);

/*
 * func:TWS fifo 延时帧数和开播扩展延时帧数  单位:128点
 * cmd命令同步后主从开始准备audio数据
 * delay:扩展延时解mute帧数  单位:128点
 * start_paly:TWS fifo 开播扩展延时帧数
 */
void tws_start(uint32_t delay,uint32_t start_paly);


/*
 * func:从TWS fifo数据帧抽取采样点
 * Buf: 数据buf
 * Samples:采样点数 128倍数
 * return :实际采样点数
 */
uint16_t TWSDataGet(void* Buf, uint16_t Samples);

/*
 * func:从TWS fifo数据采样点数
 * return :TWS fifo采样点数
 */
uint16_t TWSDataLenGet(void);


/*******************************************************************************
 **************************Audio Device Sync adjust********************************
 *********************************************************************************/
/*
 * func:TWS 主从设备外放fifo帧计数
 * Len: = 128*n 采样点数
 * return: 实际set采样点数
 */
uint16_t TwsDeviceSinkSync(uint16_t Len);

//App:
BT_TWS_ROLE tws_get_role(void);
/**************************************************************************
 *  tws audio
 *************************************************************************/
bool tws_state_audiocore_assort(TWS_SYNC_STATE State, TWS_SYNC_STATE LastState);

//Tws Audio 相位锁定
void tws_time_init(void);

uint16_t tws_device_space_len(void);



void tws_local_audio_set(uint16_t ItemRef, TWS_AUDIO_CMD CMD);

uint32_t SoundRemindItemGet(void);

uint8_t tws_local_audio_wait(void);

void RemindSoundSyncEnable(void);

bool RemindSoundSyncRequest(uint16_t ItemRef, TWS_AUDIO_CMD CMD);

void tws_local_audio_stop(uint16_t ItemRef);

void tws_device_open_isr(void);

#endif /*__BT_TWS_API_H__*/

/**
 * @}
 * @}
 */

