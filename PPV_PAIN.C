/*-----------------------------------------------------------------------------
	Paper Plane vUI													Paint
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPV_STRU.H"
#include "PPV_FUNC.H"

#if defined(_MSC_VER) && (defined(USEDIRECTWRITE) || defined(_WIN64)) && !defined(_M_ARM) && !defined(_M_ARM64)
// #include <emmintrin.h>
#endif

#pragma hdrstop

#define CP__NONE 0xffff // 変換できないコードページ
// Win95/NT4 eng RTM は VTYPE_IBM / VTYPE_ANSI / VTYPE_SYSTEMCP のみ使用可能
UINT VVTypeToCPlist[VTYPE_MAX] = {
	CP__US,		// VTYPE_IBM
	CP__LATIN1,	// VTYPE_ANSI
	CP__NONE,	// VTYPE_JIS
	CP_ACP,		// VTYPE_SYSTEMCP
	CP__NONE,	// VTYPE_EUCJP
	CP__UTF16L,	// VTYPE_UNICODE
	CP__UTF16B,	// VTYPE_UNICODEB
	CP__SJIS,	// VTYPE_SJISNEC
	CP__NONE,	// VTYPE_SJISB,
	CP__NONE,	// VTYPE_KANA
	CP_UTF8,	// VTYPE_UTF8
	CP_UTF7,	// VTYPE_UTF7
	CP_ACP,		// VTYPE_RTF
	CP__NONE	// VTYPE_OTHER
};

RECT ImageDrawedBox;

const TCHAR BitmapErrorText[] = T("Bitmap show error");
const WCHAR CtrlCodeConv[32] = {
	'.',    0x2401, 0x2402, 0x2403, 0x2404, 0x2405, 0x2406, 0x2407,
	0x21a4, 0xffeb, 0xffec, 0x21a7, 0x21a1, 0xffe9, 0x240e, 0x240f,
	0x2410, 0x2411, 0x2412, 0x2413, 0x2414, 0x2415, 0x2416, 0x2417,
	0x2418, 0x2419, 0x241a, 0x241b, 0x241c, 0x241d, 0x241e, 0x241f
};

// 精度と、範囲内に納めるための制限値
#ifdef _WIN64
	#define RDXP 0x80	// 縮小時に使う固定小数点の精度(7bit)
	#define RDX_MAX_WH 0x1ff00 // 元bitmapの大きさ制限値(17bit)
	// RDX_MAX_WH * RDX_MAX_WH * RDXP < 63/31bit
	// 0x1ff000 * 0x1ff000 * 0x80 < 0x2 0000 0000 0000
	#define RDX_MAX_XYW (0x100 * RDXP) // 平均計算可能な最大値
	// RDXP * RDXP * RDX_MAX_XYW * RDX_MAX_XYW < 31bit
	// 0x80 * 0x80 * 0x100 * 0x100 = 0x4000 0000
#else
	#define RDXP 0x20
	#define RDX_MAX_WH 0x1ff0
	// 0x1fff * 0x1fff * 0x20 ≒ 0x8000 0000
	#define RDX_MAX_XYW (0x100 * RDXP)
	// RDXP * RDXP * RDX_MAX_XYW * RDX_MAX_XYW < 31bit
	// 0x20 * 0x20 * 0x100 * 0x100 = 0x400 0000
#endif

// StretchDIBits で表示できないBMP(10bit Grayとか)を表示する
BOOL FixStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, int nSrcWidth, int nSrcHeight, const VOID * lpBits, const BITMAPINFO * lpBitsInfo)
{
	HBITMAP hTbmpDIB;
	HDC hMDC;
	BITMAPINFO bmpinfo;
	LPVOID MBits;
	int draw = 0;

	bmpinfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmpinfo.bmiHeader.biWidth = lpBitsInfo->bmiHeader.biWidth;
	bmpinfo.bmiHeader.biHeight = lpBitsInfo->bmiHeader.biHeight;
	if ( bmpinfo.bmiHeader.biHeight < 0 ){
		bmpinfo.bmiHeader.biHeight = -bmpinfo.bmiHeader.biHeight;
	}
	bmpinfo.bmiHeader.biPlanes = 1;
	bmpinfo.bmiHeader.biBitCount = (WORD)((OSver.dwMajorVersion < 6) ? 24 : 32);
	bmpinfo.bmiHeader.biCompression = BI_RGB;
	bmpinfo.bmiHeader.biSizeImage = 0;
	bmpinfo.bmiHeader.biClrUsed = 0;
	bmpinfo.bmiHeader.biClrImportant = 0;

	hMDC = CreateCompatibleDC(hdc);
	hTbmpDIB = CreateDIBSection(hdc, &bmpinfo, DIB_RGB_COLORS, &MBits, NULL, 0);
	if ( hTbmpDIB != NULL ){
		HGDIOBJ hOldBMP;

		hOldBMP = SelectObject(hMDC, hTbmpDIB);
/*
		if ( vo->bitmap.hPal != NULL ){
			SelectPalette(hMDC, vo->bitmap.hPal, FALSE);
					// 論理パレットを現在のＤＣにマッピングする。
			RealizePalette(hMDC);
		}
*/
		draw = SetDIBitsToDevice(hMDC,
				0, 0, bmpinfo.bmiHeader.biWidth, bmpinfo.bmiHeader.biHeight,
				0, 0, 0, bmpinfo.bmiHeader.biHeight,
				lpBits, lpBitsInfo, DIB_RGB_COLORS);
		if ( draw == IGDI_ERROR ) draw = 0;

		SelectObject(hMDC, hOldBMP);

		if ( draw ){
			draw = StretchDIBits(hdc, XDest, YDest, nDestWidth, nDestHeight,
					XSrc, YSrc, nSrcWidth, nSrcHeight, MBits, &bmpinfo,
					DIB_RGB_COLORS, SRCCOPY);
			if ( draw == IGDI_ERROR ) draw = 0;
		}
		DeleteObject(hTbmpDIB);
	}
	DeleteDC(hMDC);
	return draw;
}

xBITMAPV5HEADER *DupBitmapInfo(const BITMAPINFO *src)
{
	size_t copysize, profilesize = 0;
	int colors;
	xBITMAPV5HEADER *dest;

	copysize = src->bmiHeader.biSize;

	// パレット、bit fields の大きさ
	colors = src->bmiHeader.biClrUsed;
	if ( colors == 0 ){
		if ( src->bmiHeader.biBitCount < 16 ){
			colors = 1 << src->bmiHeader.biBitCount;
		}else if ( (copysize < (sizeof(BITMAPINFOHEADER) + 12) ) &&
				   (src->bmiHeader.biCompression == BI_BITFIELDS) ){
			colors = 3;
		}
	}
	copysize += colors * sizeof(RGBQUAD);

	if ( src->bmiHeader.biSize >= sizeof(xBITMAPV5HEADER) ){
		profilesize = ((xBITMAPV5HEADER *)src)->bV5ProfileSize;
	}
	dest = (xBITMAPV5HEADER *)HeapAlloc(PPvHeap, 0, copysize + profilesize);
	if ( dest != NULL ){
		memcpy(dest, src, copysize);
		if ( profilesize > 0 ){
			memcpy((BYTE *)dest + copysize,
				(BYTE *)src + ((xBITMAPV5HEADER *)src)->bV5ProfileData,
				profilesize);
			dest->bV5ProfileData = copysize;
		}
	}
	return dest;
}

#define DWTESTMSD 0

#if DRAWMODE == DRAWMODE_DW
BOOL MoreStretchDIBits(DXDRAWSTRUCT *DxDraw, PPVPAINTSTRUCT *pps, PPvViewObject *vo, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, const VOID * lpBits, const BITMAPINFO * lpBitsInfo)
#else
BOOL MoreStretchDIBits(HDC hdc, int XDest, int YDest, int nDestWidth, int nDestHeight, int XSrc, int YSrc, const VOID * lpBits, const BITMAPINFO * lpBitsInfo)
#endif
{
	xBITMAPV5HEADER *reduce_bmp;
	int draw = 0;
	LPVOID MBits;

#if DRAWMODE == DRAWMODE_DW
	HDC hdc;

	hdc = pps->ps.hdc;
	if ( hdc == DXMODEVALUE_DX ) hdc = NULL;
#endif
	if ( vo_.bitmap.ModifyCache.hBmp != NULL ){ // cache を利用
		reduce_bmp = vo_.bitmap.ModifyCache.info;
		MBits = vo_.bitmap.ModifyCache.bits;
		draw = 1;
	}else{									// 表示 bitmap を用意 + キャッシュ
		reduce_bmp = vo_.bitmap.ModifyCache.info = DupBitmapInfo(lpBitsInfo);
		if ( reduce_bmp == NULL ) return FALSE;

		reduce_bmp->bV5Width = nDestWidth;
		reduce_bmp->bV5Height = nDestHeight;
		reduce_bmp->bV5Planes = 1;
		reduce_bmp->bV5BitCount = 32;
		reduce_bmp->bV5Compression = BI_RGB;
		reduce_bmp->bV5SizeImage = 0;

		vo_.bitmap.ModifyCache.hBmp = CreateDIBSection(hdc, (BITMAPINFO *)reduce_bmp, DIB_RGB_COLORS, &MBits, NULL, 0);
		if ( vo_.bitmap.ModifyCache.hBmp != NULL ){
			int ready = 0;
			int x,y;
			size_t xw2, yw2;
			const BITMAPINFO *srcInfo;
			xBITMAPV5HEADER *temp_bmp = NULL;
			LPVOID temp_Bits;
			HBITMAP htemp_bmp = NULL;

			DIRECTXDEFINE(DxDrawFreeBMPCache(&vo->bitmap.DxCache));
			vo_.bitmap.ModifyCache.bits = MBits;

			srcInfo = lpBitsInfo;
			// src 1pix が dst の pixに対応する量(固定小数点)
			xw2 = ((size_t)srcInfo->bmiHeader.biWidth  * RDXP) / reduce_bmp->bV5Width;
			yw2 = ((size_t)srcInfo->bmiHeader.biHeight * RDXP) / reduce_bmp->bV5Height;

			// フィルタ可能かどうかを確認
			if ( (XV.img.MoreStrech == 1) &&
#if DWTESTMSD && (DRAWMODE == DRAWMODE_DW)
			// Direct2D ではカラープロファイル未対応なのでここで処理しない
				 !((hdc == NULL) && (vo->bitmap.UseICC >= 0) ) &&
#endif
			// 計算精度の範囲内かを確認
				 (srcInfo->bmiHeader.biWidth <= RDX_MAX_WH) &&
				 (srcInfo->bmiHeader.biHeight <= RDX_MAX_WH) &&
				 (xw2 > 0) && (xw2 < RDX_MAX_XYW) &&
				 (yw2 > 0) && (yw2 < RDX_MAX_XYW) ){
				// BI_RGB か RGB888 の BI_BITFIELDS のみ対応
				if ( ((srcInfo->bmiHeader.biCompression == BI_RGB) ||
					  ((srcInfo->bmiHeader.biCompression == BI_BITFIELDS) &&
					   ((xBITMAPV5HEADER *)&srcInfo->bmiHeader)->bV5GreenMask == 0xff00) ) &&
				// 8 / 24 / 32 のみ対応
					 ( (srcInfo->bmiHeader.biBitCount == 8) ||
					   (srcInfo->bmiHeader.biBitCount == 24) ||
					   (srcInfo->bmiHeader.biBitCount == 32) ) ){
					ready = 1; // そのまま変換可能
				}else{ // 対応形式に一旦変換が必要
					temp_bmp = DupBitmapInfo(lpBitsInfo);
					if ( temp_bmp != NULL ){
						temp_bmp->bV5Planes = 1;
						temp_bmp->bV5Compression = BI_RGB;
						temp_bmp->bV5SizeImage = 0;
						temp_bmp->bV5BitCount = (WORD)((srcInfo->bmiHeader.biBitCount < 8) ? 8 : 32);
						htemp_bmp = CreateDIBSection(hdc, (const BITMAPINFO *)temp_bmp, DIB_RGB_COLORS, &temp_Bits, NULL, 0);
					}
					if ( htemp_bmp != NULL ){
						HGDIOBJ hOldBMP;
						int lines;
						HDC hMDC;
						hMDC = CreateCompatibleDC(hdc);
						hOldBMP = SelectObject(hMDC, htemp_bmp);
						lines = lpBitsInfo->bmiHeader.biHeight;
						if ( lines < 0 ) lines = -lines;

						if ( IGDI_ERROR != SetDIBitsToDevice(hMDC,
								0, 0, temp_bmp->bV5Width, temp_bmp->bV5Height,
								0, 0, 0, temp_bmp->bV5Height, lpBits,
								srcInfo, DIB_RGB_COLORS) ){
							srcInfo = (const BITMAPINFO *)temp_bmp;
							lpBits = temp_Bits;
							ready = 1; // 変換したため変換可能に
						}
						SelectObject(hMDC, hOldBMP);
						DeleteDC(hMDC);
					}
				}
			}

			if ( ready ){
				size_t srcwidth; // 1行分のバイト数
				int xi, yi, xmax, xmin, ymin, ymax;
				int src_bytes;
				BYTE *dst_line; // 現在行の開始位置
				DWORD *pal;

				src_bytes = srcInfo->bmiHeader.biBitCount / 8;
				srcwidth = (size_t)DwordAlignment(srcInfo->bmiHeader.biWidth * src_bytes);
				if ( src_bytes == 1 ){
					pal = (DWORD *)(BYTE *)((BYTE *)srcInfo + srcInfo->bmiHeader.biSize);
				}
				dst_line = (BYTE *)MBits;

				for ( y = 0; y < reduce_bmp->bV5Height; y++){
					BYTE *src_line; // 現在行の開始位置
					size_t src_yl, src_yr; // src の dst に対応する y 座標

					src_yl = ((size_t)y * srcInfo->bmiHeader.biHeight * RDXP) / reduce_bmp->bV5Height;
					src_yr = src_yl / RDXP;
					src_line = (BYTE *)((BYTE *)lpBits + (src_yr * srcwidth));
					ymin = (int)(size_t)(src_yl - src_yr * RDXP);
					ymax = ymin + yw2;
					if ( ymax < RDXP ){ // 拡大時の調整
						ymin = 0;
						ymax = RDXP;
					}else if ( (LONG)((ymax / RDXP) + src_yr) >= (srcInfo->bmiHeader.biHeight - 1) ){ // src の範囲外か確認
						ymax = (srcInfo->bmiHeader.biHeight - src_yr) * RDXP;
					}
#ifdef _INCLUDED_EMM
					for ( x = 0; x < reduce_bmp->bV5Width; x++){
						BYTE *srcp;
						size_t src_xl, src_xr;
						__m128 pack_sums;

						src_xl = (((size_t)x * srcInfo->bmiHeader.biWidth * RDXP) / reduce_bmp->bV5Width);
						src_xr = src_xl / RDXP;
						srcp = src_line + src_xr * src_bytes;
						xmin = (int)(size_t)(src_xl - src_xr * RDXP);
						xmax = xmin + xw2;
						if ( xmax < RDXP ){ // 拡大時の調整
							xmin = 0;
							xmax = RDXP;
						}else if ( (LONG)((xmax / RDXP) + src_xr) >= (srcInfo->bmiHeader.biWidth - 1) ){ // src の範囲外か確認
							xmax = (srcInfo->bmiHeader.biWidth - src_xr) * RDXP;
						}
// 1
//						*(DWORD *)dstp = (*(DWORD *)srcp & 0xffffff);
// 2
						pack_sums = _mm_setzero_ps();
						yi = ymin;
						for (;;){
							BYTE *srcpy;
							int ddy, ddnormal;
							__m128 pack_ddnormal;
							__m128 pack_pix;

							srcpy = srcp;
							srcp += srcwidth;
							if ( yi < RDXP ){
								ddy = RDXP - yi;
							}else if ( (ymax - yi) >= RDXP ){
								ddy = RDXP;
							}else{
								ddy = ymax & (RDXP - 1);
							}

							xi = xmin;
							ddnormal = ddy * RDXP;
							pack_ddnormal = _mm_cvtepi32_ps(_mm_set1_epi32(ddnormal));
							for (;;){
								int ddx;
								__m128 pack_ddx;
								__m64 res_pix;

								if ( src_bytes == 3 ){ // 24bit
									res_pix.m64_u32[0] = *srcpy | (*(srcpy + 1) << 8) | (*(srcpy + 2) << 16);
									srcpy += 3;
								}else if ( src_bytes == 4 ){ // 32bit
									res_pix.m64_u32[0] = *(DWORD *)srcpy;
									srcpy += 4;
								}else{ // 8bit
									res_pix.m64_u32[0] = pal[*srcpy++];
								}

								pack_pix = _mm_cvtpu8_ps(res_pix);
								// int _mm_cvtsi128_si32(__m128i a)

								if ( xi < RDXP ){
									ddx = RDXP - xi;
									xi += ddx;
									pack_ddx = _mm_cvtepi32_ps(_mm_set1_epi32(ddx * ddy));
//									pack_sums = _mm_fmadd_ps(pack_pix, pack_ddx, pack_sums);
									pack_sums = _mm_add_ps(_mm_mul_ps(pack_pix, pack_ddx) , pack_sums);
								}else if ( (xmax - xi) >= RDXP ){
									xi += RDXP;
//									pack_sums = _mm_fmadd_ps(pack_pix, pack_ddnormal, pack_sums);
									pack_sums = _mm_add_ps(_mm_mul_ps(pack_pix, pack_ddnormal) , pack_sums);
								}else{
									ddx = xmax & (RDXP - 1);
									xi += ddx;
									pack_ddx = _mm_cvtepi32_ps(_mm_set1_epi32(ddx * ddy));
//									pack_sums = _mm_fmadd_ps(pack_pix, pack_ddx, pack_sums);
									pack_sums = _mm_add_ps(_mm_mul_ps(pack_pix, pack_ddx) , pack_sums);
								}
								if ( xi >= xmax ) break;
							}
							yi += ddy;
							if ( yi >= ymax ) break;
						}
						{
							size_t rate, f_rate;
							__m64 res_pix;
							__m128 pack_frate;

							rate = (size_t)(xmax - xmin) * (ymax - ymin);
							if ( rate == 0 ) rate = 1;
							f_rate = rate / 2;
							pack_frate = _mm_cvtepi32_ps(_mm_set1_epi32(f_rate));
							pack_sums = _mm_div_ps(pack_sums, pack_frate);
							res_pix = _mm_cvtps_pi16(pack_sums);
							*(DWORD *)dst_line = res_pix.m64_u16[0] | (res_pix.m64_u16[1] << 8) | (res_pix.m64_u16[2] << 16);

//							res_pix = _mm_cvtps_pi8(pack_sums);
//							*(DWORD *)dst_line = res_pix.m64_u32[0];
						}
						dst_line += 4;
					}
#else
					for ( x = 0; x < reduce_bmp->bV5Width; x++){
						BYTE *srcp;
						size_t src_xl, src_xr;
						size_t r, g, b;

						src_xl = (((size_t)x * srcInfo->bmiHeader.biWidth * RDXP) / reduce_bmp->bV5Width);
						src_xr = src_xl / RDXP;
						srcp = src_line + src_xr * src_bytes;
						xmin = (int)(size_t)(src_xl - src_xr * RDXP);
						xmax = xmin + xw2;
						if ( xmax < RDXP ){ // 拡大時の調整
							xmin = 0;
							xmax = RDXP;
						}else if ( (LONG)((xmax / RDXP) + src_xr) >= (srcInfo->bmiHeader.biWidth - 1) ){ // src の範囲外か確認
							xmax = (srcInfo->bmiHeader.biWidth - src_xr) * RDXP;
						}
// 1
//						*(DWORD *)dstp = (*(DWORD *)srcp & 0xffffff);
// 2
						r = g = b = 0;
						yi = ymin;
						for (;;){
							BYTE *srcpy;
							int ddy, ddnormal;

							srcpy = srcp;
							srcp += srcwidth;
							if ( yi < RDXP ){
								ddy = RDXP - yi;
							}else if ( (ymax - yi) >= RDXP ){
								ddy = RDXP;
							}else{
								ddy = ymax & (RDXP - 1);
							}

							xi = xmin;
							ddnormal = ddy * RDXP;
							for (;;){
								DWORD pix;
								int ddx;

								if ( src_bytes == 3 ){ // 24bit
									pix = *srcpy | (*(srcpy + 1) << 8) | (*(srcpy + 2) << 16);
									srcpy += 3;
								}else if ( src_bytes == 4 ){ // 32bit
									pix = *(DWORD *)srcpy;
									srcpy += 4;
								}else{ // 8bit
									pix = pal[*srcpy++];
								}
								if ( xi < RDXP ){
									ddx = RDXP - xi;
									xi += ddx;
									ddx *= ddy;
									r += (size_t)GetRValue(pix) * ddx;
									g += (size_t)GetGValue(pix) * ddx;
									b += (size_t)GetBValue(pix) * ddx;
								}else if ( (xmax - xi) >= RDXP ){
									xi += RDXP;
									r += (size_t)GetRValue(pix) * ddnormal;
									g += (size_t)GetGValue(pix) * ddnormal;
									b += (size_t)GetBValue(pix) * ddnormal;
								}else{
									ddx = xmax & (RDXP - 1);
									xi += ddx;
									ddx *= ddy;
									r += (size_t)GetRValue(pix) * ddx;
									g += (size_t)GetGValue(pix) * ddx;
									b += (size_t)GetBValue(pix) * ddx;
								}
								if ( xi >= xmax ) break;
							}
							yi += ddy;
							if ( yi >= ymax ) break;
						}
						{
							size_t rate, f_rate;

							rate = (size_t)(xmax - xmin) * (ymax - ymin);
							if ( rate == 0 ) rate = 1;
							f_rate = rate / 2;
							*(DWORD *)dst_line = RGB(((r + f_rate) / rate),
												 ((g + f_rate) / rate),
												 ((b + f_rate) / rate));
						}
						dst_line += 4;
					}
#endif
					// ※ dst が 24bit なら、ここでアラインメント調整
				}
				draw = 1;
					// 自前でできなかったときは、API を使用する
			}else {
				int oldbm;
				HGDIOBJ hOldBMP;
				int lines;
				HDC hMDC;
#if DWTESTMSD
				HDC hXXX;
				hXXX = GetDC(vinfo.info.hWnd);
				hMDC = CreateCompatibleDC(hXXX);
				ReleaseDC(vinfo.info.hWnd, hXXX);
#else
				hMDC = CreateCompatibleDC(hdc);
#endif
				hOldBMP = SelectObject(hMDC, vo_.bitmap.ModifyCache.hBmp);
/*
				if ( vo->bitmap.hPal != NULL ){
					SelectPalette(hMDC, vo->bitmap.hPal, FALSE);
							// 論理パレットを現在のＤＣにマッピングする。
					RealizePalette(hMDC);
				}
*/
#if DWTESTMSD && (DRAWMODE == DRAWMODE_DW)
				if ( (hdc == NULL) && (vo->bitmap.UseICC >= 0) ){
//					hXXX = GetDC(vinfo.info.hWnd);
//					SetICMMode(hXXX, vo->bitmap.UseICC);
					SetICMMode(hMDC, vo->bitmap.UseICC);
					((xBITMAPV5HEADER *)reduce_bmp)->bV5ProfileSize = 0;
//					ColorMatchToTarget(hMDC, hXXX, CS_ENABLE);
				}
#endif
				lines = lpBitsInfo->bmiHeader.biHeight;
				if ( lines < 0 ) lines = -lines;

				oldbm = SetStretchBltMode(hMDC, XV.img.StretchMode);
				draw = StretchDIBits(hMDC,
						0, 0, reduce_bmp->bV5Width, reduce_bmp->bV5Height,
						0, 0, lpBitsInfo->bmiHeader.biWidth, lines,
						lpBits, lpBitsInfo, DIB_RGB_COLORS, SRCCOPY);
				if ( draw == IGDI_ERROR ) draw = 0;

				if ( draw == 0 ){
					draw = FixStretchDIBits(hMDC,
						0, 0, reduce_bmp->bV5Width, reduce_bmp->bV5Height,
						0, 0, lpBitsInfo->bmiHeader.biWidth, lines,
						lpBits, lpBitsInfo);
					if ( draw == IGDI_ERROR ) draw = 0;
				}
				SetStretchBltMode(hMDC, oldbm);

#if DWTESTMSD && (DRAWMODE == DRAWMODE_DW)
				if ( (hdc == NULL) && (vo->bitmap.UseICC >= 0) ){
					SetICMMode(hMDC, ICM_OFF);
//					ReleaseDC(vinfo.info.hWnd, hXXX);
				}
#endif
				SelectObject(hMDC, hOldBMP);
				DeleteDC(hMDC);
			}

			if ( htemp_bmp != NULL ) DeleteObject(htemp_bmp);
			if ( temp_bmp != NULL ) HeapFree(PPvHeap, 0, temp_bmp);
		}
	}

	if ( draw ){ // 実際の描画を行う
		#if DRAWMODE == DRAWMODE_DW
		IfDXmode(pps->ps.hdc){
			RECT box;

			box.left = XDest - XSrc;
			box.top = YDest + YSrc;
			box.right = nDestWidth;
			box.bottom = nDestHeight;
			if ( FALSE == DxDrawDIB(DxDraw, (BITMAPINFOHEADER *)reduce_bmp, MBits, &box, &pps->view, &vo->bitmap.DxCache) ){
				draw = 0;
			}
		}else
		#endif
		{
			draw = SetDIBitsToDevice(hdc,
					XDest, YDest, nDestWidth, nDestHeight,
					XSrc, YSrc, 0, nDestHeight,
					MBits, (BITMAPINFO *)reduce_bmp, DIB_RGB_COLORS);
			if ( draw == IGDI_ERROR ) draw = 0;
		}
	}
	return draw;
}

const TCHAR *TextTypeStr(TCHAR *bufcp)
{
	if ( VOi->textC < VTYPE_OTHER ) return VO_textS[VOi->textC];

	thprintf(bufcp, 16, T("CP%d"), vo_.OtherCP.codepage);
	return bufcp;
}

DWORD countbits(DWORD bits)
{
	bits = bits - ((bits >> 1) & 0x55555555);
	bits = (bits & 0x33333333) + ((bits >> 2) & 0x33333333);
	bits = (bits + (bits >> 4)) & 0x0f0f0f0f;
	bits = bits + (bits >> 8);
	bits = bits + (bits >> 16);
	return bits & 0x3f;
}

const TCHAR *GetColorInfo(BITMAPINFOHEADER *bmpinfo, TCHAR *buf)
{
	int r, g, b, a;
	DWORD rbits, gbits;

	if ( bmpinfo->biCompression != BI_BITFIELDS ){
		thprintf(buf, 32, T("%dbits"), bmpinfo->biBitCount);
		return buf;
	}

	rbits = *(DWORD *)(BYTE *)((BYTE *)bmpinfo + sizeof(BITMAPINFOHEADER));
	gbits = *(DWORD *)(BYTE *)((BYTE *)bmpinfo + sizeof(BITMAPINFOHEADER) + sizeof(DWORD));
	r = countbits(rbits);
	if ( rbits != gbits ){
		g = countbits(gbits);
		b = countbits(*(DWORD *)(BYTE *)((BYTE *)bmpinfo + sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 2));

		if ( bmpinfo->biSize >= (sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 4) ){
			a = countbits(*(DWORD *)(BYTE *)((BYTE *)bmpinfo + sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3));
			thprintf(buf, 32, T("%d(R%dG%dB%dA%d)"), bmpinfo->biBitCount, r, g, b, a);
		}else{
			thprintf(buf, 32, T("%d(R%dG%dB%d)"), bmpinfo->biBitCount, r, g, b);
		}
	}else{
		if ( bmpinfo->biSize >= (sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 4) ){
			a = countbits(*(DWORD *)(BYTE *)((BYTE *)bmpinfo + sizeof(BITMAPINFOHEADER) + sizeof(DWORD) * 3));
			thprintf(buf, 32, T("%d(Gray%dA%d)"), bmpinfo->biBitCount, r, a);
		}else{
			thprintf(buf, 32, T("%d(Gray%d)"), bmpinfo->biBitCount, r);
		}
	}
	return buf;
}

void DrawEMetafile(PPVPAINTSTRUCT *pps, PPvViewObject *vo)
{
	HPALETTE hOldPal;
	RECT box;

	if ( pps->ps.fErase != FALSE ){			// 背景表示
		box = pps->ps.rcPaint;
		if ( box.top < pps->view.top ) box.top = pps->view.top;
		DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
	}
	if ( vo->bitmap.hPal != NULL ){
		hOldPal = SelectPalette(pps->ps.hdc, vo->bitmap.hPal, FALSE);
								// 論理パレットを現在のＤＣにマッピングする。
		RealizePalette(pps->ps.hdc);
	}
	box.left = -VOi->offX;
	box.top = pps->view.top - VOi->offY;
	if ( (XV.img.MagMode == IMGD_MM_FULLSCALE) && (XV.img.AspectRate == 0) ){ // 等倍表示 --------

		box.right = box.left + vo->bitmap.showsize.cx;
		box.bottom = box.top + vo->bitmap.showsize.cy;
	}else{
#if 0
		XFORM xf;

		SetGraphicsMode(pps->ps.hdc, GM_ADVANCED);
		xf.eM11 = xf.eM12 = XV.img.imgD[imdD_MAG] / 100.0;
		xf.eM21 = xf.eM22 = xf.eDx = xf.eDy = 0.0;
		SetWorldTransform(pps.ps.hdc, &xf);

		xf.eM11 = xf.eM12 = 1.0;
		SetWorldTransform(pps->ps.hdc, &xf);
		SetGraphicsMode(pps->ps.hdc, GM_COMPATIBLE);
#endif
		int lwidth, lheight;

		if ( XV.img.MagMode == IMGD_MM_INFRAME ){ // 窓枠の大きさ
			lwidth = WndSize.cx;
			lheight = pps->view.bottom - pps->view.top;
			if ( ((lwidth << 13)  / vo->bitmap.ShowInfo->biWidth) >
				 ((lheight << 13) / vo->bitmap.ShowInfo->biHeight) ){
				lwidth = (vo->bitmap.ShowInfo->biWidth * lheight) / vo->bitmap.ShowInfo->biHeight;
			}else{
				lheight = (vo->bitmap.ShowInfo->biHeight * lwidth) / vo->bitmap.ShowInfo->biWidth;
			}
		}else{ // 固定倍率 ---------------------------------------
			lwidth = VO_maxX;
			lheight = VO_maxY;
		}
		if ( lwidth == 0 ) lwidth = 1;
		if ( lheight == 0 ) lheight = 1;

		box.left += (lwidth >= (pps->view.right - pps->view.left)) ?
				0 : ((pps->view.right - pps->view.left) - lwidth) / 2;
		box.top += (lheight >= (pps->view.bottom - pps->view.top)) ?
				0 : ((pps->view.bottom - pps->view.top) - lheight) / 2;

		box.right = box.left + lwidth;
		box.bottom = box.top + lheight;
	}

	DxFillRectColor(DxDraw, pps->ps.hdc, &box, GetStockObject(LTGRAY_BRUSH), 0xf0f0f0);
	PlayEnhMetaFile(pps->ps.hdc, vo->eMetafile.handle, &box);

	if ( vo->bitmap.hPal != NULL ){
		SelectPalette(pps->ps.hdc, hOldPal, FALSE);
	}
}

void LongHex(TCHAR *buf, UINTHL lsize)
{
	thprintf(buf, 32, T("%04Lx"), lsize.rawdata );
}

void HexSize(TCHAR *buf, UINTHL lsize)
{
	thprintf(buf, 32, T("[%04LxH]"), lsize.rawdata );
}

void DrawHex(PPVPAINTSTRUCT *pps, PPvViewObject *vo)
{
	RECT box;
	int x, y, of, hoff;
	int spc_w;
	TCHAR buf[100];
	WCHAR bufW[100];
	POINT LP;
	UINT codepage, codepage2;
	int codeext = 0;
	BOOL drawskip;

	if ( pps->print ){
		spc_w = pps->fontsize.cx;
	}else{
		spc_w = fontSpcX;
	}

	if ( OSver.dwMajorVersion < 5 ){
		codepage = 0;
	}else{
		codepage = VOi->textC;
		if  ( codepage == VTYPE_OTHER ) codepage = vo_.OtherCP.codepage;
		codepage = (codepage < VTYPE_MAX) ? VVTypeToCPlist[codepage] : codepage;
		if ( codepage == CP_ACP ) codepage = 0;
		if ( codepage != CP__NONE ){
			if ( (codepage != 0) && (IsValidCodePage(codepage) == FALSE) ){
				codepage = 0;
			}
			codepage2 = codepage;
			if ( (codepage == 0) && (VO_textS[VTYPE_SYSTEMCP] == textcp_sjis) ){
				codepage2 = CP__SJIS;
			}
		}
	}
	of = (pps->shift.cy + pps->drawYtop) * HEXSTRWIDTH;

										// 背景表示 ===================
	if ( pps->ps.fErase != FALSE ){
		// アドレス文字列より左
		box.left   = pps->ps.rcPaint.left;
		box.right  = -pps->shift.cx * pps->fontsize.cx + pps->shiftdot.cx;
		box.top    = pps->shiftdot.cy;
		box.bottom = pps->ps.rcPaint.bottom;
		DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
		// 文字列より右
		box.left   = box.right + HEXWIDTH * pps->fontsize.cx;
		box.right  = pps->ps.rcPaint.right;
		DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
	}
	DxSetTextAlign(pps->ps.hdc, TA_LEFT | TA_TOP | TA_UPDATECP); // CP を有効に
	pps->si.fg = CV_char[CV__deftext];
	pps->si.bg = CV_char[CV__defback];

	y = pps->drawYtop;
	if ( (OSver.dwMajorVersion >= 6) && (of > 0) && (codepage != CP__NONE) ){
		drawskip = TRUE;
		y--;
		hoff = of;
		of -= HEXSTRWIDTH;
	}else{
		drawskip = FALSE;
	}
	LP.y = y * pps->lineY + pps->shiftdot.cy;

	for ( ; y <= pps->drawYbottom ; y++, LP.y += pps->lineY ){
		if ( (DWORD)of >= vo->file.UseSize ) break; // 空行

		if ( drawskip == FALSE ){
			box.top = LP.y;
			DxMoveToEx(DxDraw, pps->ps.hdc,
				-pps->shift.cx * pps->fontsize.cx + pps->shiftdot.cx, LP.y);

			// アドレス部分 -------------------------
			thprintf(buf, TSIZEOF(buf), T("%08X "), of + FileDividePointer.s.L);
			if ( !pps->print ){
				DxSetTextColor(DxDraw, pps->ps.hdc, CV_lnum[0]);
				DxSetBkColor(DxDraw, pps->ps.hdc, C_back);
			}
			DxTextOutRel(DxDraw, pps->ps.hdc, buf, HEXADRWIDTH);

			if ( !pps->print ){
				if ( fontFix == FALSE ){
					DxGetCurrentPositionEx(DxDraw, pps->ps.hdc, &LP);
					box.right = -pps->shift.cx * pps->fontsize.cx + pps->shiftdot.cx + (HEXADRWIDTH - 1) * pps->fontsize.cx + spc_w;
					if ( LP.x < box.right ){
						if ( pps->ps.fErase != FALSE ){
							box.left = LP.x;
							box.bottom = box.top + pps->fontsize.cy;
							DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
						}
						DxMoveToEx(DxDraw, pps->ps.hdc, box.right, LP.y);
					}
				}

				DxSetTextColor(DxDraw, pps->ps.hdc, pps->si.fg);

				if ( VOsel.select || VOsel.highlight ){
					if ( ((VOsel.select != FALSE) &&
						  (VOsel.bottomOY <= y) && (VOsel.topOY >= y) ) ||
						 ((y + pps->shift.cy) == VOsel.foundY)
					){
						if ( pps->si.bgmode ){
							DxSetBkMode(DxDraw, pps->ps.hdc, OPAQUE);
						}
						DxSetBkColor(DxDraw, pps->ps.hdc, C_info);
						DxSetTextColor(DxDraw, pps->ps.hdc, C_back);
					}else{
						if ( pps->si.bgmode ){
							DxSetBkMode(DxDraw, pps->ps.hdc, TRANSPARENT);
						}
						DxSetTextColor(DxDraw, pps->ps.hdc, CV_char[CV__deftext]);
						DxSetBkColor(DxDraw, pps->ps.hdc, CV_char[CV__defback]);
					}
				}
			}
										// 16進部分
			hoff = of;
			for ( x = 0 ; x < (HEXNUMSWIDTH - 1) ; x += HEXNWIDTH ){
				if ( (DWORD)hoff >= vo->file.UseSize ){
					tstrcpy(&buf[x], T("   "));
				}else{
					thprintf(&buf[x], 8, T("%02X "), *(vo->file.image + hoff));
					hoff++;
				}
				if ( fontFix == FALSE ){
					buf[x + HEXNWIDTH] = '\0';

					DxGetCurrentPositionEx(DxDraw, pps->ps.hdc, &LP);
					box.right = -pps->shift.cx * pps->fontsize.cx + pps->shiftdot.cx + (HEXADRWIDTH - 1) * pps->fontsize.cx + spc_w + x * pps->fontsize.cx;
					if ( LP.x < box.right ){
						if ( pps->ps.fErase != FALSE ){
							box.left = LP.x;
							box.bottom = box.top + pps->fontsize.cy;
							DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
						}
						DxMoveToEx(DxDraw, pps->ps.hdc, box.right, LP.y);
					}
					DrawSelectedText(pps->ps.hdc, &pps->si, buf + x, HEXNWIDTH, x, y);
				}
				if ( x == HEXNWIDTH * 7 ){
					buf[x + 3] = ' ';
					x++;
				}
			}
			if ( fontFix ){
				buf[HEXNUMSWIDTH - 1] = '\0';
				DrawSelectedText(pps->ps.hdc, &pps->si, buf, HEXNUMSWIDTH - 1, 0, y);
			}

			if ( pps->ps.fErase != FALSE ){ // １６進-文字列間
				DxGetCurrentPositionEx(DxDraw, pps->ps.hdc, &LP);

				box.left = LP.x;
				box.right = (HEXADRWIDTH + HEXNUMSWIDTH - pps->shift.cx) *
						pps->fontsize.cx + pps->shiftdot.cx;
				box.bottom = box.top + pps->fontsize.cy;
				DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
			}
		}
		// 文字列部分 ---------------------------
		if ( codepage != CP__NONE ){
			HFONT font;
			int len, nextcodeext;

			len = hoff - of;
			font = pps->hBoxFont;
			nextcodeext = 0;

			if ( len ){
				if ( (VOi->textC == VTYPE_UNICODE) || (VOi->textC == VTYPE_UNICODEB) ){
					if ( (VOi->textC == VTYPE_UNICODE) && (DWORD)(of + len + 2) <= vo->file.UseSize ){
						WCHAR *lastp;

						lastp = (WCHAR *)(BYTE *)(vo->file.image + of + len - 2);
						if ( (lastp[0] == 0x0d) && (lastp[1] == 0x0a) ){
							nextcodeext += 2;
							len += 2;
						}
					}
				}else{
					BYTE *lastp, c;

					if ( codepage == CP_UTF8 ){ // １文字途中の時は延長
						int extlen;

						extlen = 5;
						while ( (DWORD)(of + len) < vo->file.UseSize ){
							if ( (*(char *)(vo->file.image + of + len) & 0xc0) != 0x80 ){
								break;
							}
							len++;
							nextcodeext++;
							if ( --extlen == 0 ) break;
						}
					}

					lastp = (BYTE *)(vo->file.image + of + len - 1);
					c = *lastp;
					if ( (c == 0x0d) && (lastp[1] == 0x0a) ){
						nextcodeext++;
						len++;
					}else if ( (codepage2 == CP__SJIS) && IskanjiA(c) ){
						BYTE *srcp;

						srcp = vo->file.image + of + codeext;
						for (;;){
							if ( srcp >= lastp ){
								if ( srcp == lastp ){
									nextcodeext++;
									len++;
								}
								break;
							}
							srcp += IskanjiA(*srcp) ? 2 : 1;
						}
					}
				}
			}
			if ( drawskip ){
				of = hoff;
				codeext = nextcodeext;
				drawskip = FALSE;
				continue;
			}

			if ( VOi->textC <= VTYPE_ANSI ){
				font = (VOi->textC == VTYPE_IBM) ? GetIBMFont() : GetANSIFont();
				SetFontDxDraw(DxDraw, font, 1);
			}else{
				DxSelectFont(DxDraw, DXFONT_MAIN);
			}
			IfGDImode(pps->ps.hdc) SelectObject(pps->ps.hdc, font);

			if ( (pps->ps.fErase != FALSE) && (font != pps->hBoxFont) ){
				box.left   = box.right; // ※１６進-文字列間 で設定した内容
				box.right  = box.left + len * pps->fontsize.cx;
				// box.bottom は １６進-文字列間 で設定済み
				DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
			}
			DxMoveToEx(DxDraw, pps->ps.hdc,
					(HEXADRWIDTH + HEXNUMSWIDTH - pps->shift.cx) *
					pps->fontsize.cx + pps->shiftdot.cx, box.top);

			if ( OSver.dwMajorVersion <= 5 ){
				if ( VOi->textC == VTYPE_UNICODE ){
					DxTextOutRelW(DxDraw, pps->ps.hdc, (WCHAR *)(vo->file.image + of), len / 2);
				}else if ( codepage != 0 ){
					DWORD size;

					size = MultiByteToWideChar(codepage,
							(codepage >= CP__NOPREC) ? 0 : MB_PRECOMPOSED,
							(char *)(vo->file.image + of),
							len, bufW, 100);
					DxTextOutRelW(DxDraw, pps->ps.hdc, bufW, size);
				}else {
					DxTextOutRelA(DxDraw, pps->ps.hdc, (char *)vo->file.image + of, len);
				}
			}else{
				char *srcA;
				WCHAR *srcW;
				DWORD size, size2;

				srcA = (char *)vo->file.image + of;
				srcW = bufW;

				if ( (codeext > 0) && (len > 0) ){
					if ( codeext > len ) codeext = len;
					srcA += codeext;
					while (codeext--) *srcW++ = ' ';
				}
				codeext = nextcodeext;

				if ( VOi->textC == VTYPE_UNICODE ){
					memcpy(srcW, srcA, len);
					size = len / 2;
				}else if ( VOi->textC == VTYPE_UNICODEB ){
					size = len / 2;
					for(;;){
						if ( len < 2 ){
							if ( len == 1 ) *srcW = '.';
							break;
						}
						*srcW++ = (WCHAR)(((WCHAR)*srcA << 8) + (WCHAR)srcA[1]);
						srcA += 2;
						len -= 2;
					}
					srcW = bufW;
				}else{
					size = MultiByteToWideChar(
							(codepage == 0) ? CP_ACP : codepage,
							(codepage >= CP__NOPREC) ? 0 : MB_PRECOMPOSED,
							srcA, len, srcW, HEXSTRWIDTH * 2);
				}
				size2 = size;

				for (;;){
					WCHAR chr;

					if ( size2 == 0 ) break;
					chr = *srcW;
					if ( chr < ' ' ){
						if ( (chr == 0x0d) && (size2 > 1) && (*(srcW + 1) == 0x0a) ){
							*srcW = XV_ctls[CTRLSIG_CRLF];
							*(srcW + 1) = 0x2060;
							srcW += 2;
							size2 -= 2;
							continue;
						}else{
							*srcW = CtrlCodeConv[chr];
						}
					}
					srcW++;
					size2--;
				}
				DxTextOutRelW(DxDraw, pps->ps.hdc, bufW, size);
			}
			IfGDImode(pps->ps.hdc) SelectObject(pps->ps.hdc, pps->hBoxFont);
		}
		of = hoff;

		if ( pps->ps.fErase != FALSE ){
			DxGetCurrentPositionEx(DxDraw, pps->ps.hdc, &LP);
			box.right = (HEXWIDTH - pps->shift.cx) * pps->fontsize.cx + pps->shiftdot.cx;
			if ( pps->lineY > pps->fontsize.cy ){ // 行下の未表示部分
				box.left   = pps->ps.rcPaint.left;
				box.top    = LP.y + pps->fontsize.cy;
				box.bottom = LP.y + pps->lineY;
				DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
			}
			box.left = LP.x;
			if ( box.left < box.right ){	// 末端より右
				box.top    = LP.y;
				box.bottom = box.top + pps->fontsize.cy;
				DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
			}
		}

		// ラインカーソル
		if ( (VOsel.cursor != FALSE) && ((pps->shift.cy + y) == VOsel.now.y.line) ){
			HBRUSH hB;

			hB = CreateSolidBrush(CV_lcsr);
			box.left = pps->ps.rcPaint.left;
			box.right = pps->ps.rcPaint.right;
			box.bottom = LP.y + pps->fontsize.cy;
			box.top = box.bottom - 1;
			if ( fontY >= 48 ) box.top -= fontY / 48;
			DxFillRectColor(DxDraw, pps->ps.hdc, &box, hB, CV_lcsr);
			DeleteObject(hB);
		}
	}
	DxSetTextAlign(pps->ps.hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP); // CP を無効

	if ( pps->ps.fErase != FALSE ){			// 背景表示 ===================
										// 未使用行 -------------------
		if ( pps->ps.rcPaint.bottom > LP.y ){
			box.left   = pps->ps.rcPaint.left;
			box.top    = LP.y;
			box.right  = pps->ps.rcPaint.right;
			box.bottom = pps->ps.rcPaint.bottom;
			DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
		}
	}
}

void ShowBitmapErrorText(PPVPAINTSTRUCT *pps)
{
	DxTextOutAbs(DxDraw, pps->ps.hdc,
			(pps->view.left + pps->view.right) / 2,
			(pps->view.top + pps->view.bottom) / 2,
			BitmapErrorText, TSIZEOFSTR(BitmapErrorText));
}

void DrawBitmap(PPVPAINTSTRUCT *pps, PPvViewObject *vo)
{
	HPALETTE hOldPal;
	int lwidth, lheight, lleft, ltop;
	RECT box, tmpbox,*clipbox;

	IfGDImode(pps->ps.hdc){
					// hPalの論理パレットに変更する。
		 if ( vo->bitmap.hPal != NULL ){
			hOldPal = SelectPalette(pps->ps.hdc, vo->bitmap.hPal, FALSE);
					// 論理パレットを現在のＤＣにマッピングする。
			RealizePalette(pps->ps.hdc);
		}
		if ( vo->bitmap.UseICC >= 0 ) SetICMMode(pps->ps.hdc, vo->bitmap.UseICC);
	}
								// DIBを画面に転送
	if ( (XV.img.MagMode == IMGD_MM_FULLSCALE) && (XV.img.AspectRate == 0) ){ // 等倍表示 --------
		lwidth = vo->bitmap.showsize.cx;
		lheight = vo->bitmap.showsize.cy;
		lleft = (lwidth >= pps->view.right - pps->view.left) ?
			pps->view.left :
			((pps->view.left + pps->view.right) - lwidth) / 2;
		ltop = (lheight >= pps->view.bottom - pps->view.top) ?
			pps->view.top :
			((pps->view.bottom + pps->view.top) - lheight) / 2;

		ImageDrawedBox.left = lleft - VOi->offX;
		ImageDrawedBox.top = ltop - VOi->offY;
		ImageDrawedBox.right = lwidth;
		ImageDrawedBox.bottom = lheight;

#ifdef USEDIRECTX
		IfDXmode(pps->ps.hdc) {
			if ( FALSE == DxDrawDIB(DxDraw, vo->bitmap.ShowInfo, vo->bitmap.bits.ptr, &ImageDrawedBox, &pps->view, &vo->bitmap.DxCache) ){
				ShowBitmapErrorText(pps);
			}
		}else // SetDIBitsToDevice へ
#endif
		{
			if ( FALSE == SetDIBitsToDevice(pps->ps.hdc,
					lleft, ltop, lwidth, lheight,
					VOi->offX, -VOi->offY,  0, lheight,
					vo->bitmap.bits.ptr,
					(BITMAPINFO *)vo->bitmap.ShowInfo, DIB_RGB_COLORS) ){
				ShowBitmapErrorText(pps);
			}
		}
	}else{									// 拡大縮小表示 -------
		int wx, wy;

		if ( XV.img.MagMode == IMGD_MM_WIDTH ){ // 窓枠の幅に画像の幅を揃える
			lwidth = WndSize.cx;
			lheight = (vo->bitmap.showsize.cy * lwidth) / vo->bitmap.showsize.cx;
			wx = 0;
			wy = (-VOi->offY * vo->bitmap.showsize.cx) / lwidth;
		}else if ( XV.img.MagMode == IMGD_MM_HEIGHT ){ // 窓枠の高さに画像の高さを揃える
			lheight = pps->view.bottom - pps->view.top;
			lwidth = (vo->bitmap.showsize.cx * lheight) / vo->bitmap.showsize.cy; //
			wx = (VOi->offX * vo->bitmap.showsize.cy) / lheight;
			wy = 0;
		}else if ( XV.img.MagMode == IMGD_MM_INFRAME ){ // 窓枠の大きさ
			lwidth = WndSize.cx;
			lheight = pps->view.bottom - pps->view.top;
			if ( ((lwidth << 13)  / vo->bitmap.showsize.cx) >
				 ((lheight << 13) / vo->bitmap.showsize.cy) ){ // 縦長
				lwidth = (vo->bitmap.showsize.cx * lheight) / vo->bitmap.showsize.cy; //
			}else{ // 横長
				lheight = (vo->bitmap.showsize.cy * lwidth) / vo->bitmap.showsize.cx;
			}
			wx = wy = 0;
		}else{ // 固定倍率 ---------------------------------------
			lwidth = VO_maxX;
			lheight = VO_maxY;
			if ( XV.img.MagMode != IMGD_MM_FULLSCALE ){
				wx = (VOi->offX * 100) / XV.img.imgD[imdD_MAG];
				wy = (-VOi->offY * 100) / XV.img.imgD[imdD_MAG];
			}else{
				wx = VOi->offX;
				wy = -VOi->offY;
			}

			if ( XV.img.AspectRate != 0 ){
				if ( XV.img.AspectRate >= ASPACTX ){
					wy = (wy * ASPACTX) / XV.img.AspectRate;
				}else{
					wx = (wx * XV.img.AspectRate) / ASPACTX;
				}
			}
		}
		if ( lwidth == 0 ) lwidth = 1;
		if ( lheight == 0 ) lheight = 1;

		lleft = (lwidth >= pps->view.right - pps->view.left) ?
			pps->view.left :
			((pps->view.left + pps->view.right) - lwidth) / 2;
		ltop = (lheight >= pps->view.bottom - pps->view.top) ?
			pps->view.top :
			((pps->view.bottom + pps->view.top) - lheight) / 2;

		ImageDrawedBox.left = lleft - VOi->offX;
		ImageDrawedBox.top = ltop - VOi->offY;
		ImageDrawedBox.right = lwidth;
		ImageDrawedBox.bottom = lheight;

#ifdef USEDIRECTX
		IfDXmode(pps->ps.hdc){
			if ( XV.img.MoreStrech && ((vo_.bitmap.page.do_animate == FALSE) || (vo_.bitmap.page.max == 0)) ){
				MoreStretchDIBits(DxDraw, pps, vo,
						lleft, ltop, lwidth, lheight,
						VOi->offX, -VOi->offY,
						vo->bitmap.bits.ptr, (BITMAPINFO *)vo->bitmap.ShowInfo);
			}else{
				if ( FALSE == DxDrawDIB(DxDraw, vo->bitmap.ShowInfo,
						vo->bitmap.bits.ptr, &ImageDrawedBox, &pps->view,
						&vo->bitmap.DxCache) ){
					ShowBitmapErrorText(pps);
				}
			}
		}else // GDI時はSetDIBitsToDevice へ
#endif
		{
			POINT oldpos;
			int result;
			int bm, oldbm;
			bm = (XV.img.MonoStretchMode == 0) ?
					XV.img.StretchMode : XV.img.MonoStretchMode;
			oldbm = SetStretchBltMode(pps->ps.hdc, bm & STRETCH_APIMASK);

			SetBrushOrgEx(pps->ps.hdc, 0, 0, &oldpos);

			if ( (XV.img.MoreStrech | (bm & STRETCH_PPVMASK) ) &&
				 ((vo_.bitmap.page.do_animate == FALSE) || (vo_.bitmap.page.max == 0)) ){
#if DRAWMODE == DRAWMODE_DW
				result = MoreStretchDIBits(DxDraw, pps, vo,
						lleft, ltop, lwidth, lheight,
						VOi->offX, -VOi->offY,
						vo->bitmap.bits.ptr, (BITMAPINFO *)vo->bitmap.ShowInfo);
#else
				result = MoreStretchDIBits(pps->ps.hdc,
						lleft, ltop, lwidth, lheight,
						VOi->offX, -VOi->offY,
						vo->bitmap.bits.ptr, (BITMAPINFO *)vo->bitmap.ShowInfo);
#endif

			}else{
				result = StretchDIBits(pps->ps.hdc,
					lleft, ltop, lwidth, lheight,
					wx, wy, vo->bitmap.rawsize.cx, vo->bitmap.rawsize.cy,
					vo->bitmap.bits.ptr, (BITMAPINFO *)vo->bitmap.ShowInfo,
					DIB_RGB_COLORS, SRCCOPY);
			}
			if ( (result == 0) || (result == IGDI_ERROR) ){
				result = FixStretchDIBits(pps->ps.hdc,
					lleft, ltop, lwidth, lheight, wx, wy,
					vo->bitmap.rawsize.cx, vo->bitmap.rawsize.cy,
					vo->bitmap.bits.ptr, (BITMAPINFO *)vo->bitmap.ShowInfo);
				if ( result == 0 ) ShowBitmapErrorText(pps);
			}
			SetBrushOrgEx(pps->ps.hdc, oldpos.x, oldpos.y, NULL);
			SetStretchBltMode(pps->ps.hdc, oldbm);
		}
	}
	if ( pps->ps.fErase != FALSE ){	// 背景表示
		if ( Use2ndView ){
			tmpbox.left = max(pps->ps.rcPaint.left, pps->view.left);
			tmpbox.top = max(pps->ps.rcPaint.top, pps->view.top);
			tmpbox.right = max(pps->ps.rcPaint.right, pps->view.right);
			tmpbox.bottom = max(pps->ps.rcPaint.bottom, pps->view.bottom);
			clipbox = &tmpbox;
		}else{
			clipbox = &pps->ps.rcPaint;
		}

		if ( lleft > clipbox->left ){ // 左真ん中消去
			box.left   = clipbox->left;
			box.top    = ltop;
			box.right  = lleft;
			box.bottom = ltop + lheight;
			DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
		}
		if ( ltop > clipbox->top ){ // 上消去
			box.left   = clipbox->left;
			box.top    = max(clipbox->top, pps->view.top);
			box.right  = clipbox->right;
			box.bottom = ltop;
			DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
		}
		if ( clipbox->right > (lleft + lwidth) ){
			box.left   = lleft + lwidth;  // 右真ん中消去
			box.top    = ltop;
			box.right  = clipbox->right;
			box.bottom = ltop + lheight;
			DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
		}
		if ( clipbox->bottom > (ltop + lheight) ){
			box.left   = clipbox->left;  // 下消去
			box.top    = ltop + lheight;
			box.right  = clipbox->right;
			box.bottom = clipbox->bottom;
			DxFillBack(DxDraw, pps->ps.hdc, &box, C_BackBrush);
		}
	}
									// 前のパレットに戻す。
	IfGDImode(pps->ps.hdc) {
		if ( vo->bitmap.UseICC >= 0 ) SetICMMode(pps->ps.hdc, ICM_OFF);

		if ( vo->bitmap.hPal != NULL ) SelectPalette(pps->ps.hdc, hOldPal, FALSE);
	}
}

void DrawViewObject(PPVPAINTSTRUCT *pps, PPvViewObject *vo)
{
	HRGN hClipRgn;

	if ( pps->ps.rcPaint.bottom <= pps->view.top ) return;

	pps->drawYtop = (pps->ps.rcPaint.top - pps->view.top) / LineY;
	if ( pps->drawYtop < 0 ) pps->drawYtop = 0;
	pps->drawYbottom = (pps->ps.rcPaint.bottom - pps->view.top) / LineY;

	if ( Use2ndView ){
		IfGDImode(pps->ps.hdc) {
			hClipRgn = CreateRectRgnIndirect(&pps->view);
			SelectClipRgn(pps->ps.hdc, hClipRgn);
		}
	}

	switch( vo->DModeType ){
		case DISPT_HEX:	//=================================================
			pps->print = FALSE;
			pps->shift.cx = VOi->offX;
			pps->shift.cy = VOi->offY;
			pps->shiftdot.cx = XV_left;
			pps->shiftdot.cy = pps->view.top;
			pps->fontsize.cx = fontHexX;
			pps->fontsize.cy = fontY;
			pps->lineY = LineY;
			pps->hBoxFont = hBoxFont;
			DrawHex(pps, vo);
			break;

		case DISPT_DOCUMENT: //============================================
			if ( vo->DocmodeType == DOCMODE_EMETA ){ // Meta file
				DrawEMetafile(pps, vo);
				break;
			}
			// DISPT_TEXT へ
		case DISPT_TEXT:// ================================================
			PaintText(pps, vo);
			break;

		case DISPT_RAWIMAGE: //============================================
		case DISPT_IMAGE:
			DrawBitmap(pps, vo);
			break;

		default:							// 表示対象が無い場合
			if ( pps->ps.fErase != FALSE ){
				DxFillBack(DxDraw, pps->ps.hdc, &pps->view, C_BackBrush);
			}
	}

	if ( Use2ndView ){
		IfGDImode(pps->ps.hdc) {
			SelectClipRgn(pps->ps.hdc, NULL);
			DeleteObject(hClipRgn);
		}
	}

	if ( (X_swmt & 1) && (vo_.memo.bottom != NULL) && (vo_.memo.top > 0) &&
		((X_swmt < 2) || (vo->DModeType == DISPT_IMAGE)) ){
		DxSetTextColor(DxDraw, pps->ps.hdc, C_info);
		DxSetBkMode(DxDraw, pps->ps.hdc, TRANSPARENT);
#ifdef USEDIRECTX
		DxMoveToEx(DxDraw, pps->ps.hdc, pps->view.left, pps->view.top);
#endif
		DxDrawText(DxDraw, pps->ps.hdc,(const TCHAR *)vo_.memo.bottom, -1, &pps->view, DT_NOCLIP | DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_EDITCONTROL);
		if ( !pps->si.bgmode ) DxSetBkMode(DxDraw, pps->ps.hdc, OPAQUE);
	}
}

void Paint(HWND hWnd)
{
	PPVPAINTSTRUCT pps;
	int x;
	HGDIOBJ hOldFont;
	TCHAR buf[VFPS];
	TCHAR buf2[VFPS];
	TCHAR buf3[VFPS];
	TCHAR bufcp[16];
	POINT LP;
	RECT box;
	int oldBKmode;
	HDC hOldDC;
										// 表示環境の設定 ---------------------
#ifndef USEDIRECTX
	BeginPaint(hWnd, &pps.ps);
#else
	if ( (DxDraw == NULL) ||
		 ((vo_.DModeType == DISPT_DOCUMENT) && (vo_.DocmodeType == DOCMODE_EMETA)) ||
		 ((vo_.DModeType == DISPT_IMAGE) && (vo_.bitmap.UseICC == ICM_ON)) ){
		// 強制で GDI 描画(DirectX 使用不可 || .emf 描画時 || ICC bmp 描画時)
		BeginPaint(hWnd, &pps.ps);
		// DirectX 描画
	}else if ( BeginDxDraw(DxDraw, &pps.ps) != DXSTART_NODRAW ){
		if ( (vo_.DModeBit & VO_dmode_TEXTLIKE) && (VOsel.cursor != FALSE) ){
			HideCaret(hWnd);
		}
	}else{
		// DirectX 描画に失敗したため GDI 描画
		BeginPaint(hWnd, &pps.ps);
//		return;
	}
#endif
	pps.si.bgmode = FALSE;
	if ( X_fles ) IfGDImode(pps.ps.hdc) {
		InitOffScreen(&BackScreen, pps.ps.hdc, &WndSize);
		hOldDC = pps.ps.hdc;
		pps.ps.hdc = BackScreen.hOffScreenDC;

		if ( X_fles == 2 ){
			pps.ps.rcPaint.top = 0;
			pps.ps.rcPaint.bottom = WndSize.cy;
			pps.ps.fErase = FALSE;
			pps.si.bgmode = TRUE;
		}
	}

	IfGDImode(pps.ps.hdc){
		hOldFont = SelectObject(pps.ps.hdc, hBoxFont);	// フォント
	}

	if ( BackScreen.X_WallpaperType && !(vo_.DModeBit & VO_dmode_IMAGE) ){
		pps.si.bgmode = TRUE;

		DrawWallPaper(DIRECTXARG(DxDraw) &BackScreen, hWnd, &pps.ps, WndSize.cx);
		oldBKmode = DxSetBkMode(DxDraw, pps.ps.hdc, TRANSPARENT);
		pps.ps.fErase = FALSE;
	}else if ( X_ffix ){
		if ( !((vo_.DModeType == DISPT_IMAGE) && (vo_.bitmap.UseICC == ICM_ON))  ){ // ICM_ON のときは、ちらつきが発生するので抑制
			DxFillBack(DxDraw, pps.ps.hdc, &pps.ps.rcPaint, C_BackBrush);
			pps.si.bgmode = TRUE;
			oldBKmode = DxSetBkMode(DxDraw, pps.ps.hdc, TRANSPARENT);
			pps.ps.fErase = FALSE;
		}
	}

										// １行目 -----------------------------
	if ( pps.ps.rcPaint.top < BoxStatus.bottom ){
		DxSetTextAlign(pps.ps.hdc, TA_LEFT | TA_TOP | TA_UPDATECP); // CP を有効に
		DxMoveToEx(DxDraw, pps.ps.hdc, 0, 0);
		LP.x = 0;
		box.top = 0;
		box.bottom = fontY - 1;

										// 報告文字列表示 *********************
		if ( (PopMsgFlag & PMF_DISPLAYMASK) || ShowImageScale ){
			DxSetTextColor(DxDraw, pps.ps.hdc, C_res[0]);		// 文字色
			DxSetBkColor(DxDraw, pps.ps.hdc, C_res[1]);

			if ( pps.si.bgmode ) DxSetBkMode(DxDraw, pps.ps.hdc, OPAQUE);

			if ( PopMsgFlag & PMF_DISPLAYMASK ){
				DxTextOutBack(DxDraw, pps.ps.hdc, PopMsgStr, tstrlen32(PopMsgStr));
			}else if ( ShowImageScale && (vo_.DModeBit & VO_dmode_IMAGE) ){
				POINT pos;
				TCHAR *last;

				GetCursorPos(&pos);
				ScreenToClient(hWnd, &pos);
				if ( (pos.x >= ImageDrawedBox.left) &&
					 (pos.y >= ImageDrawedBox.top) &&
					 (pos.x < (ImageDrawedBox.left + ImageDrawedBox.right)) &&
					 (pos.y < (ImageDrawedBox.top + ImageDrawedBox.bottom)) ){
					pos.x = ((pos.x - ImageDrawedBox.left) *
								vo_.bitmap.ShowInfo->biWidth) /
								ImageDrawedBox.right;
					pos.y = ((pos.y - ImageDrawedBox.top) *
								vo_.bitmap.ShowInfo->biHeight) /
								ImageDrawedBox.bottom;
					if ( (vo_.bitmap.ShowInfo->biBitCount == 24) || (vo_.bitmap.ShowInfo->biBitCount == 32) ){
						int off;
						COLORREF col;
						off = ((vo_.bitmap.info->biHeight >= 0) ?
							(vo_.bitmap.info->biHeight - pos.y - 1) : pos.y) *
							DwordBitSize(vo_.bitmap.rawsize.cx * vo_.bitmap.info->biBitCount) +
							pos.x * (vo_.bitmap.info->biBitCount / 8);
						col = *((COLORREF *)(BYTE *)(vo_.bitmap.bits.ptr + off));
						last = thprintf(buf, TSIZEOF(buf), T("(%d, %d) R:%2x G:%2x B:%2x"),
								pos.x, pos.y,
								GetBValue(col), GetGValue(col), GetRValue(col));
					}else{
						last = thprintf(buf, TSIZEOF(buf), T("(%d, %d)"),
								pos.x, pos.y);
					}
					DxTextOutBack(DxDraw, pps.ps.hdc, buf, last - buf);
				}
			}
			if ( pps.si.bgmode ) DxSetBkMode(DxDraw, pps.ps.hdc, TRANSPARENT);

			DxGetCurrentPositionEx(DxDraw, pps.ps.hdc, &LP);
			box.left  = LP.x;
			box.right = LP.x += fontX / 2;
			if ( pps.ps.fErase != FALSE ){
				DxFillBack(DxDraw, pps.ps.hdc, &box, C_BackBrush);
			}
			DxMoveToEx(DxDraw, pps.ps.hdc, box.right, 0);
			ShowImageScale = 0;
		}else if ( HMpos >= 0 ){			// Hidden Menu +++++++++++
			int i;
			char **data;

			box.top = 0;
			box.bottom = fontY;
			box.left = 0;
			data = XV.HiddenMenu.data;
			for ( i = 0 ; i < XV.HiddenMenu.item ; i++, box.left = box.right ){
				int namelen;
				COLORREF *color;

				namelen = HiddenMenu_GetNameLen(*data);
				color = HiddenMenu_Color(*data, namelen + 1);

				DxMoveToEx(DxDraw, pps.ps.hdc, box.left, 0);
				box.right = box.left + fontX * XV.HiddenMenu.width;

				if ( i == HMpos ){
					if ( pps.si.bgmode ){
						DxSetBkMode(DxDraw, pps.ps.hdc, OPAQUE);
					}
					DxSetTextColor(DxDraw, pps.ps.hdc, color[HiddenMenu_ColorB]);
					DxSetBkColor(DxDraw, pps.ps.hdc, color[HiddenMenu_ColorF]);
				}else{
					DxSetTextColor(DxDraw, pps.ps.hdc, color[HiddenMenu_ColorF]);
					DxSetBkColor(DxDraw, pps.ps.hdc, color[HiddenMenu_ColorB]);
				}
				DxTextOutBack(DxDraw, pps.ps.hdc,(TCHAR *)*data, namelen);
				DxGetCurrentPositionEx(DxDraw, pps.ps.hdc, &LP);
				if ( (i == HMpos) && pps.si.bgmode ){
					DxSetBkMode(DxDraw, pps.ps.hdc, TRANSPARENT);
				}
				box.left = LP.x;
				if ( pps.ps.fErase != FALSE ){
					DxFillBack(DxDraw, pps.ps.hdc, &box, C_BackBrush);
				}
				data++;
			}
			#pragma warning(suppress:4701) // XV.HiddenMenu.item は必ず1以上
			LP.x = box.right;
		}
		buf3[0] = '\0';
		if ( FileDivideMode < FDM_NODIVMAX ){
			if ( vo_.DModeType != DISPT_HEX ){
				FormatNumber(buf2, XFN_SEPARATOR, XFNW_FULL_SEP, vo_.file.UseSize, 0);
			}else{ // DISPT_HEX
				HexSize(buf2, FileRealSize);
				thprintf(buf3, 8, T("%04x"),
						(VOsel.cursor != FALSE) ?
								((VOsel.now.y.line * 16) + (VOsel.now.x.offset / HEXNWIDTH)) :
								(VOi->offY * 16) );
			}
		}else{
			TCHAR *ptr;

			ptr = FormatNumber(buf2, XFN_SEPARATOR, 7, vo_.file.UseSize, 0);
			*ptr++ = '/';
			if ( vo_.DModeType != DISPT_HEX ){
				FormatNumber(ptr, XFN_SEPARATOR, 7, FileRealSize.s.L, FileRealSize.s.H);
			}else{ // DISPT_HEX
				HexSize(ptr, FileRealSize);
			}

			buf3[0] = '+';
			if ( IsTrue(EnableFileTrackPointer) ){
				if ( vo_.DModeType != DISPT_HEX ){
					FormatNumber(buf3 + 1, XFN_SEPARATOR, 7, FileTrackPointer.s.L, FileTrackPointer.s.H);
				}else{ // DISPT_HEX
					LongHex(buf3, FileTrackPointer);
				}
				tstrcat(buf3, T("(pause read)"));
				EnableFileTrackPointer = FALSE;
			}else{
				FormatNumber(buf3 + 1, XFN_SEPARATOR, 7, FileDividePointer.s.L, FileDividePointer.s.H);
			}
		}
		switch( vo_.DModeType ){				// Status +++++++++++++++++++++
			case DISPT_NONE:
				tstrcpy(buf, vo_.file.typeinfo);
				break;
			case DISPT_HEX: {
				if ( buf3[0] == '+' ){
					UINTHL rsize;
					DWORD line;

					line = (VOsel.cursor != FALSE) ?
							(DWORD)((VOsel.now.y.line * 16) + (VOsel.now.x.offset / HEXNWIDTH)) :
							(DWORD)(VOi->offY * 16);
					rsize = FileDividePointer;
					AddHLI(rsize, line);
					LongHex(buf3, rsize);
				}
				thprintf(buf, TSIZEOF(buf),
						T("Address:%sH TextType:%s  Type:%s Filesize:%s"),
						buf3, TextTypeStr(bufcp),  vo_.file.typeinfo, buf2);
				break;
			}
			case DISPT_TEXT: {
				int line, cline, maxline;
				TCHAR LineChar;

				line = (VOsel.cursor != FALSE) ? VOsel.now.y.line : VOi->offY;
				if ( line >= VOi->line ) line = VOi->line - 1;
				if ( !XV_numt ){	// 表示行番号
					line = line + 1;
					maxline = vo_.text.cline;
					LineChar = 'Y';
				}else{				// 論理行番号
					line = VOi->ti[line].line;
					cline = vo_.text.cline; // cline は最終行 + 1を参照
					if ( cline > VOi->line ) cline = VOi->line;
					maxline = VOi->ti[cline].line - 1;
					LineChar = 'L';
				}
				if ( VOi->textC >= 0 ){
					thprintf(buf, TSIZEOF(buf), ( (BackReader != FALSE) ?
						T("%c-Line:%u/(%u)%s TextType:%s  Type:%s Filesize:%s") :
						T("%c-Line:%u/%u%s TextType:%s  Type:%s Filesize:%s")),
						LineChar, line, maxline, buf3, TextTypeStr(bufcp),
						vo_.file.typeinfo, buf2);
				}else{
					buf[0] = '\0';
				}
				break;
			}
			case DISPT_DOCUMENT:
				if ( vo_.DocmodeType == DOCMODE_EMETA ){ // Meta file
					thprintf(buf, TSIZEOF(buf),
						T("Size:%dx%d  Type:%s Filesize:%s"),
						vo_.bitmap.ShowInfo->biWidth,
						vo_.bitmap.ShowInfo->biHeight,
						vo_.file.typeinfo, buf2);
				}else{
					int line, maxline;
					TCHAR LineChar;

					line = (VOsel.cursor != FALSE) ? VOsel.now.y.line : VOi->offY;
					if ( line >= VOi->line ) line = VOi->line;
					if ( !XV_numt ){	// 表示行番号
						line = line + 1;
						maxline = vo_.text.cline;
						LineChar = 'Y';
					}else{				// 論理行番号
						if ( VOi->ti != NULL ){
							line = VOi->ti[line].line;
							maxline = VOi->ti[vo_.text.cline].line - 1;
						}else{
							maxline = vo_.text.cline;
						}
						LineChar = 'L';
					}
					thprintf(buf, TSIZEOF(buf),
							(T("%c-Line:%u/%u  Type:%s Filesize:%s")),
							LineChar, line, maxline , vo_.file.typeinfo, buf2);
				}
				break;
			case DISPT_RAWIMAGE:
			case DISPT_IMAGE: {
				TCHAR *dst;

				dst = buf;
				if ( vo_.bitmap.page.max > 1 ){
					dst = thprintf(buf, TSIZEOF(buf), T("Page:%2d/%3d "),
							vo_.bitmap.page.current + 1,
							vo_.bitmap.page.max);
				}
				dst = thprintf(dst, 256, T("Size:%dx%d(%dx%d) Color:%s"),
						vo_.bitmap.ShowInfo->biWidth,
						vo_.bitmap.ShowInfo->biHeight,
						// pixel/m → pixel/inch 変換 (39 = 1000 / 25.4)
						vo_.bitmap.ShowInfo->biXPelsPerMeter / 39,
						vo_.bitmap.ShowInfo->biYPelsPerMeter / 39,
						GetColorInfo(vo_.bitmap.ShowInfo, buf3));
				if ( vo_.bitmap.transcolor >= 0 ){
					dst = tstpcpy(dst, T(",transparent"));
				}
				if ( vo_.bitmap.UseICC >= 0){
					dst = tstpcpy(dst, (vo_.bitmap.UseICC == ICM_ON) ?
							T(",Use Prof") : T(",have Prof"));
				}
				thprintf(dst, 256, T("  Type:%s Filesize:%s"),
						vo_.file.typeinfo, buf2);
				break;
			}
		}
		DxSetTextColor(DxDraw, pps.ps.hdc, C_info);
		DxSetBkColor(DxDraw, pps.ps.hdc, C_back);
											// 内容
		x = tstrlen32(buf);
		DxTextOutRel(DxDraw, pps.ps.hdc, buf, x);

		if ( (vo_.memo.bottom != NULL) && (vo_.memo.top > 1) ){
			DxSetTextColor(DxDraw, pps.ps.hdc, C_mes);
			DxSetBkColor(DxDraw, pps.ps.hdc, C_back);
			DxTextOutRel(DxDraw, pps.ps.hdc, T(" MEMO"), 5);
		}

		if ( BackReader ){
			DxGetCurrentPositionEx(DxDraw, pps.ps.hdc, &LP);
			if ( LP.x >= (BoxStatus.right - (fontX * (int)StrLoadingLength)) ){
				LP.x = BoxStatus.right - fontX * (int)StrLoadingLength;
				if ( pps.ps.fErase == FALSE ){
					box.left  = LP.x;
					box.right = BoxStatus.right;
					DxFillBack(DxDraw, pps.ps.hdc, &box, C_BackBrush);
				}
			}
			box.left  = LP.x;
			box.right = LP.x += fontX / 2;
			if ( pps.ps.fErase != FALSE ){
				DxFillBack(DxDraw, pps.ps.hdc, &box, C_BackBrush);
			}
			DxMoveToEx(DxDraw, pps.ps.hdc, box.right, 0);

			DxSetTextColor(DxDraw, pps.ps.hdc, C_info);
			DxSetBkColor(DxDraw, pps.ps.hdc, C_back);
			DxTextOutBack(DxDraw, pps.ps.hdc, StrLoading, StrLoadingLength);
		}
		if ( LP.x < pps.ps.rcPaint.right ){
			DxGetCurrentPositionEx(DxDraw, pps.ps.hdc, &LP);
			if ( LineY > fontY ){
				box.left   = pps.ps.rcPaint.left;
				box.right  = LP.x;
				box.bottom = box.top + LineY - 1;
				box.top    = box.bottom - (LineY - 1 - fontY);
				DxFillBack(DxDraw, pps.ps.hdc, &box, C_BackBrush);
				box.top    = box.bottom - LineY;
			}
			box.left   = LP.x;
			box.right  = pps.ps.rcPaint.right;
			DxFillBack(DxDraw, pps.ps.hdc, &box, C_BackBrush);
		}

		DxSetTextAlign(pps.ps.hdc, TA_LEFT | TA_TOP | TA_NOUPDATECP); // CP を無効
	}

	DxSetTextColor(DxDraw, pps.ps.hdc, CV_char[CV__deftext]);
	DxSetBkColor(DxDraw, pps.ps.hdc, C_back);
										// 仕切り線の表示 ---------------------
	box.left = pps.ps.rcPaint.left;
	box.right = pps.ps.rcPaint.right;
	box.bottom = BoxStatus.bottom;
	box.top = box.bottom - 1;
	if ( fontY >= 48 ) box.top -= fontY / 48;
	DxFillRectColor(DxDraw, pps.ps.hdc, &box, hStatusLine, C_line);
												// 範囲選択情報 ---------------
	VOsel.bottomOY = VOsel.bottom.y.line - VOi->offY;
	VOsel.topOY = VOsel.top.y.line - VOi->offY;
										// ２行目以降内容 --------------------
	pps.view = BoxView;
	DrawViewObject(&pps, &vo_);
	if ( Use2ndView ){
		int Main_offX, Main_offY;

		pps.view = Box2ndView;
		Main_offX = VOi->offX;
		VOi->offX = offX2;
		Main_offY = VOi->offY;
		VOi->offY = offY2;
		DrawViewObject(&pps, &vo_);
		VOi->offX = Main_offX;
		VOi->offY = Main_offY;
	}
											// 後始末 -------------------------
	IfGDImode(pps.ps.hdc) {
		if ( pps.si.bgmode ) SetBkMode(pps.ps.hdc, oldBKmode);
		SelectObject(pps.ps.hdc, hOldFont);
		if ( X_fles ){
			pps.ps.hdc = hOldDC;
			OffScreenToScreen(&BackScreen, hWnd, hOldDC, &winS, &WndSize);
		}
		EndPaint(hWnd, &pps.ps);
#ifdef USEDIRECTX
	}else{
		EndDxDraw(DxDraw);
		if ( (vo_.DModeBit & VO_dmode_TEXTLIKE) && (VOsel.cursor != FALSE) ){
			ShowCaret(hWnd);
		}
#endif
	}
}
