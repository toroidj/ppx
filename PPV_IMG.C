/*-----------------------------------------------------------------------------
	Paper Plane vUI		Bitmap Image
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#include "PPV_STRU.H"
#include "PPV_FUNC.H"
#pragma hdrstop

/* DIBのヘッダからパレットを作成する */
void CreateDIBtoPalette(PPvViewObject *vo)
{
	struct {
		WORD palVersion;
		WORD palNumEntries;
		PALETTEENTRY palPalEntry[256];
	} lPal;
	PALETTEENTRY *ppal;
	RGBQUAD *rgb;
	int ClrUsed;
	BYTE *src, *dst;
	BITMAPINFOHEADER *dib = vo->bitmap.ShowInfo;

	vo->bitmap.hPal = NULL;
	vo->bitmap.rawsize.cx = dib->biWidth;
	vo->bitmap.rawsize.cy = dib->biHeight;
	FixBitmapShowSize(vo);

	ClrUsed = dib->biClrUsed ? (int)dib->biClrUsed : 1 << dib->biBitCount;
	lPal.palNumEntries = (WORD)ClrUsed;
	lPal.palVersion = 0x300;
	ppal = lPal.palPalEntry;
	if ( ClrUsed > 256 ){ //フルカラー画像は全体に散らばったパレットを作成
		int r, g, b;

		if ( VideoBits > 8 ){
			return; // パレット作成不要
		}else{
			struct {
				BITMAPINFOHEADER dib2;
				RGBQUAD rgb2[256];
			} nb;
			rgb = nb.rgb2;
			lPal.palNumEntries = 6*6*6;
			for ( b = 0 ; b <= 255 ; b += 51 ){
				for ( g = 0 ; g <= 255 ; g += 51 ){
					for ( r = 0 ; r <= 255 ; r += 51, ppal++){
						rgb->rgbRed = ppal->peRed   = (BYTE)r;
						rgb->rgbGreen = ppal->peGreen = (BYTE)g;
						rgb->rgbBlue = ppal->peBlue  = (BYTE)b;
						rgb->rgbReserved = ppal->peFlags = 0;
						rgb++;
					}
				}
			}

			if ( dib->biBitCount == 24 ){
				int x, y;

				src = dst = vo->bitmap.bits.ptr;
				dib->biSizeImage = vo->bitmap.rawsize.cx * vo->bitmap.rawsize.cy;
				for ( y = 0; y < vo->bitmap.rawsize.cy ; y++ ){
					for ( x = 0; x < dib->biWidth ; x++ ){
						r = ((*(src++) + 25) / 51);	// red
						g = ((*(src++) + 25) / 51);	// green
													// blue
						*dst++ = (BYTE)(r * 36 + g * 6 + ((*(src++) +25)/ 51));
					}
					src += (4 - ALIGNMENT_BITS(src)) & 3;
					dst += (4 - ALIGNMENT_BITS(dst)) & 3;
				}
				dib->biBitCount = 8;
				dib->biCompression = BI_RGB;
				dib->biClrUsed = 6*6*6;
				dib->biClrImportant = 0;
				nb.dib2 = *dib;

				HeapFree(PPvHeap, 0, vo->bitmap.ShowInfo);
				vo->bitmap.AllocShowInfo = FALSE;
				vo->bitmap.ShowInfo = &nb.dib2;
			}
		}
	}else{
		BYTE alpha = 0;
		int i;

		rgb = (LPRGBQUAD)((BYTE *)dib + dib->biSize);
		if ( IsBadReadPtr(rgb, ClrUsed * sizeof(RGBQUAD)) ) return;

		if ( (XV.img.imgD[imdD_MM] & AUTOMONOMODE) &&
			(ClrUsed == 2) ){
			if ( (*(COLORREF *)rgb == C_BLACK) &&
				 (*(COLORREF *)(rgb + 1) == C_WHITE) ){
				XV.img.MonoStretchMode = BLACKONWHITE;
			}else{
				XV.img.MonoStretchMode = XV.img.imgD[imdD_MM] | 0x10;
			}
		}

		for ( i = 0 ; i < ClrUsed ; i++, rgb++, ppal++ ){
			ppal->peRed   = rgb->rgbRed;
			ppal->peGreen = rgb->rgbGreen;
			ppal->peBlue  = rgb->rgbBlue;
			ppal->peFlags = 0;
			alpha |= rgb->rgbReserved;
		}
		if ( alpha > 0x80 ){ // パレットにα値が使用されていそう→透明パレットを決定
			DWORD grid_light = 0, grid_light2 = 0;
			rgb = (LPRGBQUAD)((LPSTR)dib + dib->biSize);
			alpha = 0x60;
			vo->bitmap.grid_pal1 = 0;
			vo->bitmap.grid_pal2 = 1;
			for ( i = 0 ; i < ClrUsed ; i++, rgb++ ){
				if ( alpha > rgb->rgbReserved ){
					alpha = rgb->rgbReserved;
					vo->bitmap.transcolor = i;
				}
				if ( grid_light < 0x280 ){ // グリッドに使えそうな色を取得
					DWORD bright;

					bright = rgb->rgbRed + rgb->rgbGreen + rgb->rgbBlue;
					if ( grid_light < bright ){
						grid_light2 = grid_light;
						grid_light = bright;
						vo->bitmap.grid_pal2 = vo->bitmap.grid_pal1;
						vo->bitmap.grid_pal1 = (BYTE)i;
					}else if ( grid_light2 < bright ){
						grid_light2 = bright;
						vo->bitmap.grid_pal2 = (BYTE)i;
					}
				}
			}
		}
	}
	vo->bitmap.hPal = CreatePalette((LOGPALETTE *)&lPal);
}

void ClipBitmap(HWND hWnd)
{
	DWORD palettesize = 0, profilesize = 0;
	DWORD bitmapsize = 0, tmpsize, headersize;
	HGLOBAL hImage;
	char *img;
	int color, palette C4701CHECK;
	UINT ClipType = CF_DIB;

	HGLOBAL hImageNP = NULL;
	char *imgNP;

	headersize = vo_.bitmap.info->biSize;
	if ( headersize < sizeof(BITMAPINFOHEADER) ){
												// OS/2 形式
		color = ((BITMAPCOREHEADER *)vo_.bitmap.info)->bcBitCount;
		if ( color <= 8 ){
			palette = 1 << color;
			palettesize = palette * sizeof(RGBTRIPLE);
		}
	}else{								// WINDOWS 形式
		color = vo_.bitmap.info->biBitCount;
		palette = vo_.bitmap.info->biClrUsed;
		palettesize += palette ? palette * sizeof(RGBQUAD) :
				((color <= 8) ? (1 << color) * (DWORD)sizeof(RGBQUAD) : 0);
		bitmapsize = vo_.bitmap.info->biSizeImage;

		if ( (vo_.bitmap.info->biCompression == BI_BITFIELDS) &&
			 (headersize < (sizeof(BITMAPINFOHEADER) + 12)) ){
			headersize += 12;	// 16/32bit のときはビット割り当てがある
		}

		if ( headersize == sizeof(xBITMAPV5HEADER) ){
			ClipType = CF_DIBV5;
			if ( ((xBITMAPV5HEADER *)vo_.bitmap.info)->bV5ProfileSize > 0 ){
				profilesize = ((xBITMAPV5HEADER *)vo_.bitmap.info)->bV5ProfileSize;
			}
		}
	}
	if ( color == 1 ){		/* color = 1;(省略)*/
	}else if ( color <= 4  ){	color = 4;
	}else if ( color <= 8  ){	color = 8;
	}else if ( color <= 16 ){	color = 16;
	}else if ( color <= 24 ){	color = 24;
	}else {						color = 32;
	}
	vo_.bitmap.rawsize.cx = vo_.bitmap.ShowInfo->biWidth;
	vo_.bitmap.rawsize.cy = vo_.bitmap.ShowInfo->biHeight;
	FixBitmapShowSize(&vo_);

	tmpsize = DwordBitSize(vo_.bitmap.rawsize.cx * color) * vo_.bitmap.rawsize.cy;
	if ( (bitmapsize == 0) || (tmpsize > bitmapsize) ) bitmapsize = tmpsize;

	hImage = GlobalAlloc(GMEM_MOVEABLE, headersize + palettesize + bitmapsize + profilesize);
	if ( hImage != NULL ){
		img = GlobalLock(hImage);
		if ( img == NULL ){
			GlobalFree(hImage);
			return;
		}
		memcpy(img, vo_.bitmap.info, headersize);
		if ( headersize >= sizeof(BITMAPINFOHEADER) ){ // biSizeImage を修正
			((BITMAPINFOHEADER *)img)->biSizeImage = bitmapsize;
		}
		if ( palettesize && (vo_.bitmap.hPal != NULL) ){
			PALETTEENTRY *pe;
			int i;

			pe = (PALETTEENTRY *)(img + vo_.bitmap.info->biSize);
			GetPaletteEntries(vo_.bitmap.hPal, 0, palette, pe); // C4701ok
						// PALETTEENTRY → RGBQUAD に修復
			for ( i = 0 ; i < palette ; i++, pe++ ){
				BYTE tmp;

				tmp = pe->peRed;
				pe->peRed = pe->peBlue;
				pe->peBlue = tmp;
			}
		}
		// bitmap 転送
		memcpy(img + headersize + palettesize, vo_.bitmap.bits.ptr, bitmapsize);

		if ( headersize == sizeof(xBITMAPV5HEADER) ){
			((xBITMAPV5HEADER *)img)->bV5ProfileData = 0;
			if ( profilesize != 0 ){ // ICC プロファイルあり
				 ((xBITMAPV5HEADER *)img)->bV5ProfileData = headersize + palettesize + bitmapsize;
				// 末尾にプロファイルを用意
				memcpy(img + headersize + palettesize + bitmapsize,
						(char *)vo_.bitmap.info + headersize + palettesize,
						profilesize);

				hImageNP = GlobalAlloc(GMEM_MOVEABLE, headersize + palettesize + bitmapsize);
				// プロファイル無しBMPを用意
				if ( hImageNP != NULL ){
					imgNP = GlobalLock(hImageNP);
					#pragma warning(suppress:6011 6385 6387) // GlobalLock は失敗しないと見なす
					memcpy(imgNP, img, headersize + palettesize + bitmapsize);
					#pragma warning(suppress:6011 6385 6387) // GlobalLock は失敗しないと見なす
					((xBITMAPV5HEADER *)imgNP)->bV5ProfileData = 0;
					GlobalUnlock(hImageNP);
				}
			}
		}
		GlobalUnlock(hImage);
	}
	if ( hImage == NULL ){
		SetPopMsg(POPMSG_NOLOGMSG, T("Clipboard error"), PMF_DOCMSG);
	}else{
		OpenClipboardV(hWnd);
		EmptyClipboard();
		SetClipboardData(ClipType, hImage);
		if ( hImageNP != NULL ){ // profile無しを保存
			SetClipboardData(CF_DIB, hImageNP);
		}
		CloseClipboard();
		SetPopMsg(POPMSG_NOLOGMSG, MES_CPTX, PMF_DOCMSG);
	}
	return;
}

void Rotate(HWND hWnd, int dist)
{
	BITMAPINFOHEADER *NewBmpInfo;
	BYTE *NewDib, *src, *dest;
	HLOCAL hBmpInfo, hDib;
	DWORD width, srcwidth, bmpy;
	size_t infosize;

	if ( !(vo_.DModeBit & DOCMODE_BMP) ) return;
	if ( (vo_.bitmap.info->biBitCount > 32) ||
		 ((vo_.bitmap.info->biCompression != BI_RGB) &&
			(vo_.bitmap.info->biCompression != BI_BITFIELDS)) ){
		SetPopMsg(POPMSG_NOLOGMSG, MES_VUSD, PMF_DOCMSG);
		return;
	}
	if ( vo_.bitmap.page.do_animate ){
		vo_.bitmap.page.do_animate = FALSE;
		KillTimer(hWnd, TIMERID_ANIMATE);
	}

	srcwidth = DwordBitSize(vo_.bitmap.rawsize.cx * vo_.bitmap.info->biBitCount);
	if ( dist & RotateImage_UD ){
		width = srcwidth;
		hDib = LocalAlloc(LMEM_FIXED, width * vo_.bitmap.rawsize.cy);
	}else{
		width = DwordBitSize(vo_.bitmap.rawsize.cy * vo_.bitmap.info->biBitCount);
		hDib = LocalAlloc(LMEM_FIXED, width * vo_.bitmap.rawsize.cx);
	}
	if ( hDib == NULL ){
		SetPopMsg(POPMSG_MSG, T("Memory alloc error."), PMF_DOCMSG);
		return;
	}
	NewDib = LocalLock(hDib);
	ClearBitmapModifyCache();

	if ( vo_.bitmap.info_hlocal != NULL ){
		infosize = LocalSize(vo_.bitmap.info_hlocal); // info_hlocal が NULL だと x64でデッドロック
	}else{
		infosize = 0;
	}
	if ( infosize == 0 ){
		infosize = vo_.bitmap.info->biClrUsed;
		infosize = vo_.bitmap.info->biSize +
			( infosize ? infosize * sizeof(RGBQUAD) :
			  ((vo_.bitmap.info->biBitCount <= 8) ? ((size_t)1 << vo_.bitmap.info->biBitCount) * sizeof(RGBQUAD) : 0));
	}
	hBmpInfo = LocalAlloc(LMEM_FIXED, infosize);
	if ( hBmpInfo == NULL ) return;
	NewBmpInfo = LocalLock(hBmpInfo);
	memcpy(NewBmpInfo, vo_.bitmap.info, infosize);
	if ( !(dist & RotateImage_UD) ){
		NewBmpInfo->biWidth = vo_.bitmap.info->biHeight;
		NewBmpInfo->biHeight = vo_.bitmap.info->biWidth;
		NewBmpInfo->biSizeImage = width * vo_.bitmap.rawsize.cx;
		bmpy = NewBmpInfo->biHeight;
		if ( NewBmpInfo->biWidth < 0 ){ // トップダウン
			NewBmpInfo->biWidth = -NewBmpInfo->biWidth;
			NewBmpInfo->biHeight = -NewBmpInfo->biHeight;
		}
	}
	if ( dist & RotateImage_UD ){	// 上下反転
		DWORD y;

		bmpy = (NewBmpInfo->biHeight >= 0) ? NewBmpInfo->biHeight : -NewBmpInfo->biHeight;

		for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
			src = vo_.bitmap.bits.ptr + srcwidth * y;
			dest = NewDib + srcwidth * (bmpy - y - 1);
			memcpy(dest, src, srcwidth);
		}
		if ( !(dist & RotateImage_nosave) ){
			vo_.bitmap.rotate ^= RotateImage_UD;
		}
	}else if ( dist & RotateImage_L ){	// 左回り
		if ( vo_.bitmap.info->biBitCount == 32 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + 4 * y + srcwidth * (NewBmpInfo->biWidth - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*(DWORD *)dest = *(DWORD *)src;
					dest += 4;
					src -= srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 24 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + 3 * y + srcwidth * (NewBmpInfo->biWidth - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*dest++ = *src;
					*dest++ = *(src + 1);
					*dest++ = *(src + 2);
					src -= srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 16 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + 2 * y + srcwidth * (NewBmpInfo->biWidth - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*(WORD *)dest = *(WORD *)src;
					dest += 2;
					src -= srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 8 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + y + srcwidth * (NewBmpInfo->biWidth - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*dest++ = *src;
					src -= srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 4 ){
			BYTE bits, srcB;
			int bitI;
			DWORD x, y;

			bits = 0;
			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + y / 2 + srcwidth * (NewBmpInfo->biWidth - 1);
				srcB = (BYTE)(y & 1);
				dest = NewDib + width * y;
				bitI = 0;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					bits = (BYTE)(bits << 4);
					bits |= !srcB ? (BYTE)(*src >> 4) : (BYTE)(*src & 0xf);
					if ( (bitI = ~bitI) == 0 ) *dest++ = bits;
					src -= srcwidth;
				}
				if ( bitI ) *dest = (BYTE)(bits << 4);
			}
		}else{ // vo_.bitmap.info->biBitCount == 1
			BYTE bits, srcB;
			int bitI;
			DWORD x, y;

			bits = 0;
			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + y / 8 + srcwidth * (NewBmpInfo->biWidth - 1);
				srcB = (BYTE)(B7 >> (y & 7));
				dest = NewDib + width * y;
				bitI = 0;
				for ( x = (DWORD)NewBmpInfo->biWidth ; x ; x-- ){
					bits = (BYTE)(bits << 1);
					if ( *src & srcB ) bits |= LSBIT;
					if ( ((--bitI) & 7) == 0 ) *dest++ = bits;
					src -= srcwidth;
				}
				if ( bitI & 7 ) *dest = (BYTE)(bits << (bitI & 7));
			}
		}
	}else{ // RotateImage_R90 右回り
		if ( vo_.bitmap.info->biBitCount == 32 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + 4 * (bmpy - y - 1);
				dest = NewDib + width * y;
				for ( x = (DWORD)NewBmpInfo->biWidth ; x ; x-- ){
					*(DWORD *)dest = *(DWORD *)src;
					dest += 4;
					src += srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 24 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + 3 * (bmpy - y - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*dest++ = *src;
					*dest++ = *(src + 1);
					*dest++ = *(src + 2);
					src += srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 16 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + 2 * (bmpy - y - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*(WORD *)dest = *(WORD *)src;
					dest += 2;
					src += srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 8 ){
			DWORD x, y;

			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + (bmpy - y - 1);
				dest = NewDib + width * y;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					*dest++ = *src;
					src += srcwidth;
				}
			}
		}else if ( vo_.bitmap.info->biBitCount == 4 ){
			BYTE bits, srcB;
			int bitI;
			DWORD x, y;

			bits = 0;
			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + (bmpy - y - 1) / 2;
				srcB = (BYTE)((bmpy - y - 1) & 1);
				dest = NewDib + width * y;
				bitI = 0;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					bits = (BYTE)(bits << 4);
					bits |= !srcB ? (BYTE)(*src >> 4) : (BYTE)(*src & 0xf);
					if ( (bitI = ~bitI) == 0 ) *dest++ = bits;
					src += srcwidth;
				}
				if ( bitI ) *dest = (BYTE)(bits << 4);
			}
		}else{ // vo_.bitmap.info->biBitCount == 1
			BYTE bits, srcB;
			int bitI;
			DWORD x, y;

			bits = 0;
			for ( y = 0 ; y < (DWORD)bmpy ; y++ ){
				src = vo_.bitmap.bits.ptr + (bmpy - y - 1) / 8;
				srcB = (BYTE)(B7 >> ((bmpy - y - 1) & 7));
				dest = NewDib + width * y;
				bitI = 0;
				for ( x = NewBmpInfo->biWidth ; x ; x-- ){
					bits = (BYTE)(bits << 1);
					if ( *src & srcB ) bits |= LSBIT;
					if ( ((--bitI) & 7) == 0 ) *dest++ = bits;
					src += srcwidth;
				}
				if ( bitI & 7 ) *dest = (BYTE)(bits << (bitI & 7));
			}
		}
	}
	DIRECTXDEFINE(DxDrawFreeBMPCache(&vo_.bitmap.DxCache));
	LocalUnlock(vo_.bitmap.info_hlocal);
	LocalUnlock(vo_.bitmap.bits.mapH);
	LocalFree(vo_.bitmap.info_hlocal);
	LocalFree(vo_.bitmap.bits.mapH);
	vo_.bitmap.info_hlocal = hBmpInfo;
	vo_.bitmap.bits.mapH = hDib;
	vo_.bitmap.ShowInfo = vo_.bitmap.info = NewBmpInfo;
	vo_.bitmap.bits.ptr = NewDib;

	if ( !(dist & RotateImage_UD) && (XV.img.AspectRate != 0) ){
		XV.img.AspectRate = (ASPACTX * ASPACTX) / XV.img.AspectRate;
	}

	vo_.bitmap.rawsize.cx = vo_.bitmap.info->biWidth;
	vo_.bitmap.rawsize.cy = vo_.bitmap.info->biHeight;
	FixBitmapShowSize(&vo_);

	if ( !(dist & RotateImage_nosave) ){
//		XMessage(NULL, NULL, XM_DbgDIA, T("-> %x %x"),dist, vo_.bitmap.rotate);
		vo_.bitmap.rotate = (((vo_.bitmap.rotate | RotateImage_r_support) + (dist & RotateImage_r_mask)) & RotateImage_r_mask) | (vo_.bitmap.rotate & RotateImage_mir_mask);
		InvalidateRect(hWnd, NULL, TRUE);
		SetScrollBar();

		// 90/270 回転の場合は，反転を調整
		if ( (dist & 1) || // 90回転指示
			 ((dist & RotateImage_mir_mask) && (vo_.bitmap.rotate & 1)) ){
			if ( vo_.bitmap.rotate & RotateImage_mir_mask ){ // 90回転中の反転
				vo_.bitmap.rotate ^= RotateImage_mir_mask;
			}
		}
	}
	//	XMessage(NULL, NULL, XM_DbgDIA, T("%x %x"),dist, vo_.bitmap.rotate);
}

#pragma pack(push, 1)
typedef struct {
	BYTE B, G, R, A;
} MaskBits;
#pragma pack(pop)

#define gridsize 6
#define griddefcolor 0xc0
#define griddefcolor2 0xe0

void ModifyAlpha16(void)
{
	BYTE *bit, *maxbit;
	int gridleft, gridcountX, gridcountY, width;
	BYTE gridLcolor, gridcolor, transcolor, transcolorH, gridXCHG;

	bit = vo_.bitmap.bits.ptr;
	width = DwordAlignment(vo_.bitmap.rawsize.cx / 2);
	maxbit = bit + width * vo_.bitmap.rawsize.cy;

	gridleft = gridsize / 2;
	gridcountX = 0;
	gridcountY = 0;
	gridLcolor = gridcolor = vo_.bitmap.grid_pal1;
	gridXCHG = vo_.bitmap.grid_pal1 ^ vo_.bitmap.grid_pal2;
	transcolor = (BYTE)vo_.bitmap.transcolor;
	transcolorH = (BYTE)(transcolor << 4);

	for ( ; bit < maxbit ; ){
		BYTE c = *bit;

		if ( (c & 0xf) == transcolor ) c = (BYTE)((c & 0xf0) | gridcolor);
		if ( (c & 0xf0) == transcolorH ) c = (BYTE)((c & 0xf) | (gridcolor << 4));
		*bit++ = c;

		if ( --gridleft == 0 ){
			gridcolor ^= gridXCHG;
			gridleft = gridsize / 2;
			gridcountX += gridsize / 2;
			if ( (gridcountX + (gridsize / 2)) >= width ){
				if ( gridcountX < width ){
					gridleft = width - gridcountX;
				}else{
					gridcountX = 0;
					gridcountY++;
					if ( gridcountY >= gridsize ){
						gridLcolor ^= gridXCHG;
						gridcountY = 0;
					}
					gridcolor = gridLcolor;
				}
			}
		}
	}
}

void ModifyAlpha256(void)
{
	BYTE *bit, *maxbit;
	int gridleft, gridcountX, gridcountY, width;
	BYTE gridLcolor, gridcolor, transcolor, gridXCHG;

	bit = vo_.bitmap.bits.ptr;
	width = DwordAlignment(vo_.bitmap.rawsize.cx);
	maxbit = bit + width * vo_.bitmap.rawsize.cy;

	gridleft = gridsize;
	gridcountX = 0;
	gridcountY = 0;
	gridLcolor = gridcolor = vo_.bitmap.grid_pal1;
	gridXCHG = vo_.bitmap.grid_pal1 ^ vo_.bitmap.grid_pal2;
	transcolor = (BYTE)vo_.bitmap.transcolor;

	for ( ; bit < maxbit ; bit++ ){
		if ( *bit == transcolor ) *bit = gridcolor;

		if ( --gridleft == 0 ){
			gridcolor ^= gridXCHG;
			gridleft = gridsize;
			gridcountX += gridsize;
			if ( (gridcountX + gridsize) >= width ){
				if ( gridcountX < width ){
					gridleft = width - gridcountX;
				}else{
					gridcountX = 0;
					gridcountY++;
					if ( gridcountY >= gridsize ){
						gridLcolor ^= gridXCHG;
						gridcountY = 0;
					}
					gridcolor = gridLcolor;
				}
			}
		}
	}
}

void ModifyAlpha(void)
{
	MaskBits *bit, *maxbit;
	int gridleft, gridcountX, gridcountY;
	WORD gridLcolor, gridcolor;

	DIRECTXDEFINE(DxDrawFreeBMPCache(&vo_.bitmap.DxCache));
	if ( vo_.bitmap.info->biBitCount < 32 ){
		if ( vo_.bitmap.info->biBitCount == 8 ){
			ModifyAlpha256();
		}else if ( vo_.bitmap.info->biBitCount == 4 ){
			ModifyAlpha16();
		}
	}else{
		bit = (MaskBits *)vo_.bitmap.bits.ptr;
		// 32bit なのでビット境界調整は不要
		maxbit = bit + vo_.bitmap.rawsize.cx * vo_.bitmap.rawsize.cy;

		gridleft = gridsize;
		gridcountX = 0;
		gridcountY = 0;
		gridLcolor = gridcolor = griddefcolor;

		for ( ; bit < maxbit ; bit++ ){
			WORD A, gridAcolor;

			A = (WORD)bit->A;
			if ( A != 0xff ){
				gridAcolor = (WORD)(gridcolor * (0x100 - A));
				bit->B = (BYTE)(WORD)( (((WORD)bit->B * A) + gridAcolor) >> 8);
				bit->G = (BYTE)(WORD)( (((WORD)bit->G * A) + gridAcolor) >> 8);
				bit->R = (BYTE)(WORD)( (((WORD)bit->R * A) + gridAcolor) >> 8);
				bit->A = 0xff;
			}
			if ( --gridleft == 0 ){
				gridcolor ^= (griddefcolor ^ griddefcolor2);
				gridleft = gridsize;
				gridcountX += gridsize;
				if ( (gridcountX + gridsize) >= vo_.bitmap.rawsize.cx ){
					if ( gridcountX < vo_.bitmap.rawsize.cx ){
						gridleft = vo_.bitmap.rawsize.cx - gridcountX;
					}else{
						gridcountX = 0;
						gridcountY++;
						if ( gridcountY >= gridsize ){
							gridLcolor ^= (griddefcolor ^ griddefcolor2);
							gridcountY = 0;
						}
						gridcolor = gridLcolor;
					}
				}
			}
		}
	}
}
