/*-----------------------------------------------------------------------------
	Paper Plane cUI													画面描画
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPCOMBO.H"
#pragma hdrstop

const TCHAR StrSpace[] = T(" ");
const TCHAR StrMark[] = T("Mark:");
const TCHAR StrEntry[] = T("Entry:");
const TCHAR StrFree[] = T("Free:");
const TCHAR StrUsed[] = T("Used:");
const TCHAR StrTotal[] = T("Total:");
const TCHAR StrCache[] = T("(cache:%d%%)");
const TCHAR StrPPc[] = T("Paper Plane cUI (c)TORO");

#define SizeDispLen 8 // 各種サイズの標準表示桁数 "xxx,xxxK" or " xxx.xxK"

void PaintEnterState(PPC_APPINFO *cinfo, PAINTSTRUCT *ps)
{
	TCHAR buf[3];

#ifdef UNICODE
	buf[0] = (WCHAR)0x21b5;
#else
	buf[0] = 'v';
#endif
	buf[1] = (TCHAR)(cinfo->DDpagemode - DDPAGEMODE_DIR_ENTER + '0');
	buf[2] = '\0';
	DxSetTextColor(cinfo->DxDraw, ps->hdc, C_res[0]);		// 文字色
	DxSetBkColor(cinfo->DxDraw, ps->hdc, C_res[1]);
	DxMoveToEx(cinfo->DxDraw, ps->hdc,
			cinfo->MouseStat.PushClientPoint.x,
			cinfo->MouseStat.PushClientPoint.y - cinfo->fontY);
	DxTextOutBack(cinfo->DxDraw, ps->hdc, buf, 2);
}

void PaintFixPathBar(PPC_APPINFO *cinfo, HDC hDC)
{
	TCHAR path[VFPS + 0x80], *ptr;
	int len;

	ptr = tstrchr(cinfo->Caption, ')');
	ptr = (ptr != NULL) ? ptr + 1 : cinfo->path;
	len = thprintf(path, TSIZEOF(path), T("[%s]%s"), cinfo->RegSubCID + 1, ptr) - path;
	DxTextOutRel(cinfo->DxDraw, hDC, path, len);
}

void PaintPathBar(PPC_APPINFO *cinfo, PAINTSTRUCT *ps)
{
	COLORREF bg;
	RECT box;
	POINT LP;
	int oldBKmode;

	oldBKmode = DxSetBkMode(cinfo->DxDraw, ps->hdc, OPAQUE);
	if ( hComboFocusWnd == cinfo->info.hWnd ){
		DxSetTextColor(cinfo->DxDraw, ps->hdc, C_ActiveText);
		bg = C_ActiveBack;
	}else{
		HWND hPairWnd;
		int base;

		base = GetPairPaneComboBaseIndex(hComboFocusWnd);
		if ( base < 0 ){
			hPairWnd = NULL;
		}else{
			hPairWnd = Combo.base[base].hWnd;
		}
		if ( cinfo->info.hWnd == hPairWnd ){
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_PairText);
			bg = C_PairBack;
		}else{
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_InActiveText);
			bg = C_InActiveBack;
		}
	}
	DxSetBkColor(cinfo->DxDraw, ps->hdc, bg);
#ifdef USEDIRECTX
	IfDXmode(ps->hdc) {
		box.left   = ps->rcPaint.left;
		box.top    = ps->rcPaint.top;
		box.right  = ps->rcPaint.right;
		box.bottom = cinfo->BoxStatus.top;
		DxDrawBack(cinfo->DxDraw, ps->hdc, &box, bg | 0xff000000);
	}
#endif
	DxMoveToEx(cinfo->DxDraw, ps->hdc, 0, cinfo->X_lspc);

	if ( RunasState != NULL ){
		PaintFixPathBar(cinfo, ps->hdc);
	}else{
		DxTextOutRel(cinfo->DxDraw, ps->hdc, cinfo->Caption + 3, tstrlen32(cinfo->Caption + 3));
	}

	DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
	if ( cinfo->X_lspc ){
		HBRUSH hB;

		hB = CreateSolidBrush(bg);
		box.left   = 0;
		box.top    = 0;
		box.right  = LP.x;
		box.bottom = cinfo->X_lspc;
		DxFillBack(cinfo->DxDraw, ps->hdc, &box, hB);
		DeleteObject(hB);
	}
	if ( LP.x < ps->rcPaint.right ){
		HBRUSH hB;

		hB = CreateSolidBrush(bg);
		box.left   = LP.x;
		box.top    = ps->rcPaint.top;
		box.right  = ps->rcPaint.right;
		box.bottom = cinfo->BoxStatus.top;
		DxFillBack(cinfo->DxDraw, ps->hdc, &box, hB);
		DeleteObject(hB);
	}
#if 0	// 最小化&閉じる ボタン表示
	if ( X_combos ){
		int len;

		box.top    = 0;
		box.bottom = cinfo->BoxStatus.top;
		len = (box.bottom - box.top);
		box.right  = cinfo->wnd.Area.cx - len;
		box.left   = box.right - len;
		SetTextAlign(ps->hdc, TA_LEFT | TA_TOP);
		DrawFrameControl(ps->hdc, &box, DFC_CAPTION, DFCS_CAPTIONMIN);
		box.left += len;
		box.right += len;
		DrawFrameControl(ps->hdc, &box, DFC_CAPTION, DFCS_CAPTIONCLOSE);

		SetTextAlign(ps->hdc, TA_LEFT | TA_TOP | TA_UPDATECP);
	}
#endif
	DxSetBkMode(cinfo->DxDraw, ps->hdc, oldBKmode);
}

void DispLimFormatNumber( DIRECTXARG(DXDRAWSTRUCT *DxDraw) HDC hDC, TCHAR *buf, UINTHL num)
{
	if ( num.u.HighPart != MAX32 ){
		FormatNumber(buf, CXFN_SUM, SizeDispLen, num.u.LowPart, num.u.HighPart);
		buf[SizeDispLen] = ' ';
		buf[SizeDispLen + 1] = '\0';
	}else{
		tstrcpy(buf, T("-------- "));
	}
	DxSetTextColor(DxDraw, hDC, C_info);
	DxTextOutRel(DxDraw, hDC, buf, SizeDispLen + 1);
}

#define VLine(hdc, x) {linebox.left = x; linebox.right = linebox.left + 1; DxFillRectColor(cinfo->DxDraw, hdc, &linebox, cinfo->linebrush, LINE_NORMAL);}
#define STATUS_UNDERLINE 1
void PaintStatusLine(PPC_APPINFO *cinfo, DIRECTXARG(DXDRAWSTRUCT *DxDraw) PAINTSTRUCT *ps, RECT *BoxStatus, ENTRYINDEX EI_No)
{
	TCHAR buf[VFPS], buf1[VFPS];
	int fontY;
	RECT box;
	POINT LP;

	box.top = BoxStatus->top;
	LP.x = BoxStatus->left;
	LP.y = box.top + (cinfo->X_lspc >> 1);
	box.bottom = BoxStatus->bottom - STATUS_UNDERLINE;
	DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
	fontY = cinfo->fontY;
	if ( EI_No == DISPENTRY_NO_OUTPANE ){
		TEXTMETRIC tm;

		GetTextMetrics(ps->hdc, &tm);
		fontY = tm.tmHeight + cinfo->X_lspc;
		if ( fontY <= 0 ) fontY = 1;
	}

	if ( cinfo->stat.height > 1 ){ // XC_stat表示(２行以上の時) ***************
		box.left = LP.x;
		box.right = BoxStatus->right;

		DxSetBkColor(cinfo->DxDraw, ps->hdc, cinfo->BackColor);
		DispEntry(cinfo, ps->hdc, DIRECTXARG(cinfo->DxDraw) &cinfo->stat, EI_No, ps->rcPaint.right, &box);
		DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);

		if ( cinfo->bg.X_WallpaperType ){
			DxSetBkMode(cinfo->DxDraw, ps->hdc, OPAQUE);
		}
	}
									// 報告文字列表示 *************************
	if ( cinfo->PopMsgFlag & PMF_DISPLAYMASK ){
		DxSetTextColor(cinfo->DxDraw, ps->hdc, C_res[0]);		// 文字色
		DxSetBkColor(cinfo->DxDraw, ps->hdc, C_res[1]);
		DxTextOutBack(cinfo->DxDraw, ps->hdc, cinfo->PopMsgStr, tstrlen32(cinfo->PopMsgStr));
		DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
		box.left = LP.x;
		LP.x += 3;
		box.right = LP.x;

		if ( cinfo->bg.X_WallpaperType == 0 ){
			DxFillBack(cinfo->DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
		}
	}
									// 作業中表示 *********
	if ( TinyCheckCellEdit(cinfo) ){
		DxSetTextColor(cinfo->DxDraw, ps->hdc, C_info);
		DxSetBkColor(cinfo->DxDraw, ps->hdc, cinfo->BackColor);
		DxTextOutBack(cinfo->DxDraw, ps->hdc, StrBusy, StrBusyLength);
		DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
	}
									// 非同期読み込み状況表示 *********
	if ( cinfo->AloadCount ){
		int len;

		len = thprintf(buf, TSIZEOF(buf), StrCache, (cinfo->AloadCount * 100) / ((cinfo->e.cellDataMax > 0) ? cinfo->e.cellDataMax : 1) ) - buf;
		DxSetTextColor(cinfo->DxDraw, ps->hdc, C_info);
		DxSetBkColor(cinfo->DxDraw, ps->hdc, cinfo->BackColor);
		DxTextOutRel(cinfo->DxDraw, ps->hdc, buf, len);
		DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
	}
#if SHOWFRAMERATE
	{
		int len;

		len = thprintf(buf, TSIZEOF(buf), T("%03dfps"), cinfo->FrameRate);
		DxSetTextColor(cinfo->DxDraw, ps->hdc, C_info);
		DxSetBkColor(cinfo->DxDraw, ps->hdc, cinfo->BackColor);
		DxTextOutRel(cinfo->DxDraw, ps->hdc, buf, len);
		DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
	}
#endif
	if ( (cinfo->stat.height > 1) && cinfo->bg.X_WallpaperType ){
		DxSetBkMode(cinfo->DxDraw, ps->hdc, TRANSPARENT);
	}
									// スタック表示 ***********************
	if ( (cinfo->e.cellStack != 0) && (LP.x < ps->rcPaint.right) ){
		int i, x, xm, sx, s;
		ENTRYCELL *cell;

		if ( cinfo->bg.X_WallpaperType ) DxSetBkMode(cinfo->DxDraw, ps->hdc, OPAQUE);
		x = LP.x;
		xm = BoxStatus->right - x;
		sx = ((cinfo->e.cellStack * 14) <= (xm / cinfo->fontX + 1)) ?
				(14 * cinfo->fontX) :					// 余裕あるとき
				( (xm - 14 * cinfo->fontX) / cinfo->e.cellStack ); // 余裕ないとき
		s = min(sx, 3);
		DxSetTextColor(cinfo->DxDraw, ps->hdc, cinfo->BackColor);	// 文字色
		for ( i = 0 ; i < (cinfo->e.cellStack - 1) ; i++ ){
			cell = &CEL(cinfo->e.cellIMax + i);

			DxSetBkColor(cinfo->DxDraw, ps->hdc, C_entry[cell->type]);
			DxMoveToEx(cinfo->DxDraw, ps->hdc, x + i * sx, LP.y);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, cell->f.cFileName,
					max( 1, min(
							(size_t)(sx-1) / cinfo->fontX + 1,
							tstrlen32(cell->f.cFileName) ) ));
			DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
			if ( cinfo->bg.X_WallpaperType == 0 ){
				box.left   = min( x + (i + 1) * sx - s , LP.x );
				box.right  = x + (i + 1) * sx - 1;
				DxFillBack(cinfo->DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
			}
		}
		cell = &CEL(cinfo->e.cellIMax + i);
		DxSetBkColor(cinfo->DxDraw, ps->hdc, C_entry[cell->type]);
		DxMoveToEx(cinfo->DxDraw, ps->hdc, x + i * sx, LP.y);
		DxTextOutRel(cinfo->DxDraw, ps->hdc, cell->f.cFileName,
				tstrlen32(cell->f.cFileName));

		DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
		if ( cinfo->bg.X_WallpaperType ){
			DxSetBkMode(cinfo->DxDraw, ps->hdc, TRANSPARENT);
		}else{
			box.left  = LP.x;
			box.right = LP.x += 3;
			DxFillBack(cinfo->DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
		}
	}
									// XC_stat情報表示 ************************
	DxSetBkColor(cinfo->DxDraw, ps->hdc,
			(cinfo->stat.bc == C_AUTO) ? cinfo->BackColor : cinfo->stat.bc );
	if ( cinfo->stat.fmt[0] != 0 ){
		if ( cinfo->stat.height <= 1 ){ // XC_stat は、１行表示時のみここで表示
			box.left = LP.x;
			box.right = BoxStatus->right;

			DispEntry(cinfo, ps->hdc, DIRECTXARG(cinfo->DxDraw) &cinfo->stat, EI_No, ps->rcPaint.right, &box);
		}
	}else{	// デフォルト表示
		RECT linebox;
		int sepx;

		linebox.top = box.top;
		linebox.bottom = box.top + fontY;
		if ( cinfo->X_lspc ){
			if ( cinfo->bg.X_WallpaperType == 0 ){
				box.left  = ps->rcPaint.left;
				box.right = ps->rcPaint.right;
				box.bottom =  LP.y;
				DxFillBack(cinfo->DxDraw, ps->hdc, &box, cinfo->C_BackBrush);

				box.bottom = BoxStatus->bottom - STATUS_UNDERLINE;
				box.top  = box.bottom - (cinfo->X_lspc - (cinfo->X_lspc >> 1));
				DxFillBack(cinfo->DxDraw, ps->hdc, &box, cinfo->C_BackBrush);

				box.top = BoxStatus->top;
				linebox.bottom = box.bottom;
			}
		}
		#ifndef USEDIRECTX
		{
			SIZE spcsize;
			GetTextExtentPoint32(ps->hdc, StrSpace, 1, &spcsize);
			sepx = spcsize.cx >> 1;
		}
		#else
		IfGDImode(ps->hdc) {
			SIZE spcsize;
			GetTextExtentPoint32(ps->hdc, StrSpace, 1, &spcsize);
			sepx = spcsize.cx >> 1;
		}else{
			sepx = cinfo->fontX >> 1;
		}
		#endif
		if ( LP.x < ps->rcPaint.right ){			// マーク数・サイズ +++++++
			int len;

			DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_mes);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, StrMark, TSIZEOFSTR(StrMark));

			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_info);
			FormatNumber(buf1, CXFN_SUM, SizeDispLen, cinfo->e.MarkSize.s.L, cinfo->e.MarkSize.s.H);
			len = thprintf(buf, TSIZEOF(buf), T("%3u/%s "), cinfo->e.markC, buf1) - buf;
			DxTextOutRel(cinfo->DxDraw, ps->hdc, buf, len);

			DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
			VLine(ps->hdc, LP.x - sepx);
		}
		if ( LP.x < ps->rcPaint.right ){			// エントリ数 +++++++++++++
			DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_mes);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, StrEntry, TSIZEOFSTR(StrEntry));

			thprintf(buf, TSIZEOF(buf), T("%3u/%3u "), cinfo->e.cellIMax, cinfo->e.cellDataMax);
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_info);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, buf, tstrlen32(buf));

			DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
			VLine(ps->hdc, LP.x - sepx);
		}
		if ( LP.x < ps->rcPaint.right ){			// 空き容量 +++++++++++++++
			DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_mes);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, StrFree, TSIZEOFSTR(StrFree));

			DispLimFormatNumber( DIRECTXARG(cinfo->DxDraw) ps->hdc, buf, cinfo->DiskSizes.free);

			DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
			VLine(ps->hdc, LP.x - sepx);
		}
		if ( LP.x < ps->rcPaint.right ){			// 使用容量 +++++++++++++++
			int xm;
			BOOL ShowUseSize;

			xm = LP.x;
			if ( (cinfo->wnd.Area.cx - LP.x) > (27 * cinfo->fontX) ){
				ShowUseSize = TRUE;
				DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
				DxSetTextColor(cinfo->DxDraw, ps->hdc, C_mes);
				DxTextOutRel(cinfo->DxDraw, ps->hdc, StrUsed, TSIZEOFSTR(StrUsed));

				DispLimFormatNumber( DIRECTXARG(cinfo->DxDraw) ps->hdc, buf, cinfo->DiskSizes.used);

				DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
				VLine(ps->hdc, LP.x - sepx);
			}else{
				ShowUseSize = FALSE;
			}
			if ( ShowUseSize != cinfo->ShowUseSize ){
				box.left	= xm;
				box.right	= cinfo->wnd.Area.cx;
				InvalidateRect(cinfo->info.hWnd, &box, FALSE);	// 更新指定
			}
			cinfo->ShowUseSize = ShowUseSize;
		}
		if ( LP.x < ps->rcPaint.right ){			// 総容量 +++++++++++++++++
			DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_mes);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, StrTotal, TSIZEOFSTR(StrTotal));

			DispLimFormatNumber( DIRECTXARG(cinfo->DxDraw) ps->hdc, buf, cinfo->DiskSizes.total);

			DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
			VLine(ps->hdc, LP.x - sepx);
		}
		if ( LP.x < ps->rcPaint.right ){			// パスの種類 +++++++++++++
			if ( cinfo->e.Dtype.Name[0] ){
				DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
				DxSetTextColor(cinfo->DxDraw, ps->hdc, C_info);
				DxTextOutRel(cinfo->DxDraw, ps->hdc, cinfo->e.Dtype.Name, tstrlen32(cinfo->e.Dtype.Name));

				DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
				VLine(ps->hdc, LP.x - sepx);
			}
		}
		if ( LP.x < ps->rcPaint.right ){			// Title ++++++++++++++++++
			DxMoveToEx(cinfo->DxDraw, ps->hdc, LP.x, LP.y);
			DxSetTextColor(cinfo->DxDraw, ps->hdc, C_DBLACK);
			DxTextOutRel(cinfo->DxDraw, ps->hdc, StrPPc, TSIZEOFSTR(StrPPc));

			DxGetCurrentPositionEx(cinfo->DxDraw, ps->hdc, &LP);
		}
												// 右側の空白 +++++++++++++
		if ( LP.x < ps->rcPaint.right ){
			if ( cinfo->bg.X_WallpaperType == 0 ){
				box.left   = LP.x;
				box.right  = ps->rcPaint.right;
				DxFillBack(cinfo->DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
			}
		}
	}
}

void PaintInfoLine(PPC_APPINFO *cinfo, DIRECTXARG(DXDRAWSTRUCT *DxDraw) PAINTSTRUCT *ps, RECT *BoxInfo, ENTRYINDEX EI_No)
{
	RECT box;
	POINT LP;
	int fontY;

	fontY = cinfo->fontY;
	if ( EI_No == DISPENTRY_NO_OUTPANE ){
		TEXTMETRIC tm;

		#ifdef USEDIRECTX
		IfDXmode( ps->hdc ) {
			GetFontSizeDxDraw(DxDraw, &tm);
		}else
		#endif
		{
			GetTextMetrics(ps->hdc, &tm);
		}
		fontY = tm.tmHeight + cinfo->X_lspc;
		if ( fontY <= 0 ) fontY = 1;
	}

										// アイコン表示 ***********************
	if ( ps->rcPaint.left < cinfo->iconR ){
		switch ( cinfo->dset.infoicon ){
//			case DSETI_NOSPACE: // 詰める(上の if でスキップされるため不要)
			case DSETI_BLANK: // 空白
				if ( cinfo->bg.X_WallpaperType == 0 ){
					box.left   = 0;
					box.top    = BoxInfo->top;
					box.right  = cinfo->iconR;
					box.bottom = BoxInfo->bottom;
					DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
				}
				break;

			case DSETI_BOX: // 枠
				box.left   = 0;
				box.top    = BoxInfo->top;
				box.right  = cinfo->iconR - 1;
				box.bottom = BoxInfo->bottom;
				if ( cinfo->bg.X_WallpaperType == 0 ){
					DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
				}
				box.left = box.right;
				box.right++;
				DxFillRectColor(DxDraw, ps->hdc, &box, cinfo->linebrush, LINE_NORMAL);
				break;

//			case DSETI_EXTONLY:	// 拡張子のみ
//			case DSETI_NORMAL:	// 完全
//			case DSETI_OVL:		// 完全オーバレイ
//			case DSETI_OVLNOC:	// 完全オーバレイ(キャッシュ無し)
//			case DSETI_OVLSINGLE:	// 完全オーバレイ(キャッシュ無し+シングル)
			default:
			// アイコン表示とアイコン削除が被らないようにする
			if ( !TinyCheckCellEdit(cinfo) ){
				DWORD boxH, iconH;
				int iconY;

				EnterCellEdit(cinfo);

				boxH = BoxInfo->bottom - BoxInfo->top;
				iconH = (boxH > (DWORD)cinfo->iconR) ? (DWORD)cinfo->iconR : boxH;
				iconY = BoxInfo->bottom - iconH;
				if ( BoxInfo == &cinfo->BoxInfo ) iconH--;
				#ifdef USEDIRECTX
				IfDXmode( ps->hdc ) {
					box.left = BoxInfo->left;
					box.top = iconY;
					box.right = cinfo->iconR;
					box.bottom = iconH;
					if ( IsTrue(cinfo->InfoIcon_DirtyCache) ){
						FreeInfoIconCache(cinfo);
					}
					DxDrawIcon(DxDraw, cinfo->hInfoIcon, &box, &cinfo->InfoIcon_Cache);
				}else
				#endif
				{
					DrawIconEx(ps->hdc, BoxInfo->left, iconY,
							cinfo->hInfoIcon, cinfo->iconR, iconH, 0,
							cinfo->bg.X_WallpaperType ?
									NULL : cinfo->C_BackBrush, DI_NORMAL);
				}
				LeaveCellEdit(cinfo);
				if ( (BoxInfo->top != iconY) && (cinfo->bg.X_WallpaperType == 0) ){
					box.left   = 0;
					box.top    = BoxInfo->top;
					box.right  = cinfo->iconR;
					box.bottom = iconY;
					DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
				}
				break;
			}
		}
	}
	if ( ps->rcPaint.right < cinfo->iconR ) return;
	if ( (BoxInfo->bottom - BoxInfo->top) > (fontY * 2) ){
		if ( cinfo->bg.X_WallpaperType == 0 ){
			box.left   = cinfo->iconR;
			box.top    = BoxInfo->top;
			box.right  = ps->rcPaint.right;
			box.bottom = BoxInfo->bottom - (fontY * 2);
			DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
		}
	}

	DxSetTextColor(DxDraw, ps->hdc, C_mes);
	DxSetBkColor(DxDraw, ps->hdc, cinfo->BackColor);
									// MENU 表示 **************************
	if ( cinfo->HMpos >= 0 ){
		int i;
		int x, xm, y;
		char **data;

		i = 0;
		data = cinfo->HiddenMenu.data;
		xm = (cinfo->HiddenMenu.item + 1) >> 1;
		box.top = BoxInfo->bottom - (fontY * 2) + (cinfo->X_lspc >> 1);

		if ( cinfo->bg.X_WallpaperType == 0 ){
			if ( box.top > BoxInfo->top ){
				box.bottom = box.top;
				box.top = BoxInfo->top;
				box.left = cinfo->iconR;
				box.right = ps->rcPaint.right;
				DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
				box.top = box.bottom;
			}
		}

		box.right = cinfo->iconR;
		for ( y = 0 ; y < 2 ; y++, box.top += fontY ){
			box.bottom = box.top + fontY;
			box.left = cinfo->iconR;
			for ( x = 0 ; x < xm ; x++, box.left = box.right ){
				DxMoveToEx(DxDraw, ps->hdc, box.left, box.top);
				box.right = box.left + cinfo->fontX * cinfo->HiddenMenu.width;

				if ( cinfo->X_lspc && (cinfo->bg.X_WallpaperType == 0) ){	// 行間消去
					DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
				}

				if ( i < cinfo->HiddenMenu.item ){
					int namelen;
					COLORREF *color;

					namelen = HiddenMenu_GetNameLen(*data);
					color = HiddenMenu_Color(*data, namelen + 1);
					if ( (i == cinfo->HMpos) /* && ((cinfo->DownHMpos < 0) || (cinfo->HMpos == cinfo->DownHMpos)) */ ){
					// ※現在、IntervalSubThread でDownHMpos<0 になる
						if ( cinfo->bg.X_WallpaperType ){
							DxSetBkMode(DxDraw, ps->hdc, OPAQUE);
						}
						DxSetTextColor(DxDraw, ps->hdc, color[HiddenMenu_ColorB]);
						DxSetBkColor(DxDraw, ps->hdc, color[HiddenMenu_ColorF]);
					}else{
						DxSetTextColor(DxDraw, ps->hdc, color[HiddenMenu_ColorF]);
						DxSetBkColor(DxDraw, ps->hdc, color[HiddenMenu_ColorB]);
					}
					DxTextOutRel(DxDraw, ps->hdc, (TCHAR *)*data, namelen);
					DxGetCurrentPositionEx(DxDraw, ps->hdc, &LP);
					box.left = LP.x;
					if ( (i == cinfo->HMpos) && cinfo->bg.X_WallpaperType ){
						DxSetBkMode(DxDraw, ps->hdc, TRANSPARENT);
					}
					i++;
				}
				data++;
				// 未描画部分と空白を消去
				if ( cinfo->bg.X_WallpaperType == 0 ){
					DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
				}
			}
		}
		DxSetTextColor(DxDraw, ps->hdc, C_mes);		// 文字色
		if ( box.right < ps->rcPaint.right ){
			box.left	= box.right;
			box.top		= BoxInfo->top;
			box.right	= ps->rcPaint.right;
			box.bottom	= BoxInfo->bottom - 1;
			if ( cinfo->bg.X_WallpaperType == 0 ){
				DxFillBack(DxDraw, ps->hdc, &box, cinfo->C_BackBrush);
			}
		}
	}else{
		box.left = cinfo->iconR;
		box.right = BoxInfo->right;
		box.bottom = BoxInfo->bottom - fontY * cinfo->inf2.height;
		if ( ps->rcPaint.top < box.bottom ){	// 2行目:XC_inf1(LFN名) *******
#if 0
			TCHAR buf[200];

			thprintf(buf, TSIZEOF(buf),
					T("VArea(%d,%d),Area(%d,%d),cellWMin:%d,cellN:%d"),
					cinfo->cel.VArea.cx, cinfo->cel.VArea.cy,
					cinfo->cel.Area.cx, cinfo->cel.Area.cy,
					cinfo->cellWMin, cinfo->e.cellN);
			DxSetTextColor(DxDraw, ps->hdc, C_WindowBack);
			DxSetBkColor(DxDraw, ps->hdc, C_WindowText);
			DxMoveToEx(DxDraw, ps->hdc, cinfo->iconR, BoxInfo->top);

			DxTextOutRel(DxDraw, ps->hdc, buf, tstrlen(buf));
#else
			box.top = box.bottom - fontY * cinfo->inf1.height;
			DispEntry(cinfo, ps->hdc, DIRECTXARG(DxDraw) &cinfo->inf1, EI_No, ps->rcPaint.right, &box);
#endif
		}
		if ( ps->rcPaint.bottom >= box.bottom ){ // 3行目:XC_inf2(情報表示) ***
			box.top = box.bottom;
			box.bottom = BoxInfo->bottom;
			DispEntry(cinfo, ps->hdc, DIRECTXARG(DxDraw) &cinfo->inf2, EI_No, ps->rcPaint.right, &box);
		}
	}
}

void Paint(PPC_APPINFO *cinfo)
{
	PAINTSTRUCT ps;
	HGDIOBJ hOldFont;
	RECT box;
	HDC hOldDC C4701CHECK;
	int oldBKmode C4701CHECK;
	DWORD DrawTargetFlags;

// 表示環境の設定 -------------------------------------------------------------
  #ifndef USEDIRECTX
	BeginPaint(cinfo->info.hWnd, &ps);
  #else
	if ( cinfo->DxDraw == NULL ){
		BeginPaint(cinfo->info.hWnd, &ps);
	}else{
		if ( BeginDxDraw(cinfo->DxDraw, &ps) == DXSTART_NODRAW ) return;
	}
  #endif
	if ( cinfo->e.cellDataMax <= 0 ) goto PaintLast;

  #if DRAWMODE != DRAWMODE_DW
	DrawTargetFlags = cinfo->DrawTargetFlags;
	if ( DrawTargetFlags == 0 ) DrawTargetFlags = DRAWT_ALL;
  #else
	DrawTargetFlags = DRAWT_ALL;
  #endif
	cinfo->DrawTargetFlags = 0;

	IfGDImode(ps.hdc) {
		if ( X_fles != 0 ){
			InitOffScreen(&cinfo->bg, ps.hdc, &cinfo->wnd.NCArea);
			hOldDC = ps.hdc;
			ps.hdc = cinfo->bg.hOffScreenDC;

			if ( X_fles == 2 ){
				ps.rcPaint.left = 0;
				ps.rcPaint.top = 0;
				ps.rcPaint.right = cinfo->wnd.NCArea.cx;
				ps.rcPaint.bottom = cinfo->wnd.NCArea.cy;
				ps.fErase = TRUE;
			}
		}
		hOldFont = SelectObject(ps.hdc, cinfo->hBoxFont);
		SetTextAlign(ps.hdc, TA_LEFT | TA_TOP | TA_UPDATECP); // CP を有効に
	}

	if ( cinfo->bg.X_WallpaperType ){
		DrawWallPaper(DIRECTXARG(cinfo->DxDraw) &cinfo->bg, cinfo->info.hWnd, &ps, cinfo->wnd.Area.cx);
		oldBKmode = DxSetBkMode(cinfo->DxDraw, ps.hdc, TRANSPARENT);
		DrawTargetFlags = DRAWT_ALL;
	}
#if USEDELAYCURSOR
	else if ( IsTrue(cinfo->freeCell) ){
		DxFillBack(cinfo->DxDraw, ps.hdc, &ps.rcPaint, cinfo->C_BackBrush);
		oldBKmode = DxSetBkMode(cinfo->DxDraw, ps.hdc, TRANSPARENT);
		DrawTargetFlags = DRAWT_ALL;
	}
	if ( IsTrue(cinfo->freeCell) ){ // 遅延移動カーソルを表示
		#ifndef USEDIRECTX
			DrawGradientBox(ps.hdc,
					cinfo->cellNpos.x, cinfo->cellNpos.y,
					cinfo->cellNpos.x + cinfo->cel.Size.cx,
					cinfo->cellNpos.y + cinfo->cel.Size.cy,
					cinfo->cellNbc,
					C_eInfo[!(cinfo->X_inag & INAG_UNFOCUS) ? ECS_UDCSR : ECS_NOFOCUS]);
		#else
			box.left = cinfo->cellNpos.x;
			box.top  = cinfo->cellNpos.y;
			box.right = cinfo->cellNpos.x + cinfo->cel.Size.cx;
			box.bottom = cinfo->cellNpos.y + cinfo->cel.Size.cy;
			DxDrawCursor(cinfo->DxDraw, ps.hdc, &box,
					C_eInfo[(cinfo->X_inag & INAG_UNFOCUS) ? ECS_NOFOCUS : ECS_SELECT]);
		#endif
		DrawTargetFlags = DRAWT_ALL;
	}
#endif
// ０行目：パス表示 -----------------------------------------------------------
	if ( (ps.rcPaint.top < cinfo->BoxStatus.top) &&
		 (DrawTargetFlags & DRAWT_PATHLINE) ){
		PaintPathBar(cinfo, &ps);
	}
// １行目：各種情報 -----------------------------------------------------------
	if ( (DrawTargetFlags & DRAWT_STATUSLINE) &&
		 (cinfo->BoxStatus.top < cinfo->BoxStatus.bottom) ){
#ifndef USRDIRECTX
		if ( ps.rcPaint.top < cinfo->BoxStatus.bottom ){
			PaintStatusLine(cinfo, DIRECTXARG(cinfo->DxDraw) &ps, &cinfo->BoxStatus, DISPENTRY_NO_INPANE);
		}
#endif
// 1行目下端：区切り線 -------------------------------------------------------
		if ( (ps.rcPaint.top <= (cinfo->BoxStatus.bottom - 1)) &&
			 (ps.rcPaint.bottom >= (cinfo->BoxStatus.bottom - 1)) ){
			box.top = cinfo->BoxStatus.bottom - 1;
			box.bottom = box.top + 1;
			box.left = ps.rcPaint.left;
			box.right = ps.rcPaint.right;
			if ( cinfo->fontY >= 48 ) box.top -= cinfo->fontY / 48;
			DxFillRectColor(cinfo->DxDraw, ps.hdc, &box, cinfo->linebrush, LINE_NORMAL);
		}
#ifdef USRDIRECTX
		if ( ps.rcPaint.top < cinfo->BoxStatus.bottom ){
			PaintStatusLine(cinfo, DIRECTXARG(cinfo->DxDraw) &ps, &cinfo->BoxStatus, DISPENTRY_NO_INPANE);
		}
#endif
	}
// 2/3行目：選択ファイル情報/MENU ---------------------------------------------
	if ( DrawTargetFlags & DRAWT_INFOLINE ){
#ifndef USRDIRECTX
		if ( (cinfo->BoxInfo.bottom != cinfo->BoxInfo.top) &&
			 (ps.rcPaint.top < cinfo->BoxInfo.bottom) &&
			 (ps.rcPaint.bottom >= cinfo->BoxInfo.top) ){
			PaintInfoLine(cinfo, DIRECTXARG(cinfo->DxDraw) &ps, &cinfo->BoxInfo, DISPENTRY_NO_INPANE);
		}
#endif
// ３行目下端：区切り線 -------------------------------------------------------
		if ( (ps.rcPaint.top <= (cinfo->BoxInfo.bottom - 1)) &&
			 (ps.rcPaint.bottom >= (cinfo->BoxInfo.bottom - 1)) ){
			box.top = cinfo->BoxInfo.bottom - 1;
			box.bottom = box.top + 1;
			box.left = ps.rcPaint.left;
			box.right = ps.rcPaint.right;
			if ( cinfo->fontY >= 48 ) box.top -= cinfo->fontY / 48;
			DxFillRectColor(cinfo->DxDraw, ps.hdc, &box, cinfo->linebrush, LINE_NORMAL);
		}
#ifdef USRDIRECTX
		if ( (cinfo->BoxInfo.bottom != cinfo->BoxInfo.top) &&
			 (ps.rcPaint.top < cinfo->BoxInfo.bottom) &&
			 (ps.rcPaint.bottom >= cinfo->BoxInfo.top) ){
			PaintInfoLine(cinfo, DIRECTXARG(cinfo->DxDraw) &ps, &cinfo->BoxInfo, DISPENTRY_NO_INPANE);
		}
#endif
	}
// ４行以降：ファイル表示 -----------------------------------------------------
	if ( cinfo->cel.Size.cx && cinfo->cel.Size.cy &&
		 (DrawTargetFlags & (DRAWT_ENTRY | DRAWT_1ENTRY)) ){
		int xLef, xRig, y, yTop, yBtm;
		RECT *ebox;

		cinfo->RedrawCheck = FALSE;
		if ( DrawTargetFlags & DRAWT_ENTRY ){
			ebox = &ps.rcPaint;
		}else{ // DRAWT_ENTRY が設定されていない(DRAWT_1ENTRYのみ)
			ebox = &cinfo->DrawTargetCell;
		}

		xLef = (ebox->left - cinfo->BoxEntries.left ) / cinfo->cel.Size.cx;
		if ( xLef < 0 ) xLef = 0;
		box.right = xLef * cinfo->cel.Size.cx + cinfo->BoxEntries.left;

		xRig = (ebox->right - cinfo->BoxEntries.left - 1) / cinfo->cel.Size.cx;
//		if ( xRig >= cinfo->cel.VArea.cx ) xRig = cinfo->cel.VArea.cx - 1;
		// if ( xRig < 0 ) xRig = 0; // x で範囲調整している

		yTop = (ebox->top - cinfo->BoxEntries.top) / cinfo->cel.Size.cy;
		if ( yTop < 0 ) yTop = 0;

		yBtm = (ebox->bottom - cinfo->BoxEntries.top - 1) / cinfo->cel.Size.cy;
		if ( yBtm >= cinfo->cel.VArea.cy ) yBtm = cinfo->cel.VArea.cy - 1;

		if ( cinfo->EntriesOffset.x | cinfo->EntriesOffset.y ){
			int dx, dy;
			if ( cinfo->EntriesOffset.x ){
				int dd = cinfo->EntriesOffset.x % cinfo->cel.Size.cx;
				dx = cinfo->EntriesOffset.x / cinfo->cel.Size.cx;
				if ( dd > 0 ){ // 左追加
					xLef -= dx + 1;
					box.right += dd;
				}else{
					xRig += dx + 1;
					box.right += dd;
				}
			}else{
				dy = cinfo->EntriesOffset.y / cinfo->cel.Size.cy;
				if ( cinfo->EntriesOffset.y > 0 ){
					yBtm += dy + 1;
				}else{
					yTop -= dy + 1;
				}
			}
		}

		if ( cinfo->list.orderZ == 0 ){ // N 並び
			xLef *= cinfo->cel.VArea.cy;
			xRig *= cinfo->cel.VArea.cy;
			for ( ; xLef <= xRig ; xLef += cinfo->cel.Area.cy){
				box.left = box.right;
				box.right += cinfo->cel.Size.cx;

				box.bottom = yTop * cinfo->cel.Size.cy + cinfo->BoxEntries.top;
				for ( y = yTop ; y <= yBtm ; y++){
					box.top = box.bottom;
					box.bottom += cinfo->cel.Size.cy;
#if FREEPOSMODE
					{
						int tmpno;

						tmpno = cinfo->cellWMin + xLef + y;
						if ( (tmpno >= cinfo->e.cellM) || (CEL(tmpno).pos.x != NOFREEPOS) ){
							if ( !cinfo->bg.X_WallpaperType && !cinfo->freeCell ){
								DxFillBack(cinfo->DxDraw, ps.hdc, &box, cinfo->C_BackBrush);
							}
						}else{
							DispEntry(cinfo, ps.hdc, DIRECTXARG(cinfo->DxDraw)
									&cinfo->celF, tmpno,
									ps.rcPaint.right, &box);
						}
					}
#else
					DispEntry(cinfo, ps.hdc, DIRECTXARG(cinfo->DxDraw)
							&cinfo->celF, cinfo->cellWMin + xLef + y,
							ps.rcPaint.right, &box);
#endif
				}
			}
		}else{ // Z 並び
			int x, xl;

			xl = box.right;

			if ( xRig >= cinfo->cel.VArea.cx ){ // 表示段数より右が空いている→右を消去する
				xRig = cinfo->cel.VArea.cx - 1;

				if ( cinfo->bg.X_WallpaperType == 0 ){
					box.top = cinfo->BoxEntries.top;
					box.left	= cinfo->BoxEntries.left + (cinfo->cel.Size.cx * cinfo->cel.VArea.cx);
					box.right	= ps.rcPaint.right;
					box.bottom	= cinfo->BoxEntries.bottom;
					DxFillBack(cinfo->DxDraw, ps.hdc, &box, cinfo->C_BackBrush);
				}
			}

			box.bottom = (yTop * cinfo->cel.Size.cy) + cinfo->BoxEntries.top;
			yTop *= cinfo->cel.Area.cx;
			yBtm *= cinfo->cel.Area.cx;
//  XMessage(NULL, NULL, XM_DbgLOG, T("%d %d %d Y %d-%d X %d-%d"),cinfo->cel.Area.cx,cinfo->cel.VArea.cx,cinfo->cellWMin, yTop,yBtm, xLef,xRig);

			for ( y = yTop ; y <= yBtm ; y += cinfo->cel.Area.cx){
				box.top = box.bottom;
				box.bottom += cinfo->cel.Size.cy;
				box.right = xl;
				for ( x = xLef; x <= xRig ; x++){
					box.left = box.right;
					box.right += cinfo->cel.Size.cx;
//					XMessage(NULL, NULL, XM_DbgLOG, T("Z:%d %d,%d "), x + y, box.left, box.top);
					DispEntry(cinfo, ps.hdc, DIRECTXARG(cinfo->DxDraw)
							&cinfo->celF, cinfo->cellWMin + x + y,
							ps.rcPaint.right, &box);
				}
			}
		}
#if FREEPOSMODE
		{ // 浮動エントリを全て表示
			int i, index;

			for ( i = 0 ; i < cinfo->FreePosEntries ; i++ ){
				index = cinfo->FreePosList[i].index;

				box.left   = CEL(index).pos.x - CalcFreePosOffX(cinfo);
				box.top    = CEL(index).pos.y - CalcFreePosOffY(cinfo);
				box.right  = box.left + cinfo->cel.Size.cx;
				box.bottom = box.top + cinfo->cel.Size.cy;

				DispEntry(cinfo, ps.hdc, DIRECTXARG(cinfo->DxDraw)
						&cinfo->celF, cinfo->FreePosList[i].index,
						ps.rcPaint.right, &box);
			}
		}
#endif
		memset(&cinfo->DrawTargetCell, 0, sizeof(cinfo->DrawTargetCell));
	}
// ホバー表示
	if ( (cinfo->DDpagemode >= DDPAGEMODE_DIR_ENTER) && (cinfo->DDpagemode <= DDPAGEMODE_DIR_COUNTDOWN) ){
		PaintEnterState(cinfo, &ps);
	}

// 最下部分：余白 -----------------------------------------------------
	IfGDImode(ps.hdc) {
		if ( cinfo->bg.X_WallpaperType == 0 ){
			box.top = cinfo->BoxEntries.bottom;
			if ( ps.rcPaint.bottom > box.top ){
				box.left	= max(ps.rcPaint.left, cinfo->BoxEntries.left);
				box.right	= ps.rcPaint.right;
				box.bottom	= ps.rcPaint.bottom;
				DxFillBack(cinfo->DxDraw, ps.hdc, &box, cinfo->C_BackBrush);
			}
		}else{
// 終了処理
			DxSetBkMode(cinfo->DxDraw, ps.hdc, oldBKmode); // C4701ok
		}
		SetTextAlign(ps.hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP); // CP を無効
		SelectObject(ps.hdc, hOldFont);	// フォント

		if ( X_fles ){
			ps.hdc = hOldDC; // C4701ok
			OffScreenToScreen(&cinfo->bg, cinfo->info.hWnd, hOldDC,
					cinfo->combo ? NULL : &cinfo->wnd.NCRect, &cinfo->wnd.Area);
		}
	}

PaintLast:
#ifndef USEDIRECTX
	EndPaint(cinfo->info.hWnd, &ps);
#else
	IfDXmode(ps.hdc) {
		EndDxDraw(cinfo->DxDraw);
	}else{
		EndPaint(cinfo->info.hWnd, &ps);
	}
#endif

#if SHOWFRAMERATE
	{
		DWORD tick = GetTickCount() / 1000;

		cinfo->FrameCount++;
		if ( tick != cinfo->FrameTime ){
			cinfo->FrameTime = tick;
			cinfo->FrameRate = cinfo->FrameCount;
			cinfo->FrameCount = 0;
		}
		if ( (GetAsyncKeyState(VK_CONTROL) & KEYSTATE_PUSH) &&
			 (GetAsyncKeyState('L') & KEYSTATE_PUSH) ){
			InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
			DxSetBenchmarkMode(cinfo->DxDraw, TRUE);
		}else{
			DxSetBenchmarkMode(cinfo->DxDraw, FALSE);
		}
	}
#endif
}
/*-----------------------------------------------------------------------------
-----------------------------------------------------------------------------*/
void Repaint(PPC_APPINFO *cinfo)
{
	cinfo->DrawTargetFlags = DRAWT_ALL;
	InvalidateRect(cinfo->info.hWnd, NULL, FALSE);
	if ( cinfo->combo ){
		// ●一体化時に、１枚目の表示に失敗するようなので入れてみた 1.69+3
		UpdateWindow(cinfo->info.hWnd);

		PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_drawinfo, 0);
		PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_drawstatus, 0);
	}
	if ( cinfo->docks.t.hWnd != NULL ){
		InvalidateRect(cinfo->docks.t.hWnd, NULL, FALSE);
	}
	if ( cinfo->docks.b.hWnd != NULL ){
		InvalidateRect(cinfo->docks.b.hWnd, NULL, FALSE);
	}
	if ( cinfo->hHeaderWnd != NULL ){
		InvalidateRect(cinfo->hHeaderWnd, NULL, FALSE);
	}
	if ( cinfo->hScrollBarWnd != NULL ){
		InvalidateRect(cinfo->hScrollBarWnd, NULL, FALSE);
	}
}

void RefleshInfoBox(PPC_APPINFO *cinfo, int flags)
{
	RECT rect;

	// パス行
	if ( DE_ATTR_PATH & flags ){
		rect = cinfo->BoxStatus;
		rect.bottom = rect.top;
		rect.top = 0;
		InvalidateRect(cinfo->info.hWnd, &rect, FALSE);
	}
	// ステータス行
	if ( cinfo->stat.attr & flags ){
		RefleshStatusLine(cinfo);
	}
	if ( cinfo->DrawTargetFlags != 0 ){
		setflag(cinfo->DrawTargetFlags, DRAWT_INFOLINE);
	}
	// 情報行
	if ( (cinfo->inf1.attr | cinfo->inf2.attr ) & flags ){
		DocksInfoRepaint(&cinfo->docks);
		PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_drawinfo, 0);
		if ( !cinfo->combo || !(X_combos[0] & CMBS_COMMONINFO) ){
			rect = cinfo->BoxInfo;
			rect.bottom--;
			InvalidateRect(cinfo->info.hWnd, &rect, FALSE);
		}
	}
}
// ステータス行を画面更新指定する
void RefleshStatusLine(PPC_APPINFO *cinfo)
{
	RECT rect;

	if ( cinfo->DrawTargetFlags != 0 ){
		setflag(cinfo->DrawTargetFlags, DRAWT_STATUSLINE);
	}
	rect = cinfo->BoxStatus;
	rect.bottom--;
	InvalidateRect(cinfo->info.hWnd, &rect, FALSE);	// 更新指定

	DocksStatusRepaint(&cinfo->docks);
	if ( cinfo->combo ){
		SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_drawstatus, 0);
	}
}

// Cell 位置を画面更新指定する
void RefleshCell(PPC_APPINFO *cinfo, int cell)
{
	RECT rect, rect2;

#if FREEPOSMODE
	if ( CEL(cell).pos.x != NOFREEPOS ){
		rect.left = CEL(cell).pos.x - CalcFreePosOffX(cinfo);
		rect.top  = CEL(cell).pos.y - CalcFreePosOffY(cinfo);
	}else{
#endif
		int deltaNo;

		deltaNo = cell - cinfo->cellWMin;
		rect.left = CalcCellX(cinfo, deltaNo);
		rect.top  = CalcCellY(cinfo, deltaNo);
#if FREEPOSMODE
	}
#endif
	rect.right	= rect.left + cinfo->cel.Size.cx;
	rect.bottom	= rect.top	+ cinfo->cel.Size.cy;
	InvalidateRect(cinfo->info.hWnd, &rect, FALSE);	// 更新指定
	UnionRect(&rect2, &rect, &cinfo->DrawTargetCell);
	cinfo->DrawTargetCell = rect2;
	if ( (cell == cinfo->e.cellN) && !(cinfo->X_inag & INAG_UNFOCUS) ){
		SetCaretPos(rect.left, rect.top + (cinfo->fontY >> 1));
	}

	if ( cinfo->HMpos != -1 ){
		UpdateWindow_Part(cinfo->info.hWnd);
		cinfo->HMpos = -1;
		rect = cinfo->BoxInfo;
		rect.bottom--;
		InvalidateRect(cinfo->info.hWnd, &rect, FALSE);	// 更新指定
		if ( cinfo->DrawTargetFlags != 0 ){
			setflag(cinfo->DrawTargetFlags, DRAWT_INFOLINE);
		}
	}
}
