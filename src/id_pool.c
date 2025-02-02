/* Copyright (C) 2019 Nunuhara Cabbage <nunuhara@haniwa.technology>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://gnu.org/licenses/>.
 */

#include <assert.h>

#include "system4.h"
#include "id_pool.h"

/*
 * Generic data structure for managing a pool of objects accessed by integer IDs.
 */

#define ID_POOL_INIT_SLOTS 32
#define ID_POOL_ALLOC_HEADROOM 256

void id_pool_init(struct id_pool *pool)
{
	pool->slots = xcalloc(ID_POOL_INIT_SLOTS, sizeof(void*));
	pool->nr_slots = ID_POOL_INIT_SLOTS;
}

static void id_pool_realloc(struct id_pool *pool, int new_size)
{
	pool->slots = xrealloc_array(pool->slots, pool->nr_slots, new_size, sizeof(void*));
	pool->nr_slots = new_size;
}

void id_pool_release(struct id_pool *pool, int id)
{
	if (pool->slots[id])
		pool->slots[id] = NULL;
}

int id_pool_get_unused(struct id_pool *pool)
{
	for (int i = 0; i < pool->nr_slots; i++) {
		if (!pool->slots[i])
			return i;
	}

	int id = pool->nr_slots;
	id_pool_realloc(pool, id + ID_POOL_ALLOC_HEADROOM);
	return id;
}

void *id_pool_get(struct id_pool *pool, int id)
{
	if (id < 0 || id >= pool->nr_slots)
		return NULL;
	return pool->slots[id];
}

void *id_pool_set(struct id_pool *pool, int id, void *data)
{
	assert(id >= 0);
	if (id >= pool->nr_slots)
		id_pool_realloc(pool, id + ID_POOL_ALLOC_HEADROOM);

	void *old = pool->slots[id];
	pool->slots[id] = data;
	return old;
}
