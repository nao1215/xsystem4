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

#include <math.h>
#include <time.h>
#include "hll.h"
#include "../system4.h"
#include "../cg.h"
#include "../queue.h"
#include "../sdl_core.h"

struct SACT2_sprite {
	TAILQ_ENTRY(SACT2_sprite) entry;
	struct cg *cg;
	Point pos;
	int z;
	int width, height;
	int r, g, b, a;
	bool show;
	bool used;
};

static struct SACT2_sprite *sprites = NULL;
static int nr_sprites = 0;

TAILQ_HEAD(listhead, SACT2_sprite) visible_sprites =
	TAILQ_HEAD_INITIALIZER(visible_sprites);

static void sprite_set_show(struct SACT2_sprite *sp, bool show)
{
	if (!sp->show == !show)
		return;

	sp->show = show;

	if (show) {
		struct SACT2_sprite *p;
		TAILQ_FOREACH(p, &visible_sprites, entry) {
			if (p->z > sp->z) {
				TAILQ_INSERT_BEFORE(p, sp, entry);
				return;
			}
		}
		TAILQ_INSERT_TAIL(&visible_sprites, sp, entry);
	} else {
		TAILQ_REMOVE(&visible_sprites, sp, entry);
	}
}

// int Init(imain_system pIMainSystem, int nCGCacheSize)
hll_defun(Init, args)
{

	sdl_initialize();
	hll_return(1);
}

// int Error(string strErr)
hll_unimplemented(SACT2, Error)
// int SetWP(int nCG)
hll_unimplemented(SACT2, SetWP)
// int SetWP_Color(int nR, int nG, int nB)
hll_unimplemented(SACT2, SetWP_Color)
// int WP_GetSP(int nSP)
hll_unimplemented(SACT2, WP_GetSP)
// int WP_SetSP(int nSP)
hll_unimplemented(SACT2, WP_SetSP)

// int GetScreenWidth(void)
hll_defun(GetScreenWidth, _)
{
	hll_return(config.view_width);
}

// int GetScreenHeight(void)
hll_defun(GetScreenHeight, _)
{
	hll_return(config.view_height);
}

// int GetMainSurfaceNumber(void)
hll_unimplemented(SACT2, GetMainSurfaceNumber)
// int Update(void)
hll_unimplemented(SACT2, Update)
// int Effect(int nType, int nTime, int nfKey)
hll_unimplemented(SACT2, Effect)
// int EffectSetMask(int nCG)
hll_unimplemented(SACT2, EffectSetMask)
// int EffectSetMaskSP(int nSP)
hll_unimplemented(SACT2, EffectSetMaskSP)
// void QuakeScreen(int nAmplitudeX, int nAmplitudeY, int nTime, int nfKey)
hll_unimplemented(SACT2, QuakeScreen)
// void QUAKE_SET_CROSS(int nAmpX, int nAmpY)
hll_unimplemented(SACT2, QUAKE_SET_CROSS)
// void QUAKE_SET_ROTATION(int nAmp, int nCycle)
hll_unimplemented(SACT2, QUAKE_SET_ROTATION)

// int SP_GetUnuseNum(int nMin)
hll_defun(SP_GetUnuseNum, args)
{
	int nMin = args[0].i;

	for (int i = nMin; i < nr_sprites; i++) {
		if (!sprites[i].used)
			hll_return(i);
	}

	int n = max(nMin, nr_sprites);
	nr_sprites = n + 256;
	sprites = xrealloc(sprites, sizeof(struct SACT2_sprite) * nr_sprites);
	hll_return(n);
}

// int SP_Count(void)
hll_unimplemented(SACT2, SP_Count)
// int SP_Enum(ref array@int anSP)
hll_unimplemented(SACT2, SP_Enum)
// int SP_GetMaxZ(void)
hll_unimplemented(SACT2, SP_GetMaxZ)

// int SP_SetCG(int nSP, int nCG)
hll_defun(SP_SetCG, args)
{
	int sp = args[0].i;
	int cg = args[1].i;

	WARNING("SACT2.SP_SetCG(%d, %d)", sp, cg);
	sprites[sp].cg = cg_load(cg);
	hll_return(!!sprites[sp].cg);
}

// int SP_SetCGFromFile(int nSP, string pIStringFileName)
hll_unimplemented(SACT2, SP_SetCGFromFile)
// int SP_SaveCG(int nSP, string pIStringFileName)
hll_unimplemented(SACT2, SP_SaveCG)

// int SP_Create(int nSP, int nWidth, int nHeight, int nR, int nG, int nB, int nBlendRate)
hll_defun(SP_Create, args)
{
	int sp     = args[0].i;
	int width  = args[1].i;
	int height = args[2].i;
	int r      = args[3].i;
	int g      = args[4].i;
	int b      = args[5].i;
	int a      = args[6].i;

	sprites[sp] = (struct SACT2_sprite) {
		.pos = (Point) {
			.x = 0,
			.y = 0
		},
		.z = 0,
		.width = width,
		.height = height,
		.r = r,
		.g = g,
		.b = b,
		.a = a,
		.cg = NULL,
		.show = false,
		.used = true
	};
	sprite_set_show(&sprites[sp], true);
	hll_return(1);
}

// int SP_CreatePixelOnly(int nSP, int nWidth, int nHeight)
hll_unimplemented(SACT2, SP_CreatePixelOnly)
// int SP_CreateCustom(int nSP)
hll_unimplemented(SACT2, SP_CreateCustom)
// int SP_Delete(int nSP)
hll_unimplemented(SACT2, SP_Delete)
// int SP_SetPos(int nSP, int nX, int nY)
hll_defun(SP_SetPos, args)
{
	int sp = args[0].i;
	sprites[sp].pos.x = args[1].i;
	sprites[sp].pos.y = args[2].i;
	hll_return(1);
}

// int SP_SetX(int nSP, int nX)
hll_defun(SP_SetX, args)
{
	sprites[args[0].i].pos.x = args[1].i;
	hll_return(1);
}

// int SP_SetY(int nSP, int nY)
hll_defun(SP_SetY, args)
{
	sprites[args[0].i].pos.y = args[1].i;
	hll_return(1);
}

// int SP_SetZ(int nSP, int nZ)
hll_defun(SP_SetZ, args)
{
	sprites[args[0].i].z = args[1].i;
	hll_return(1);
}

// int SP_SetBlendRate(int nSP, int nBlendRate)
hll_defun(SP_SetBlendRate, args)
{
	sprites[args[0].i].a = args[1].i;
	hll_return(1);
}

// int SP_SetShow(int nSP, int nfShow)
hll_defun(SP_SetShow, args)
{
	sprite_set_show(&sprites[args[0].i], args[1].i);
	hll_return(1);
}

// int SP_SetDrawMethod(int nSP, int nMethod)
hll_unimplemented(SACT2, SP_SetDrawMethod)
// int SP_IsUsing(int nSP)
hll_unimplemented(SACT2, SP_IsUsing)
// int SP_ExistAlpha(int nSP)
hll_unimplemented(SACT2, SP_ExistAlpha)

// int SP_GetPosX(int nSP)
hll_defun(SP_GetPosX, args)
{
	hll_return(sprites[args[0].i].pos.x);
}

// int SP_GetPosY(int nSP)
hll_defun(SP_GetPosY, args)
{
	hll_return(sprites[args[0].i].pos.y);
}

// int SP_GetWidth(int nSP)
hll_defun(SP_GetWidth, args)
{
	struct SACT2_sprite *sp = &sprites[args[0].i];
	if (sp->cg)
		hll_return(sp->cg->width);
	hll_return(sp->width);
}

// int SP_GetHeight(int nSP)
hll_defun(SP_GetHeight, args)
{
	struct SACT2_sprite *sp = &sprites[args[0].i];
	if (sp->cg)
		hll_return(sp->cg->height);
	hll_return(sp->width);
}

// int SP_GetZ(int nSP)
hll_defun(SP_GetZ, args)
{
	hll_return(sprites[args[0].i].z);
}

// int SP_GetBlendRate(int nSP)
hll_defun(SP_GetBlendRate, args)
{
	hll_return(sprites[args[0].i].a);
}

// int SP_GetShow(int nSP)
hll_defun(SP_GetShow, args)
{
	hll_return(sprites[args[0].i].show);
}

// int SP_GetDrawMethod(int nSP)
hll_warn_unimplemented(SACT2, SP_GetDrawMethod, 0)
// int SP_SetTextHome(int nSP, int nX, int nY)
hll_warn_unimplemented(SACT2, SP_SetTextHome, 0)
// int SP_SetTextLineSpace(int nSP, int nPx)
hll_unimplemented(SACT2, SP_SetTextLineSpace)
// int SP_SetTextCharSpace(int nSP, int nPx)
hll_unimplemented(SACT2, SP_SetTextCharSpace)
// int SP_SetTextPos(int nSP, int nX, int nY)
hll_warn_unimplemented(SACT2, SP_SetTextPos, 0)
// int SP_TextDraw(int nSP, string text, struct tm)
hll_unimplemented(SACT2, SP_TextDraw)
// int SP_TextClear(int nSP)
hll_unimplemented(SACT2, SP_TextClear)
// int SP_TextHome(int nSP, int nTextSize)
hll_unimplemented(SACT2, SP_TextHome)
// int SP_TextNewLine(int nSP, int nTextSize)
hll_unimplemented(SACT2, SP_TextNewLine)
// int SP_TextBackSpace(int nSP)
hll_unimplemented(SACT2, SP_TextBackSpace)
// int SP_TextCopy(int nDstSP, int nSrcSP)
hll_unimplemented(SACT2, SP_TextCopy)
// int SP_GetTextHomeX(int nSP)
hll_unimplemented(SACT2, SP_GetTextHomeX)
// int SP_GetTextHomeY(int nSP)
hll_unimplemented(SACT2, SP_GetTextHomeY)
// int SP_GetTextCharSpace(int nSP)
hll_unimplemented(SACT2, SP_GetTextCharSpace)
// int SP_GetTextPosX(int nSP)
hll_unimplemented(SACT2, SP_GetTextPosX)
// int SP_GetTextPosY(int nSP)
hll_unimplemented(SACT2, SP_GetTextPosY)
// int SP_GetTextLineSpace(int nSP)
hll_unimplemented(SACT2, SP_GetTextLineSpace)
// int SP_IsPtIn(int nSP, int nX, int nY)
hll_unimplemented(SACT2, SP_IsPtIn)
// int SP_IsPtInRect(int nSP, int nX, int nY)
hll_unimplemented(SACT2, SP_IsPtInRect)
// int GAME_MSG_GetNumof(void)
hll_unimplemented(SACT2, GAME_MSG_GetNumof)
// void GAME_MSG_Get(int nIndex, ref string text)
hll_unimplemented(SACT2, GAME_MSG_Get)
// void IntToZenkaku(ref string s, int nValue, int nFigures, int nfZeroPadding)
hll_unimplemented(SACT2, IntToZenkaku)
// void IntToHankaku(ref string s, int nValue, int nFigures, int nfZeroPadding)
hll_unimplemented(SACT2, IntToHankaku)
// int StringPopFront(ref string sDst, ref string sSrc)
hll_unimplemented(SACT2, StringPopFront)
// int Mouse_GetPos(ref int pnX, ref int pnY)
hll_unimplemented(SACT2, Mouse_GetPos)
// int Mouse_SetPos(int nX, int nY)
hll_unimplemented(SACT2, Mouse_SetPos)
// void Mouse_ClearWheel(void)
hll_unimplemented(SACT2, Mouse_ClearWheel)
// void Mouse_GetWheel(ref int pnForward, ref int pnBack)
hll_unimplemented(SACT2, Mouse_GetWheel)
// void Joypad_ClearKeyDownFlag(int nNum)
hll_unimplemented(SACT2, Joypad_ClearKeyDownFlag)
// int Joypad_IsKeyDown(int nNum, int nKey)
hll_unimplemented(SACT2, Joypad_IsKeyDown)
// int Joypad_GetNumof(void)
hll_unimplemented(SACT2, Joypad_GetNumof)
// void JoypadQuake_Set(int nNum, int nType, int nMagnitude)
hll_unimplemented(SACT2, JoypadQuake_Set)
// int Key_ClearFlag(void)
hll_unimplemented(SACT2, Key_ClearFlag)
// int Key_IsDown(int nKeyCode)
hll_unimplemented(SACT2, Key_IsDown)

// int Timer_Get(void)
hll_defun(Timer_Get, _)
{
	int ms;
	time_t s;
	struct timespec spec;

	clock_gettime(CLOCK_MONOTONIC, &spec);

	s = spec.tv_sec;
	ms = lround(spec.tv_nsec / 1.0e6);
	if (ms > 999) {
		s++;
		ms = 0;
	}
	hll_return(ms);
}

// int CG_IsExist(int nCG)
hll_defun(CG_IsExist, args)
{
	hll_return(cg_exists(args[0].i));
}

// int CG_GetMetrics(int nCG, ref struct cm)
hll_unimplemented(SACT2, CG_GetMetrics)
// int CSV_Load(string pIStringFileName)
hll_unimplemented(SACT2, CSV_Load)
// int CSV_Save(void)
hll_unimplemented(SACT2, CSV_Save)
// int CSV_SaveAs(string pIStringFileName)
hll_unimplemented(SACT2, CSV_SaveAs)
// int CSV_CountLines(void)
hll_unimplemented(SACT2, CSV_CountLines)
// int CSV_CountColumns(void)
hll_unimplemented(SACT2, CSV_CountColumns)
// void CSV_Get(ref string pIString, int nLine, int nColumn)
hll_unimplemented(SACT2, CSV_Get)
// int CSV_Set(int nLine, int nColumn, string pIStringData)
hll_unimplemented(SACT2, CSV_Set)
// int CSV_GetInt(int nLine, int nColumn)
hll_unimplemented(SACT2, CSV_GetInt)
// void CSV_SetInt(int nLine, int nColumn, int nData)
hll_unimplemented(SACT2, CSV_SetInt)
// void CSV_Realloc(int nLines, int nColumns)
hll_unimplemented(SACT2, CSV_Realloc)
// int Music_IsExist(int nNum)
hll_unimplemented(SACT2, Music_IsExist)
// int Music_Prepare(int nCh, int nNum)
hll_unimplemented(SACT2, Music_Prepare)
// int Music_Unprepare(int nCh)
hll_unimplemented(SACT2, Music_Unprepare)
// int Music_Play(int nCh)
hll_unimplemented(SACT2, Music_Play)
// int Music_Stop(int nCh)
hll_unimplemented(SACT2, Music_Stop)
// int Music_IsPlay(int nCh)
hll_unimplemented(SACT2, Music_IsPlay)
// int Music_SetLoopCount(int nCh, int nCount)
hll_unimplemented(SACT2, Music_SetLoopCount)
// int Music_GetLoopCount(int nCh)
hll_unimplemented(SACT2, Music_GetLoopCount)
// int Music_SetLoopStartPos(int nCh, int dwPos)
hll_unimplemented(SACT2, Music_SetLoopStartPos)
// int Music_SetLoopEndPos(int nCh, int dwPos)
hll_unimplemented(SACT2, Music_SetLoopEndPos)
// int Music_Fade(int nCh, int nTime, int nVolume, int bStop)
hll_unimplemented(SACT2, Music_Fade)
// int Music_StopFade(int nCh)
hll_unimplemented(SACT2, Music_StopFade)
// int Music_IsFade(int nCh)
hll_unimplemented(SACT2, Music_IsFade)
// int Music_Pause(int nCh)
hll_unimplemented(SACT2, Music_Pause)
// int Music_Restart(int nCh)
hll_unimplemented(SACT2, Music_Restart)
// int Music_IsPause(int nCh)
hll_unimplemented(SACT2, Music_IsPause)
// int Music_GetPos(int nCh)
hll_unimplemented(SACT2, Music_GetPos)
// int Music_GetLength(int nCh)
hll_unimplemented(SACT2, Music_GetLength)
// int Music_GetSamplePos(int nCh)
hll_unimplemented(SACT2, Music_GetSamplePos)
// int Music_GetSampleLength(int nCh)
hll_unimplemented(SACT2, Music_GetSampleLength)
// int Music_Seek(int nCh, int dwPos)
hll_unimplemented(SACT2, Music_Seek)
// int Sound_IsExist(int nNum)
hll_unimplemented(SACT2, Sound_IsExist)
// int Sound_GetUnuseChannel(void)
hll_unimplemented(SACT2, Sound_GetUnuseChannel)
// int Sound_Prepare(int nCh, int nNum)
hll_unimplemented(SACT2, Sound_Prepare)
// int Sound_Unprepare(int nCh)
hll_unimplemented(SACT2, Sound_Unprepare)
// int Sound_Play(int nCh)
hll_unimplemented(SACT2, Sound_Play)
// int Sound_Stop(int nCh)
hll_unimplemented(SACT2, Sound_Stop)
// int Sound_IsPlay(int nCh)
hll_unimplemented(SACT2, Sound_IsPlay)
// int Sound_SetLoopCount(int nCh, int nCount)
hll_unimplemented(SACT2, Sound_SetLoopCount)
// int Sound_GetLoopCount(int nCh)
hll_unimplemented(SACT2, Sound_GetLoopCount)
// int Sound_Fade(int nCh, int nTime, int nVolume, int bStop)
hll_unimplemented(SACT2, Sound_Fade)
// int Sound_StopFade(int nCh)
hll_unimplemented(SACT2, Sound_StopFade)
// int Sound_IsFade(int nCh)
hll_unimplemented(SACT2, Sound_IsFade)
// int Sound_GetPos(int nCh)
hll_unimplemented(SACT2, Sound_GetPos)
// int Sound_GetLength(int nCh)
hll_unimplemented(SACT2, Sound_GetLength)
// int Sound_ReverseLR(int nCh)
hll_unimplemented(SACT2, Sound_ReverseLR)
// int Sound_GetVolume(int nCh)
hll_unimplemented(SACT2, Sound_GetVolume)
// int Sound_GetTimeLength(int nCh)
hll_unimplemented(SACT2, Sound_GetTimeLength)
// int Sound_GetGroupNum(int nCh)
hll_unimplemented(SACT2, Sound_GetGroupNum)
// bool Sound_PrepareFromFile(int nCh, string szFileName)
hll_unimplemented(SACT2, Sound_PrepareFromFile)
// void System_GetDate(ref int pnYear, ref int pnMonth, ref int pnDay, ref int pnDayOfWeek)
hll_unimplemented(SACT2, System_GetDate)
// void System_GetTime(ref int pnHour, ref int pnMinute, ref int pnSecond, ref int pnMilliSeconds)
hll_unimplemented(SACT2, System_GetTime)
// void CG_RotateRGB(int nDest, int nDx, int nDy, int nWidth, int nHeight, int nRotateType)
hll_unimplemented(SACT2, CG_RotateRGB)
// void CG_BlendAMapBin(int nDest, int nDx, int nDy, int nSrc, int nSx, int nSy, int nWidth, int nHeight, int nBorder)
hll_unimplemented(SACT2, CG_BlendAMapBin)
// void Debug_Pause(void)
hll_unimplemented(SACT2, Debug_Pause)
// void Debug_GetFuncStack(ref string sz, int nNest)
hll_unimplemented(SACT2, Debug_GetFuncStack)
// int SP_GetAMapValue(int nSP, int nX, int nY)
hll_unimplemented(SACT2, SP_GetAMapValue)
// bool SP_GetPixelValue(int nSP, int nX, int nY, ref int pnR, ref int pnG, ref int pnB)
hll_unimplemented(SACT2, SP_GetPixelValue)
// int SP_SetBrightness(int nSP, int nBrightness)
hll_unimplemented(SACT2, SP_SetBrightness)
// int SP_GetBrightness(int nSP)
hll_unimplemented(SACT2, SP_GetBrightness)

hll_deflib(SACT2) {
	hll_export(Init),
	hll_export(Error),
	hll_export(SetWP),
	hll_export(SetWP_Color),
	hll_export(WP_GetSP),
	hll_export(WP_SetSP),
	hll_export(GetScreenWidth),
	hll_export(GetScreenHeight),
	hll_export(GetMainSurfaceNumber),
	hll_export(Update),
	hll_export(Effect),
	hll_export(EffectSetMask),
	hll_export(EffectSetMaskSP),
	hll_export(QuakeScreen),
	hll_export(QUAKE_SET_CROSS),
	hll_export(QUAKE_SET_ROTATION),
	hll_export(SP_GetUnuseNum),
	hll_export(SP_Count),
	hll_export(SP_Enum),
	hll_export(SP_GetMaxZ),
	hll_export(SP_SetCG),
	hll_export(SP_SetCGFromFile),
	hll_export(SP_SaveCG),
	hll_export(SP_Create),
	hll_export(SP_CreatePixelOnly),
	hll_export(SP_CreateCustom),
	hll_export(SP_Delete),
	hll_export(SP_SetPos),
	hll_export(SP_SetX),
	hll_export(SP_SetY),
	hll_export(SP_SetZ),
	hll_export(SP_SetBlendRate),
	hll_export(SP_SetShow),
	hll_export(SP_SetDrawMethod),
	hll_export(SP_IsUsing),
	hll_export(SP_ExistAlpha),
	hll_export(SP_GetPosX),
	hll_export(SP_GetPosY),
	hll_export(SP_GetWidth),
	hll_export(SP_GetHeight),
	hll_export(SP_GetZ),
	hll_export(SP_GetBlendRate),
	hll_export(SP_GetShow),
	hll_export(SP_GetDrawMethod),
	hll_export(SP_SetTextHome),
	hll_export(SP_SetTextLineSpace),
	hll_export(SP_SetTextCharSpace),
	hll_export(SP_SetTextPos),
	hll_export(SP_TextDraw),
	hll_export(SP_TextClear),
	hll_export(SP_TextHome),
	hll_export(SP_TextNewLine),
	hll_export(SP_TextBackSpace),
	hll_export(SP_TextCopy),
	hll_export(SP_GetTextHomeX),
	hll_export(SP_GetTextHomeY),
	hll_export(SP_GetTextCharSpace),
	hll_export(SP_GetTextPosX),
	hll_export(SP_GetTextPosY),
	hll_export(SP_GetTextLineSpace),
	hll_export(SP_IsPtIn),
	hll_export(SP_IsPtInRect),
	hll_export(GAME_MSG_GetNumof),
	hll_export(GAME_MSG_Get),
	hll_export(IntToZenkaku),
	hll_export(IntToHankaku),
	hll_export(StringPopFront),
	hll_export(Mouse_GetPos),
	hll_export(Mouse_SetPos),
	hll_export(Mouse_ClearWheel),
	hll_export(Mouse_GetWheel),
	hll_export(Joypad_ClearKeyDownFlag),
	hll_export(Joypad_IsKeyDown),
	hll_export(Joypad_GetNumof),
	hll_export(JoypadQuake_Set),
	hll_export(Key_ClearFlag),
	hll_export(Key_IsDown),
	hll_export(Timer_Get),
	hll_export(CG_IsExist),
	hll_export(CG_GetMetrics),
	hll_export(CSV_Load),
	hll_export(CSV_Save),
	hll_export(CSV_SaveAs),
	hll_export(CSV_CountLines),
	hll_export(CSV_CountColumns),
	hll_export(CSV_Get),
	hll_export(CSV_Set),
	hll_export(CSV_GetInt),
	hll_export(CSV_SetInt),
	hll_export(CSV_Realloc),
	hll_export(Music_IsExist),
	hll_export(Music_Prepare),
	hll_export(Music_Unprepare),
	hll_export(Music_Play),
	hll_export(Music_Stop),
	hll_export(Music_IsPlay),
	hll_export(Music_SetLoopCount),
	hll_export(Music_GetLoopCount),
	hll_export(Music_SetLoopStartPos),
	hll_export(Music_SetLoopEndPos),
	hll_export(Music_Fade),
	hll_export(Music_StopFade),
	hll_export(Music_IsFade),
	hll_export(Music_Pause),
	hll_export(Music_Restart),
	hll_export(Music_IsPause),
	hll_export(Music_GetPos),
	hll_export(Music_GetLength),
	hll_export(Music_GetSamplePos),
	hll_export(Music_GetSampleLength),
	hll_export(Music_Seek),
	hll_export(Sound_IsExist),
	hll_export(Sound_GetUnuseChannel),
	hll_export(Sound_Prepare),
	hll_export(Sound_Unprepare),
	hll_export(Sound_Play),
	hll_export(Sound_Stop),
	hll_export(Sound_IsPlay),
	hll_export(Sound_SetLoopCount),
	hll_export(Sound_GetLoopCount),
	hll_export(Sound_Fade),
	hll_export(Sound_StopFade),
	hll_export(Sound_IsFade),
	hll_export(Sound_GetPos),
	hll_export(Sound_GetLength),
	hll_export(Sound_ReverseLR),
	hll_export(Sound_GetVolume),
	hll_export(Sound_GetTimeLength),
	hll_export(Sound_GetGroupNum),
	hll_export(Sound_PrepareFromFile),
	hll_export(System_GetDate),
	hll_export(System_GetTime),
	hll_export(CG_RotateRGB),
	hll_export(CG_BlendAMapBin),
	hll_export(Debug_Pause),
	hll_export(Debug_GetFuncStack),
	hll_export(SP_GetAMapValue),
	hll_export(SP_GetPixelValue),
	hll_export(SP_SetBrightness),
	hll_export(SP_GetBrightness),
	NULL
};
