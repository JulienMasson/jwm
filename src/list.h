/*
 * This file is part of the jwm distribution (https://github.com/JulienMasson/jwm).
 * Copyright (c) 2017 Julien Masson.
 *
 * jwm is derived from 2bwm (https://github.com/venam/2bwm)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>

struct item {
	void *		data;
	struct item *	prev;
	struct item *	next;
};

struct item *additem(struct item **mainlist) // Create space for a new item and add it to the head of mainlist.
{                                   // Returns item or NULL if out of memory.
	struct item *item;

	if (NULL == (item = (struct item *)malloc(sizeof(struct item)))) return NULL;
	/* First in the list. */
	if (NULL == *mainlist) {
		item->prev = item->next = NULL;
	} else {
		/* Add to beginning of list. */
		item->next = *mainlist;
		item->next->prev = item;
		item->prev = NULL;
	}
	*mainlist = item;
	return item;
}

void delitem(struct item **mainlist, struct item *item)
{
	struct item *ml = *mainlist;

	if (NULL == mainlist || NULL == *mainlist || NULL == item) return;
	/* First entry was removed. Remember the next one instead. */
	if (item == *mainlist) {
		*mainlist = ml->next;
		if (item->next != NULL)
			item->next->prev = NULL;
	} else {
		item->prev->next = item->next;
		/* This is not the last item in the list. */
		if (NULL != item->next) item->next->prev = item->prev;
	}
	free(item);
}

void freeitem(struct item **list, int *stored, struct item *item)
{
	if (NULL == list || NULL == *list || NULL == item) return;

	if (NULL != item->data) {
		free(item->data);
		item->data = NULL;
	}
	delitem(list, item);

	if (NULL != stored) (*stored)--;
}
