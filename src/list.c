/*
 * This file is part of the jwm distribution:
 * https://github.com/JulienMasson/jwm
 *
 * Copyright (c) 2017 Julien Masson.
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

#include <stdlib.h>
#include "list.h"

struct list *list_add(struct list **list_head, void *data)
{
	struct list *element = (struct list *)malloc(sizeof(struct list));
	struct list *tail;

	/* allocation failed */
	if (element == NULL)
		return NULL;

	/* get the tail */
	tail = *list_head;
	while ((tail != NULL) && (tail->next != NULL)) {
		tail = tail->next;
	}

	/* first element of the list */
	if (*list_head == NULL) {
		*list_head = element;
		element->prev = element->next = NULL;
	} else {
		/* put element at the tail of the list */
		element->prev = tail;
		element->prev->next = element;
		element->next = NULL;
	}
	element->data = data;

	return element;
}

void list_remove(struct list **list_head, struct list *element)
{
	struct list *head = NULL;

	/* check args */
	if (list_head == NULL || *list_head == NULL || element == NULL)
		return;

	/* point to the head of the list */
	head = *list_head;

	/* check element at the head */
	if (element == *list_head) {
		*list_head = head->next;
		/* put next element at the head */
		if (element->next != NULL)
			element->next->prev = NULL;
	} else {
		element->prev->next = element->next;
		if (element->next != NULL)
			element->next->prev = element->prev;
	}

	/* free space */
	if (element->data != NULL) {
		free(element->data);
		element->data = NULL;
	}
	free(element);
}
