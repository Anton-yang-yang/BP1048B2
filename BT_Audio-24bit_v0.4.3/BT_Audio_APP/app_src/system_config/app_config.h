/******************************************************************************
 * @file    app_config.h
 * @author
 * @version V_NEW
 * @date    2021-06-25
 * @maintainer
 * @brief
 ******************************************************************************
 * @attention
 * <h2><center>&copy; COPYRIGHT 2013 MVSilicon </center></h2>
 */
#ifndef __APP_CONFIG_H__
#define __APP_CONFIG_H__

#include "type.h"
//************************************************************************************************************
//    ｱｾﾏｵﾍｳﾄｬﾈﾏｿｪﾆ�ｸｵﾍｳﾈｫｾﾖｺ凜ｬﾔﾚIDEｹ､ｳﾌﾅ葷ﾃ(Build Settings-Compiler-Symbols)｣ｬｴﾋｴｦﾓﾃﾓﾚﾌ瞎ﾑ
//*CFG_APP_CONFIG ｺﾍ FUNC_OS_EN*/
//************************************************************************************************************

//************************************************************************************************************
//    ｹｦﾄﾜｿｪｹﾘﾋｵﾃｺ
// *CFG_APP_*  : ﾈ晴�ｵﾍｳﾓｦﾓﾃﾄ｣ﾊｽｿｪﾆｬｱﾈﾈ躑SB Uﾅﾌｲ･ｷﾅｸ霽昞ｦﾓﾃﾄ｣ﾊｽｵﾄﾑ｡ﾔ�
// *CFG_FUNC_* : ﾈ晴�ｦﾄﾜｿｪｹﾘ
// *CFG_PARA_* : ﾏｵﾍｳﾈ晴�ﾎﾊ�靹ﾃ
// *CFG_RES_*  : ﾏｵﾍｳﾓｲｼ�ﾊﾔｴﾅ葷ﾃ
// ************************************************************************************************************


//24bit ﾒｧｿｪｹﾘｺ�ﾘﾖﾆ
//24bit SDK RAMｿｪﾏ忞盒ﾈﾔﾓ
#define  CFG_AUDIO_WIDTH_24BIT


//****************************************************************************************
//       ﾐｾﾆｬﾐﾍｺﾅﾑ｡ﾔ葷ﾃ
// ﾓｦﾓﾃｿｪｷ｢ﾊｱ｣ｬﾒｻｶｨﾒｪﾑ｡ｶﾔﾐﾍｺﾅ,ｷﾀﾖｹﾉﾏﾎｻｻ昕ﾔﾊｾｵﾄmicｺﾍlineinﾊ菠�ﾍﾓｲｼ�ﾓｿﾚｲｻﾒｻﾖﾂ
//****************************************************************************************
//#define  CFG_CHIP_BP10128
//#define  CFG_CHIP_BP1064A2	// flashﾈﾝﾁｿ 2M
//#define  CFG_CHIP_BP1064L2	// flashﾈﾝﾁｿ 2M
//#define  CFG_CHIP_BP1048A2	// flashﾈﾝﾁｿ 2M
#define  CFG_CHIP_BP1048B2   	// flashﾈﾝﾁｿ 2M
//#define  CFG_CHIP_BP1048B1   	// flashﾈﾝﾁｿ 1M  ｿﾍｻｧﾐ鞅ﾔﾐﾐｰﾑｴ惲�ﾘﾖﾆﾔﾚ 1M ﾒﾔﾄﾚ
//#define  CFG_CHIP_MT166      	// flashﾈﾝﾁｿ 1M  ｿﾍｻｧﾐ鞅ﾔﾐﾐｰﾑｴ惲�ﾘﾖﾆﾔﾚ 1M ﾒﾔﾄﾚ
//#define  CFG_CHIP_BP1032A1   	// flashﾈﾝﾁｿ 1M  ｿﾍｻｧﾐ鞅ﾔﾐﾐｰﾑｴ惲�ﾘﾖﾆﾔﾚ 1M ﾒﾔﾄﾚ
//#define  CFG_CHIP_BP1032A2   	// flashﾈﾝﾁｿ 2M
//#define  CFG_CHIP_BP1048P2    // flashﾈﾝﾁｿ 2M
//#define  CFG_CHIP_BP1048P4    // flashﾈﾝﾁｿ 4M

//****************************************************************************************
//       ﾕ�ﾔ1MB FLASHﾐｾﾆｬﾅ葷ﾃ
//****************************************************************************************
/**ﾒｵSDKｰ豎ｾｺﾅ｣ｺV1.0.0**/
/*0xB1｣ｺﾐｾﾆｬB1X｣ｬ01｣ｺｴ豎ｾｺﾅ｣ｬ 00｣ｺﾐ｡ｰ豎ｾｺﾅ｣ｬ 00｣ｺﾓﾃｻｧﾐﾞｶｩｺﾅ｣ｨﾓﾉﾓﾃｻｧﾉ雜ｨ｣ｬｿﾉｽ蘯ﾏｲｹｶ｡ｺﾅ｣ｩ｣ｻﾊｵｼﾊｴ豢｢ﾗﾖｽﾚﾐｺ1A 01 00 00 ｣ｬﾂﾔﾇﾚsdkｰ豎ｾ*/
/*ｿｪﾆlash_bootﾊｱ｣ｬﾓﾃflashbootﾉ�ｶusercodeｺｬbootﾃ�ﾍcodeﾃ�ﾈ�xB8ｺﾍ0xB8+0x10000)ﾁｽｸｵｻ盍ｻﾍｬ｣ｬﾇｰﾕﾟﾊﾇburnerﾉﾕﾂｼﾊｱｰ豎ｾ｣ｬｺﾟﾊﾇmvaｰ豎ｾﾐ雹ﾘﾗ｢*/
#define	 CFG_SDK_VER_CHIPID			(0xB1)
#define  CFG_SDK_MAJOR_VERSION		(0)
#define  CFG_SDK_MINOR_VERSION		(4)
#define  CFG_SDK_PATCH_VERSION	    (3)

//****************************************************************************************
//       B0,B1ｿﾚﾗｪDEBUGｷﾂﾕ貉ｦﾄﾜﾑ｡ﾔ葷ﾃ
// ﾋｵﾃ�
//    1.ﾈﾃｵｽlinein4(B0,B1),ﾎｪﾁﾋｷﾀﾖｹﾗｪｵｽlinein4ﾍｨｵﾀｺﾞｷｨｷﾂﾕ譽ｬﾐ靨ｪｿｪﾆﾋｺ凜ｻ
//    2.ﾈﾃｵｽlinein4(B0,B1),ﾁｿｲ摠ｱﾐ靨ｪｹﾘｱﾕｴﾋｺ凜ｬｷｪｵｽｴﾋﾍｨｵﾀﾊｱｻ瞠ﾐPOPﾉｻ
//****************************************************************************************
#define CFG_FUNC_SW_DEBUG_EN

//****************************************************************************************
// ﾏｵﾍｳAppｹｦﾄﾜﾄ｣ﾊｽﾑ｡ﾔ�
//****************************************************************************************
//#define CFG_APP_IDLE_MODE_EN
#define CFG_APP_BT_MODE_EN
//#define	CFG_APP_USB_PLAY_MODE_EN
//#define	CFG_APP_CARD_PLAY_MODE_EN
//#define	CFG_APP_LINEIN_MODE_EN
//#define CFG_APP_RADIOIN_MODE_EN
#define CFG_APP_USB_AUDIO_MODE_EN

//#define CFG_APP_I2SIN_MODE_EN
//#define CFG_FUNC_I2S_MIX_MODE

//#define	CFG_APP_OPTICAL_MODE_EN	// SPDIF ｹ簪ﾋﾄ｣ﾊｽ
//#define CFG_APP_COAXIAL_MODE_EN	// SPDIF ﾍｬﾖ眛｣ﾊｽ
//#define CFG_APP_HDMIIN_MODE_EN //HDMI INﾄ｣ﾊｽ

#define CFG_FUNC_OPEN_SLOW_DEVICE_TASK

#ifdef CFG_APP_LINEIN_MODE_EN
#if defined(CFG_CHIP_BP1064A2) || defined(CFG_CHIP_BP1064L2) || defined(CFG_CHIP_BP10128)
#define LINEIN_INPUT_CHANNEL				(ANA_INPUT_CH_LINEIN1)
#elif defined(CFG_CHIP_BP1032)
#define LINEIN_INPUT_CHANNEL				(ANA_INPUT_CH_LINEIN4)
#else
#define LINEIN_INPUT_CHANNEL				(ANA_INPUT_CH_LINEIN5)
#endif
#endif

#ifdef CFG_APP_RADIOIN_MODE_EN
    #define FUNC_RADIO_RDA5807_EN
    //#define FUNC_RADIO_QN8035_EN  //ﾐｾﾆｬioﾊｱﾖﾓﾖｻﾖｧｳﾖ12M｣ｬqn8035ｱﾘﾐ�篁ﾒｾｧﾕ�
#if defined(FUNC_RADIO_RDA5807_EN) && defined(FUNC_RADIO_QN8035_EN)
   #error Conflict: radio type error //ｲｻﾄﾜﾍｬﾊｱﾑ｡ﾔｽﾖﾖﾏﾔﾊｾﾄ｣ﾊｽ
#endif
#endif

/**USBﾉｨ｣ｬｶﾁｿｨﾆｬﾒｻﾏﾟﾍｨｹｦﾄﾜ **/
#define CFG_FUNC_USB_DEVICE_DETECT
#ifdef  CFG_APP_USB_AUDIO_MODE_EN
	#define CFG_PARA_USB_MODE	AUDIO_MIC
	#define CFG_RES_AUDIO_USB_IN_EN		//ﾈｱﾊ｡ ﾗｪﾄ｣ﾊｽﾍｨﾂｷﾊｹﾄﾜ
	#define CFG_PARA_AUDIO_USB_IN_SYNC	//ﾊｱﾖﾓﾆｫｲ霻�ﾄ ｲﾉﾑ耋ｬｲｽ
	#define CFG_PARA_AUDIO_USB_IN_SRC	//ﾗｪｲﾉﾑｼｱｸ

	#define CFG_RES_AUDIO_USB_OUT_EN
	#define CFG_PARA_AUDIO_USB_OUT_SYNC	//ﾊｱﾖﾓﾆｫｲ霻�ﾄ ｲﾉﾑ耋ｬｲｽ
	#define CFG_PARA_AUDIO_USB_OUT_SRC	//ﾗｪｲﾉﾑｼｱｸ
	#define CFG_RES_AUDIO_USB_VOL_SET_EN
	//#define USB_READER_EN
#endif

//IDLEﾄ｣ﾊｽ(ｼﾙｴ��,powerkey/deepsleepｿﾉﾒﾔﾍｬﾊｱﾑ｡ﾖﾐﾒｲｿﾉﾒﾔｵ･ｶﾀﾅ葷ﾃ
//ﾍｨｹ�鋧｢ｽ�ｻﾍｬｵﾄﾄ｣ﾊｽ
//MSG_DEEPSLEEP/MSG_POWER/MSG_POWERDOWN --> ｽ�DLEﾄ｣ﾊｽ(ｼﾙｴ��
//MSG_DEEPSLEEP --> ｽ�DLEﾄ｣ﾊｽﾒﾔｺ郢淸FG_IDLE_MODE_DEEP_SLEEPｴｪｽ�eepsleep
//MSG_POWERDOWN --> ｽ�DLEﾄ｣ﾊｽﾒﾔｺ郢淸FG_IDLE_MODE_POWER_KEYｴｪｽ�owerdown
#ifdef  CFG_APP_IDLE_MODE_EN
	#define CFG_IDLE_MODE_POWER_KEY	//power key
	#define CFG_IDLE_MODE_DEEP_SLEEP //deepsleep
	#ifdef CFG_IDLE_MODE_POWER_KEY
		#define BAKEUP_FIRST_ENTER_POWERDOWN		//ｵﾚﾒｻｴﾎﾉﾏｵ醪靨ｪｰｴﾏﾂPowerKey
		#define POWERKEY_MODE		POWERKEY_MODE_SLIDE_SWITCH_LPD//POWERKEY_MODE_PUSH_BUTTON
		#if (POWERKEY_MODE == POWERKEY_MODE_SLIDE_SWITCH_LPD) || (POWERKEY_MODE == POWERKEY_MODE_SLIDE_SWITCH_HPD)
			#define POWERKEY_CNT					20
		#else
			#define POWERKEY_CNT					2000
		#endif

		#if (POWERKEY_MODE == POWERKEY_MODE_PUSH_BUTTON)
			//powerkeyｸｴﾓﾃｶﾌｰｴｼ�ｦﾄﾜ｡｣
			#define USE_POWERKEY_PUSH_BUTTON_MSG_SP		MSG_PLAY_PAUSE
		#endif	
	#endif
	#ifdef CFG_IDLE_MODE_DEEP_SLEEP
		/*ｺ�箍ｴｼ�ｽﾐﾑ,ﾗ｢ﾒ磴FG_PARA_WAKEUP_GPIO_IRｺﾍ ｻｽﾐﾑｼ鶴R_KEY_POWERﾉ靹ﾃ*/
		#define CFG_PARA_WAKEUP_SOURCE_IR		SYSWAKEUP_SOURCE9_IR//ｹﾌｶｨsource9
		/*ADCKeyｻｽﾐﾑ ﾅ莠ﾏCFG_PARA_WAKEUP_GPIO_ADCKEY ｻｽﾐﾑｼ魵DCKEYWAKEUPﾉ靹ﾃｼｰﾆ莊酥ｽ*/
		#define CFG_PARA_WAKEUP_SOURCE_ADCKEY	SYSWAKEUP_SOURCE1_GPIO//ﾈ醋ｹﾓﾃADCｻｽﾐﾑ｣ｬｱﾘﾐ�ｪCFG_RES_ADC_KEY_SCAN ｺﾍCFG_RES_ADC_KEY_USE
		#define CFG_PARA_WAKEUP_SOURCE_POWERKEY SYSWAKEUP_SOURCE6_POWERKEY
		#define CFG_PARA_WAKEUP_SOURCE_IOKEY1	SYSWAKEUP_SOURCE3_GPIO
		#define CFG_PARA_WAKEUP_SOURCE_IOKEY2	SYSWAKEUP_SOURCE4_GPIO
	#endif
#endif

#if  defined(CFG_CHIP_MT166) || defined(CFG_CHIP_BP1032A1) || defined(CFG_CHIP_BP1032A2)
	#define CFG_RES_MIC_SELECT      (1)//0=NO MIC, 1= MIC1, 2= MIC2, 3 = MCI1+MIC2
#else
	#define CFG_RES_MIC_SELECT      (3)//0=NO MIC, 1= MIC1, 2= MIC2, 3 = MCI1+MIC2
#endif

//****************************************************************************************
//                 ﾒｵﾊ莎ｨｵﾀﾏ犹ﾘﾅ葷ﾃ
//ﾋｵﾃ�
//    ﾈ醯ﾂﾊ莎ｴｿﾉﾍｬﾊｱﾊ莎ｻ
//****************************************************************************************
/**DAC-0ﾍｨｵﾀﾅ葷ﾃﾑ｡ﾔ�*/
#define CFG_RES_AUDIO_DAC0_EN

/**DAC-Xﾍｨｵﾀﾅ葷ﾃﾑ｡ﾔ�*/
#define CFG_RES_AUDIO_DACX_EN

/**I2Sﾒｵﾊ莎ｨｵﾀﾅ葷ﾃﾑ｡ﾔ�*/
//#define CFG_RES_AUDIO_I2SOUT_EN

//****************************************************************************************
//     I2Sﾏ犹ﾘﾅ葷ﾃﾑ｡ﾔ�
//ﾋｵﾃ�
//    1.I2Sﾊ莎ｲﾊｹﾄﾜﾊｱｶﾋｿﾚﾑ｡ﾔﾍﾄ｣ﾊｽﾐ靨ｪﾗ｢ﾒ箜｣ｳﾖﾒｻﾖﾂ;
//	  2｡｢ﾈｱﾊ｡ﾅ葷ﾃﾎｪMaster｣ｬI2Sﾍ篷靨ｵﾊｹﾓﾃMasterｵﾄMCLK,ｷﾔｷｽｲｻﾎ｢ｵｱﾖﾓｾﾍｻ瞠ﾐpopﾒ｣
//    3.ｽｪﾆ2S slave｣ｬI2Sﾍ篷雎ﾘﾐ�盪ｩﾎﾈｶｨﾊｱﾖﾓ｣ｬﾈ郢鉷篷霖ｬﾏﾟｹ､ﾗｻﾎﾈｶｨ｣ｬﾇ�靹ﾃAudioIOSet.Sync = FALSE｣ｻﾗﾔﾐﾐﾔﾓDetectLockｵ焏ync
//****************************************************************************************
#if defined(CFG_APP_I2SIN_MODE_EN) || defined(CFG_RES_AUDIO_I2SOUT_EN)
	#include"i2s.h"
	#define	CFG_RES_I2S 						1//I2S0_MODULE:I2S0ﾗﾜﾏﾟ   I2S1_MODULE:I2S1ﾗﾜﾏﾟ;
	#define	CFG_RES_I2S_MODE					0//0:master mode ;1:slave mode ﾍ篷靜ｴｽﾓｲｻﾒｪﾅ舖lave
	#if (CFG_RES_I2S == 0)						//MCLK-LRCLK-BCLK-DOUT-DIN Port 2/3ﾈｱｶﾋｿﾚｺﾍｹｦﾄﾜ｣ｬｽ葷ﾃ
		#define	CFG_RES_I2S_MODULE				I2S0_MODULE
		#define	CFG_RES_I2S_IO_PORT				0//  0:i2S0_0-A0-1-2-3-4; 		1:I2S0_1-A24-A20-21-22-23; 	2:I2S0_2-X-X-X-A4-3;	3:I2S0_3-X-X-X-A23-22 //2ｺﾍ3 ﾈｱclk ｴ惲�ﾐﾎｴﾅ葷ﾃ
	#elif (CFG_RES_I2S == 1)
		#define	CFG_RES_I2S_MODULE				I2S1_MODULE
		#define	CFG_RES_I2S_IO_PORT				1//  0:i2S1_0-A27-28-29-30-31; 	1:I2S1_1-A7-8-9-10-11; 		2:I2S1_2-X-A1-2-31-30;	3:I2S1_3-X-A20-21-11-10
	#endif
	#define CFG_PARA_I2S_SAMPLERATE				44100
	#define CFG_FUNC_I2S_IN_SYNC_EN				//ﾈｱﾊ｡ﾎｪSRA
	#define CFG_FUNC_I2S_OUT_SYNC_EN
#endif

#ifdef CFG_FUNC_I2S_MIX_MODE
	#define	CFG_RES_I2S0_EN	 // 0:i2S0_0-A0-1-2-3-4; 		1:I2S0_1-A24-A20-21-22-23;  2:I2S0_2-X-X-X-A4-3;	3:I2S0_3-X-X-X-A23-22 //2ｺﾍ3 ﾈｱclk ｴ惲�ﾐﾎｴﾅ葷ﾃ
	#define	CFG_RES_I2S1_EN	 // 0:i2S1_0-A27-28-29-30-31; 	1:I2S1_1-A7-8-9-10-11; 	    2:I2S1_2-X-A1-2-31-30;	3:I2S1_3-X-A20-21-11-10
	
	#ifdef CFG_RES_I2S0_EN
	#define CFG_RES_AUDIO_I2S0IN_EN
	#define CFG_RES_AUDIO_I2S0OUT_EN
	//#define CFG_FUNC_I2S0IN_SRC_EN  			//I2Sﾊ菠�ﾉﾑﾊﾗｪｻｻ
	//#define CFG_FUNC_I2S0IN_SRA_EN  			//I2Sﾊ菠�ﾉﾑﾊﾎ｢ｵ�ﾄﾚｲｿﾊｱﾖﾓ,slaverﾊｱｿｪﾆ�
	//#define CFG_FUNC_I2S0OUT_SRC_EN  			//I2Sﾊ莎ﾉﾑﾊﾗｪｻｻ
	//#define CFG_FUNC_I2S0OUT_SRA_EN  			//I2Sﾊ莎ﾉﾑﾊﾎ｢ｵ�ﾄﾚｲｿﾊｱﾖﾓ,slaverﾊｱｿｪﾆ�
	//#define CFG_PARA_I2S0_SAMPLE  	44100 	//I2Sｲﾉﾑﾊ｣ｬﾒｻｰ羇ｻﾓﾃ｣ｬﾓﾉｵﾄｼ�ｨ
    #define CFG_FUNC_I2S0_MIX_EN        0		// 0 = ｶﾀﾁ｢ﾒｧｴｦﾀ�   	1 = ﾓ�usic_pcmｻ�ﾏｺｻﾆｦﾀ橫�
    #define	CFG_RES_I2S0_IO_PORT		0		// 0:i2S0_0-A0-1-2-3-4; 	1:I2S0_1-A24-A20-21-22-23;  2:I2S0_2-X-X-X-A4-3;	3:I2S0_3-X-X-X-A23-22 //2ｺﾍ3 ﾈｱclk ｴ惲�ﾐﾎｴﾅ葷ﾃ
	#endif
	
	#ifdef CFG_RES_I2S1_EN
	#define CFG_RES_AUDIO_I2S1IN_EN
	#define CFG_RES_AUDIO_I2S1OUT_EN
	//#define CFG_FUNC_I2S1IN_SRC_EN  			//I2Sﾊ菠�ﾉﾑﾊﾗｪｻｻ
	//#define CFG_FUNC_I2S1IN_SRA_EN  			//I2Sﾊ菠�ﾉﾑﾊﾎ｢ｵ�ﾄﾚｲｿﾊｱﾖﾓ,slaverﾊｱｿｪﾆ�
	//#define CFG_FUNC_I2S1OUT_SRC_EN  			//I2Sﾊ莎ﾉﾑﾊﾗｪｻｻ
	//#define CFG_FUNC_I2S1OUT_SRA_EN  			//I2Sﾊ莎ﾉﾑﾊﾎ｢ｵ�ﾄﾚｲｿﾊｱﾖﾓ,slaverﾊｱｿｪﾆ�
	//#define CFG_PARA_I2S1_SAMPLE  	44100 	//I2Sｲﾉﾑﾊ｣ｬﾒｻｰ羇ｻﾓﾃ｣ｬﾓﾉｵﾄｼ�ｨ
    #define CFG_FUNC_I2S1_MIX_EN        0		// 0=ｶﾀﾁ｢ﾒｧｴｦﾀ�   1=ﾓ�usic_pcmｻ�ﾏｺｻﾆｦﾀ橫�
    #define	CFG_RES_I2S1_IO_PORT		1		// 0:i2S1_0-A27-28-29-30-31; 	1:I2S1_1-A7-8-9-10-11;  2:I2S1_2-X-A1-2-31-30;	3:I2S1_3-X-A20-21-11-10
	#endif

	#define CFG_PARA_I2S_SAMPLERATE				44100
	#define CFG_FUNC_I2S_IN_SYNC_EN				//ﾈｱﾊ｡ﾎｪSRA
	#define CFG_FUNC_I2S_OUT_SYNC_EN	
#endif

//****************************************************************************************
//                 ｽ篦�獎ﾍﾑ｡ﾔ�
//ﾋｵﾃ�
//    ﾈ醯ﾂｽ篦�獎ﾍﾑ｡ﾔ瞠ｰﾏ�ode size;
//****************************************************************************************
#define USE_MP3_DECODER
#define USE_WMA_DECODER
#define USE_SBC_DECODER
#define USE_WAV_DECODER
//#define USE_DTS_DECODER
#define USE_FLAC_DECODER	//24bit 1.5Mbpsｸﾟﾂ�ﾊﾊｱ｣ｬﾐ靨ｪﾀｩｴECODER_FIFO_SIZE_FOR_PLAYER ﾊ莎ifo｣ｬｻｩｴ菠�ｺFLAC_INPUT_BUFFER_CAPACITY
//#define USE_AAC_DECODER
//#define USE_AIF_DECODER
//#define USE_AMR_DECODER
#define USE_APE_DECODER

//****************************************************************************************
//                 ﾗﾜﾒｧｹｦﾄﾜﾅ葷ﾃ
//****************************************************************************************
//ｸﾟｵﾍﾒﾚｹｦﾄﾜﾅ葷ﾃﾋｵﾃ�
//    1.ｴﾋｹｦﾄﾜﾊﾇｻﾚMIC OUT EQｽﾐﾊﾖｶｯﾉ靹ﾃｵﾄ｣ｬﾐ靨ｪﾔﾚｵﾎﾊ�ﾐﾊｹﾄﾜｴﾋEQ｣ｻ
//    2.ﾄｬﾈﾏf5ｶﾔﾓｦbass gain,f6ｶﾔﾓｦtreb gain,ﾈ酖賈ﾏﾐﾞｸﾄｴﾋEQ filterﾊ�ｿ｣ｬﾐ靨ｪｶﾔﾓｦﾐﾞｸﾄBassTrebAjust()ﾖﾐｶﾔﾓｦﾐﾅ｣ｻ
//EQﾄ｣ﾊｽｹｦﾄﾜﾅ葷ﾃﾋｵﾃ�
//    1.ｴﾋｹｦﾄﾜﾊﾇｻﾚMUSIC EQｽﾐﾊﾖｶｯﾉ靹ﾃｵﾄ｣ｬﾐ靨ｪﾔﾚｵﾎﾊ�ﾐﾊｹﾄﾜｴﾋEQ｣ｻ
//    2.ｿﾉﾔﾚflat/classic/pop/jazz/pop/vocal boostﾖｮｼ莽ｨｹ�EYﾀｴﾇﾐｻｻ   
#define CFG_FUNC_AUDIO_EFFECT_EN //ﾗﾜﾒｧﾊｹﾄﾜｿｪｹﾘ
#ifdef CFG_FUNC_AUDIO_EFFECT_EN

    //#define CFG_FUNC_ECHO_DENOISE          //ﾏ﨤��ﾙｵﾚdelayﾊｱｵﾄﾔﾓﾒｬ
 	//#define CFG_FUNC_MUSIC_EQ_MODE_EN     //Music EQﾄ｣ﾊｽｹｦﾄﾜﾅ葷ﾃ
	#ifdef CFG_FUNC_MUSIC_EQ_MODE_EN	    
 		#define CFG_FUNC_EQMODE_FADIN_FADOUT_EN    //EQﾄ｣ﾊｽﾇﾐｻｻﾊｱfade in/fade outｴｦﾀ晥ｦﾄﾜﾑ｡ﾔ�ｵﾚEQﾄ｣ﾊｽﾖﾐﾓﾐPOPﾉｱ｣ｬｽｨﾒ魘ｪ 		
    #endif
	#define CFG_FUNC_MUSIC_TREB_BASS_EN    //Musicｸﾟｵﾍﾒﾚｹｦﾄﾜﾅ葷ﾃ 
    //#define CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN     //ﾎﾞﾐﾅｺﾅﾗﾔｶｯｹﾘｻ彧ｦﾄﾜ｣ｬ
    #ifdef CFG_FUNC_SILENCE_AUTO_POWER_OFF_EN      
		#define  SILENCE_THRESHOLD                 120        //ﾉ靹ﾃﾐﾅｺﾅｼ�篥ﾅﾏﾞ｣ｬﾐ｡ﾓﾚﾕ篋ｵﾈﾏﾎｪﾎﾞﾐﾅｺﾅ
		#define  SILENCE_POWER_OFF_DELAY_TIME      10*60*100     //ﾎﾞﾐﾅｺﾅｹﾘｻ昉ﾓﾊｱﾊｱｼ茱ｬｵ･ﾎｻ｣ｺms
    #endif

	#define CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN//ﾔﾚﾏﾟｵ�
	#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN

		//**ﾒｵSDKｰ豎ｾｺﾅ,ｲｻﾒｪﾐﾞｸﾄ**/
		#define  CFG_EFFECT_MAJOR_VERSION						(CFG_SDK_MAJOR_VERSION)
		#define  CFG_EFFECT_MINOR_VERSION						(CFG_SDK_MINOR_VERSION)
		#define  CFG_EFFECT_USER_VERSION						(CFG_SDK_PATCH_VERSION)
	
		/**ﾔﾚﾏﾟｵｲｼ�ﾓｿﾚﾓﾐUSB HIDｽﾓｿﾚ｣ｬｻﾟUARTｽﾓｿﾚ*/
		/**ｽｨﾒ鯡ｹﾓﾃUSB HIDｽﾓｿﾚ｣ｬﾊﾕｷ｢bufｹｲ512Byte*/
		/**UARTｽﾓｿﾚﾕｼﾓﾃ2ﾂｷDMA｣ｬﾊﾕｷ｢Bufｹｲ2k Byte*/
		/**ﾊｹﾓﾃUSB HIDﾗｪｵﾓｿﾚ｣ｬDMAﾗﾊﾔｴｽﾅ*/
		#define  CFG_COMMUNICATION_BY_USB		// usb or uart

//		#define	 CFG_UART_COMMUNICATION_TX_PIN					GPIOA10
//		#define  CFG_UART_COMMUNICATION_TX_PIN_MUX_SEL			(3)
//		#define  CFG_UART_COMMUNICATION_RX_PIN					GPIOA9
//		#define  CFG_UART_COMMUNICATION_RX_PIN_MUX_SEL			(1)

		#define  CFG_COMMUNICATION_CRYPTO						(0)////ｵｨﾑｶｼﾓﾃﾜ=1 ｵｨﾑｶｲｻｼﾓﾃﾜ=0
		#define  CFG_COMMUNICATION_PASSWORD                     0x11223344//////ﾋﾄﾗﾖｽﾚｵﾄｳ､ｶﾈﾃﾜﾂ�
	#endif

	//ﾊｹﾓﾃflashｴ貅ﾃｵﾄｵﾎﾊ�
	//ﾒｧｲﾎﾊ�豢｢ﾓﾚflashｹﾌｶｨﾇﾐ
	//#define CFG_EFFECT_PARAM_IN_FLASH_EN
	#ifdef CFG_EFFECT_PARAM_IN_FLASH_EN
		#define CFG_EFFECT_PARAM_IN_FLASH_MAX_NUM				(10)//ﾖｧｳﾖflashｲﾎﾊ�鋗鯡�ｬflashｿﾕｼ菴ﾅﾊｱｽｨﾒ魏ｴﾐ霰葷ﾃ
		#define	CFG_EFFECT_PARAM_IN_FLASH_ACTIVCE_NUM			(6)
		#ifdef CFG_FUNC_AUDIO_EFFECT_ONLINE_TUNING_EN
			#define CFG_EFFECT_PARAM_UPDATA_BY_ACPWORKBENCH			//ｴｪｸﾃｺ�ﾉﾒﾔﾍｨｹ�CPWorkBench｣ｬｶ猾釚ﾄ4156 Byte RAM｣ｬﾁｿｲ摠ｱﾈ郢炫AMｽﾅｿﾉﾒﾔｹﾘｱﾕﾕ篋�
		#endif
	#endif

#endif

//****************************************************************************************
//     ﾒｵﾏ犹ﾘﾅ葷ﾃｲﾎﾊ�
//****************************************************************************************
#define	CFG_PARA_SAMPLE_RATE				(44100)//(48000)//
#define CFG_BTHF_PARA_SAMPLE_RATE			(16000)//ﾀｶﾑﾀHFPﾄ｣ﾊｽﾏﾂﾍｳﾒｻｲﾉﾑﾊﾎｪ16K
#define	CFG_PARA_SAMPLES_PER_FRAME          (256)//(512)	//ﾏｵﾍｳﾄｬﾈﾏｵﾄﾖ｡ﾊ�｡ //ﾗ｢:ｹﾘｱﾕﾗﾜﾒｧｿｪｹﾘﾊｱ,ｱﾘﾐ�葷ﾃﾎｪ256,ｺﾍﾍｨｻｰsampleﾅ葷ﾃｲﾎﾊ�｣ｳﾖﾒｻﾖﾂ
#define	CFG_BTHF_PARA_SAMPLES_PER_FRAME     (256)			//ﾀｶﾑﾀﾍｨｻｰﾄ｣ﾊｽﾏﾂﾖ｡ﾊ�｡
#define CFG_PARA_MIN_SAMPLES_PER_FRAME		(256)//         //ﾏｵﾍｳﾖ｡ﾗ隯｡ﾖｵ｣ｬｱ｣ﾖ､mic delayﾗ隯｡,ﾗ｢ﾒ�ｸﾄﾎｪ128ﾊｱ twsｴｧｲ･Uﾅﾌmipsﾓ狠ｿｲｻﾗ罍｣
#define CFG_PARA_MAX_SAMPLES_PER_FRAME		(512)//(512)

#if (BT_AVRCP_VOLUME_SYNC == ENABLE) && defined(CFG_APP_BT_MODE_EN)
#define CFG_PARA_MAX_VOLUME_NUM		        (16)	//SDK 16 ｼｶﾒｿ,ﾕ�ﾔiphoneﾊﾖｻ惕ｶﾑﾀﾒｿﾍｬｲｽｹｦﾄﾜｶｨﾖﾆ｣ｬﾒｿｱ�6ｼｶﾄﾜﾒｻﾒｻｶﾔﾓｦﾊﾖｻ巐ﾋﾒｿｼｶｱ�
#define CFG_PARA_SYS_VOLUME_DEFAULT			(12)	//SDKﾄｬﾈﾏﾒｿ
#else
#define CFG_PARA_MAX_VOLUME_NUM		        (32)	//SDK 32 ｼｶﾒｿ
#define CFG_PARA_SYS_VOLUME_DEFAULT			(25)	//SDKﾄｬﾈﾏﾒｿ
#endif

//****************************************************************************************
//     ﾗｪｲﾉﾑｦﾄﾜﾑ｡ﾔ�
//ﾋｵﾃ�
//    1.ﾊｹﾄﾜｸﾃｺ�桄ｾﾏｵﾍｳｻ眇ｫｻ瞑ﾔﾍｳﾒｻｵﾄｲﾉﾑﾊﾊ莎ｬﾄｬﾈﾏﾊｹﾓﾃ44.1KHz;
//    2.ｴﾋｰ豎ｾﾄｬﾈﾏｴｪ｣ｬﾇ�ﾘｱﾕ!!!!!!!!!!
//****************************************************************************************	
#define	CFG_FUNC_MIXER_SRC_EN

//     ｲﾉﾑﾊﾓｲｼ�｢ｵｦﾄﾜﾑ｡ﾔ�
//ﾋｵﾃ�
//	     ﾓｲｼ�｢ｵｬﾒｻﾊｱｿﾌﾖｻｿﾉﾊｹﾄﾜｿｪﾆｻｸ｢ｵ｣ﾊｹﾏｵﾍｳAUPLLﾊｱﾖﾓｸ撝贅ｴ
//****************************************************************************************	
//#define	CFG_FUNC_FREQ_ADJUST
#ifdef CFG_FUNC_FREQ_ADJUST
	#define CFG_PARA_BT_FREQ_ADJUST		//Btplay ﾄ｣ﾊｽﾐ貳ﾚｼ�ﾓｲｼ�｢ｵｬﾓ�FG_PARA_BT_SYNCｺ�莠ﾏ
 	#define CFG_PARA_HFP_FREQ_ADJUST	//ﾍｨｻｰﾄ｣ﾊｽ ﾐ貳ﾚｼ�ﾓｲｼ�｢ｵ� ﾊｹﾓﾃﾉﾏﾐﾐﾎ｢ｵｬﾏﾂﾐﾐｸ撝譯｣ ﾓ�FG_PARA_HFP_SYNCﾅ莠ﾏ
#endif

//****************************************************************************************
//                 ﾂｼﾒｦﾄﾜﾅ葷ﾃ
//****************************************************************************************
//#define CFG_FUNC_RECORDER_EN     // ｴﾋｹｦﾄﾜﾔﾝｲｻｿﾉﾓﾃ｣ｬｴ�ﾅｻｯ
#ifdef CFG_FUNC_RECORDER_EN
	#define CFG_FUNC_RECORD_SD_UDISK	//ﾂｼﾒｽSDｿｨｻﾟUﾅﾌ
	//#define CFG_FUNC_RECORD_FLASHFS 	//ｲｻｿﾉﾍｬﾊｱｿｪﾆ�CFG_FUNC_RECORD_SD_UDISK
//ﾔﾝｲｻﾖｧｳﾖflash
//#ifdef CFG_APP_FLASH_FATFS_PLAY_MODE_EN
//	//#define CFG_FUNC_RECORD_FLASHFATFS	//rec to extern fatfs
//#endif

	#ifdef CFG_FUNC_RECORD_SD_UDISK
		#define CFG_FUNC_RECORD_UDISK_FIRST				//Uﾅﾌｺﾍｿｨﾍｬﾊｱｴ贇ﾚﾊｱ｣ｬﾂｼﾒ雎ｸﾓﾅﾏﾈﾑ｡ﾔﾅﾌ｣ｬｷﾅﾏﾈﾑ｡ﾔｼﾒｽSDｿｨ｡｣
		#define CFG_PARA_RECORDS_FOLDER 		"REC"	//ﾂｼｿｨﾂｼUﾅﾌﾊｱｸｿﾂｼﾎﾄｼ�ﾐ｡｣ﾗ｢ﾒ稠fpresearch_init ﾊｹﾓﾃｻﾘｵｯﾊ��ﾋﾗﾖｷ逸ｮ｡｣
		#define CFG_FUNC_RECORDS_MIN_TIME		1000	//ｵ･ﾎｻms｣ｬｿｪﾆﾋｺ�ｬﾐ｡ﾓﾚﾕ篋､ｶﾈｵﾄﾗﾔｶｯﾉｾｳ�｣
		#define CFG_PARA_REC_MAX_FILE_NUM       256     //ﾂｼﾒﾄｼ�鋗�ｿ

		#define MEDIAPLAYER_SUPPORT_REC_FILE            // UﾅﾌｻFｿｨﾄ｣ﾊｽﾏﾂ｣ｬｴｪｴﾋｹｦﾄﾜ｣ｬﾔｧｳﾖｲ･ｷﾅﾂｼﾒﾄｼ�ｻｷｬﾖｻﾄﾜﾔﾚﾂｼﾒﾘｷﾅﾄ｣ﾊｽﾏﾂｲ･ｷﾅﾂｼﾒﾄｼ�
        //#define AUTO_DEL_REC_FILE_FUNCTION            //ﾂｼﾒﾄｼ��ｽﾗ鋗�ｬﾗﾔｶｯﾉｾｳ�ｫｲｿﾂｼﾒﾄｼ�ﾄｹｦﾄﾜﾑ｡ﾏ�
	#endif

	#define CFG_PARA_REC_GAIN		        (8191)	    //ﾊ菠�ｼﾒ�  8191:+6db;7284:+5db;6492:+4db;5786:+3db;5157:+2db;4596:+1db;4095:0db;

	#define DEL_REC_FILE_EN


	/*ﾗ｢ﾒ稠lashｿﾕｼ茱ｬｱﾜﾃ箋袁ｻ   middleware/flashfs/file.h FLASH_BASE*/
	#ifdef CFG_FUNC_RECORD_FLASHFS
		#define CFG_PARA_FLASHFS_FILE_NAME		"REC.MP3"//RECORDER_FILE_NAME
	#endif

	//N >= 2 ｣ｻｿｼﾂﾇ128ﾏｵﾍｳﾖ｡ﾒﾔｼｰｼﾓﾒｧMIPSｽﾏｸﾟ｣ｬﾓﾅﾏﾈｼｶﾎｪ3ｵﾄｱ狡�ﾌｴｦﾀ桄�ﾝｽﾏﾂ�ｬﾍﾆｼｵﾎｪ 6｡｣ﾌ盧ﾟﾏｵﾍｳﾖ｡｣ｬmipsｵﾍﾊｱｿﾉﾒﾔｵ｡N,ｽﾚﾔｼram｡｣
	#define MEDIA_RECORDER_FIFO_N				6
	#define MEDIA_RECORDER_FIFO_LEN				(CFG_PARA_MAX_SAMPLES_PER_FRAME * MEDIA_RECORDER_CHANNEL * MEDIA_RECORDER_FIFO_N)
	//ｵ鋧ﾂﾁﾐｲﾎﾊ�ｬﾂｼﾒ鰒ﾊｿﾉﾄﾜﾐ靨ｪﾖﾘﾗ貶ﾝﾐﾔｲ簗ﾔ ﾊﾊﾅ腱ILE_WRITE_FIFO_LEN｡｣
	#define MEDIA_RECORDER_CHANNEL				2
	#define MEDIA_RECORDER_BITRATE				96 //Kbps
	#define MEDIA_RECODER_IO_BLOCK_TIME			1000//ms
	//FIFO_Len=(ﾂ�ﾊ(96) / 8 * ｻｺｳ衞ｱｼ舂s(1000) ｣ｨﾂ�ﾊｵ･ﾎｻKbps,ｵﾈﾐｧｺﾁﾃ�ｩ
	//ｸﾝSDIOﾐｭﾒ鬟ｬﾐｴｿｨﾗ霾逸贇ﾚ250*2msﾗ霾�ｿﾉﾄﾜ｣ｬﾊｵｲ箚ｿｷﾖUﾅﾌｴ贇ﾚ785msﾖﾜﾆﾚﾐﾔﾐｴﾈ�霾訒ｬﾒｪﾇ狡��ﾝfifoｿﾕｼ�ﾈｷｱ｣ｳｬｹ�篋､ｶﾈｵﾄﾁｽｱｶ(ｺｬﾍｬｲｽ)｡｣
	#define FILE_WRITE_FIFO_LEN					((((MEDIA_RECORDER_BITRATE / 8) * MEDIA_RECODER_IO_BLOCK_TIME ) / 512) * 512)//(ﾒﾀｾﾝUﾅﾌ/Cardｼ貶ﾝﾐﾔﾐ霽ﾍRAMﾗﾊﾔｴｿﾉﾑ｡400~1500ms｡｣ｰｴﾉﾈﾇ�12ｶﾔﾆ�
#endif //CFG_FUNC_RECORDER_EN
//****************************************************************************************
//                 UﾅﾌｻDｿｨﾄ｣ﾊｽﾏ犹ﾘｹｦﾄﾜﾅ葷ﾃ
//    
//****************************************************************************************
#if(defined(CFG_APP_USB_PLAY_MODE_EN) || defined(CFG_APP_CARD_PLAY_MODE_EN) || BT_AVRCP_SONG_TRACK_INFOR)
/**LRCｸ雍ﾊｹｦﾄﾜ **/
//#define CFG_FUNC_LRC_EN			 	// LRCｸ雍ﾊﾎﾄｼ�簧�

/*------browser function------*/
//#define FUNC_BROWSER_PARALLEL_EN  		//browser Parallel
//#define FUNC_BROWSER_TREE_EN  			//browser tree
#if	defined(FUNC_BROWSER_PARALLEL_EN)||defined(FUNC_BROWSER_TREE_EN)
#define FUNCTION_FILE_SYSTEM_REENTRY
#if defined(FUNC_BROWSER_TREE_EN)||defined(FUNC_BROWSER_PARALLEL_EN)
#define GUI_ROW_CNT_MAX		5		//ﾗ鋐猾ﾔﾊｾｶ猖ﾙﾐﾐ
#else
#define GUI_ROW_CNT_MAX		1		//ﾗ鋐猾ﾔﾊｾｶ猖ﾙﾐﾐ
#endif
#endif
/*------browser function------*/

/**ﾗﾖｷ﨣狡�獎ﾍﾗｪｻｻ **/
/**ﾄｿﾇｰﾖｧｳﾖUnicode     ==> Simplified Chinese (DBCS)**/
/**ﾗﾖｷ錥ｪｻｻｿ簽ﾉfatfsﾌ盪ｩ｣ｬｹﾊﾐ靨ｪｰ�ｬﾎﾄｼ�ｵﾍｳ**/
/**ﾈ郢鋓ｧｳﾖﾗｪｻｻﾆ萢﨧�ﾔ｣ｬﾐ靨ｪﾐﾞｸﾄfatfsﾅ葷ﾃｱ�*/
/**ﾗ｢ﾒ筌ｺ ﾖｧｳﾖﾗﾖｿ篁ｦﾄﾜﾐ靨ｪﾊｹﾓﾃ flashﾈﾝﾁｿ>=2Mｵﾄﾐｾﾆｬ**/

//#define CFG_FUNC_STRING_CONVERT_EN	// ﾖｧｳﾖﾗﾖｷ﨣狡�ｪｻｻ

/**ﾈ｡ﾏ涬A55ﾅﾐｶﾏ**/
/*fatfsﾄﾚｴﾅﾅﾌﾏｵﾍｳMBRｺﾍDBRﾉﾈﾇ睾ｲﾓﾐｴﾋｱ�ﾇｼ�筌ｬﾎｪﾌ盧ﾟｷﾇｱ�獎ﾍﾅﾌｼ貶ﾝﾐﾔ｣ｬｿﾉｿｪﾆﾋﾏ� ﾎｪﾓﾐﾐｧｼｨﾎﾞﾐｧﾅﾌ｣ｬｴﾋﾏ鍗ｬﾈﾏｹﾘｱﾕ*/
//#define	CANCEL_COMMON_SIGNATURE_JUDGMENT
//#define FUNC_UPDATE_CONTROL   //ﾉ�ｶｽｻｻ･ｹ�ﾌｿﾘﾖﾆ(ﾍｨｹ�ｴｼ�ｷﾈﾏﾉ�ｶ)
#endif

/**USB Hostｼ�篁ｦﾄﾜ**/
#if(defined(CFG_APP_USB_PLAY_MODE_EN))
#define CFG_RES_UDISK_USE
#define CFG_FUNC_UDISK_DETECT
#endif

/**USB Deviceｼ�篁ｦﾄﾜ**/
#if (defined (CFG_APP_USB_AUDIO_MODE_EN)) || (defined(CFG_COMMUNICATION_BY_USB))
	#define CFG_FUNC_USB_DEVICE_EN
#endif


//****************************************************************************************
//                 BTｹｦﾄﾜﾅ葷ﾃ
//ﾋｵﾃ�
//    1.ﾀｶﾑﾀﾏ犹ﾘｹｦﾄﾜﾅ葷ﾃﾏｸｽﾚﾇ�ﾚbt_config.hﾖﾐｶｨﾖﾆ!!!!!!!!!!
//    
//****************************************************************************************
#include "bt_config.h"
#ifdef BT_TWS_SUPPORT
// sync device
#ifdef	CFG_RES_AUDIO_DAC0_EN
	#define TWS_DAC0_OUT	1
#endif
#ifdef CFG_RES_AUDIO_DACX_EN
	#define TWS_DACX_OUT	2
#endif
#ifdef CFG_RES_AUDIO_I2SOUT_EN
	#if (CFG_RES_I2S == 0)
		#define TWS_IIS0_OUT	3
	#else
		#define TWS_IIS1_OUT	4
	#endif
#endif

	#define TWS_SINK_DEV_FRAME			(CFG_PARA_MAX_SAMPLES_PER_FRAME)
	#define TWS_SINK_DEV_FIFO_SAMPLES	(TWS_SINK_DEV_FRAME * 2)
//Key device for sync
	#define TWS_AUDIO_OUT_PATH	TWS_DAC0_OUT//TWS_IIS0_OUT//TWS_IIS1_OUT

#else
	#define TWS_AUDIO_OUT_PATH	0xff//ﾊﾍｷﾅﾒｵﾄﾚｴ�
#endif



//****************************************************************************************
//****************************************************************************************
/**OSｲﾙﾗｵﾍｳｽ�DLEﾊｱｾｭcoreｽ�ﾝﾃﾟﾗｴﾌｬ｣ｬﾒﾔｴ�ｽｽｵｵﾍｹｦｺﾄﾄｿｵﾄ**/
/*ﾗ｢ﾒ筌ｬﾕ簗ﾇOSｵﾈｵﾄIDLE｣ｬｲ｢ｷﾇﾓｦﾓﾃｲ紜PPMODE｣ｬﾓｦﾓﾃｲ耒ﾞﾐ雹ﾘﾐﾄ*/
#define CFG_FUNC_IDLE_TASK_LOW_POWER
#ifdef	CFG_FUNC_IDLE_TASK_LOW_POWER
	#define	CFG_GOTO_SLEEP_USE
#endif

//****************************************************************************************
//                 SHELLｹｦﾄﾜﾅ葷ﾃ
//ﾋｵﾃ�
//    1.shellｹｦﾄﾜﾆﾃﾒﾔｼｰﾅ葷ﾃﾇ�ｽshell.cﾖﾐﾅ葷ﾃ;
//    2.shellｹｦﾄﾜﾄｬﾈﾏｹﾘｱﾕ｣ｬﾄｬﾈﾏﾊｹﾓﾃUART1;
//****************************************************************************************
//#define CFG_FUNC_SHELL_EN

//****************************************************************************************
//                 UART DEBUGｹｦﾄﾜﾅ葷ﾃ
//ﾗ｢ﾒ筌ｺDEBUGｴｪｺｬｻ瞞icﾍｨﾂｷｵﾄdelay｣ｬｲｻﾐ靨ｪDEBUGｵﾔｴ惲�ｱ｣ｬｽｨﾒ鮖ﾘｱﾕｵ｡
//****************************************************************************************
#define CFG_FUNC_DEBUG_EN
#ifdef CFG_FUNC_DEBUG_EN
	//#define CFG_USE_SW_UART
	#ifdef CFG_USE_SW_UART
		#define SW_UART_IO_PORT				    SWUART_GPIO_PORT_A//SWUART_GPIO_PORT_B
		#define SW_UART_IO_PORT_PIN_INDEX	    1//bit num
		#define  CFG_SW_UART_BANDRATE   		512000//software uart baud rate select:38400 57600 115200 256000 512000 1000000 ,default 512000
	#else
		#define CFG_UART_TX_PORT 				(0)//tx port  0--A6｣ｬ1--A10, 2--A25, 3--A0, 4--A1
		#define  CFG_UART_BANDRATE   			115200//hardware uart baud set
	#endif
#endif

//****************************************************************************************
//                ﾌ睫ｾﾒｦﾄﾜﾅ葷ﾃ
//ﾋｵﾃ�
//    1.ﾉﾕﾂｼｹ､ｾﾟｲﾎｼ濵Vs26_SDK\tools\script｣ｻ
//    2.ﾌ睫ｾﾒｦﾄﾜｿｪﾆｬﾗ｢ﾒ稠lashﾖﾐconst dataﾌ睫ｾﾒ�ﾝﾐ靨ｪﾔ､ﾏﾈﾉﾕﾂｼ｣ｬｷｻｻ盍･ｷﾅ;
//    3.const dataﾊ�ﾝｿｪｻ恝�鬟ｬﾓｰﾏ�ｪｻ撝ﾙｶﾈ｣ｬﾖｪﾓﾃﾓﾚﾑ鰒､｡｣
//****************************************************************************************
#define CFG_FUNC_REMIND_SOUND_EN
#ifdef CFG_FUNC_REMIND_SOUND_EN
	#define CFG_PARAM_FIXED_REMIND_VOL   	10		//ｹﾌｶｨﾌ睫ｾﾒｿﾖｵ,0ｱ桄ｾﾊﾜmusic volﾍｬｲｽｿﾘﾖﾆ
#endif

//****************************************************************************************
//                 ｶﾏｵ羮ﾇﾒ荵ｦﾄﾜﾅ葷ﾃ        
//****************************************************************************************
#define CFG_FUNC_BREAKPOINT_EN
#ifdef CFG_FUNC_BREAKPOINT_EN
	#define BP_PART_SAVE_TO_NVM			// ｶﾏｵ耙ﾅﾏ｢ｱ｣ｴ豬ｽNVM
	#define BP_SAVE_TO_FLASH			// ｶﾏｵ醪ﾅﾏ｢ｱ｣ｴ豬ｽFlash
	#define FUNC_MATCH_PLAYER_BP		// ｻ｡FSﾉｨﾃ霄�･ｷﾅﾄ｣ﾊｽｶﾏｵ耙ﾅﾏ｢ﾆ･ﾅ莊ﾄﾎﾄｼ�｣ﾎﾄｼ�ﾐｺﾍIDｺﾅ
#endif

//****************************************************************************************
//                 Key ｰｴｼ�犹ﾘﾅ葷ﾃ
//****************************************************************************************
/**ｰｴｼ�eepﾒｦﾄﾜ**/
//#define  CFG_FUNC_BEEP_EN
    #define CFG_PARA_BEEP_DEFAULT_VOLUME    15//ﾗ｢ﾒ�ﾈｶﾑﾀﾒｿﾍｬｲｽｹｦﾄﾜｿｪﾆｬｴﾋﾖｵﾗ鋗ｪ16

/**ｰｴｼ�ｫｻｦﾄﾜ**/
#define  CFG_FUNC_DBCLICK_MSG_EN
#ifdef CFG_FUNC_DBCLICK_MSG_EN
	#define  CFG_PARA_CLICK_MSG             MSG_PLAY_PAUSE //ｵ･ｻｴﾐﾐﾏ鋧｢
	#define  CFG_PARA_DBCLICK_MSG           MSG_BT_HF_REDAIL_LAST_NUM   //ﾋｫｻｴﾐﾐﾏ鋧｢
	#define  CFG_PARA_DBCLICK_DLY_TIME      35       //ﾋｫｻﾐﾐｧｼ荳ｱｼ�4ms*20=80ms
#endif

/**ADCｰｴｼ�*/
#define CFG_RES_ADC_KEY_SCAN				//ﾔﾚdevice service ﾖﾐﾆﾃKeyﾉｨﾃ錢DCKEY
#if defined(CFG_RES_ADC_KEY_SCAN) || defined(CFG_PARA_WAKEUP_SOURCE_ADCKEY)
	#define CFG_RES_ADC_KEY_USE				//ADCｰｴｼ�ｦﾄﾜ ﾆﾃ
#endif

/**IRｰｴｼ�*/
#define CFG_RES_IR_KEY_SCAN				//ﾆﾃdevice service Keyﾉｨﾃ鍼RKey

/**ｱ狡��･ｰｴｼ�*/
//#define	CFG_RES_CODE_KEY_USE

/**GPIOｰｴｼ�*/
//#define CFG_RES_IO_KEY_SCAN	

/**ｵ醫ｻﾆｦﾄﾜﾑ｡ﾔ�*/
//#define CFG_ADC_LEVEL_KEY_EN 

//***************************************************************************************
//					RTC/ﾄﾖﾖﾓｹｦﾄﾜﾅ葷ﾃ
//ﾗ｢ﾒ�
//   1.RTCﾊｱﾖﾓﾔｴﾑ｡ﾔ�2Kｾｧﾕｽｰｸ｣ｬﾖｻﾓﾐBP1064L2ﾐﾍｺﾅﾖｧｳﾖ;
//   2.ﾆ萢鋗ｾﾆｬﾐﾍｺﾅ｣ｬRTCﾊｱﾖﾓﾔｴﾄｬﾈﾏﾑ｡ﾔcﾊｱﾖﾓ｣ｨｾｫｶﾈｵﾍ｣ｩ｣ｻ
//   3.ﾆ萢鋗ｾﾆｬﾐﾍｺﾅ｣ｬﾍﾆｼ�2Mｾｧﾕ｣ﾊｽ｣ｬdeepsleepｹｦｺﾄﾓﾐﾋｧｸﾟ
//	 4.ｴｪRTCｹｦﾄﾜ
//		a.ﾐ靨ｪｹﾘｱﾕTWSﾄ｣ﾊｽ,ｴｪCFG_FUNC_RTC_ENｺ�
//		b.ﾊﾖｶｯﾉｾｳ�T_Audio_APP\middleware\bluetoothﾎﾄｼ�ﾐﾏﾂlibtws.aｿ筌ｬｲ｢ﾇﾒﾔﾚｹ､ｳﾌﾅ葷ﾃﾖﾐﾈ･ｵwsｿ箏ﾄﾒ�ﾃ
//***************************************************************************************
#ifndef BT_TWS_SUPPORT
#ifdef CFG_CHIP_BP1064L2
	#define CFG_FUNC_RTC_EN
#else
	#ifndef CFG_FUNC_BACKUP_EN
		//#define CFG_FUNC_RTC_EN
	#endif
#endif

#ifdef CFG_FUNC_RTC_EN
	#define CFG_RES_RTC_EN
	#ifdef CFG_RES_RTC_EN
		#define CFG_PARA_RTC_12M 	//ﾔ､ﾁ葷ﾃｲﾎﾊ�
	#endif
	
	#define CFG_FUNC_ALARM_EN  //ﾄﾖﾖﾓｹｦﾄﾜ,ｱﾘﾐ�ｪﾊｱﾖﾓ
	#define CFG_FUNC_LUNAR_EN  //ﾍ��ｱﾘﾐ�ｪﾊｱﾖﾓ
	#ifdef CFG_FUNC_ALARM_EN
		#define CFG_FUNC_SNOOZE_EN //ﾄﾖﾖﾓﾌｰﾋｯｹｦﾄﾜ
	#endif
#endif
#endif

//****************************************************************************************
//                Display ﾏﾔﾊｾﾅ葷ﾃ
//****************************************************************************************
//#define CFG_FUNC_DISPLAY_TASK_EN	//display task
//#define CFG_FUNC_DISPLAY_EN
#ifdef CFG_FUNC_DISPLAY_EN
//  #define  DISP_DEV_SLED
  #define  DISP_DEV_7_LED
/**8ｶﾎLEDﾏﾔﾊｾｲﾙﾗ�*/
/*LEDﾏﾔｴ賤｢ﾐﾂﾐ靨ｪﾔﾚTimer1msﾖﾐｶﾏｽﾐ｣ｬｶﾁﾐｴflashｲﾙﾗｱｻ盪ﾘｱﾕﾖﾐｶﾏ*/
/*ﾋﾔﾐ靨ｪﾗﾘﾊ箒ｦﾀ惞ｬﾇ�ﾘﾗ｢ｸﾃｺ��ｬｵﾄｴ惲�ﾎ*/
/*ﾗ｢ﾒ穰imerﾖﾐｶﾏｷ�ｯﾊ�ﾍｵﾃｵｽｵﾄAPIｱﾘﾐ��CM｣ｬｺｬｵﾃｵﾄﾋﾐapi｣ｬｿ篌ｯﾊ��ﾉﾑｯﾖｧｳﾖ*/
/*ｿｪﾆﾋｺ凜ｬﾒｪｹﾘﾗ｢ﾋﾐﾊｹﾓﾃNVIC_SetPriority ﾉ靹ﾃﾎｪ0ｵﾄｴ惲�ｬｱﾜﾃ筝ﾔﾓｦﾖﾐｶﾏｵﾃｷﾇTCMｴ惲��ﾀｻ弴ｴﾎｻ*/
#ifdef DISP_DEV_7_LED
  #define	CFG_FUNC_LED_REFRESH
#endif

#if defined(DISP_DEV_SLED) && defined(DISP_DEV_7_LED) && defined(LED_IO_TOGGLE)
   #error Conflict: display setting error //ｲｻﾄﾜﾍｬﾊｱﾑ｡ﾔｽﾖﾖﾏﾔﾊｾﾄ｣ﾊｽ
#endif
#endif

//****************************************************************************************
//				   ｶ忞嵂蟆ﾎｼ�篁ｦﾄﾜﾑ｡ﾔ靹ﾃ
//****************************************************************************************
//#define  CFG_FUNC_DETECT_PHONE_EN                            

//****************************************************************************************
//				   3ﾏﾟ｣ｬ4ﾏﾟｶ忞惕獎ﾍｼ�篁ｦﾄﾜﾑ｡ﾔ靹ﾃ
//****************************************************************************************
//#define  CFG_FUNC_DETECT_MIC_SEG_EN  

//****************************************************************************************
//                            ﾅ葷ﾃｳ袁ｻ ｱ默�ｯｸ�
//****************************************************************************************

#if defined(CFG_FUNC_SHELL_EN) && defined(CFG_USE_SW_UART)
#error	Conflict: shell  X  SW UART No RX!
#endif

// ﾊｹﾄﾜﾗﾔｶｯｻｯｲ簗ﾔ
//#define AUTO_TEST_ENABLE

// ﾊｹﾄﾜﾀｶﾑﾀOTA
// OTAｴｪﾗﾔｶｯｿｪﾆｶﾑﾀSPP
//#define  CFG_FUNC_BT_OTA_EN

#include "sys_gpio.h"


//************************************************************************************************************
//dumpｹ､ｾﾟ,ｿﾉﾒﾔｽｫﾊ�ﾝｷ｢ﾋﾍｵｽdumpｹ､ｾﾟ,ﾓﾃﾓﾚｷﾖﾎ�
//************************************************************************************************************

//#define CFG_DUMP_DEBUG_EN
#ifdef CFG_DUMP_DEBUG_EN

//UARTﾅ葷ﾃ,ﾐ靨ｪｺﾍｴｮｿﾚﾈﾕﾖｾｴ｡ﾎｪｲｻﾍｬｵﾄUARTﾗ�
//UART0: A0/A1/A6
//UART1: A10/A25/A19
	#define CFG_DUMP_UART_TX_PORT 				(4)//tx port  0--A6｣ｬ1--A10, 2--A25, 3--A0, 4--A1, 5--A19
#if ((CFG_DUMP_UART_TX_PORT == 1) || (CFG_DUMP_UART_TX_PORT == 2) || (CFG_DUMP_UART_TX_PORT == 5))
	#define CFG_DUMP_UART_TX_PORT_GROUP			(1)
#else
	#define CFG_DUMP_UART_TX_PORT_GROUP			(0)
#endif
	#define CFG_DUMP_UART_BANDRATE 				(2000000)

	#define DUMP_UART_MODE_DAC					(0)
	#define DUMP_UART_MODE_LININ				(1)
	#define DUMP_UART_MODE_AEC					(2)
	#define DUMP_UART_MODE_SBC					(3)
	#define DUMP_UART_MODE_SCO					(4)

	#define CFG_DUMP_UART_MODE					(DUMP_UART_MODE_DAC)

	//ﾅ葷ﾃｳ袁ｻ ｱ默�ｯｸ�
	//DUMP_DEBUG ｺﾍ UART DEBUG ｲｻﾄﾜﾊｹﾓﾃﾍｬﾒｻﾗ魘ｮｿﾚ
#ifdef CFG_FUNC_DEBUG_EN
	#if(((CFG_UART_TX_PORT == 1) || (CFG_UART_TX_PORT == 2) || (CFG_UART_TX_PORT == 5)) && ((CFG_DUMP_UART_TX_PORT == 1) || (CFG_DUMP_UART_TX_PORT == 2) || (CFG_DUMP_UART_TX_PORT == 5)))
		#error "Different ports are required !!!"
	#endif
	#if(((CFG_UART_TX_PORT == 0) || (CFG_UART_TX_PORT == 3) || (CFG_UART_TX_PORT == 4)) && ((CFG_DUMP_UART_TX_PORT == 0) || (CFG_DUMP_UART_TX_PORT == 3) || (CFG_DUMP_UART_TX_PORT == 4)))
		#error "Different ports are required !!!"
	#endif
#endif

	//a19 ﾊﾇusbｽﾓｿﾚｵﾄdp｣ｬﾊｹﾓﾃｴﾋioﾐ靨ｪｹﾘｱﾕﾋﾐusbｹｦﾄﾜ
#if(CFG_DUMP_UART_TX_PORT == 5)
	#ifdef CFG_APP_USB_PLAY_MODE_EN
		#error "Need Close CFG_APP_USB_PLAY_MODE_EN !!!"
	#endif
	#ifdef CFG_APP_USB_AUDIO_MODE_EN
		#error "Need Close CFG_APP_USB_AUDIO_MODE_EN !!!"
	#endif
	#ifdef CFG_COMMUNICATION_BY_USB
		#error "Need Close CFG_COMMUNICATION_BY_USB !!!"
	#endif
#endif

#endif /* CFG_DUMP_DEBUG_EN */


#endif /* APP_CONFIG_H_ */
