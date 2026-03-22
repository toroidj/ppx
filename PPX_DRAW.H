/*-----------------------------------------------------------------------------
	Paper Plane cUI
-----------------------------------------------------------------------------*/
#ifndef _PPX_DRAW_H_
#define _PPX_DRAW_H_

#define NOTIMPCACHE NULL

#define DRAWMODE_D3D 0
#define DRAWMODE_GDI 1
#define DRAWMODE_DW  2

#ifdef __cplusplus
extern "C" {
#endif

#define TIME_ANIMATEDRAW 50

enum {
	DXMOTION_NewWindow = 1, DXMOTION_Launch,
	DXMOTION_ChangePath, DXMOTION_ChangeDrive,
	DXMOTION_Root, DXMOTION_UpDir, DXMOTION_DownDir,
	DXMOTION_Busy, DXMOTION_StopBusy
};
// GDI 版 ---------------------------------------------------------------------
#if !defined(USEDIRECTX) && !defined(USEDIRECTWRITE)
#define DRAWMODE DRAWMODE_GDI

#define DIRECTXARG(DxDraw)
#define DIRECTXDEFINE(define)
#define IfGDImode(hDC)
#define IfDXmode(hDC) ><ERROR><

// 初期化・リセット・終了
#define CreateDxDraw(DxDrawPtr, hwnd, RenderMode)
#define CloseDxDraw(DxDrawPtr)
#define ResetDxDraw(DxDraw)
#define ChangeSizeDxDraw(DxDraw, color)

// 描画開始・終了・設定
#define BeginDxDraw(DxDraw) ><ERROR><
#define EndDxDraw(DxDraw) ><ERROR><
#define DxSetBkMode(DxDraw, hDC, mode) SetBkMode(hDC, mode)

// 描画位置指定
#define DxMoveToEx(DxDraw, hDC, x, y) MoveToEx(hDC, x, y, NULL)
#define DxGetCurrentPositionEx(DxDraw, hDC, lp) GetCurrentPositionEx(hDC, lp)
#define DxSetTextAlign(hDC, mode) SetTextAlign(hDC, mode)

// フォント設定
#define SetFontDxDraw(DxDraw, font, type)
#define DxSelectFont(DxDraw, type)

// 色設定
#define DxSetTextColor(DxDraw, hDC, color) SetTextColor(hDC, color)
#define DxGetTextColor(DxDraw, hDC) GetTextColor(hDC)
#define DxSetBkColor(DxDraw, hDC, color) SetBkColor(hDC, color)
#define DxGetBkColor(DxDraw, hDC) GetBkColor(hDC)

// テキスト描画
#define DxTextOutAbs(DxDraw, hDC, x, y, str, len)  TextOut(hDC, x, y, str, len)
#define DxTextOutRelA(DxDraw, hDC, str, len) TextOutA(hDC, 0, 0, str, len)
#define DxTextOutRelW(DxDraw, hDC, str, len) TextOutW(hDC, 0, 0, str, len)
#define DxTextOutRel(DxDraw, hDC, str, len)  TextOut(hDC, 0, 0, str, len)
#define DxTextOutBack(DxDraw, hDC, str, len) TextOut(hDC, 0, 0, str, len)
#define DxSetUnderLineHeight(DxDraw, heighr)
#define DxDrawText(DxDraw, hDC, str, len, box, flags) DrawText(hDC, str, len, box, flags)

// テキスト描画長方形
#define DxGetTextExtentExPoint(DxDraw, hDC, str, len, maxwidth, fitlem, alptbl, blksize) GetTextExtentExPoint(hDC, str, len, maxwidth, fitlem, alptbl, blksize)

// 長方形枠描画
#define DxFrameRectColor(DxDraw, hDC, box, brush, color) FrameRect(hDC, box, brush)

// 長方形描画
#define DxFillBack(DxDraw, hDC, box, brush) FillBox(hDC, box, brush)
// #define DxFillRect(DxDraw, hDC, box, brush) FillBox(hDC, box, brush)
#define DxFillRectColor(DxDraw, hDC, box, brush, color) FillBox(hDC, box, brush)

// ビットマップ描画
#define DxDrawFreeBMPCache(cache)
#define ResetDxDrawAtlas(DxDraw)

// 特殊処理
#define DxDrawCursor(DxDraw, hDC, box, color)
#define DxSetBenchmarkMode(DxDraw, mode)
#define DxSetMotion(DxDraw, mode)
#define DxSetRotate(DxDraw, rel_angle)
#define DxTransformPoint(DxDraw, lParam)

// DirectX9 / Direct Write 共通定義 -------------------------------------------
#else // USEDIRECTX

typedef struct tagDXDRAWSTRUCT DXDRAWSTRUCT;
typedef struct tagDXBMPCACHE DXBMPCACHE;
extern OSVERSIONINFO OSver; // OS 情報
#define DXMODEVALUE_DX NULL
#define DIRECTXARG(DxDraw) DxDraw,
#define DIRECTXDEFINE(define) define;
#define IfGDImode(hDC) if ( hDC != DXMODEVALUE_DX )
#define IfDXmode(hDC)  if ( hDC == DXMODEVALUE_DX )
#define D3DCOLORfromCOLORREF(color) ( (((color) & 0xff) << 16) | ((color) & 0xff00ff00) | (((color) & 0xff0000) >> 16) )

// 初期化・リセット・終了
#define DxRENDER_HWND 0 // ウィンドウに対して描画
#define DxRENDER_DC 1 // GDC の DC に対して描画
extern BOOL CreateDxDraw(DXDRAWSTRUCT **DxDrawPtr, HWND hWnd, int RenderType);
extern BOOL CloseDxDraw(DXDRAWSTRUCT **DxDrawPtr);
extern void ResetDxDraw(DXDRAWSTRUCT *DxDraw);
extern BOOL ChangeSizeDxDraw(DXDRAWSTRUCT *DxDraw, COLORREF backcolor);

// 描画開始・終了・設定
#define DXSTART_NODRAW 0
#define DXSTART_GDI 1
#define DXSTART_DX 2
extern int BeginDxDraw(DXDRAWSTRUCT *DxDraw, PAINTSTRUCT *ps);
extern int StartDcDraw(DXDRAWSTRUCT *DxDraw, PAINTSTRUCT *ps);
extern BOOL EndDxDraw(DXDRAWSTRUCT *DxDraw);

extern int DxSetBkMode(DXDRAWSTRUCT *DxDraw, HDC hDC, int mode);

// 描画位置指定
extern void USEFASTCALL DxMoveToEx(DXDRAWSTRUCT *DxDraw, HDC hDC, int x, int y);
extern void USEFASTCALL DxGetCurrentPositionEx(DXDRAWSTRUCT *DxDraw, HDC hDC, POINT *lp);
#define DxSetTextAlign(hDC, mode) {IfGDImode(hDC) SetTextAlign(hDC, mode);}

// Direct Write 版 ------------------------------------------------------------
#ifdef USEDIRECTWRITE
#define DRAWMODE DRAWMODE_DW

// フォント設定
extern DWORD SetFontDxDraw(DXDRAWSTRUCT *DxDraw, HFONT hFont, int type);
extern void GetFontSizeDxDraw(DXDRAWSTRUCT *DxDraw, TEXTMETRIC *tm);

#define DXFONT_MAIN 0
#define DXFONT_PROPORTIONAL 1 // for PPv
#define DXFONT_EN_US 2 // for PPv
#define DXFONT_MAIN_RIGHT 3 // for PPc
extern void DxSelectFont(DXDRAWSTRUCT *DxDraw, int type);

// 色設定
extern COLORREF DxSetTextColor(DXDRAWSTRUCT *DxDraw, HDC hDC, COLORREF color);
#define DxGetTextColor(DxDraw, hDC) //GetTextColor(hDC)
extern COLORREF DxSetBkColor(DXDRAWSTRUCT *DxDraw, HDC hDC, COLORREF color);
#define DxGetBkColor(DxDraw, hDC) //GetBkColor(hDC)

// テキスト描画
#define DxTextOutAbs(DxDraw, hDC, x, y, str, len) { DxMoveToEx(DxDraw, hDC, x, y);DxTextOutRel(DxDraw, hDC, str, len);}
#ifdef UNICODE
extern void USEFASTCALL DxTextOutRelA(DXDRAWSTRUCT *DxDraw, HDC hDC, const char *str, int len);
#define DxTextOutRelW(DxDraw, hDC, str, len) DxTextOutRel(DxDraw, hDC, str, len)
#else
#define DxTextOutRelA(DxDraw, hDC, str, len) DxTextOutBack(DxDraw, hDC, str, len)
extern void DxTextOutRelW(DXDRAWSTRUCT *DxDraw, HDC hDC, const WCHAR *str, int len);
#endif
extern void DxTextOutRel(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len);
#define DxTextOutBack(DxDraw, hDC, str, len) DxTextOutRel(DxDraw, hDC, str, len)

#define XDT_TOP_ELLIPSIS 0x40000 // ※ DT_WORD_ELLIPSIS と同じ値
extern void DxDrawText(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len, RECT *box, DWORD flags);

// テキスト描画長方形
extern BOOL DxGetTextExtentExPoint(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len, int maxwidth, int *fitlen, int *alptbl, SIZE *blksize);

// 長方形枠描画
extern void DxDrawFrameRect(DXDRAWSTRUCT *DxDraw, HDC hDC, const RECT *box, COLORREF color); // 点線枠→通常の枠
#define DxFrameRectColor(DxDraw, hDC, box, brush, color) {IfGDImode(hDC){FrameRect(hDC, box, brush);}else{DxDrawFrameRect(DxDraw, hDC, box, (color) | 0xff000000);}}

// 長方形描画
extern void DxDrawBack(DXDRAWSTRUCT *DxDraw, HDC hDC, const RECT *box, COLORREF color);
#define DxFillBack(DxDraw, hDC, box, brush) {IfGDImode(hDC){FillBox(hDC, box, brush);}}
// #define DxFillRect(DxDraw, hDC, box, brush) {IfGDImode(hDC){FillBox(hDC, box, brush);}else{DxDrawBack(DxDraw, hDC, box, 0xffffffff);}}
#define DxFillRectColor(DxDraw, hDC, box, brush, color) {IfGDImode(hDC){FillBox(hDC, box, brush);}else{DxDrawBack(DxDraw, hDC, box, (color) | 0xff000000);}}

// ビットマップ描画
extern BOOL DxDrawDIB(DXDRAWSTRUCT *DxDraw, BITMAPINFOHEADER *bmiHeader, LPVOID lpBits, const RECT *box, const RECT *clipArea, DXBMPCACHE **cache);
extern void ResetDxDrawAtlas(DXDRAWSTRUCT *DxDraw);
extern BOOL DxDrawAtlas_Check(DXDRAWSTRUCT *DxDraw, DWORD *CacheID);
extern void DxDrawAtlas(DXDRAWSTRUCT *DxDraw, BITMAPINFOHEADER *bmiHeader, LPVOID lpBits, const RECT *box);

extern void DxDrawIcon(DXDRAWSTRUCT *DxDraw, HICON icon, const RECT *box, DXBMPCACHE **cache);
extern void DxDrawFreeBMPCache(DXBMPCACHE **cache);
extern BOOL DxDrawBGpattern(DXDRAWSTRUCT *DxDraw, const RECT *box);

// 特殊処理
#define DxDrawCursor(DxDraw, hDC, box, color) DxDrawBack(DxDraw, hDC, box, (color) & 0x80ffffff)
#define DxSetBenchmarkMode(DxDraw, mode)
#define DxSetMotion(DxDraw, mode)
#define DxExtDraw(DxDraw, client)
extern void DxSetUnderLineHeight(DXDRAWSTRUCT *DxDraw, int height);

extern void DxSetRotate(DXDRAWSTRUCT *DxDraw, int rel_angle);
extern void DxTransformPoint(DXDRAWSTRUCT *DxDraw, LPARAM *lParam);

// DirectX 9 版 ---------------------------------------------------------------
#else // USEDIRECTWRITE
#define DRAWMODE DRAWMODE_D3D

// フォント設定
extern DWORD SetFontDxDraw(DXDRAWSTRUCT *DxDraw, HFONT hFont, int type);
#define DxSelectFont(DxDraw, type)

// 色設定
extern COLORREF DxSetTextColor(DXDRAWSTRUCT *DxDraw, HDC hDC, COLORREF color);
extern COLORREF USEFASTCALL DxGetTextColor(DXDRAWSTRUCT *DxDraw, HDC hDC);
extern COLORREF DxSetBkColor(DXDRAWSTRUCT *DxDraw, HDC hDC, COLORREF color);
extern COLORREF USEFASTCALL DxGetBkColor(DXDRAWSTRUCT *DxDraw, HDC hDC);

// テキスト描画
extern void DxTextOutRel(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len);
#ifdef UNICODE
extern void USEFASTCALL DxTextOutRelA(DXDRAWSTRUCT *DxDraw, HDC hDC, const char *str, int len);
#define DxTextOutRelW(DxDraw, hDC, str, len) DxTextOutRel(DxDraw, hDC, str, len)
#else
#define DxTextOutRelA(DxDraw, hDC, str, len) DxTextOutBack(DxDraw, hDC, str, len)
extern void DxTextOutRelW(DXDRAWSTRUCT *DxDraw, HDC hDC, const WCHAR *str, int len);
#endif
extern void DxTextOutBack(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len);
extern void DxDrawText(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len, const RECT *box, DWORD flags);

// テキスト描画長方形
extern BOOL DxGetTextExtentExPoint(DXDRAWSTRUCT *DxDraw, HDC hDC, const TCHAR *str, int len, int maxwidth, int *fitlen, int *alptbl, SIZE *blksize);

// 長方形枠描画
#define DxDrawFrameRect(DxDraw, hDC, box, color) DxDrawBack(DxDraw, hDC, box, (color) | 0xff000000) // 点線枠→通常の枠
#define DxFrameRectColor(DxDraw, hDC, box, brush, color) {IfGDImode(hDC){FrameRect(hDC, box, brush);}else{DxDrawBack(DxDraw, hDC, box, (color) | 0xff000000);}}

// 長方形描画
extern void DxDrawBack(DXDRAWSTRUCT *DxDraw, HDC hDC, const RECT *box, COLORREF color);
#define DxFillBack(DxDraw, hDC, box, brush) {IfGDImode(hDC){FillBox(hDC, box, brush);}}
// #define DxFillRect(DxDraw, hDC, box, brush) {IfGDImode(hDC){FillBox(hDC, box, brush);}else{DxDrawBack(DxDraw, hDC, box, 0xffffffff);}}
#define DxFillRectColor(DxDraw, hDC, box, brush, color) {IfGDImode(hDC){FillBox(hDC, box, brush);}else{DxDrawBack(DxDraw, hDC, box, (color) | 0xff000000);}}

// ビットマップ描画
extern BOOL DxDrawDIB(DXDRAWSTRUCT *DxDraw, BITMAPINFOHEADER *bmiHeader, LPVOID lpBits, const RECT *box, const RECT *clipArea, DXBMPCACHE **cache);
#define ResetDxDrawAtlas(DxDraw)
extern BOOL DxDrawAtlas_Check(DXDRAWSTRUCT *DxDraw, DWORD *CacheID);
extern void DxDrawAtlas(DXDRAWSTRUCT *DxDraw, BITMAPINFOHEADER *bmiHeader, LPVOID lpBits, const RECT *box);
extern void DxDrawIcon(DXDRAWSTRUCT *DxDraw, HICON icon, const RECT *box, DXBMPCACHE **cache);
extern void DxDrawFreeBMPCache(DXBMPCACHE **cache);

// 特殊処理
extern void DxDrawCursor(DXDRAWSTRUCT *DxDraw, HDC hDC, const RECT *box, COLORREF color);
extern void USEFASTCALL DxSetBenchmarkMode(DXDRAWSTRUCT *DxDraw, BOOL mode);
extern void DxSetMotion(DXDRAWSTRUCT *DxDraw, int mode);
extern void USEFASTCALL DxExtDraw(DXDRAWSTRUCT *DxDraw, RECT *client);

#define DxSetRotate(DxDraw, rel_angle)
#define DxTransformPoint(DxDraw, lParam)

#endif // USEDIRECTWRITE
#endif // USEDIRECTX
#ifdef __cplusplus
}
#endif // __cplusplus
#endif // _PPX_DRAW_H_
