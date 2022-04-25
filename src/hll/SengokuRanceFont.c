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

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <limits.h>

#include "system4.h"
#include "system4/file.h"
#include "system4/fnl.h"
#include "system4/hashtable.h"
#include "system4/string.h"

#include "gfx/gfx.h"
#include "vm/page.h"
#include "sact.h"
#include "hll.h"
#include "xsystem4.h"

static struct fnl *fontlib;

struct sr_text_properties {
	float x;
	int y;
	float home_x;
	int home_y;
	int line_space;
	struct text_style ts;
};

static struct sr_text_properties default_text_properties = {
	.x = 0.0,
	.y = 0,
	.home_x = 0.0,
	.home_y = 0,
	.line_space = 0,
	.ts = {
		.font_type = 0,
		.size = 16.0,
		.bold_width = 0.0,
		.color = { 255, 255, 255, 255 },
		.edge_width = 0.0,
		.edge_color = { 0, 0, 0, 255 },
		.scale_x = 1.0,
		.space_scale_x = 1.0,
		.font_spacing = 0.0,
		.font_size = NULL
	}
};

static struct sr_text_properties current_text_properties;

struct hash_table *sr_tp_table = NULL;

static struct ht_slot *_get_sp_properties(int sp_no)
{
	if (!sr_tp_table) {
		sr_tp_table = ht_create(256);
	}
	return ht_put_int(sr_tp_table, sp_no, NULL);
}

static struct sr_text_properties *get_sp_properties(int sp_no)
{
	struct ht_slot *slot = _get_sp_properties(sp_no);
	if (slot->value)
		return slot->value;

	struct sr_text_properties *p = xcalloc(1, sizeof(struct sr_text_properties));
	*p = default_text_properties;
	slot->value = p;
	return slot->value;
}

static void free_sp_properties(int sp_no)
{
	struct ht_slot *slot = _get_sp_properties(sp_no);
	free(slot->value);
	slot->value = NULL;
}

static unsigned get_font_type(int type)
{
	unsigned font = type - 256;
	if (font >= fontlib->nr_fonts) {
		WARNING("Invalid font type: %d", type);
		// XXX: Really not sure why this is how it works...
		font = type == 1 ? 2 : 1;
	}
	return font;
}

static void text_style_update(struct text_style *style)
{
	style->font_size = fnl_get_font_size(&fontlib->fonts[style->font_type], style->size);
}

//static void SengokuRanceFont_AlphaComposite(int spriteNumberDest, int destX, int destY, int spriteNumberSrc, int srcX, int srcY, int w, int h);

static void SengokuRanceFont_SetFontType(int type)
{
	current_text_properties.ts.font_type = get_font_type(type);
	text_style_update(&current_text_properties.ts);
}

static void SengokuRanceFont_SetFontSize(float size)
{
	current_text_properties.ts.size = size;
	text_style_update(&current_text_properties.ts);
}

static void SengokuRanceFont_SetFontColor(int r, int g, int b)
{
	current_text_properties.ts.color = (SDL_Color) { r, g, b, 255 };
}

static void SengokuRanceFont_SetBoldWidth(float bold_width)
{
	current_text_properties.ts.bold_width = bold_width;
}

static void SengokuRanceFont_SetEdgeColor(int r, int g, int b)
{
	current_text_properties.ts.edge_color = (SDL_Color) { r, g, b, 255 };
}

static void SengokuRanceFont_SetEdgeWidth(float w)
{
	current_text_properties.ts.edge_width = w;
}

static float SengokuRanceFont_GetTextWidth(struct string *str)
{
	return fnl_size_text(fontlib, &current_text_properties.ts, str->text);
}

//static float SengokuRanceFont_GetCharacterWidth(int charCode);

static void SengokuRanceFont_SetTextX(float x)
{
	current_text_properties.x = x;
}

static void SengokuRanceFont_SetTextY(int y)
{
	current_text_properties.y = y;
}

static void SengokuRanceFont_SetTextPos(float x, int y)
{
	current_text_properties.x = x;
	current_text_properties.y = y;
}

static void SengokuRanceFont_SetScaleX(float scale_x)
{
	current_text_properties.ts.scale_x = scale_x;
}

//static void SengokuRanceFont_DrawTextToSprite(int spriteNumber, struct string *str);
//static void SengokuRanceFont_DrawTextPreload(struct string *str);
//static void SengokuRanceFont_SetFontFileName(struct string *fontName);

static void SengokuRanceFont_SetCharacterSpacing(float char_space)
{
	current_text_properties.ts.font_spacing = char_space;
}

static void SengokuRanceFont_SetSpaceScaleX(float space_scale_x)
{
	current_text_properties.ts.space_scale_x = space_scale_x;
}

static void SengokuRanceFont_SetReduceDescender(int reduce_descender)
{
	if (reduce_descender)
		WARNING("SengokuRanceFont_SetReduceDescender(%d)", reduce_descender);
}

static void SengokuRanceFont_SP_ClearState(int sp_no)
{
	free_sp_properties(sp_no);
}

static float SengokuRanceFont_SP_GetTextCharSpace(int sp_no)
{
	return get_sp_properties(sp_no)->ts.font_spacing;
}

static float SengokuRanceFont_SP_GetTextHomeX(int sp_no)
{
	return get_sp_properties(sp_no)->home_x;
}

static int SengokuRanceFont_SP_GetTextHomeY(int sp_no)
{
	return get_sp_properties(sp_no)->home_y;
}

static int SengokuRanceFont_SP_GetTextLineSpace(int sp_no)
{
	return get_sp_properties(sp_no)->line_space;
}

static float SengokuRanceFont_SP_GetTextPosX(int sp_no)
{
	return get_sp_properties(sp_no)->x;
}

static int SengokuRanceFont_SP_GetTextPosY(int sp_no)
{
	return get_sp_properties(sp_no)->y;
}

static void SengokuRanceFont_SP_SetTextCharSpace(int sp_no, float space)
{
	get_sp_properties(sp_no)->ts.font_spacing = space;
}

static void SengokuRanceFont_SP_SetTextHome(int sp_no, float x, int y)
{
	struct sr_text_properties *p = get_sp_properties(sp_no);
	p->home_x = x;
	p->home_y = y;
}

static void SengokuRanceFont_SP_SetTextLineSpace(int sp_no, int space)
{
	struct sr_text_properties *p = get_sp_properties(sp_no);
	p->line_space = space;
}

static void SengokuRanceFont_SP_SetTextPos(int sp_no, float x, int y)
{
	struct sr_text_properties *p = get_sp_properties(sp_no);
	p->x = x;
	p->y = y;
}

static void SengokuRanceFont_SP_TextClear(int sp_no)
{
	struct sact_sprite *sp = sact_try_get_sprite(sp_no);
	if (!sp) return;
	gfx_fill_with_alpha(sprite_get_texture(sp), 0, 0, sp->rect.w, sp->rect.h, 0, 0, 0, 0);
	sprite_dirty(sp);
}

static void SengokuRanceFont_SP_TextCopy(int dno, int sno)
{
	struct sact_sprite *dsp = sact_try_get_sprite(dno);
	struct sact_sprite *ssp = sact_try_get_sprite(sno);
	if (!dsp || !ssp)
		return;

	// copy texture
	gfx_delete_texture(&dsp->texture);
	if (ssp->texture.handle) {
		struct texture *dtex = sprite_get_texture(dsp);
		struct texture *stex = sprite_get_texture(ssp);
		gfx_copy_with_alpha_map(dtex, 0, 0, stex, 0, 0, dsp->rect.w, dsp->rect.h);
	}

	// copy properties
	struct sr_text_properties *d_prop = get_sp_properties(dno);
	struct sr_text_properties *s_prop = get_sp_properties(sno);
	*d_prop = *s_prop;
	sprite_dirty(dsp);
}

static void SengokuRanceFont_SP_TextHome(int sp_no, float size)
{
	struct sr_text_properties *p = get_sp_properties(sp_no);
	p->x = p->home_x;
	p->y = p->home_y;
}

static void SengokuRanceFont_SP_TextNewLine(int sp_no, float font_size)
{
	struct sr_text_properties *p = get_sp_properties(sp_no);
	p->x = p->home_x;
	p->y += font_size;
}

static void SengokuRanceFont_SP_SetTextMetricsClassic(int sp_no, struct page *page)
{
	struct text_style *ts = &get_sp_properties(sp_no)->ts;
	ts->font_type     = get_font_type(256);
	ts->color.r       = page->values[0].i;
	ts->color.g       = page->values[1].i;
	ts->color.b       = page->values[2].i;
	ts->color.a       = 255;
	ts->size          = page->values[3].i;
	//ts->weight        = page->values[4].i;
	//ts->face          = page->values[5].i;
	ts->edge_width    = max(page->values[6].i, max(page->values[7].i, max(page->values[8].i, page->values[9].i)));
	ts->edge_color.r  = page->values[10].i;
	ts->edge_color.g  = page->values[11].i;
	ts->edge_color.b  = page->values[12].i;
	ts->edge_color.a  = 255;
	ts->bold_width    = 0; // FIXME: use weight?
	ts->scale_x       = 1;
	ts->space_scale_x = 1;
	ts->font_spacing  = 1;
	ts->font_size     = fnl_get_font_size(&fontlib->fonts[ts->font_type], ts->size);
}

static void SengokuRanceFont_SP_SetTextStyle(int sp_no, struct page *page)
{
	struct text_style *ts = &get_sp_properties(sp_no)->ts;
	ts->font_type          = get_font_type(page->values[0].i < 0 ? 256 : page->values[0].i);
	ts->size               = page->values[1].f;
	ts->bold_width         = page->values[2].f < 0 ? 0 : page->values[2].f;
	ts->color.r            = page->values[3].i & 0xFF;
	ts->color.r            = page->values[3].i & 0xFF;
	ts->color.g            = page->values[4].i & 0xFF;
	ts->color.b            = page->values[5].i & 0xFF;
	ts->color.a            = 255;
	ts->edge_width         = page->values[6].f < 0.0 ? 0.0 : page->values[6].f;
	ts->edge_color.r       = page->values[7].i & 0xFF;
	ts->edge_color.g       = page->values[8].i & 0xFF;
	ts->edge_color.b       = page->values[9].i & 0xFF;
	ts->edge_color.a       = 255;
	ts->scale_x            = page->values[10].f < 0.0 ? 1.0 : page->values[10].f;
	ts->space_scale_x      = page->values[11].f < 0.0 ? 1.0 : page->values[11].f;
	ts->font_spacing       = page->values[12].f < 0.0 ? 1.0 : page->values[12].f;
	ts->font_size          = fnl_get_font_size(&fontlib->fonts[ts->font_type], ts->size);
}

static float sp_text_draw(struct sact_sprite *sp, struct text_style *ts, struct string *text, float x, int y)
{
	// in case of uninitialized text style...
	if (ts->font_type >= fontlib->nr_fonts) {
		ts->font_type = 0;
	}
	if (ts->size < 8) {
		ts->size = 8;
	}
	if (!ts->font_size) {
		ts->font_size = fnl_get_font_size(&fontlib->fonts[ts->font_type], ts->size);
	}

	struct texture *tex = sprite_get_texture(sp);

	sprite_dirty(sp);
	return x + fnl_draw_text(fontlib, ts, tex, x, y, text->text);
}

static void SengokuRanceFont_SP_TextDraw(int sp_no, struct string *text)
{
	struct sact_sprite *sp = sact_try_get_sprite(sp_no);
	if (!sp) {
		WARNING("Failed to get sprite %d", sp_no);
		return;
	}

	struct sr_text_properties *p = get_sp_properties(sp_no);
	p->x += sp_text_draw(sp, &p->ts, text, p->x, p->y);
}

// As far as I can tell, this just renders the text slightly higher than
// SengokuRanceFont.SP_TextDraw
static void SengokuRanceFont_SP_TextDrawClassic(int sp_no, struct string *text)
{
	struct sact_sprite *sp = sact_try_get_sprite(sp_no);
	if (!sp) {
		WARNING("Failed to get sprite %d", sp_no);
		return;
	}

	struct sr_text_properties *p = get_sp_properties(sp_no);
	int y = p->y - p->ts.size * 0.1875;
	p->x += sp_text_draw(sp, &p->ts, text, p->x, y);
}

HLL_WARN_UNIMPLEMENTED( , void, SengokuRanceFont, SP_TextDrawPreload, int sp_no, struct string *text);

static void SengokuRanceFont_SP_SetFontSize(int sp_no, float font_size)
{
	struct text_style *ts = &get_sp_properties(sp_no)->ts;
	ts->size = font_size;
	ts->font_size = fnl_get_font_size(&fontlib->fonts[ts->font_type], ts->size);
}

static void SengokuRanceFont_SP_SetFontType(int sp_no, int type)
{
	struct text_style *ts = &get_sp_properties(sp_no)->ts;
	ts->font_type = get_font_type(type);
	text_style_update(ts);
}

static void SengokuRanceFont_SP_SetFontColor(int sp_no, int r, int g, int b)
{
	get_sp_properties(sp_no)->ts.color = (SDL_Color) { r, g, b, 255 };
}

static void SengokuRanceFont_SP_SetBoldWidth(int sp_no, float bold_width)
{
	get_sp_properties(sp_no)->ts.bold_width = bold_width;
}

static void SengokuRanceFont_SP_SetEdgeColor(int sp_no, int r, int g, int b)
{
	get_sp_properties(sp_no)->ts.edge_color = (SDL_Color) { r, g, b, 255 };
}

static void SengokuRanceFont_SP_SetEdgeWidth(int sp_no, float edge_width)
{
	get_sp_properties(sp_no)->ts.edge_width = edge_width;
}

static void SengokuRanceFont_SP_SetScaleX(int sp_no, float scale_x)
{
	get_sp_properties(sp_no)->ts.scale_x = scale_x;
}

static void SengokuRanceFont_SP_SetSpaceScaleX(int sp_no, float space_scale_x)
{
	get_sp_properties(sp_no)->ts.space_scale_x = space_scale_x;
}

static float SengokuRanceFont_SP_GetFontSize(int sp_no)
{
	return get_sp_properties(sp_no)->ts.size;
}

static int SengokuRanceFont_SP_GetFontType(int sp_no)
{
	return get_sp_properties(sp_no)->ts.font_type + 256;
}

static void SengokuRanceFont_SP_GetFontColor(int sp_no, int *r, int *g, int *b)
{
	SDL_Color *c = &get_sp_properties(sp_no)->ts.color;
	*r = c->r;
	*g = c->g;
	*b = c->b;
}

static float SengokuRanceFont_SP_GetBoldWidth(int sp_no)
{
	return get_sp_properties(sp_no)->ts.bold_width;
}

static void SengokuRanceFont_SP_GetEdgeColor(int sp_no, int *r, int *g, int *b)
{
	SDL_Color *c = &get_sp_properties(sp_no)->ts.edge_color;
	*r = c->r;
	*g = c->g;
	*b = c->b;
}

static float SengokuRanceFont_SP_GetEdgeWidth(int sp_no)
{
	return get_sp_properties(sp_no)->ts.edge_width;
}

static float SengokuRanceFont_SP_GetScaleX(int sp_no)
{
	return get_sp_properties(sp_no)->ts.scale_x;
}

static float SengokuRanceFont_SP_GetSpaceScaleX(int sp_no)
{
	return get_sp_properties(sp_no)->ts.space_scale_x;
}

static float SengokuRanceFont_GetActualFontSize(int font_type, float font_size)
{
	font_type = get_font_type(font_type);
	return fnl_get_font_size(&fontlib->fonts[font_type], font_size)->size;
}

static float SengokuRanceFont_GetActualFontSizeRoundDown(int font_type, float font_size)
{
	font_type = get_font_type(font_type);
	return fnl_get_font_size_round_down(&fontlib->fonts[font_type], font_size)->size;
}

//static int SengokuRanceFont_SP_GetTextLastCharCode(int spriteNumber);
//static void SengokuRanceFont_SP_SetTextLastCharCode(int spriteNumber, int lastChar);
//static int SengokuRanceFont_SP_GetReduceDescender(int spriteNumber);

static void SengokuRanceFont_SP_SetReduceDescender(int sp_no, int reduce_descender)
{
	if (reduce_descender)
		WARNING("SengokuRanceFont.SP_SetReduceDescender(%d, %d)", sp_no, reduce_descender);
}

static void SengokuRanceFont_ModuleInit(void)
{
	UDIR *dir;
	char *d_name;
	char path[PATH_MAX];

	if (!(dir = opendir_utf8(config.game_dir))) {
		ERROR("Failed to open directory: %s", display_utf0(config.game_dir));
	}

	while ((d_name = readdir_utf8(dir))) {
		size_t name_len = strlen(d_name);
		if (name_len > 4 && strcasecmp(d_name+name_len-4, ".fnl")) {
			free(d_name);
			continue;
		}

		snprintf(path, PATH_MAX, "%s/%s", config.game_dir, d_name);
		free(d_name);

		if (!(fontlib = fnl_open(path)))
			ERROR("Error opening font library '%s'", display_utf0(path));
		if (fontlib->nr_fonts < 1)
			ERROR("Font library doesn't contain any fonts");
		break;
	}

	closedir_utf8(dir);

	current_text_properties = default_text_properties;
	text_style_update(&current_text_properties.ts);
}

static void SengokuRanceFont_ModuleFini(void)
{
	fnl_renderer_free(fontlib);
}

HLL_LIBRARY(SengokuRanceFont,
	    HLL_EXPORT(_ModuleInit, SengokuRanceFont_ModuleInit),
	    HLL_EXPORT(_ModuleFini, SengokuRanceFont_ModuleFini),
	    //HLL_EXPORT(AlphaComposite, SengokuRanceFont_AlphaComposite),
	    HLL_EXPORT(SetFontType, SengokuRanceFont_SetFontType),
	    HLL_EXPORT(SetFontSize, SengokuRanceFont_SetFontSize),
	    HLL_EXPORT(SetFontColor, SengokuRanceFont_SetFontColor),
	    HLL_EXPORT(SetBoldWidth, SengokuRanceFont_SetBoldWidth),
	    HLL_EXPORT(SetEdgeColor, SengokuRanceFont_SetEdgeColor),
	    HLL_EXPORT(SetEdgeWidth, SengokuRanceFont_SetEdgeWidth),
	    HLL_EXPORT(GetTextWidth, SengokuRanceFont_GetTextWidth),
	    //HLL_EXPORT(GetCharacterWidth, SengokuRanceFont_GetCharacterWidth),
	    HLL_EXPORT(SetTextX, SengokuRanceFont_SetTextX),
	    HLL_EXPORT(SetTextY, SengokuRanceFont_SetTextY),
	    HLL_EXPORT(SetTextPos, SengokuRanceFont_SetTextPos),
	    HLL_EXPORT(SetScaleX, SengokuRanceFont_SetScaleX),
	    //HLL_EXPORT(DrawTextToSprite, SengokuRanceFont_DrawTextToSprite),
	    //HLL_EXPORT(DrawTextPreload, SengokuRanceFont_DrawTextPreload),
	    //HLL_EXPORT(SetFontFileName, SengokuRanceFont_SetFontFileName),
	    HLL_EXPORT(SetCharacterSpacing, SengokuRanceFont_SetCharacterSpacing),
	    HLL_EXPORT(SetSpaceScaleX, SengokuRanceFont_SetSpaceScaleX),
	    HLL_EXPORT(SetReduceDescender, SengokuRanceFont_SetReduceDescender),
	    HLL_EXPORT(SP_ClearState, SengokuRanceFont_SP_ClearState),
	    HLL_EXPORT(SP_GetTextCharSpace, SengokuRanceFont_SP_GetTextCharSpace),
	    HLL_EXPORT(SP_GetTextHomeX, SengokuRanceFont_SP_GetTextHomeX),
	    HLL_EXPORT(SP_GetTextHomeY, SengokuRanceFont_SP_GetTextHomeY),
	    HLL_EXPORT(SP_GetTextLineSpace, SengokuRanceFont_SP_GetTextLineSpace),
	    HLL_EXPORT(SP_GetTextPosX, SengokuRanceFont_SP_GetTextPosX),
	    HLL_EXPORT(SP_GetTextPosY, SengokuRanceFont_SP_GetTextPosY),
	    HLL_EXPORT(SP_SetTextCharSpace, SengokuRanceFont_SP_SetTextCharSpace),
	    HLL_EXPORT(SP_SetTextHome, SengokuRanceFont_SP_SetTextHome),
	    HLL_EXPORT(SP_SetTextLineSpace, SengokuRanceFont_SP_SetTextLineSpace),
	    HLL_EXPORT(SP_SetTextPos, SengokuRanceFont_SP_SetTextPos),
	    HLL_EXPORT(SP_TextClear, SengokuRanceFont_SP_TextClear),
	    HLL_EXPORT(SP_TextCopy, SengokuRanceFont_SP_TextCopy),
	    HLL_EXPORT(SP_TextHome, SengokuRanceFont_SP_TextHome),
	    HLL_EXPORT(SP_TextNewLine, SengokuRanceFont_SP_TextNewLine),
	    HLL_EXPORT(SP_SetTextMetricsClassic, SengokuRanceFont_SP_SetTextMetricsClassic),
	    HLL_EXPORT(SP_SetTextStyle, SengokuRanceFont_SP_SetTextStyle),
	    HLL_EXPORT(SP_TextDrawClassic, SengokuRanceFont_SP_TextDrawClassic),
	    HLL_EXPORT(SP_TextDraw, SengokuRanceFont_SP_TextDraw),
	    HLL_EXPORT(SP_TextDrawPreload, SengokuRanceFont_SP_TextDrawPreload),
	    HLL_EXPORT(SP_SetFontSize, SengokuRanceFont_SP_SetFontSize),
	    HLL_EXPORT(SP_SetFontType, SengokuRanceFont_SP_SetFontType),
	    HLL_EXPORT(SP_SetFontColor, SengokuRanceFont_SP_SetFontColor),
	    HLL_EXPORT(SP_SetBoldWidth, SengokuRanceFont_SP_SetBoldWidth),
	    HLL_EXPORT(SP_SetEdgeColor, SengokuRanceFont_SP_SetEdgeColor),
	    HLL_EXPORT(SP_SetEdgeWidth, SengokuRanceFont_SP_SetEdgeWidth),
	    HLL_EXPORT(SP_SetScaleX, SengokuRanceFont_SP_SetScaleX),
	    HLL_EXPORT(SP_SetSpaceScaleX, SengokuRanceFont_SP_SetSpaceScaleX),
	    HLL_EXPORT(SP_GetFontSize, SengokuRanceFont_SP_GetFontSize),
	    HLL_EXPORT(SP_GetFontType, SengokuRanceFont_SP_GetFontType),
	    HLL_EXPORT(SP_GetFontColor, SengokuRanceFont_SP_GetFontColor),
	    HLL_EXPORT(SP_GetBoldWidth, SengokuRanceFont_SP_GetBoldWidth),
	    HLL_EXPORT(SP_GetEdgeColor, SengokuRanceFont_SP_GetEdgeColor),
	    HLL_EXPORT(SP_GetEdgeWidth, SengokuRanceFont_SP_GetEdgeWidth),
	    HLL_EXPORT(SP_GetScaleX, SengokuRanceFont_SP_GetScaleX),
	    HLL_EXPORT(SP_GetSpaceScaleX, SengokuRanceFont_SP_GetSpaceScaleX),
	    HLL_EXPORT(GetActualFontSize, SengokuRanceFont_GetActualFontSize),
	    HLL_EXPORT(GetActualFontSizeRoundDown, SengokuRanceFont_GetActualFontSizeRoundDown),
	    //HLL_EXPORT(SP_GetTextLastCharCode, SengokuRanceFont_SP_GetTextLastCharCode),
	    //HLL_EXPORT(SP_SetTextLastCharCode, SengokuRanceFont_SP_SetTextLastCharCode),
	    //HLL_EXPORT(SP_GetReduceDescender, SengokuRanceFont_SP_GetReduceDescender),
	    HLL_EXPORT(SP_SetReduceDescender, SengokuRanceFont_SP_SetReduceDescender)
	);
