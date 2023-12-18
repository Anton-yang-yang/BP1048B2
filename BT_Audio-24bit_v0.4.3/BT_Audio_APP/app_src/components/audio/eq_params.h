#ifndef __EQ_PARAMS_H__
#define __EQ_PARAMS_H__

#ifdef  __cplusplus
extern "C" {
#endif//__cplusplus


#define EQ_PARAM_LEN   104//112//直接配置为固定值，EQ音效参数为固定值


//EQ style: Classical
extern const unsigned char Classical[104];

//EQ style: Vocal Booster
extern const unsigned char Vocal_Booster[104];

//EQ style: Flat
extern const unsigned char Flat[104];

//EQ style: Pop
extern const unsigned char Pop[104];

//EQ style: Rock
extern const unsigned char Rock[104];

//EQ style: Jazz
extern const unsigned char Jazz[104];


#ifdef  __cplusplus
}
#endif//__cplusplus

#endif//__EQ_PARAMS_H__
