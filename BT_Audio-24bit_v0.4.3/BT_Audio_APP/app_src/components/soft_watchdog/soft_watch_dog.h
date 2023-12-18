/*
 * soft_watch_dog.h
 *
 *  Created on: Nov 10, 2021
 *      Author: Ben
 */
#include "app_config.h"

//#define SOFT_WACTH_DOG_ENABLE
#ifdef SOFT_WACTH_DOG_ENABLE
typedef enum __DOG_INDEX{
	DOG_INDEX0_MainTask = (1<<0),                        /**<DOG bit0 macro*/
	DOG_INDEX1_ModeTask = (1<<1),                        /**<DOG bit1 macro*/
	DOG_INDEX2_AudioCoreTask = (1<<2),                        /**<DOG bit2 macro*/
	DOG_INDEX3_BtStackTask = (1<<3),                        /**<DOG bit3 macro*/
	DOG_INDEX4_SlowDeviceTask = (1<<4),                        /**<DOG bit4 macro*/
	DOG_INDEX5 = (1<<5),                        /**<DOG bit5 macro*/
	DOG_INDEX6 = (1<<6),                        /**<DOG bit6 macro*/
	DOG_INDEX7 = (1<<7),                        /**<DOG bit7 macro*/
	DOG_INDEX8 = (1<<8),                        /**<DOG bit8 macro*/
	DOG_INDEX9 = (1<<9),                        /**<DOG bit9 macro*/
	DOG_INDEX10 = (1<<10),                      /**<DOG bit10 macro*/
	DOG_INDEX11 = (1<<11),                      /**<DOG bit11 macro*/
	DOG_INDEX12 = (1<<12),                      /**<DOG bit12 macro*/
	DOG_INDEX13 = (1<<13),                      /**<DOG bit13 macro*/
	DOG_INDEX14 = (1<<14),                      /**<DOG bit14 macro*/
	DOG_INDEX15 = (1<<15),                      /**<DOG bit15 macro*/
	DOG_INDEX16 = (1<<16),                      /**<DOG bit16 macro*/
	DOG_INDEX17 = (1<<17),                      /**<DOG bit17 macro*/
	DOG_INDEX18 = (1<<18),                      /**<DOG bit18 macro*/
	DOG_INDEX19 = (1<<19),                      /**<DOG bit19 macro*/
	DOG_INDEX20 = (1<<20),                      /**<DOG bit20 macro*/
	DOG_INDEX21 = (1<<21),                      /**<DOG bit21 macro*/
	DOG_INDEX22 = (1<<22),                      /**<DOG bit22 macro*/
	DOG_INDEX23 = (1<<23),                      /**<DOG bit23 macro*/
	DOG_INDEX24 = (1<<24),                      /**<DOG bit24 macro*/
	DOG_INDEX25 = (1<<25),                      /**<DOG bit25 macro*/
	DOG_INDEX26 = (1<<26),                      /**<DOG bit26 macro*/
	DOG_INDEX27 = (1<<27),                      /**<DOG bit27 macro*/
	DOG_INDEX28 = (1<<28),                      /**<DOG bit28 macro*/
	DOG_INDEX29 = (1<<29),                      /**<DOG bit29 macro*/
	DOG_INDEX30 = (1<<30),                      /**<DOG bit30 macro*/
	DOG_INDEX31 = (1<<31),                      /**<DOG bit31 macro*/
}DOG_INDEX;

extern void little_dog_adopt(DOG_INDEX dog_site);
//×¢ÏúÐ¡¹·
extern void little_dog_deadopt(DOG_INDEX dog_site);
extern void little_dog_feed(DOG_INDEX dog_num);
extern void big_dog_feed(void);
#endif
