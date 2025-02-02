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

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "system4.h"
#include "system4/cg.h"

#include "asset_manager.h"
#include "gfx/gfx.h"
#include "queue.h"
#include "scene.h"
#include "xsystem4.h"

bool scene_is_dirty = true;

static TAILQ_HEAD(listhead, sprite) sprite_list = TAILQ_HEAD_INITIALIZER(sprite_list);

static Texture wp = {0};

void scene_register_sprite(struct sprite *sp)
{
	if (sp->in_scene)
		return;

	struct sprite *p;
	TAILQ_FOREACH(p, &sprite_list, entry) {
		if (p->z == sp->z) {
			if (p->z2 > sp->z2) {
				TAILQ_INSERT_BEFORE(p, sp, entry);
				sp->in_scene = true;
				return;
			}
		} else if (p->z > sp->z) {
			TAILQ_INSERT_BEFORE(p, sp, entry);
			sp->in_scene = true;
			return;
		}
	}
	TAILQ_INSERT_TAIL(&sprite_list, sp, entry);
	sp->in_scene = true;
	scene_dirty();
}

void scene_unregister_sprite(struct sprite *sp)
{
	if (!sp->in_scene)
		return;
	TAILQ_REMOVE(&sprite_list, sp, entry);
	sp->in_scene = false;
	scene_dirty();
}

void scene_render(void)
{
	gfx_clear();
	if (wp.handle) {
		Rectangle r = RECT(0, 0, wp.w, wp.h);
		gfx_render_texture(&wp, &r);
	}

	struct sprite *sp;
	TAILQ_FOREACH(sp, &sprite_list, entry) {
		if (sp->render)
			sp->render(sp);
		else
			WARNING("sprite in scene without render function");
	}
}

int scene_set_wp(int cg_no) {
	if (!cg_no) {
		gfx_delete_texture(&wp);
		scene_dirty();
		return 1;
	}

	struct cg *cg = asset_cg_load(cg_no);
	if (!cg)
		return 0;

	gfx_delete_texture(&wp);
	gfx_init_texture_with_cg(&wp, cg);
	cg_free(cg);
	scene_dirty();
	return 1;
}

int scene_set_wp_color(int r, int g, int b)
{
	gfx_set_clear_color(r, g, b, 255);
	scene_dirty();
	return 1;
}

int scene_set_wp_texture(Texture *tex)
{
	gfx_delete_texture(&wp);
	gfx_init_texture_rgb(&wp, tex->w, tex->h, (SDL_Color){ 0, 0, 0, 255 });
	gfx_copy(&wp, 0, 0, tex, 0, 0, tex->w, tex->h);

	scene_dirty();
	return 1;
}

void scene_set_sprite_z(struct sprite *sp, int z)
{
	sp->z = z;
	if (sp->in_scene) {
		scene_unregister_sprite(sp);
		scene_register_sprite(sp);
	}
}

void scene_set_sprite_z2(struct sprite *sp, int z, int z2)
{
	sp->z = z;
	sp->z2 = z2;
	if (sp->in_scene) {
		scene_unregister_sprite(sp);
		scene_register_sprite(sp);
	}
}

void scene_print_sprite(struct sprite *sp, int indent)
{
	printf("{\n");
	indent++;
	indent_printf(indent, "z = (%d,%d),\n", sp->z, sp->z2);
	indent_printf(indent, "has_pixel = %s,\n", sp->has_pixel ? "true" : "false");
	indent_printf(indent, "has_alpha = %s,\n", sp->has_alpha ? "true" : "false");
	indent_printf(indent, "hidden = %s,\n", sp->hidden ? "true" : "false");
	indent_printf(indent, "in_scene = %s,\n", sp->in_scene ? "true" : "false");
	indent--;
	printf("}");

}

void scene_print(void)
{
	struct sprite *p;
	TAILQ_FOREACH(p, &sprite_list, entry) {
		if (p->debug_print) {
			p->debug_print(p);
		} else {
			printf("unknown_scene_entity ");
			scene_print_sprite(p, 0);
			putchar('\n');
		}
	}
}
