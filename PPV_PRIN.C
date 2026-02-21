/*-----------------------------------------------------------------------------
	Paper Plane vUI			Print
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPVUI.RH"
#include "PPV_STRU.H"
#include "PPV_FUNC.H"
#pragma hdrstop

//=============================================================================
BOOL PrintAbort;
HWND hDlgPrint;

typedef struct {
	SIZE PaperSize; // ページサイズ
	SIZE PrtSize; // 印刷可能サイズ
	SIZE DPI; // 解像度
	SIZE PageCount; // ページ数

	SIZE BmpSize;
	SIZE BmpWindow;
	int BmpZoom;

} PRINTPAGEINFO;

const TCHAR MAKETEXTERROR[] = T("MakeTextError");
const TCHAR CprinttitleString[] = T("PPv Continuation printing");
const TCHAR comdlg32Name[] = T("comdlg32.dll");

static PRINTDLG PDef = {
	sizeof(PRINTDLG),	// lStructSize
	NULL,				// hwndOwner
	NULL,				// hDevMode
	NULL,				// hDevNames
	NULL,				// hDC
	PD_ALLPAGES | PD_COLLATE | PD_RETURNDC | PD_NOSELECTION, // Flags
	1,					// nFromPage
	1,					// nToPage
	1,					// nMinPage
	1,					// nMaxPage
	1,					// nCopies
	NULL,				// hInstance
	0,					// lCustData
	NULL,				// lpfnPrintHook
	NULL,				// lpfnSetupHook
	NULL,				// lpPrintTemplateName
	NULL,				// lpSetupTemplateName
	NULL,				// hPrintTemplate
	NULL				// hSetupTemplate
};
int PrintPage(HDC hDC, int pageno, PRINTINFO *pinfo);
int PrintMain(HDC hDC, RECT *PrintRect, int pageno, PRINTINFO *pinfo);

void LoadPrint(void)
{
	if ( hComdlg32 != NULL ) return;
	hComdlg32 = LoadSystemDLL(SYSTEMDLL_COMDLG32);
	if ( hComdlg32 == NULL ) return;
	PPrintDlg = (impPPRINTDLG)GetProcAddress(hComdlg32, "PrintDlg" TAPITAIL);
}

void PPvDrawText(PAINTSTRUCT *ps, PPVPAINTSTRUCT *pps)
{
	int y, startY, i;
	BYTE *ptr;
	COLORREF fg, bg, deffg, defbg;
	POINT LP;
	RECT box;
	HFONT font;
	int XV_bctl3bk;
	MAKETEXTINFO mti;

	if ( (pps->drawYtop + pps->shift.cy) >= VOi->line  ) return;

	XV_bctl3bk = XV_bctl[2];
	XV_bctl[2] = 0;

	SetTextAlign(ps->hdc, TA_LEFT | TA_TOP | TA_UPDATECP);	// CP を有効に
												// 色の設定 -------------------
	fg = VOi->ti[pps->drawYtop + pps->shift.cy].Fclr;
	deffg = fg = (fg == CV__deftext) ? C_BLACK : CV_char[fg];
	bg = VOi->ti[pps->drawYtop + pps->shift.cy].Bclr;
	defbg = bg = (bg == CV__defback) ? C_WHITE : CV_char[bg];

	InitMakeTextInfo(&mti);
	mti.srcmax = mtinfo.img + mtinfo.MemSize;
	mti.writetbl = FALSE;
	mti.paintmode = TRUE;

	for ( y = pps->drawYtop ; y <= pps->drawYbottom ; y++ ){
		startY = y + pps->shift.cy;
		box.top = y * pps->lineY + pps->shiftdot.cy;
		box.bottom = box.top + pps->fontsize.cy;
		MoveToEx(ps->hdc, pps->shiftdot.cx, box.top, NULL);

		if ( startY >= VOi->line ){
			ptr = (BYTE *)"";
		}else{
												// 書体の設定 -----------------
			font = pps->hBoxFont;
			if ( VOi->ti[startY].type == VTYPE_IBM ) font = GetIBMFont();
			if ( VOi->ti[startY].type == VTYPE_ANSI ) font = GetANSIFont();
			SelectObject(ps->hdc, font);

			VOi->MakeText(&mti, &VOi->ti[startY]);
			ptr = mti.destbuf;
		}
		while( *ptr != VCODE_END ){
			switch(*ptr++){ // VCODE_SWITCH
				case VCODE_CONTROL:
				case VCODE_ASCII:	// Text 表示 ------------------------------
					SetTextColor(ps->hdc, fg);
					SetBkColor(ps->hdc, bg);
					i = strlen32((char *)ptr);
					TextOutA(ps->hdc, 0, 0, (char *)ptr, i);
					ptr += i + 1;
					break;

				case VCODE_UNICODEF:	// Dummy(WORD境界合わせ用) ------------
					ptr++;
				case VCODE_UNICODE:{	// Text 表示 --------------------------
					DWORD size = 0;
					WCHAR *last, *w;

					w = (WCHAR *)ptr;
					for ( last = w ; *last++ ; size++ );
					TextOutW(ps->hdc, 0, 0, w, size);
					ptr = (BYTE *)last;
					break;
				}

				case VCODE_COLOR:	// Color ----------------------------------
					fg = ( *ptr == CV__deftext ) ? C_BLACK : CV_char[*ptr];
					bg = ( *(ptr + 1) == CV__defback ) ? C_WHITE : CV_char[*(ptr + 1)];
					ptr += 2;
					break;

				case VCODE_F_COLOR:	// F Color --------------------------------
					fg = *(COLORREF *)ptr;
					ptr += sizeof(COLORREF);
					break;

				case VCODE_B_COLOR: {// B Color --------------------------------
					COLORREF nbg = *(COLORREF *)ptr;
					ptr += sizeof(COLORREF);
//					if ( nbg == C_BLACK ) nbg = C_BLACK;
					bg = nbg;
					break;
				}

				case VCODE_DEFCOLOR:
					bg = defbg;
				// VCODE_F_DEFCOLOR へ
				case VCODE_F_DEFCOLOR:
					fg = deffg;
					break;

				case VCODE_B_DEFCOLOR:
					bg = defbg;
					break;

				case VCODE_FONT:	// Font -----------------------------------
					switch(*ptr){
						case 0:
							font = GetIBMFont();
							break;
						case 1:
							font = GetANSIFont();
							break;
						default:
							font = pps->hBoxFont;
							break;
					}
					SelectObject(ps->hdc, font);
					ptr++;
					break;

				case VCODE_TAB: {	// Tab ------------------------------------
					int tabwidth;

					GetCurrentPositionEx(ps->hdc, &LP);
					box.left = LP.x;
					tabwidth = VOi->tab * pps->fontsize.cx;
					box.right = (((LP.x - pps->shiftdot.cx) / tabwidth) + 1) *
							tabwidth + pps->shiftdot.cx;
					MoveToEx(ps->hdc, box.right, box.top, NULL);
					break;
				}

				case VCODE_RETURN: // return ----------------------------------
					ptr++;
				case VCODE_PARA:
				case VCODE_PAGE:
					break;

				case VCODE_LINK:// Link ---------------------------------------
					SetTextColor(ps->hdc, CV_link);
					break;

				default:		// 未定義コード -------------------------------
					SetTextColor(ps->hdc, C_CYAN);
					TextOut(ps->hdc, 0, 0, MAKETEXTERROR, 13);
					ptr = (BYTE *)"";
					break;
			}
		}
		GetCurrentPositionEx(ps->hdc, &LP);
	}
	ReleaseMakeTextInfo(&mti);
	SetTextAlign(ps->hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP);	// CP を無効に
	XV_bctl[2] = XV_bctl3bk;
}
// 印刷・描画部分 -------------------------------------------------------------
// joint = 1	複数のファイル印刷をまとめて１文書とみなす
int PrintOneDocument(HDC hDC, DOCINFO *dinfo, BOOL joint, PRINTINFO *pinfo)
{
	int CopyDoc, CopyDocMax;
	int CopyPage, CopyPageMax;
	int Page;
	int status = 0;
							// 部数設定
//	if ( PDef.Flags & PD_COLLATE ){	// ドライバによっては反応しないので部数限定
		CopyDocMax	= PDef.nCopies;
		CopyPageMax	= 1;
//	}else{
//		CopyDocMax	= 1;
//		CopyPageMax	= PDef.nCopies;
//	}
	if ( ! joint ){
		status = StartDoc(hDC, dinfo);
		if ( status <= 0 ) return status;
	}
	for ( CopyDoc = 0 ; CopyDoc < CopyDocMax ; CopyDoc++ ){
		for ( Page = PDef.nFromPage - 1 ; Page < PDef.nToPage ; Page++ ){
			for ( CopyPage = 0 ; CopyPage < CopyPageMax ; CopyPage++ ){
				status = PrintPage(hDC, Page, pinfo);
				if ( status <= 0 ) return status;
						// 同じものを複数印刷する時は一旦初期化→両面印刷を考慮
				if ( !joint && (CopyPageMax > 1) ){
					status = EndDoc(hDC);
					if ( status <= 0 ) return status;
					if ( PrintAbort ) return -1;
					status = StartDoc(hDC, dinfo);
					if ( status <= 0 ) return status;
				}
			}
		}
						// 同じものを複数印刷する時は一旦初期化→両面印刷を考慮
		if ( !joint && (CopyDocMax > 1) && ((CopyDoc + 1) < CopyDocMax) ){
			status = EndDoc(hDC);
			if ( status <= 0 ) return status;
			if ( PrintAbort ) return -1;
			status = StartDoc(hDC, dinfo);
			if ( status <= 0 ) return status;
		}
	}
	if ( ! joint ) status = EndDoc(hDC);
	return status;
}

// 1page分を印刷する
int PrintPage(HDC hDC, int pageno, PRINTINFO *pinfo)
{
	int status;
	RECT PrintRect;
										// 頁開始を通知 -----------------------
	status = StartPage(hDC);
	if ( status <= 0 ) return status;
	status = ExtEscape(hDC, NEXTBAND, 0, NULL, sizeof(PrintRect), (LPSTR)&PrintRect);
	if ( status <= 0 ){					// バンド非サポート処理 ---------------
		PrintRect.top		= 0;
		PrintRect.left		= 0;
		PrintRect.right		= GetDeviceCaps(hDC, HORZRES);
		PrintRect.bottom	= GetDeviceCaps(hDC, VERTRES);
		status = PrintMain(hDC, &PrintRect, pageno, pinfo);
		if ( status < 0 ) return status;
	}else{								// バンド処理 -------------------------
		while( !IsRectEmpty(&PrintRect) && !PrintAbort ){
			status = PrintMain(hDC, &PrintRect, pageno, pinfo);
			if ( status < 0 ) return status;
			status = ExtEscape(hDC, NEXTBAND, 0, NULL, sizeof(PrintRect), (LPSTR)&PrintRect);
			if ( status < 0 ) return status;
		}
	}
	status = EndPage(hDC);
	return status;
}

#define MilliToDevice(size, dpi) ((size) * 10 * (dpi) / 254)
HFONT InitPrintParam(HDC hDC, RECT *PrintRect, int pageno, PPVPAINTSTRUCT *pps, PRINTPAGEINFO *prt)
{
	HGDIOBJ hOldFont;
	TEXTMETRIC tm;
	LOGFONTWITHDPI lBoxFont;
	RECT marginpix;
	RECT PageBound; // 印刷可能範囲(Page単位)

	// 用紙関係
	prt->DPI.cx = GetDeviceCaps(hDC, LOGPIXELSX);
	prt->DPI.cy = GetDeviceCaps(hDC, LOGPIXELSY);
	prt->PrtSize.cx = GetDeviceCaps(hDC, HORZRES);
	prt->PrtSize.cy = GetDeviceCaps(hDC, VERTRES);
	prt->PaperSize.cx = GetDeviceCaps(hDC, PHYSICALWIDTH);
	prt->PaperSize.cy = GetDeviceCaps(hDC, PHYSICALHEIGHT);
	PageBound.left = GetDeviceCaps(hDC, PHYSICALOFFSETX);
	PageBound.top = GetDeviceCaps(hDC, PHYSICALOFFSETY);
	PageBound.right = PageBound.left + prt->PrtSize.cx;
	PageBound.bottom = PageBound.top + prt->PrtSize.cy;

	marginpix.left =  MilliToDevice(PrintInfo.X_prts.margin.left,  prt->DPI.cx);
	marginpix.right = MilliToDevice(PrintInfo.X_prts.margin.right, prt->DPI.cx);
	marginpix.top =   MilliToDevice(PrintInfo.X_prts.margin.top,   prt->DPI.cy);
	marginpix.bottom = MilliToDevice(PrintInfo.X_prts.margin.bottom, prt->DPI.cy);

	// 表示開始位置決定
	pps->shiftdot.cx = marginpix.left - PageBound.left;
	if ( pps->shiftdot.cx < 0 ) pps->shiftdot.cx = 0; // 印刷可能範囲外なのでずらす
	pps->shiftdot.cy = marginpix.top - PageBound.top;
	if ( pps->shiftdot.cy < 0 ) pps->shiftdot.cy = 0; // 印刷可能範囲外なのでずらす

	// フォント
	GetPPxFont(PPXFONT_F_fix, 0, &lBoxFont);
	lBoxFont.font.lfHeight = 0;
	pps->print = TRUE;
	pps->hBoxFont = CreateFontIndirect(&lBoxFont.font);
						// 現在のディスプレイコンテキストにフォントを設定する
	hOldFont = SelectObject(hDC, pps->hBoxFont);
										// フォント情報を入手
	GetAndFixTextMetrics(hDC, &tm);
	pps->fontsize.cx = tm.tmAveCharWidth;
	pps->fontsize.cy = tm.tmHeight;
	pps->lineY = pps->fontsize.cy + pps->fontsize.cy / 2; // 行間隔1.5
	if ( pps->lineY <= 0 ) pps->lineY = 1;

	// 印刷範囲を決定
	prt->BmpSize.cx = prt->PaperSize.cx - pps->shiftdot.cx - marginpix.right;
	if ( (PageBound.left + pps->shiftdot.cx + prt->BmpSize.cx) > PageBound.right ){
		prt->BmpSize.cx = PageBound.right - PageBound.left - pps->shiftdot.cx; // 印刷可能範囲内におさめる
	}
	if ( prt->BmpSize.cx <= 0 ) prt->BmpSize.cx = 1;

	prt->BmpSize.cy = prt->PaperSize.cy - pps->shiftdot.cy - marginpix.bottom;
	if ( (PageBound.top + pps->shiftdot.cy + prt->BmpSize.cy) > PageBound.bottom ){
		prt->BmpSize.cy = PageBound.bottom - PageBound.top - pps->shiftdot.cy; // 印刷可能範囲内におさめる
	}
	if ( prt->BmpSize.cy <= 0 ) prt->BmpSize.cy = 1;

	if ( vo_.DModeType != DISPT_IMAGE ){ // テキスト関係
		SIZE PageUnit; // ページあたりの表示量(テキスト：行桁/画像：pixel)

		PageUnit.cx = 1;
		prt->PageCount.cx = 1;

		PageUnit.cy = prt->BmpSize.cy / pps->lineY;
		if ( PageUnit.cy <= 0 ) PageUnit.cy = 1;
		prt->PageCount.cy = (VOi->line + PageUnit.cy - 1) / PageUnit.cy;

		if ( PrintRect != NULL ){
			pps->drawYtop = max(PrintRect->top - pps->shiftdot.cy, 0) / pps->lineY;
			pps->drawYbottom = min(PrintRect->bottom - pps->shiftdot.cy, prt->BmpSize.cy - pps->lineY) / pps->lineY;
			pps->shift.cx = 0;
			pps->shift.cy = pageno * PageUnit.cy;
		}
	}else{ // 画像関係
		int bmpx, bmpy;

		bmpx = vo_.bitmap.showsize.cx;
		bmpy = vo_.bitmap.showsize.cy;

		prt->BmpZoom = PrintInfo.X_prts.imagedivision;
		#define MINDPI 80 // 自動調整時の最小DPI
		if ( prt->BmpZoom > 0 ){ // 固定倍率
			prt->BmpZoom *= 100;
		}else{ // 0以下 調整有り
			int DPIx, DPIy;

			DPIx = bmpx * prt->DPI.cx * 100 / prt->BmpSize.cx + 1;
			DPIy = bmpy * prt->DPI.cy * 100 / prt->BmpSize.cy + 1;

			if ( prt->BmpZoom == -1 ){ // 用紙一杯にする
				prt->BmpZoom = (DPIx > DPIy) ? DPIx : DPIy;
			}else if ( prt->BmpZoom == -2 ){ // 横幅一杯にする
				prt->BmpZoom = DPIx;
			}else if ( prt->BmpZoom == -3 ){ // 縦幅一杯にする
				prt->BmpZoom = DPIy;
			}else if ( prt->BmpZoom <= 0 ){
				prt->BmpZoom = 200 * 100;
				if ( (bmpx > 2400) ||
					 (bmpy > 3866) ){
					prt->BmpZoom = (DPIx > DPIy) ? DPIx : DPIy;
				}else if ( (bmpx > 1200) ||
						   (bmpy > 1933) ){
					prt->BmpZoom = 400 * 100;
				}
			}
			if ( prt->BmpZoom < (MINDPI * 100) ) prt->BmpZoom = (MINDPI * 100);
		}
		prt->BmpWindow.cx = (bmpx * 100 * prt->DPI.cx) / prt->BmpZoom;
		prt->BmpWindow.cy = (bmpy * 100 * prt->DPI.cy) / prt->BmpZoom;

		prt->PageCount.cx = (prt->BmpWindow.cx / prt->BmpSize.cx) + 1;
		prt->PageCount.cy = (prt->BmpWindow.cy / prt->BmpSize.cy) + 1;
	}
	return hOldFont;
}

// 本体
int PrintMain(HDC hDC, RECT *PrintRect, int pageno, PRINTINFO *pinfo)
{
	PAINTSTRUCT ps;
	PPVPAINTSTRUCT pps;
	HFONT hOldFont;
	PRINTPAGEINFO prt;

	hOldFont = InitPrintParam(hDC, PrintRect, pageno, &pps, &prt);
										// 表示環境の設定 ---------------------
	ps.hdc = hDC;
	ps.fErase = FALSE;
//	ps.rcPaint	背景描画の時しか使わないので未定義
	pps.si.bgmode = TRUE;
	pps.ps = ps;

	SetBkColor(hDC, C_WHITE);
	SetTextColor(hDC, C_BLACK /*CV_char[CV__deftext]*/); // 文字色
										// ２行目以降内容 --------------------
	if ( PrintRect->bottom > pps.lineY ) switch(vo_.DModeType){
		case DISPT_HEX:					//=====================================
			DrawHex(&pps, &vo_);
			break;

		case DISPT_DOCUMENT:			//=====================================
			if ( vo_.DocmodeType == DOCMODE_EMETA ){ // metafile
				RECT box;
				HPALETTE hOldPal C4701CHECK;
				int DPI;

				if ( vo_.bitmap.hPal != NULL ){
					hOldPal = SelectPalette(hDC, vo_.bitmap.hPal, FALSE);
								// 論理パレットを現在のＤＣにマッピングする。
					RealizePalette(hDC);
				}

				DPI = pinfo->X_prts.imagedivision;
				if ( DPI <= 0 ) DPI = 95;

				box.right = vo_.bitmap.showsize.cx * prt.DPI.cx / DPI;
				box.bottom = vo_.bitmap.showsize.cy * prt.DPI.cy / DPI;
				box.left = (prt.PrtSize.cx - box.right) >> 1;
				box.top = (prt.PrtSize.cy - box.bottom) >> 1;
				box.right += box.left;
				box.bottom += box.top;
				PlayEnhMetaFile(hDC, vo_.eMetafile.handle, &box);
				if ( vo_.bitmap.hPal != NULL ) SelectPalette(hDC, hOldPal, FALSE); // C4701ok
				break;
			}
		// DISPT_TEXT へ
		case DISPT_TEXT:				//=====================================
			PPvDrawText(&ps, &pps);
			break;

		case DISPT_IMAGE: {				//=====================================
			POINT oldpos;
			HPALETTE hOldPal C4701CHECK;
			int xOff, yOff, xOffsize, yOffsize, oldmode;
			int xDIBPage, yDIBPage;
			int xDIBoff, yDIBoff;
			int result;
											// hPalの論理パレットに変更する。
			if ( vo_.bitmap.hPal != NULL ){
				hOldPal = SelectPalette(hDC, vo_.bitmap.hPal, FALSE);
								// 論理パレットを現在のＤＣにマッピングする。
				RealizePalette(hDC);
			}
			if ( vo_.bitmap.UseICC >= 0 ) SetICMMode(hDC, vo_.bitmap.UseICC);

											// DIBを画面に転送
			if ( prt.PageCount.cx == 1 ){
				xOffsize = prt.BmpWindow.cx;
				xOff = pps.shiftdot.cx + (prt.BmpSize.cx - xOffsize) / 2;
				xDIBPage = vo_.bitmap.rawsize.cx;
				xDIBoff = 0;
			}else{
				int overx;

				xOff = pps.shiftdot.cx;
				xDIBPage = xOffsize = prt.BmpSize.cx;
				if ( XV.img.AspectRate != 0 ){
					if ( XV.img.AspectRate < ASPACTX ){
						xDIBPage = (xDIBPage * XV.img.AspectRate) / ASPACTX;
					}
				}
				xDIBPage = (xDIBPage * prt.BmpZoom / 100) / prt.DPI.cx;
				xDIBoff = xDIBPage * (pageno / prt.PageCount.cy);
				overx = vo_.bitmap.rawsize.cx - xDIBoff;
				if ( overx < xDIBPage ){
					xOffsize = xOffsize * overx / xDIBPage;
					xDIBPage = overx;
				}
			}

			if ( prt.PageCount.cy == 1 ){
				yOffsize = prt.BmpWindow.cy;
				yOff = pps.shiftdot.cy + (prt.BmpSize.cy - yOffsize) / 2;
				yDIBPage = vo_.bitmap.rawsize.cy;
				yDIBoff = 0;
			}else{
				yOff = pps.shiftdot.cy;
				yDIBPage = yOffsize = prt.BmpSize.cy;
				if ( XV.img.AspectRate != 0 ){
					if ( XV.img.AspectRate >= ASPACTX ){
						yDIBPage = (yDIBPage * ASPACTX) / XV.img.AspectRate;
					}
				}
				yDIBPage = (yDIBPage * prt.BmpZoom / 100) / prt.DPI.cy;
				yDIBoff = vo_.bitmap.rawsize.cy - yDIBPage * ((pageno % prt.PageCount.cy) + 1);
				if ( yDIBoff < 0 ){
					yOffsize = yOffsize * (yDIBPage + yDIBoff) / yDIBPage;
					yDIBPage = yDIBPage + yDIBoff;
					yDIBoff = 0;
				}
			}

			oldmode = SetStretchBltMode(hDC, XV.img.StretchMode);
			SetBrushOrgEx(hDC, 0, 0, &oldpos);

			result = StretchDIBits(hDC,
					xOff, yOff, xOffsize, yOffsize,
					xDIBoff, yDIBoff, xDIBPage, yDIBPage,
					vo_.bitmap.bits.ptr, (BITMAPINFO *)vo_.bitmap.ShowInfo,
					DIB_RGB_COLORS, SRCCOPY);
			if ( (result == 0) || (result == IGDI_ERROR) ){
				FixStretchDIBits(hDC,
						xOff, yOff, xOffsize, yOffsize,
						xDIBoff, yDIBoff, xDIBPage, yDIBPage,
						vo_.bitmap.bits.ptr, (BITMAPINFO *)vo_.bitmap.ShowInfo);
			}

			SetBrushOrgEx(hDC, oldpos.x, oldpos.y, NULL);
			SetStretchBltMode(hDC, oldmode);
			if ( vo_.bitmap.UseICC >= 0 ) SetICMMode(hDC, ICM_OFF);
											// 前のパレットに戻す。
			if ( vo_.bitmap.hPal != NULL ) SelectPalette(hDC, hOldPal, FALSE); // C4701ok
		}
	}
											// 後始末 -------------------------
	SelectObject(hDC, hOldFont);	// フォント
	DeleteObject(pps.hBoxFont);
	return 1;
}

// "Printing.." dialog-message ------------------------------------------------
#pragma argsused
INT_PTR CALLBACK PrintDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	UnUsedParam(lParam);

	switch (msg){
		case WM_CLOSE:
			PrintAbort = TRUE;
			EnableWindow(GetParent(hDlg), TRUE);
			DestroyWindow(hDlg);
			hDlgPrint = NULL;
			return TRUE;

		case WM_INITDIALOG:
			SetWindowText(hDlg, StrPPvTitle);
			LocalizeDialogText(hDlg, IDD_PRINTDLG);
			CenterWindow(hDlg);
			return TRUE;

		case WM_COMMAND :
			if ( LOWORD(wParam) == IDCANCEL ){
				PostMessage(hDlg, WM_CLOSE, 0, 0);
			}
			return TRUE;
	}
	return PPxDialogHelper(hDlg, msg, wParam, lParam);
}
// "Printing.." dialog-message loop -------------------------------------------
#pragma argsused
int CALLBACK AbortProc(HDC hdcPrn, int iCode)
{
	MSG msg;
	UnUsedParam(hdcPrn);UnUsedParam(iCode);

	while( !PrintAbort && PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ){
		if ( msg.message == WM_QUIT ) break;
		if ( !hDlgPrint || !IsDialogMessage (hDlgPrint, &msg)){
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return !PrintAbort;
}

int FixPrintPages(void)
{
	HFONT hOldFont;
	PPVPAINTSTRUCT pps;
	PRINTPAGEINFO prt;
	int maxpages;

	hOldFont = InitPrintParam(PDef.hDC, NULL, 1, &pps, &prt);
	SelectObject(PDef.hDC, hOldFont);
	DeleteObject(pps.hBoxFont);
	if ( pps.shift.cy < 0 ){
		maxpages = 1;
		PDef.nToPage = 1;
	}else{
		maxpages = prt.PageCount.cx * prt.PageCount.cy;
		PDef.nToPage = (WORD)min(PDef.nToPage, maxpages);
	}
	return maxpages;
}

void InitPrintsPages(void)
{
	PDef.nToPage = PDef.nMaxPage = 1;
	switch(vo_.DModeType){
		case DISPT_DOCUMENT:
			if ( vo_.DocmodeType == DOCMODE_EMETA ) break;
		case DISPT_TEXT:
		case DISPT_HEX:
			break;
	}
	PDef.nToPage = PDef.nMaxPage = (WORD)FixPrintPages();
}

BOOL InitPrint(HWND hWnd, UINT DialogID)
{
	LoadPrint();
	if ( PPrintDlg == NULL ){
		SetPopMsg(POPMSG_MSG, MES_EC32, 0);
		return FALSE;
	}
	GetCustData(T("X_prts"), &PrintInfo.X_prts, sizeof(PrintInfo.X_prts));
	BusyPPx(hWnd, hWnd);
	PDef.hwndOwner = hWnd;

	if ( DialogID == IDD_PRINTDLG ){
		resetflag(PDef.Flags, PD_NOPAGENUMS);
	}else{ // IDD_MULTIPRINTDLG
		setflag(PDef.Flags, PD_NOPAGENUMS);
	}

	PDef.hDC = NULL;
	PDef.nToPage = PDef.nMaxPage = 1;
	if ( PDef.hDevMode == NULL ){
		setflag(PDef.Flags, PD_RETURNDEFAULT);
		if ( !PPrintDlg(&PDef) ){ // プリンタ情報を取得(ダイアログ表示無し)
			BusyPPx(hWnd, INVALID_HANDLE_VALUE);
			return FALSE;
		}
		resetflag(PDef.Flags, PD_RETURNDEFAULT);
	}else{
		DEVMODE *dm;

		dm = GlobalLock(PDef.hDevMode);
		PDef.hDC = CreateDC(NULL, (TCHAR *)dm->dmDeviceName, NULL, NULL);
		GlobalUnlock(PDef.hDevMode);
	}
	PDef.nToPage = PDef.nMaxPage = (WORD)FixPrintPages(); // ページ数算出
	DeleteDC(PDef.hDC);
	PDef.hDC = NULL;
	if ( !PPrintDlg(&PDef) ){
		BusyPPx(hWnd, INVALID_HANDLE_VALUE);
		return FALSE;
	}
	if ( PDef.hDevMode != NULL ){ // 印刷部数を取得
		DEVMODE *dm;

		dm = GlobalLock(PDef.hDevMode);
		#pragma warning(suppress:6011) // GlobalLock は失敗しないと見なす
		if ( dm->dmCopies != DMCOLLATE_FALSE ){ // ドライバの複数部印刷未対応
												// →アプリ側で対応する
			PDef.nCopies = max(dm->dmCopies, PDef.nCopies);
			dm->dmCopies = 1;
		}
		GlobalUnlock(PDef.hDevMode);
	}
	if ( !(PDef.Flags & (PD_PAGENUMS | PD_SELECTION)) ){
		PDef.nFromPage = 1;
		PDef.nToPage = PDef.nMaxPage;
	}
	FixPrintPages(); // ページ数再算出
										// 中止ダイアログの表示 ---------------
	EnableWindow(hWnd, FALSE);
	PrintAbort = FALSE;
	hDlgPrint = CreateDialog(hInst, MAKEINTRESOURCE(DialogID), hWnd, PrintDlgProc);
	return TRUE;
}

BOOL EndPrint(HWND hWnd, int status)
{
	BusyPPx(hWnd, INVALID_HANDLE_VALUE);
	if ( !PrintAbort ){
		EnableWindow(hWnd, TRUE);
		DestroyWindow(hDlgPrint);
		DeleteDC(PDef.hDC);
		return status >= 0;
	}else{
		DeleteDC(PDef.hDC);
		return FALSE;
	}
}

// 通常印刷 -------------------------------------------------------------------
BOOL PPVPrint(HWND hWnd)
{
	DOCINFO dinfo = {sizeof(DOCINFO), NULL, NULL, NULL, 0};
	TCHAR buf[VFPS + 16];
	int status;

	if ( InitPrint(hWnd, IDD_PRINTDLG) == FALSE ) return FALSE;

	thprintf(buf, TSIZEOF(buf), T("PPv: %s"), vo_.file.name);
	dinfo.lpszDocName = buf;
	SetAbortProc(PDef.hDC, AbortProc);
	status = PrintOneDocument(PDef.hDC, &dinfo, FALSE, &PrintInfo);
	if ( status < 0 ) ErrorPathBox(hWnd, T("Print"), NULL, PPERROR_GETLASTERROR);
	return EndPrint(hWnd, status);
}

// 1document化印刷 ------------------------------------------------------------
BOOL PPVPrintFiles(HWND hWnd, HDROP hDrop)
{
	DOCINFO dinfo = {sizeof(DOCINFO), CprinttitleString, NULL, NULL, 0};
	TCHAR name[VFPS], buf[VFPS];
	int document, documentleft, status = 1, Copy, CopyMax;

	if ( InitPrint(hWnd, IDD_MULTIPRINTDLG) == FALSE ) return FALSE;

	// ファイル一覧を抽出
	documentleft = DragQueryFile(hDrop, MAX32, NULL, 0);
	for ( document = 0 ; document < documentleft ; document++ ){
		DragQueryFile(hDrop, document, name, TSIZEOF(name));
		SendDlgItemMessage(hDlgPrint, IDL_PT_LIST, LB_ADDSTRING, 0, (LPARAM)name);
	}
	DragFinish(hDrop);

	if ( SetAbortProc(PDef.hDC, AbortProc) < 0 ){
		ErrorPathBox(hWnd, T("Print"), NULL, PPERROR_GETLASTERROR);
	}

	CopyMax = PDef.nCopies;
	PDef.nCopies = 1;
	if ( StartDoc(PDef.hDC, &dinfo) >= 0 ){
		for ( Copy = 0 ; Copy < CopyMax ; Copy++ ){
			for ( document = 0 ; document < documentleft ; document++ ){
											// 読み込み処理
				SendDlgItemMessage(hDlgPrint,
						IDL_PT_LIST, LB_SETCURSEL, (WPARAM)document, 0);
				SendDlgItemMessage(hDlgPrint,
						IDL_PT_LIST, LB_GETTEXT, (WPARAM)document, (LPARAM)name);
				thprintf(buf, TSIZEOF(buf), T("Printing [%d/%d]"), document + 1, documentleft);
				SetDlgItemText(hDlgPrint, IDS_PRINTDLGTXT, buf);

				if ( GetFileAttributesL(name) & FILE_ATTRIBUTE_DIRECTORY ){
					HANDLE hFF;
					WIN32_FIND_DATA ff;
					int cnt;

					cnt = 0;
					CatPath(buf, name, T("*"));
					hFF = FindFirstFileL(buf, &ff);
					if ( hFF == INVALID_HANDLE_VALUE ){
						status = 1;
						break;
					}else do{
						if (!(ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
							CatPath(buf, name, ff.cFileName);
							OpenViewObject(buf, NULL, NULL, 0);
							InvalidateRect(hWnd, NULL, TRUE);
							UpdateWindow(hWnd);
							InitPrintsPages();
							if ( vo_.DModeBit & VO_dmode_IMAGE ){
								status = PrintOneDocument(PDef.hDC,
												&dinfo, TRUE, &PrintInfo);
								if ( status <= 0 ) break;
								cnt++;
							}
						}
					}while(FindNextFile(hFF, &ff) != FALSE);
					FindClose(hFF);
					if ( cnt && ((document + 1) < documentleft) ){
						EndDoc(PDef.hDC);
						if (StartDoc(PDef.hDC, &dinfo) < 0){
							status = -1;
							break;
						}
					}
				}else{
					OpenViewObject(name, NULL, NULL, 0);
					InvalidateRect(hWnd, NULL, TRUE);
					UpdateWindow(hWnd);
					InitPrintsPages();
					status = PrintOneDocument(PDef.hDC, &dinfo, TRUE, &PrintInfo);
				}
											// 印刷処理
				if (status <= 0) break;
			}
			if (status <= 0) break;
		}
		if (status >= 0) EndDoc(PDef.hDC);
	}
	PDef.nCopies = (WORD)CopyMax;
	if ( status < 0 ) ErrorPathBox(hWnd, T("Print"), NULL, PPERROR_GETLASTERROR);
	return EndPrint(hWnd, status);
}
