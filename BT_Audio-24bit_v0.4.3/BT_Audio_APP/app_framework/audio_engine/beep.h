/*******************************************************
 *         MVSilicon Audio Effects Parameters
 *
 *                All Right Reserved
 *******************************************************/

#ifndef __BEEP_H__
#define __BEEP_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus

void BeepEnable(void);
void Beep(int32_t *OutBuf,uint16_t n);

#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__BEEP_H__
