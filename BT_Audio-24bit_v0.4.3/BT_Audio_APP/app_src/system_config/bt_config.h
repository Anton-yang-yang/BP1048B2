
///////////////////////////////////////////////////////////////////////////////
//               Mountain View Silicon Tech. Inc.
//  Copyright 2012, Mountain View Silicon Tech. Inc., Shanghai, China
//                       All rights reserved.
//  Filename: bt_config.h
//  maintainer: keke
///////////////////////////////////////////////////////////////////////////////
#ifndef __BT_DEVICE_CFG_H__
#define __BT_DEVICE_CFG_H__
#include "type.h"
#include "app_config.h"

/* "sys_param.h"蓝牙基础参数*/
#define ENABLE						TRUE
#define DISABLE						FALSE

/*****************************************************************
 * 蓝牙功能开关
 *****************************************************************/
#ifdef CFG_APP_BT_MODE_EN
//TWS开关
#define BT_TWS_SUPPORT

//蓝牙双手机连接开关
//#define BT_MULTI_LINK_SUPPORT
#endif

//#define BT_USER_VISIBILITY_STATE
#ifdef BT_USER_VISIBILITY_STATE
#define BT_USER_PAIR_TTS
#endif
#define  RECON_ADD
//BLE和BT(classic)模块宏定义开关
#define BLE_SUPPORT					DISABLE
#define BT_SUPPORT			        ENABLE

/*****************************************************************
 * Note: 蓝牙部分参数已移植到system_config/parameter.ini文件内进行配置,具体如下:
 * 1. 蓝牙名称  BT_NAME/BLE_NAME
 * 2. 蓝牙发射功率  TxPowerLevel/PagePowerLevel
 * 3. 默认的频偏参数  BtTrim
 * 4. TWS主从之间音量控制同步标志  TwsVolSyncEnable
 *****************************************************************/
#define BT_ADDR_SIZE				6
#define BT_LSTO_DFT					8000    //连接超时时间 (换算公式: 8000*0.625=5s)
#define BT_PAGE_TIMEOUT				8000	//page timeout(ms)  //8000*0.625=5s  

#define	CFG_PARA_BT_SYNC					//BtPlay 异步时钟 采样点同步
#define CFG_PARA_HFP_SYNC					//通话 异步时钟 采样点同步
	
/*****************************************************************
 * 蓝牙协议宏定义开关
 *****************************************************************/
/*
 * 以下宏请勿随意修改，否则会引起编译错误
 */
#define BT_A2DP_SUPPORT				ENABLE //A2DP和AVRCP关联
#if CFG_RES_MIC_SELECT
#define BT_HFP_SUPPORT				ENABLE
#endif
#define BT_SPP_SUPPORT				DISABLE
	
	
//在非蓝牙模式下,播放音乐自动切换到播放模式
//#define BT_AUTO_ENTER_PLAY_MODE

//开机蓝牙不可见只回连状态
//#define POWER_ON_BT_ACCESS_MODE_SET

/*****************************************************************
 * 宏定义开关警告
 *****************************************************************/
#if (BT_SUPPORT != ENABLE)
#if (defined(CFG_APP_BT_MODE_EN))
#error Conflict: CFG_APP_BT_MODE_EN and BT_SUPPORT setting error 
#endif
#endif


/*****************************************************************
 * 双手机链路
 *****************************************************************/
#ifdef BT_MULTI_LINK_SUPPORT
#define BT_LINK_DEV_NUM				2 	//蓝牙连接手机个数 (1 or 2)
#define BT_DEVICE_NUMBER			2	//蓝牙ACL连接个数 (1 or 2)
#define BT_SCO_NUMBER				2	//蓝牙通话链路个数 (1 or 2),BT_SCO_NUMBER必须小于BT_DEVICE_NUMBER

#if (BT_LINK_DEV_NUM == 2)
#define LAST_PLAY_PRIORITY				//后播放优先
//#define BT_LINK_2DEV_ACCESS_DIS_CON		//开启后,第一个手机连上后,第二个手机需要能搜索到; 关闭后,第二个手机搜索不到,能回连上
#endif

#else
#define BT_LINK_DEV_NUM				1 	//蓝牙连接手机个数 (1 or 2)
#define BT_DEVICE_NUMBER			1	//蓝牙ACL连接个数 (1 or 2)
#define BT_SCO_NUMBER				1	//蓝牙通话链路个数 (1 or 2) ,BT_SCO_NUMBER必须小于BT_DEVICE_NUMBER
#endif

/*****************************************************************
 * 宏定义开关警告
 *****************************************************************/
#ifdef BT_DEVICE_NUMBER
#if ((BT_DEVICE_NUMBER != 2)&&(BT_DEVICE_NUMBER != 1))
#error Conflict: BT_DEVICE_NUMBER setting error 
#endif
#endif

#ifdef BT_SCO_NUMBER
#if ((BT_SCO_NUMBER != 2)&&(BT_SCO_NUMBER != 1)&&(BT_SCO_NUMBER > BT_DEVICE_NUMBER))
#error Conflict: BT_SCO_NUMBER setting error 
#endif
#endif

/*****************************************************************
 * A2DP config
 *****************************************************************/
#if BT_A2DP_SUPPORT == ENABLE

#include "bt_a2dp_api.h"

//Note:开启AAC,需要同步开启解码器类型USE_AAC_DECODER(app_config.h)
//Note:目前1V2跟AAC解码不能同时打开
//#define BT_AUDIO_AAC_ENABLE
#ifdef BT_AUDIO_AAC_ENABLE
#if (!defined(USE_AAC_DECODER))
#define USE_AAC_DECODER
#endif

//Note:目前1V2跟AAC解码不能同时打开
#ifdef BT_MULTI_LINK_SUPPORT
#error Conflict: BT_AUDIO_AAC_ENABLE and BT_MULTI_LINK_SUPPORT setting error
#endif
#endif

/*****************************************************************
 * AVRCP config
 *****************************************************************/
#include "bt_avrcp_api.h"
/*
 * If it doesn't support Advanced AVRCP, TG side will be ignored
 * 音量同步功能需要开启该宏开关
 */
#define BT_AVRCP_VOLUME_SYNC			DISABLE

/*
 * If it doesn't support Advanced AVRCP, TG side will be ignored
 * 和音量同步功能都用到AVRCP TG
 * player application setting and value和音量同步宏定义开关一致(eg:EQ/repeat mode/shuffle/scan configuration)
 */
#define BT_AVRCP_PLAYER_SETTING			DISABLE

/*
 * If it doesn't support Advanced AVRCP, song play state will be ignored
 * 歌曲播放时间
 */
#define BT_AVRCP_SONG_PLAY_STATE		DISABLE

/*
 * If it doesn't support Advanced AVRCP, song track infor will be ignored
 * 歌曲ID3信息反馈
 * 歌曲信息有依赖播放时间来获取,请和BT_AVRCP_SONG_PLAY_STATE同步开启
 */
#define BT_AVRCP_SONG_TRACK_INFOR		DISABLE

/*
 * AVRCP连接成功后，自动播放歌曲
 */
#define BT_AUTO_PLAY_MUSIC				DISABLE

#endif /* BT_A2DP_SUPPORT == ENABLE */

/*****************************************************************
 * HFP config
 *****************************************************************/
#if BT_HFP_SUPPORT == ENABLE

#include "bt_hfp_api.h"

//DISABLE: only cvsd
//ENABLE: cvsd + msbc
#define BT_HFP_SUPPORT_WBS				ENABLE

/*
 * If it doesn't support WBS, only PCM format data can be
 * transfered to application.
 */
#define BT_HFP_AUDIO_DATA				HFP_AUDIO_DATA_mSBC

//电池电量同步(开启需要和 CFG_FUNC_POWER_MONITOR_EN 关联)
//#define BT_HFP_BATTERY_SYNC

//开启HFP连接，不使能进入HFP相关模式(K歌模式和通话模式)
//该应用适用于某些需要连接HFP协议，但是不需要HFP相关功能的场合
//#define BT_HFP_MODE_DISABLE

/*
 * 通话相关配置
 */
//AEC相关参数配置 (MIC gain, AGC, DAC gain, 降噪参数)
#define BT_HFP_AEC_ENABLE
#define BT_REMOTE_AEC_DISABLE			//关闭手机端AEC

//MIC无运放,使用如下参数(参考)
#define BT_HFP_MIC_PGA_GAIN				15  //ADC PGA GAIN +18db(0~31, 0:max, 31:min)
#define BT_HFP_MIC_PGA_GAIN_BOOST_SEL	2
#define BT_HFP_MIC_DIGIT_GAIN			4095
#define BT_HFP_INPUT_DIGIT_GAIN			1100

//MIC有运放,使用如下参数(参考开发板)
//#define BT_HFP_MIC_PGA_GAIN				14  //ADC PGA GAIN +2db(0~31, 0:max, 31:min)
//#define BT_HFP_MIC_DIGIT_GAIN				4095
//#define BT_HFP_INPUT_DIGIT_GAIN			4095

#define BT_HFP_AEC_ECHO_LEVEL			4 //Echo suppression level: 0(min)~5(max)
#define BT_HFP_AEC_NOISE_LEVEL			2 //Noise suppression level: 0(min)~5(max)

#define BT_HFP_AEC_MAX_DELAY_BLK		32
#define BT_HFP_AEC_DELAY_BLK			4 //MIC无运放参考值
//#define BT_HFP_AEC_DELAY_BLK			14 //MIC有运放参考值(参考开发板)

//来电通话时长配置选项
#define BT_HFP_CALL_DURATION_DISP


#endif /* BT_HFP_SUPPORT == ENABLE */

/*****************************************************************
 * TWS config
 *****************************************************************/
#ifndef BT_TWS_SUPPORT
#define BT_TWS_LINK_NUM				0		//不开起TWS功能,只能将此参数配置为0
#if (BT_TWS_LINK_NUM != 0)
#error Conflict: BT_TWS_LINK_NUM setting error 
#endif
#endif

#ifdef BT_TWS_SUPPORT
#include "bt_tws_api.h"

#define BT_TWS_LINK_NUM				1		//TWS参数仅能为1
#if (BT_TWS_LINK_NUM != 1)
#error Conflict: BT_TWS_LINK_NUM setting error 
#endif

#define CFG_TWS_ONLY_IN_BT_MODE		DISABLE //enable:仅在蓝牙模式下工作, disable:在所有模式下工作
#define TWS_STATRT_PLAY_FRAM		56*2    //取值范围[56*1 - 56*4]; 对应music通路的delay时间是56*3*[1 - 4] = [168 - 672]ms；此值越大，主副箱距离越远
#define CFG_CMD_DELAY				10      //ms 主从CMD发送到处理延时5ms左右，信道质量差时更长，考虑被打断会形成7~20mS延时，此处设10ms较合适。

//1.针对2.0的应用，可选择CFG_EFFECT_MUSIC_MASTER为1，由主机端统一调音；
//2.针对2.1的应用(dac0+dacx)，必须选择CFG_EFFECT_MUSIC_MASTER为0,主机和从机单独调音，否则会存在DACX相位不一致的问题！
#define CFG_EFFECT_MUSIC_MASTER		0		//0=master,slaver 独立有调音音效，1=只有master有调音，音效功能，slaver仅做为接收，

//TWS组网条件判断
#define TWS_FILTER_NAME						//过滤名称
//#define TWS_FILTER_USER_DEFINED			//自定义过滤条件(max:6bytes)
#ifdef TWS_FILTER_USER_DEFINED
//默认定义为字符串,假如是定义参数,请移步到配置处自行赋值
#define TWS_FILTER_INFOR		"TWS-MV" //(length:6bytes)
#endif

#define CFG_TWS_ROLE_RANDOM			0		//适用于双键组网		主从角色在组网前随机
#define CFG_TWS_ROLE_MASTER			1		//Soundbar主			暂不支持
#define CFG_TWS_ROLE_SLAVE			2		//Soundbar从			暂不支持
#define CFG_TWS_PEER_MASTER			3		//对箱主				按键发起配对的机器组网成功后默认就是MASTER
#define CFG_TWS_PEER_SLAVE			4		//对箱从				按键发起配对的机器组网成功后默认就是SLAVE

/*
 * 组网配对方式选择
 */
#define TWS_PAIRING_MODE			CFG_TWS_PEER_MASTER


#if ((TWS_PAIRING_MODE == CFG_TWS_PEER_MASTER)||(TWS_PAIRING_MODE == CFG_TWS_ROLE_RANDOM))
#define CFG_AUTO_ENTER_TWS_SLAVE_MODE		//slave连接成功后自动切换到tws_slave模式
#endif

/*
 * 打开表示从机连上TWS后能够切模式 (需同时打开后台蓝牙)
 * 关闭表示从机连上TWS后不能切模式 (需同时打开后台蓝牙)
 */
//#define TWS_SLAVE_MODE_SWITCH_EN

/*
 * 打开表示支持TWS主从同步关机；
 * 关闭表示单箱关机，哪个音箱按键关机就关哪个音箱
 */
#define TWS_POWEROFF_MODE_SYNC

#endif /* #ifdef BT_TWS_SUPPORT */

#endif /*__BT_DEVICE_CFG_H__*/

