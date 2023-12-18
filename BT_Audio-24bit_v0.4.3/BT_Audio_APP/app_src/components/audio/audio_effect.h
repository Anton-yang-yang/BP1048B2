/*******************************************************
 *         MVSilicon Audio Effects Process
 *
 *                All Right Reserved
 *******************************************************/

#ifndef __AUDIO_EFFECT_H__
#define __AUDIO_EFFECT_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

#include "audio_core_api.h"

//音效Apply抽象函数指针
typedef void (*AudioEffectApplyFunc)(void *effectUint, int16_t *pcm_in, int16_t *pcm_out, uint32_t n);


#define AUDIO_EFFECT_GROUP_NUM			3//音效列表组数
#define AUDIO_EFFECT_NODE_NUM			12//一组音效最大个数

#define AUDIO_EFFECT_SUM				(AUDIO_EFFECT_GROUP_NUM * AUDIO_EFFECT_NODE_NUM)

typedef enum __NodeType
{
	NodeType_Normal = 0,//典型
	NodeType_Bifurcate,	//一分为二
	NodeType_Group1,	//分叉后的第一组起始节点
	NodeType_Group2,	//分叉后的第二组起始节点
	NodeType_Group3,	//分叉后的第三组起始节点
	NodeType_Mix,		//分叉后合并节点
	NodeType_Vol,		//5
	NodeType_AEC,		//6,AEC为特殊算法，需要特殊处理
//	NodeType_TONE,		//7,TREB BASS调节
//	NodeType_EqMode,	//8,EQ模式切换		//优化EQ模式切换
} NodeType;

/*音效节点*/
typedef struct __EffectNode
{
	uint8_t					Enable;
	uint8_t					EffectType;		//音效类型
	uint8_t					NodeType;		//节点的类型,是否有分叉，是否合并，是否为分叉之后的另一组起始节点
	uint8_t					Width;			//音效的位宽
	uint8_t					Index;			//位于所有音效中的位置
	void*		 			EffectUnit; 	//数据域
	AudioEffectApplyFunc	FuncAudioEffect;//音效节点处理函数
} EffectNode;

typedef struct __EffectNodeList
{
	uint8_t		Channel;		//用于表示音效处理类型为单声道还是立体声，一组音效节点均需要保持一致
	EffectNode	EffectNode[AUDIO_EFFECT_NODE_NUM];
} EffectNodeList;

extern EffectNodeList  gEffectNodeList[AUDIO_EFFECT_GROUP_NUM];
//当前模式下使用的音效位于音效组合列表中的编号
extern uint8_t AudioModeIndex ;

//flash 存储音效解析, 申请音效节点
bool AudioEffectParsePackage(uint8_t* add, uint16_t PackageLen, uint8_t* CommParam, bool IsReload);

void EffectPcmBufClear(uint32_t SampleLen);
void EffectPcmBufMalloc(uint32_t SampleLen);
void AudioEffectsInit(void);
void AudioEffectsDeInit(void);
void AudioEffectsLoadInit(bool IsReload, uint8_t mode);
void AudioEffectProcess(AudioCoreContext *pAudioCore);
void AudioMusicProcess(AudioCoreContext *pAudioCore);
void AudioBypassProcess(AudioCoreContext *pAudioCore);
void AudioEffectProcessBTHF(AudioCoreContext *pAudioCore);
void AudioNoAppProcess(AudioCoreContext *pAudioCore);
void EffectPcmBufRelease(void);

void du_efft_fadein_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch);		
void du_efft_fadeout_sw(int16_t* pcm_in, uint16_t pcm_length, uint16_t ch);		

uint8_t FineAudioEffectParamasIndex(uint8_t mode);
uint8_t GetAudioEffectParamasIndex(void);
extern bool IsEffectChange;
extern bool IsEffectChangeReload;

void AudioAPPDigitalGianProcess(uint32_t AppMode);
#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__AUDIO_EFFECT_H__
