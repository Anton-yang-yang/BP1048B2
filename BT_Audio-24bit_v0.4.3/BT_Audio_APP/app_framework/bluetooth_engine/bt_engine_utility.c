/**
 **************************************************************************************
 * @file    bluetooth_utility.c
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

#include "bt_engine_utility.h"

/***********************************************************************************
 * bt stack list
 **********************************************************************************/
int  btstack_list_empty(btstack_list_t * list)
{
	return *list == (void *) 0;
}

/***********************************************************************************
 * btstack_list_get_last_item
 **********************************************************************************/
btstack_item_t * btstack_list_get_last_item(btstack_list_t * list)
{	// <-- find the last item in the list
	btstack_item_t *lastItem = NULL;
	btstack_item_t *it;
	for (it = *list; it ; it = it->next)
	{
		if (it) 
		{
			lastItem = it;
		}
	}
	return lastItem;
}


/***********************************************************************************
 * btstack_list_add
 **********************************************************************************/
void btstack_list_add(btstack_list_t * list, btstack_item_t *item)
{	// <-- add item to list
	// check if already in list
	btstack_item_t *it;
	for (it = *list; it ; it = it->next)
	{
		if (it == item) 
		{
			return;
		}
	}
	// add first
	item->next = *list;
	*list = item;
}

/***********************************************************************************
 * btstack_list_add_tail
 **********************************************************************************/
void btstack_list_add_tail(btstack_list_t * list, btstack_item_t *item)
{	// <-- add item to list as last element
	// check if already in list
	btstack_item_t *it;
	for (it = (btstack_item_t *) list; it->next ; it = it->next)
	{
		if (it->next == item) 
		{
			return;
		}
	}
	item->next = (btstack_item_t*) 0;
	it->next = item;
}

/***********************************************************************************
 * btstack_list_remove
 **********************************************************************************/
int  btstack_list_remove(btstack_list_t * list, btstack_item_t *item)
{	// <-- remove item from list
	if (!item) return -1;
	btstack_item_t *it;
	for (it = (btstack_item_t *) list; it ; it = it->next)
	{
		if (it->next == item)
		{
			it->next =  item->next;
			return 0;
		}
	}
	return -1;
}

/***********************************************************************************
 * @brief  btstack_list_count
 * #return number of items in list
 **********************************************************************************/
int btstack_list_count(btstack_list_t * list)
{
	btstack_item_t *it;
	int counter = 0;
	for (it = (btstack_item_t *) list; it->next ; it = it->next) 
	{
		counter++;
	}
	return counter; 
}
 
/***********************************************************************************
 * btstack_list_get_first_item
 * @brief  get first element
 **********************************************************************************/
btstack_item_t * btstack_list_get_first_item(btstack_list_t * list)
{
	return * list;
}

/***********************************************************************************
 * btstack_list_pop
 * @brief  pop (get + remove) first element
 **********************************************************************************/
btstack_item_t * btstack_list_pop(btstack_list_t * list)
{
	btstack_item_t * item = *list;
	if (!item) return NULL;
	*list = item->next;
	return item;
}




