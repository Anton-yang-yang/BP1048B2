/**
 **************************************************************************************
 * @file    bluetooth_utility.h
 * @brief   
 *
 * @author  KK
 * @version V1.0.0
 *
 * $Created: 2021-4-18 18:00:00$
 *
 * @Copyright (C) Shanghai Mountain View Silicon Co.,Ltd. All rights reserved.
 **************************************************************************************
 */

#ifndef __BLUETOOTH_UTILITY_H__
#define __BLUETOOTH_UTILITY_H__

#include "type.h"


typedef struct btstack_item {
    struct btstack_item *next; // <-- next element in list, or NULL
} btstack_item_t;

typedef btstack_item_t * btstack_list_t;


/***********************************************************************************
 * bt stack list
 **********************************************************************************/
int  btstack_list_empty(btstack_list_t * list);

/***********************************************************************************
 * btstack_list_get_last_item
 **********************************************************************************/
btstack_item_t * btstack_list_get_last_item(btstack_list_t * list);


/***********************************************************************************
 * btstack_list_add
 **********************************************************************************/
void btstack_list_add(btstack_list_t * list, btstack_item_t *item);

/***********************************************************************************
 * btstack_list_add_tail
 **********************************************************************************/
void btstack_list_add_tail(btstack_list_t * list, btstack_item_t *item);

/***********************************************************************************
 * btstack_list_remove
 **********************************************************************************/
int  btstack_list_remove(btstack_list_t * list, btstack_item_t *item);

/***********************************************************************************
 * @brief  btstack_list_count
 * #return number of items in list
 **********************************************************************************/
int btstack_list_count(btstack_list_t * list);
 
/***********************************************************************************
 * btstack_list_get_first_item
 * @brief  get first element
 **********************************************************************************/
btstack_item_t * btstack_list_get_first_item(btstack_list_t * list);

/***********************************************************************************
 * btstack_list_pop
 * @brief  pop (get + remove) first element
 **********************************************************************************/
btstack_item_t * btstack_list_pop(btstack_list_t * list);


#endif/*__BLUETOOTH_UTILITY_H__*/


