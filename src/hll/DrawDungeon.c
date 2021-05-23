/* Copyright (C) 2021 kichikuou <KichikuouChrome@gmail.com>
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "system4.h"

#include "dungeon/dgn.h"
#include "dungeon/dungeon.h"
#include "dungeon/map.h"
#include "dungeon/tes.h"
#include "hll.h"

static struct dungeon_context *current_context = NULL;

struct dungeon_context *dungeon_get_context(int surface)
{
	if (!current_context || surface != current_context->surface)
		return NULL;
	return current_context;
}

void dungeon_update(void)
{
	if (current_context && current_context->loaded && current_context->draw_enabled)
		dungeon_render(current_context);
}

static struct dgn_cell *dungeon_get_cell(int surface, int x, int y, int z)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn || !dgn_is_in_map(ctx->dgn, x, y, z))
		return NULL;
	return dgn_cell_at(ctx->dgn, x, y, z);
}

static int DrawDungeon_Init(int surface)
{
	if (current_context)
		VM_ERROR("Dungeon is already associated with surface %d", current_context->surface);
	if (surface < 0)
		return 0;
	current_context = dungeon_context_create(surface);
	return 1;
}

static void DrawDungeon_Release(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	dungeon_context_free(ctx);
	current_context = NULL;
}

static void DrawDungeon_SetDrawFlag(int surface, int flag)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return;
	ctx->draw_enabled = flag;
}

static bool DrawDungeon_BeginLoad(int surface, int num)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return false;
	return dungeon_load(ctx, num);
}

static bool DrawDungeon_IsLoading(int surface)
{
	return false;  // We don't use background loading.
}

static bool DrawDungeon_IsSucceededLoading(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx)
		return false;
	return ctx->loaded;
}

//float DrawDungeon_GetLoadingPercent(int surface);
//int DrawDungeon_LoadFromFile(int surface, struct string *file_name, int num);
//int DrawDungeon_LoadTexture(int surface, struct string *file_name);
//int DrawDungeon_LoadEventTexture(int surface, struct string *file_name);

static int DrawDungeon_GetMapX(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return ctx->dgn->size_x;
}

static int DrawDungeon_GetMapY(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return ctx->dgn->size_y;
}

static int DrawDungeon_GetMapZ(int surface)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return ctx->dgn->size_z;
}

static int DrawDungeon_IsInMap(int surface, int x, int y, int z)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->dgn)
		return -1;
	return dgn_is_in_map(ctx->dgn, x, y, z);
}

#define CELL_GETTER(name, defval, expr) \
	static int name(int surface, int x, int y, int z) \
	{ \
		struct dgn_cell *cell = dungeon_get_cell(surface, x, y, z); \
		if (!cell) \
			return defval; \
		return expr; \
	}

CELL_GETTER(DrawDungeon_IsFloor, 0, cell->floor != -1);
//bool DrawDungeon_IsFloorFillTexture(int surface, int x, int y, int z);
CELL_GETTER(DrawDungeon_IsStair, 0, cell->stairs_orientation != -1);
CELL_GETTER(DrawDungeon_IsStairN, 0, cell->stairs_orientation == 0);
CELL_GETTER(DrawDungeon_IsStairW, 0, cell->stairs_orientation == 1);
CELL_GETTER(DrawDungeon_IsStairS, 0, cell->stairs_orientation == 2);
CELL_GETTER(DrawDungeon_IsStairE, 0, cell->stairs_orientation == 3);
CELL_GETTER(DrawDungeon_IsWallN, 0, cell->north_wall != -1);
CELL_GETTER(DrawDungeon_IsWallW, 0, cell->west_wall != -1);
CELL_GETTER(DrawDungeon_IsWallS, 0, cell->south_wall != -1);
CELL_GETTER(DrawDungeon_IsWallE, 0, cell->east_wall != -1);
CELL_GETTER(DrawDungeon_IsDoorN, 0, cell->north_door != -1);
CELL_GETTER(DrawDungeon_IsDoorW, 0, cell->west_door != -1);
CELL_GETTER(DrawDungeon_IsDoorS, 0, cell->south_door != -1);
CELL_GETTER(DrawDungeon_IsDoorE, 0, cell->east_door != -1);
CELL_GETTER(DrawDungeon_IsEnter, 0, cell->enterable);
CELL_GETTER(DrawDungeon_IsEnterN, 0, cell->enterable_north);
CELL_GETTER(DrawDungeon_IsEnterW, 0, cell->enterable_west);
CELL_GETTER(DrawDungeon_IsEnterS, 0, cell->enterable_south);
CELL_GETTER(DrawDungeon_IsEnterE, 0, cell->enterable_east);
CELL_GETTER(DrawDungeon_GetEvFloor, 0, cell->floor_event);
CELL_GETTER(DrawDungeon_GetEvWallN, 0, cell->north_event);
CELL_GETTER(DrawDungeon_GetEvWallW, 0, cell->west_event);
CELL_GETTER(DrawDungeon_GetEvWallS, 0, cell->south_event);
CELL_GETTER(DrawDungeon_GetEvWallE, 0, cell->east_event);
CELL_GETTER(DrawDungeon_GetEvFloor2, 0, cell->floor_event2);
CELL_GETTER(DrawDungeon_GetEvWallN2, 0, cell->north_event2);
CELL_GETTER(DrawDungeon_GetEvWallW2, 0, cell->west_event2);
CELL_GETTER(DrawDungeon_GetEvWallS2, 0, cell->south_event2);
CELL_GETTER(DrawDungeon_GetEvWallE2, 0, cell->east_event2);
CELL_GETTER(DrawDungeon_GetTexFloor, -1, cell->floor);
CELL_GETTER(DrawDungeon_GetTexCeiling, -1, cell->ceiling);
CELL_GETTER(DrawDungeon_GetTexWallN, -1, cell->north_wall);
CELL_GETTER(DrawDungeon_GetTexWallW, -1, cell->west_wall);
CELL_GETTER(DrawDungeon_GetTexWallS, -1, cell->south_wall);
CELL_GETTER(DrawDungeon_GetTexWallE, -1, cell->east_wall);
CELL_GETTER(DrawDungeon_GetTexDoorN, -1, cell->north_door);
CELL_GETTER(DrawDungeon_GetTexDoorW, -1, cell->west_door);
CELL_GETTER(DrawDungeon_GetTexDoorS, -1, cell->south_door);
CELL_GETTER(DrawDungeon_GetTexDoorE, -1, cell->east_door);
CELL_GETTER(DrawDungeon_GetTexStair, -1, cell->stairs_texture);
CELL_GETTER(DrawDungeon_GetBattleBack, -1, cell->battle_background);

#define CELL_SETTER(name, expr) \
	static void name(int surface, int x, int y, int z, int value) \
	{ \
		struct dungeon_context *ctx = dungeon_get_context(surface); \
		if (!ctx || !ctx->dgn || !dgn_is_in_map(ctx->dgn, x, y, z)) \
			return; \
		struct dgn_cell *cell = dgn_cell_at(ctx->dgn, x, y, z); \
		expr = value; \
		dungeon_map_update_cell(ctx, x, y, z); \
	}

HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetDoorNAngle, int surface, int x, int y, int z, float angle);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetDoorWAngle, int surface, int x, int y, int z, float angle);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetDoorSAngle, int surface, int x, int y, int z, float angle);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetDoorEAngle, int surface, int x, int y, int z, float angle);
CELL_SETTER(DrawDungeon_SetEvWallN, cell->north_event);
CELL_SETTER(DrawDungeon_SetEvWallW, cell->west_event);
CELL_SETTER(DrawDungeon_SetEvWallS, cell->south_event);
CELL_SETTER(DrawDungeon_SetEvWallE, cell->east_event);
CELL_SETTER(DrawDungeon_SetEvFloor2, cell->floor_event2);
CELL_SETTER(DrawDungeon_SetEvWallN2, cell->north_event2);
CELL_SETTER(DrawDungeon_SetEvWallW2, cell->west_event2);
CELL_SETTER(DrawDungeon_SetEvWallS2, cell->south_event2);
CELL_SETTER(DrawDungeon_SetEvWallE2, cell->east_event2);
//void DrawDungeon_SetEvMag(int surface, int x, int y, int z, float mag);
CELL_SETTER(DrawDungeon_SetEnter, cell->enterable);
CELL_SETTER(DrawDungeon_SetEnterN, cell->enterable_north);
CELL_SETTER(DrawDungeon_SetEnterW, cell->enterable_west);
CELL_SETTER(DrawDungeon_SetEnterS, cell->enterable_south);
CELL_SETTER(DrawDungeon_SetEnterE, cell->enterable_east);
//void DrawDungeon_SetSkyTextureSet(int surface, int set);

HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetDrawMapRadar, int surface, int flag);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetDrawHalfFlag, int surface, int flag);
HLL_WARN_UNIMPLEMENTED(0, int,  DrawDungeon, GetDrawHalfFlag, int surface);
//void DrawDungeon_SetWalkedAll(int surface);
//int DrawDungeon_CalcNumofFloor(int surface);
//int DrawDungeon_CalcNumofWalk(int surface);
//int DrawDungeon_CalcNumofWalk2(int surface);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetInterlaceMode, int surface, int flag);
HLL_WARN_UNIMPLEMENTED(1, bool, DrawDungeon, SetDirect3DMode, int surface, int flag);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SaveDrawSettingFlag, int direct3D, int interlace, int half);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, GetDrawSettingFlag, int *direct3D, int *half);
HLL_WARN_UNIMPLEMENTED(1, bool, DrawDungeon, IsDrawSettingFile, void);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, DeleteDrawSettingFile, void);
HLL_WARN_UNIMPLEMENTED(1, bool, DrawDungeon, IsSSEFlag, void);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, SetSSEFlag, bool flag);
bool DrawDungeon_IsPVSData(int surface)
{
	return false;
}

static int DrawDungeon_GetTexSound(int surface, int type, int num)
{
	struct dungeon_context *ctx = dungeon_get_context(surface);
	if (!ctx || !ctx->tes)
		return -1;
	return tes_get(ctx->tes, type, num);
}

HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, StopTimer, void);
HLL_WARN_UNIMPLEMENTED( , void, DrawDungeon, RestartTimer, void);

HLL_LIBRARY(DrawDungeon,
			HLL_EXPORT(Init, DrawDungeon_Init),
			HLL_EXPORT(Release, DrawDungeon_Release),
			HLL_EXPORT(SetDrawFlag, DrawDungeon_SetDrawFlag),
			HLL_EXPORT(BeginLoad, DrawDungeon_BeginLoad),
			HLL_EXPORT(IsLoading, DrawDungeon_IsLoading),
			HLL_EXPORT(IsSucceededLoading, DrawDungeon_IsSucceededLoading),
			HLL_TODO_EXPORT(GetLoadingPercent, DrawDungeon_GetLoadingPercent),
			HLL_TODO_EXPORT(LoadFromFile, DrawDungeon_LoadFromFile),
			HLL_TODO_EXPORT(LoadTexture, DrawDungeon_LoadTexture),
			HLL_TODO_EXPORT(LoadEventTexture, DrawDungeon_LoadEventTexture),
			HLL_EXPORT(GetMapX, DrawDungeon_GetMapX),
			HLL_EXPORT(GetMapY, DrawDungeon_GetMapY),
			HLL_EXPORT(GetMapZ, DrawDungeon_GetMapZ),
			HLL_EXPORT(SetCamera, dungeon_set_camera),
			HLL_EXPORT(IsInMap, DrawDungeon_IsInMap),
			HLL_EXPORT(IsFloor, DrawDungeon_IsFloor),
			HLL_TODO_EXPORT(IsFloorFillTexture, DrawDungeon_IsFloorFillTexture),
			HLL_EXPORT(IsStair, DrawDungeon_IsStair),
			HLL_EXPORT(IsStairN, DrawDungeon_IsStairN),
			HLL_EXPORT(IsStairW, DrawDungeon_IsStairW),
			HLL_EXPORT(IsStairS, DrawDungeon_IsStairS),
			HLL_EXPORT(IsStairE, DrawDungeon_IsStairE),
			HLL_EXPORT(IsWallN, DrawDungeon_IsWallN),
			HLL_EXPORT(IsWallW, DrawDungeon_IsWallW),
			HLL_EXPORT(IsWallS, DrawDungeon_IsWallS),
			HLL_EXPORT(IsWallE, DrawDungeon_IsWallE),
			HLL_EXPORT(IsDoorN, DrawDungeon_IsDoorN),
			HLL_EXPORT(IsDoorW, DrawDungeon_IsDoorW),
			HLL_EXPORT(IsDoorS, DrawDungeon_IsDoorS),
			HLL_EXPORT(IsDoorE, DrawDungeon_IsDoorE),
			HLL_EXPORT(IsEnter, DrawDungeon_IsEnter),
			HLL_EXPORT(IsEnterN, DrawDungeon_IsEnterN),
			HLL_EXPORT(IsEnterW, DrawDungeon_IsEnterW),
			HLL_EXPORT(IsEnterS, DrawDungeon_IsEnterS),
			HLL_EXPORT(IsEnterE, DrawDungeon_IsEnterE),
			HLL_EXPORT(GetEvFloor, DrawDungeon_GetEvFloor),
			HLL_EXPORT(GetEvWallN, DrawDungeon_GetEvWallN),
			HLL_EXPORT(GetEvWallW, DrawDungeon_GetEvWallW),
			HLL_EXPORT(GetEvWallS, DrawDungeon_GetEvWallS),
			HLL_EXPORT(GetEvWallE, DrawDungeon_GetEvWallE),
			HLL_EXPORT(GetEvFloor2, DrawDungeon_GetEvFloor2),
			HLL_EXPORT(GetEvWallN2, DrawDungeon_GetEvWallN2),
			HLL_EXPORT(GetEvWallW2, DrawDungeon_GetEvWallW2),
			HLL_EXPORT(GetEvWallS2, DrawDungeon_GetEvWallS2),
			HLL_EXPORT(GetEvWallE2, DrawDungeon_GetEvWallE2),
			HLL_EXPORT(GetTexFloor, DrawDungeon_GetTexFloor),
			HLL_EXPORT(GetTexCeiling, DrawDungeon_GetTexCeiling),
			HLL_EXPORT(GetTexWallN, DrawDungeon_GetTexWallN),
			HLL_EXPORT(GetTexWallW, DrawDungeon_GetTexWallW),
			HLL_EXPORT(GetTexWallS, DrawDungeon_GetTexWallS),
			HLL_EXPORT(GetTexWallE, DrawDungeon_GetTexWallE),
			HLL_EXPORT(GetTexDoorN, DrawDungeon_GetTexDoorN),
			HLL_EXPORT(GetTexDoorW, DrawDungeon_GetTexDoorW),
			HLL_EXPORT(GetTexDoorS, DrawDungeon_GetTexDoorS),
			HLL_EXPORT(GetTexDoorE, DrawDungeon_GetTexDoorE),
			HLL_EXPORT(GetTexStair, DrawDungeon_GetTexStair),
			HLL_EXPORT(GetBattleBack, DrawDungeon_GetBattleBack),
			HLL_EXPORT(SetDoorNAngle, DrawDungeon_SetDoorNAngle),
			HLL_EXPORT(SetDoorWAngle, DrawDungeon_SetDoorWAngle),
			HLL_EXPORT(SetDoorSAngle, DrawDungeon_SetDoorSAngle),
			HLL_EXPORT(SetDoorEAngle, DrawDungeon_SetDoorEAngle),
			HLL_EXPORT(SetEvFloor, dungeon_set_event_floor),
			HLL_EXPORT(SetEvWallN, DrawDungeon_SetEvWallN),
			HLL_EXPORT(SetEvWallW, DrawDungeon_SetEvWallW),
			HLL_EXPORT(SetEvWallS, DrawDungeon_SetEvWallS),
			HLL_EXPORT(SetEvWallE, DrawDungeon_SetEvWallE),
			HLL_EXPORT(SetEvFloor2, DrawDungeon_SetEvFloor2),
			HLL_EXPORT(SetEvWallN2, DrawDungeon_SetEvWallN2),
			HLL_EXPORT(SetEvWallW2, DrawDungeon_SetEvWallW2),
			HLL_EXPORT(SetEvWallS2, DrawDungeon_SetEvWallS2),
			HLL_EXPORT(SetEvWallE2, DrawDungeon_SetEvWallE2),
			HLL_TODO_EXPORT(SetEvMag, DrawDungeon_SetEvMag),
			HLL_EXPORT(SetEvRate, dungeon_set_event_blend_rate),
			HLL_EXPORT(SetEnter, DrawDungeon_SetEnter),
			HLL_EXPORT(SetEnterN, DrawDungeon_SetEnterN),
			HLL_EXPORT(SetEnterW, DrawDungeon_SetEnterW),
			HLL_EXPORT(SetEnterS, DrawDungeon_SetEnterS),
			HLL_EXPORT(SetEnterE, DrawDungeon_SetEnterE),
			HLL_EXPORT(SetTexFloor, dungeon_set_texture_floor),
			HLL_EXPORT(SetTexCeiling, dungeon_set_texture_ceiling),
			HLL_EXPORT(SetTexWallN, dungeon_set_texture_north),
			HLL_EXPORT(SetTexWallW, dungeon_set_texture_west),
			HLL_EXPORT(SetTexWallS, dungeon_set_texture_south),
			HLL_EXPORT(SetTexWallE, dungeon_set_texture_east),
			HLL_EXPORT(SetTexDoorN, dungeon_set_door_north),
			HLL_EXPORT(SetTexDoorW, dungeon_set_door_west),
			HLL_EXPORT(SetTexDoorS, dungeon_set_door_south),
			HLL_EXPORT(SetTexDoorE, dungeon_set_door_east),
			HLL_TODO_EXPORT(SetSkyTextureSet, DrawDungeon_SetSkyTextureSet),
			HLL_EXPORT(DrawMap, dungeon_map_draw),
			HLL_EXPORT(SetMapAllViewFlag, dungeon_map_set_all_view),
			HLL_EXPORT(SetDrawMapFloor, dungeon_map_set_small_map_floor),
			HLL_EXPORT(DrawLMap, dungeon_map_draw_lmap),
			HLL_EXPORT(SetMapCG, dungeon_map_set_cg),
			HLL_EXPORT(SetDrawLMapFloor, dungeon_map_set_large_map_floor),
			HLL_EXPORT(SetDrawMapRadar, DrawDungeon_SetDrawMapRadar),
			HLL_EXPORT(SetDrawHalfFlag, DrawDungeon_SetDrawHalfFlag),
			HLL_EXPORT(GetDrawHalfFlag, DrawDungeon_GetDrawHalfFlag),
			HLL_EXPORT(SetWalked, dungeon_map_set_walked),
			HLL_TODO_EXPORT(SetWalkedAll, DrawDungeon_SetWalkedAll),
			HLL_TODO_EXPORT(CalcNumofFloor, DrawDungeon_CalcNumofFloor),
			HLL_TODO_EXPORT(CalcNumofWalk, DrawDungeon_CalcNumofWalk),
			HLL_TODO_EXPORT(CalcNumofWalk2, DrawDungeon_CalcNumofWalk2),
			HLL_EXPORT(CalcConquer, dungeon_map_calc_conquer),
			HLL_EXPORT(SetInterlaceMode, DrawDungeon_SetInterlaceMode),
			HLL_EXPORT(SetDirect3DMode, DrawDungeon_SetDirect3DMode),
			HLL_EXPORT(SaveDrawSettingFlag, DrawDungeon_SaveDrawSettingFlag),
			HLL_EXPORT(GetDrawSettingFlag, DrawDungeon_GetDrawSettingFlag),
			HLL_EXPORT(IsDrawSettingFile, DrawDungeon_IsDrawSettingFile),
			HLL_EXPORT(DeleteDrawSettingFile, DrawDungeon_DeleteDrawSettingFile),
			HLL_EXPORT(IsSSEFlag, DrawDungeon_IsSSEFlag),
			HLL_EXPORT(SetSSEFlag, DrawDungeon_SetSSEFlag),
			HLL_EXPORT(IsPVSData, DrawDungeon_IsPVSData),
			HLL_EXPORT(GetTexSound, DrawDungeon_GetTexSound),
			HLL_EXPORT(StopTimer, DrawDungeon_StopTimer),
			HLL_EXPORT(RestartTimer, DrawDungeon_RestartTimer),
			HLL_EXPORT(LoadWalkData, dungeon_map_load_walk_data),
			HLL_EXPORT(SaveWalkData, dungeon_map_save_walk_data)
			);
