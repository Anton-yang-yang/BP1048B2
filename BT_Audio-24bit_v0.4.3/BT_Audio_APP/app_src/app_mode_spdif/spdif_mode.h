#ifndef __SPDIF_MODE_H__
#define __SPDIF_MODE_H__

#ifdef __cplusplus
extern "C"{
#endif // __cplusplus

#define SPDIF_SOURCE_NUM				APP_SOURCE_NUM

bool SpdifPlayInit(void);
void SpdifPlayRun(uint16_t msgId);
bool SpdifPlayDeinit(void);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __SPDIF_MODE_H__

