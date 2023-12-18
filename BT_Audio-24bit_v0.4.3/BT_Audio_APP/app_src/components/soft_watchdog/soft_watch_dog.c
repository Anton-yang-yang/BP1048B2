/*
 * soft_watch_dog.c
 *
 *  Created on: Nov 10, 2021
 *      Author: Ben
 */
#include "soft_watch_dog.h"
#include "watchdog.h"
#ifdef SOFT_WACTH_DOG_ENABLE

//小狗槽
volatile static uint32_t g_little_dog_group_1 = 0;
//小狗注册信息
volatile static uint32_t g_little_dog_adopt_mask = 0x2;


//注册小狗
void little_dog_adopt(DOG_INDEX dog_site)
{
	g_little_dog_adopt_mask |= dog_site;
}
//注销小狗
void little_dog_deadopt(DOG_INDEX dog_site)
{
	g_little_dog_adopt_mask &= ~dog_site;
}

//喂小狗
void little_dog_feed(DOG_INDEX dog_num)//uint32_t* little_dog_group,
{
	g_little_dog_group_1 |= dog_num;
}

//检查小狗都有心跳
//DOG_MASK 表征被激活的dog site
bool big_dog_check(uint32_t dog_mask)//uint32_t* little_dog_group,
{
	if((g_little_dog_group_1 & dog_mask) != dog_mask)
	{
		//DBG("Miss Little WatchDog: %x\n",DOG_MASK);
		return 0;
	}else
	{
		g_little_dog_group_1 = 0x0;
		return 1;
	}
}

//喂大狗
//最优是放在软件定时器中（可以支持某些超长时间间隔的喂狗）
void big_dog_feed(void)
{
	//static uint32_t feed_freq_count = 0;//example1
	//feed_freq_count ++;
	if(!big_dog_check(g_little_dog_adopt_mask))
	{
		return;
	}
	//if((feed_freq_count%100 == 0)&&(!big_dog_check(&g_little_dog_group_2,0x0000FFFF)))
//	if(!big_dog_check(&g_little_dog_group_2,0x0000FFFF))
//	{
//		return;
//	}

	WDG_Feed();
}

#endif //SOFT_WACTH_DOG_ENABLE
