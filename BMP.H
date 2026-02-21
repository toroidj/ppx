/*-----------------------------------------------------------------------------
	Paper Plane xUI			お手軽BMP表示
-----------------------------------------------------------------------------*/
typedef struct {
	BYTE *dibfile; // BMPファイルイメージ
	BYTE *bits; // ビットマップのイメージ
	HLOCAL info, bm;
	HPALETTE hPalette;
	SIZE size; // 画像の大きさ(DIBの値はトップダウンもあるので使わないこと)
	BITMAPINFOHEADER *DIB;
	DWORD PaletteOffset;

	struct {		// 減色パレット
		BITMAPINFOHEADER dib2;
		RGBQUAD rgb2[256];
	} nb;
} HTBMP;

#pragma pack(push, 4)
typedef struct {
	DWORD bV5Size;
	LONG  bV5Width;
	LONG  bV5Height;
	WORD  bV5Planes;
	WORD  bV5BitCount;
	// ^ BITMAPCOREHEADER
	DWORD bV5Compression;
	DWORD bV5SizeImage;
	LONG  bV5XPelsPerMeter;
	LONG  bV5YPelsPerMeter;
	DWORD bV5ClrUsed;
	DWORD bV5ClrImportant;
	// ^ BITMAPINFOHEADER
	DWORD bV5RedMask;
	DWORD bV5GreenMask;
	DWORD bV5BlueMask;
	DWORD bV5AlphaMask;
	DWORD bV5CSType;
	DWORD bV5Endpoints_[3 * 3];
	DWORD bV5GammaRed;
	DWORD bV5GammaGreen;
	DWORD bV5GammaBlue;
	// ^ BITMAPV4HEADER
	DWORD bV5Intent;
	DWORD bV5ProfileData;
	DWORD bV5ProfileSize;
	DWORD bV5Reserved;
} xBITMAPV5HEADER;
#pragma pack(pop)
#define BITMAPV4HEADERsize 0x6c
#define BITMAPV5HEADERsize 0x7c

extern PPXDLL BOOL PPXAPI LoadBMP(HTBMP *hTbmp, const TCHAR *filename, int bright);
#define BMPFIX_TOOLBAR -1000
#define BMPFIX_PREVIEW -1001
extern PPXDLL BOOL PPXAPI DrawBMP(HDC hDC, _In_ HTBMP *hTbmp, int x, int y);
extern PPXDLL BOOL PPXAPI FreeBMP(_Inout_ HTBMP *hTbmp);
extern PPXDLL BOOL PPXAPI InitBMP(HTBMP *hTbmp, const TCHAR *filename, DWORD size, int bright);
