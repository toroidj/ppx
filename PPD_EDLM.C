/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library			拡張エディット エディタ系コマンド
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "PPX.H"
#include "VFS.H"
#include "PPD_DEF.H"
#include "PPD_EDL.H"
#pragma hdrstop

//abcdefg  j lmnopq stu w  z
const PPXINMENU amenu[] = {
	{ K_c | 'Z',	T("%G\"JMUN|Undo\"(&U)\tCtrl+Z")},
	{PPXINMENY_SEPARATE, NULL},
	{ K_c | 'X',	T("%G\"JMCU|Cut\"(&T)\tCtrl+X")},
	{ K_c | 'C',	T("%G\"JMCL|Copy\"(&C)\tCtrl+C")},
	{ K_c | 'V',	T("%G\"JMPA|Paste\"(&P)\tCtrl+V")},
	{ K_del,		T("%G\"JMDE|Delete\"(&D)\tDelete")},
	{PPXINMENY_SEPARATE, NULL},
	{ K_c | 'A',	T("%G\"JMAL|Select All\"(&A)\tCtrl+A")},

	{ KE_defmenu | INMENU_BREAK,	T("%G\"MEDM|Original menu\"(&B)\tShift+F10")},
	{ K_c | ']',	T("%G\"MEMD|File &menu\"\tCtrl+]")},
	{ K_c | 'Q',	T("%G\"MEED|Edit menu\"\tCtrl+&Q")},
	{ K_s | K_F2,	T("%G\"MEST|Settings menu\"(&S)\tShift+F2")},
	{PPXINMENY_SEPARATE, NULL},
	{ KE_qu,		T("%G\"SWCA|Word case transfer\"(&W)\tCtrl+Q-U")},
	{ KE_kz,		T("%G\"SWCZ|&Zen/han transfer\"\tCtrl+K-Z")},
	{ KE_er,		T("&Run as admin\tESC-R")},
//	{ KE_ee,		T("&Execute command\tESC-E")},

	{ KE_qj | INMENU_BREAK,	T("%G\"SJLN|&Jump to Line\"\tCtrl+Q-J")},
	{ K_c | 'F',	T("%G\"VCHF|Find\"(&F)\tCtrl+F")},
	{PPXINMENY_SEPARATE, NULL},
	{ K_raw | K_s | K_c | 'P', T("Path List(&N)\tCtrl+Shift+P")},
	{ K_raw | K_s | K_c | 'L', T("PPc &List\tCtrl+Shift+L")},
	{ K_c | K_s | 'D', T("F&older dialog...\tCtrl+Shift+D")},
	{ K_c | K_s | 'I', T("%G\"VIMF|Insert selected filename...\"(&G)\tCtrl+Shift+I")},
	{ 0, NULL }
};

const TCHAR amenu2str[] = T("&Insert");
const PPXINMENU amenu2[] = {
	{ K_c | 'N',	T("File&name\tCtrl+N")},
	{ K_c | 'P',	T("Full&path Filename\tCtrl+P")},
	{ K_c | 'E',	T("Filename without &ext\tCtrl+E")},
	{ K_c | 'T',	T("File ext\tCtrl+&T")},
	{ K_c | 'R',	T("Filename on curso&r\tCtrl+&R")},
	{ K_c | '0',	T("PPx path\tCtrl+&0")},
	{ K_c | '1',	T("Current path\tCtrl+&1")},
	{ K_c | '2',	T("Pair path\tCtrl+&2")},
	{ K_F5,			T("now &Date\tF5")},
	{ 0, NULL }
};

const PPXINMENU f2menu[] = {
	{ KE_qj,	MES_SJLN},
//	{ KE_2b,	T("&Block Top/End")},
//	{ KE_2l,	T("Restore &Line")},
//	{ KE_2s,	T("&Screen Lines")}, // 行数変更
//	{ KE_kr,	T("&Read Only")},
	{ KE_qv,	MES_MERO}, //T("&View Mode")},
	{ KE_2t,	T("&TAB stop...")},
	{ KE_2c,	T("&Char code...")},
	{ KE_2r,	T("&Return code...")},
	{ KE_2p,	MES_MEWW}, // T("&Word wrap")},
	{ K_c | K_s | 'G',	T("Syntax highlight (&G)")}, // T("&Word wrap")},
	{ KE_2x,	MES_MERS}, // T("Regexp serach(&X)")},
//	{ KE_2a,	T("&Auto Save")},
	{ 0, NULL }
};

const PPXINMENU escmenu[] = {
	{ K_c | 'O',	MES_MCOP },
	{ K_F12,		T("&Save as...")},
//	{ KE_en,		T("&New Files")},
//	{ ,				T("&Read Files")},
//	{ K_c | 'L',	T("&Load File")}, // 差し替え読み込み
	{ KE_ea,		T("&Append to file") },
//	{ ,				T("&Path Rename")}, // filename を変更、保存はしない
	{ KE_ed,		T("&Duplicate PPe")},
//	{ KE_eu,		T("&Undo Edit")}, // 再読み込み
	{ KE_ei,		T("&Insert from file")},
//	{ ,				T("Close all(&X)")},
	{ K_c | 'Q',	MES_MEED}, // T("edit menu")},
	{ K_s | K_F2,	T("%G\"MEST|settings menu\"(&G)")},
	{PPXINMENY_SEPARATE, NULL },
	{ KE_kp	,		T("&Print by PPv")}, // 印刷
	{PPXINMENY_SEPARATE, NULL },
	{ K_s | K_F1,	T("&Help")},
	{PPXINMENY_SEPARATE, NULL },
	{ KE_er,		T("&Run as admin")},
	{ KE_ee,		T("&Exec command")},
	{ KE_ec,		MES_TABC },
//	{ KE_eq,		T("&Quit")},
	{ 0, NULL }
};

const PPXINMENU kmenu[] = {
	{ KE_kp,	T("&Print by PPv")},
//	{ KE_ku,	T("&Undo Paste")},
//	{ KE_kc,	T("Paste(&Copy)")},
//	{ KE_kk,	T("Copy Line/Bloc&k")},
	{ KE_kd,	MES_MEDL}, // T("&Duplicate line")},
	{ KE_kz,	MES_SWCZ},
//	{ KE_kr,	T("&Read only")},
//	{ KE_kb,	T("Column &Block")},
//	{ KE_ky,	T("&Y clear stack")},
//	{ KE_k0,	T("Set Marker #&0")},
//	{ KE_k1,	T("Set Marker #&1")},
//	{ KE_k2,	T("Set Marker #&2")},
//	{ KE_k3,	T("Set Marker #&3")},
//	{ KE_k4,	T("Set Marker #&4")},
	{ 0, NULL }
};

const PPXINMENU qmenu[] = {
//	{ KE_qh,	T("Cut word left(&H)")},
//	{ KE_qt,	T("Cu&t BOL")},
//	{ KE_qy,	T("Cut EOL(&Y)")},
	{ KE_qu,	T("%G\"SWCA|Word case\"(&U)")},
	{ KE_kz,	MES_SWCZ},
	{ KE_kd,	MES_MEDL}, // T("&Duplicate line")},
	{ KE_qv,	MES_MERO}, //T("&View Mode")},
//	{ KE_ql,	T("Restore &Line")},
//	{ KE_qf,	T("Set &Find string")},
//	{ KE_qo,	T("Replece Next(&O)")},
//	{ KE_qp,	T("&Put File Name")},
	{ K_c | K_s | 'I', T("%G\"VIMF|Insert selected filename...\"(&N)")},
	{ K_s | K_F7,	T("%G\"MEFS|&Insert find str\"\tShift+F7")},

	{ KE_qj,	MES_SJLN},
	{ KE_qr,	MES_SJTT}, // T("Top of text(&R)")},
	{ KE_qc,	MES_SJTE}, // T("End of text(&C)")},
//	{ KE_qp,	T("Last &Position")},
//	{ KE_qo,	T("Replace Next(&O)")},
//	{ KE_qlw,	T("Left of Window(&[)")},
//	{ KE_qrw,	T("Right of Window(&])")},
//	{ KE_qs,	T("Top of Line(&S)")},
//	{ KE_qd,	T("End of Line(&D)")},
	{ KE_qe,	MES_SJWT}, // T("Top of Window(&E)")},
	{ KE_qx,	MES_SJWE}, // T("End of Window(&X)")},
//	{ KE_qk,	T("Jump Brac&ket")},
//	{ KE_qm,	T("Set &Marker #0")},
//	{ KE_q0,	T("Jump to Marker #&0")},
//	{ KE_q1,	T("Jump to Marker #&1")},
//	{ KE_q2,	T("Jump to Marker #&2")},
//	{ KE_q3,	T("Jump to Marker #&3")},
//	{ KE_q4,	T("Jump to Marker #&4")},
//	{ KE_qb,	T("&Block Top/End")},
	{ 0, NULL }
};

const PPXINMENU returnmenu[] = {
	{ VTYPE_CRLF + 1,	T("C&R LF(Windows)")},
	{ VTYPE_CR + 1,		T("&CR")},
	{ VTYPE_LF + 1,		T("&LF(Unix)")},
	{ 0, NULL }
};

const PPXINMENU charmenu[charmenu_items] = {
	{ CP__US,			T("US(&A)")},
	{ CP__LATIN1,		T("&Latin1")},
	{ VTYPE_SYSTEMCP,	T("&Shift_JIS")},
	{ VTYPE_EUCJP,		T("&EUC-JP")},
	{ CP__UTF16L,		T("UTF-1&6")},
	{ VTYPE_UNICODE,	T("&UTF-16LE(BOM)")},
	{ CP__UTF16B,		T("UTF-1&6BE")},
	{ VTYPE_UNICODEB,	T("UTF-1&6BE(BOM)")},
	{ CP_UTF8,			T("UTF-&8")},
	{ VTYPE_UTF8,		T("UTF-8(&BOM)")},
	{ 0, NULL }, //	{ VTYPE_SYSTEM/CP_UTF8,	charmenustr_lcp},
	{ 0, NULL }, //	{ VTYPE_OTHER		, T("codepage : %d")}
	{ 0, NULL }
};

const TCHAR GetFileExtsStr[] = T("All Files\0*.*\0\0");

BOOL SjisToEUCjp(char **text, DWORD *size)
{
	BYTE *ptr, *maxptr;
	BYTE *bottom, *top;
	DWORD maxsize;

	maxptr = (BYTE *)(*text + *size);
	maxsize = *size + ThSTEP;
	top = bottom = HeapAlloc(DLLheap, 0, maxsize);
	if ( bottom == NULL ) return FALSE;
	ptr = (BYTE *)*text;
	while ( ptr < maxptr ){
		WORD code1;

		if ( (top + 0x8) > (bottom + maxsize) ){
			BYTE *newbottom;

			maxsize += ThNextAllocSizeM(maxsize);
			newbottom = HeapReAlloc(DLLheap, 0, bottom, maxsize);
			if ( newbottom == NULL ) break;
#pragma warning(suppress:6001) // サイズ計算のみに使用
			top = newbottom + (top - bottom);
			bottom = newbottom;
		}
		code1 = *ptr++;

		if ( code1 < 0x80 ){
			*top++ = (BYTE)code1;
			continue;
		}
		if ( IskanjiA(code1) ){
			BYTE code2;

			code2 = *ptr++;
			if ( code2 >= 0x9f ){
				code1 = (BYTE)(code1 * 2 - (code1 >= 0xe0 ? 0xe0 : 0x60));
				code2 += (BYTE)2;
			}else if (code2){
				code1 = (BYTE)(code1 * 2 - (code1 >= 0xe0 ? 0xe1 : 0x61));
				code2 += (BYTE)(0x60 + (code2 < 0x7f));
			}
			*top++ = (BYTE)code1;
			*top++ = (BYTE)code2;
		}else{
			*top++ = (BYTE)0x8e;
			*top++ = (BYTE)code1;
		}
	}
	HeapFree(ProcHeap, 0, *text);
	*text = (char *)bottom;
	*size = top - bottom;
	return TRUE;
}

void InitEditCharCode(PPxEDSTRUCT *PES)
{
	if ( PES->CharCP != 0 ) return;
	PES->CharCP = VTYPE_SYSTEMCP;
	GetCustData(T("X_newcp"), &PES->CharCP, sizeof(DWORD));
}

/*-----------------------------------------------------------------------------
	result	FALSE:cancel
-----------------------------------------------------------------------------*/
BOOL FileSave(PPxEDSTRUCT *PES, int mode)
{
	TCHAR filename[VFPS], newname[VFPS], savedir[VFPS];
	BOOL result = FALSE;

	InitEditCharCode(PES);

	savedir[0] = '\0';
	GetCustTable(StrCustOthers, T("SaveDir"), &savedir, sizeof(savedir));
	if ( savedir[0] == '\0' ){
		VFSGetRealPath(NULL, savedir, T("#5:\\"));
	}else if ( VFSGetDriveType(savedir, NULL, NULL) == NULL ){
		VFSFullPath(NULL, savedir, NULL);
	}

	ThGetString(&PES->LocalStringValue, LSV_filename, filename, VFPS);
	if ( filename[0] != '\0' ){
		VFSFullPath(newname, filename, savedir);
	}else{
		newname[0] = '\0';
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			GetWindowText(GetParentCaptionWindow(PES->hWnd), newname, TSIZEOF(newname));
		}
		if ( newname[0] <= ' ' ){
			GetCustTable(StrCustOthers, T("NewFile"), newname, TSTROFF(VFPS));
			if ( newname[0] != '\0' ){
				tstrcat(newname, T(".txt"));
				PP_ExtractMacro(PES->hWnd, NULL, NULL, newname, newname, XEO_EXTRACTEXEC);
			}
			if ( newname[0] <= ' ' ){
				tstrcpy(newname, T("edittext.txt"));
			}
			VFSFullPath(NULL, newname, savedir);
			GetUniqueEntryName(newname);
		}else{
			VFSFullPath(NULL, newname, savedir);
		}
	}

	while ( (!(mode & EDL_FILEMODE_DIALOG) &&
			  (mode & EDL_FILEMODE_NODIALOG) &&
			  (filename[0] != '\0') ) ||
			( tInput(PES->hWnd, (mode & EDL_FILEMODE_APPEND) ?
					MES_TAPN : MES_TSVN, newname, TSIZEOF(newname),
					PPXH_NAME_R, PPXH_PATH) > 0) ){
		HANDLE hFile;
		DWORD size;
		char *text;

		VFSFixPath(NULL, newname, savedir, VFSFIX_PATH);
		if ( !(mode & EDL_FILEMODE_APPEND) && tstricmp(newname, filename) ){
			hFile = CreateFileL(newname, GENERIC_READ,
					FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
			if ( hFile != INVALID_HANDLE_VALUE ){	// 同名ファイルあり
				CloseHandle(hFile);
				if ( PMessageBox(PES->hWnd, MES_QSAM, T("File exist"),
							MB_APPLMODAL | MB_OKCANCEL |
							MB_DEFBUTTON1 | MB_ICONQUESTION) != IDOK ){
					resetflag(mode, EDL_FILEMODE_NODIALOG);
					continue;
				}
			}
		}
		hFile = CreateFileL(newname, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL,
				(mode & EDL_FILEMODE_APPEND) ? OPEN_ALWAYS : CREATE_ALWAYS,
				FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if ( hFile == INVALID_HANDLE_VALUE ){
			ErrorPathBox(PES->hWnd, T("File open error"), newname, PPERROR_GETLASTERROR);
			resetflag(mode, EDL_FILEMODE_NODIALOG);
			continue;
		}
		tstrcpy(filename, newname);
		ThSetString(&PES->LocalStringValue, LSV_filename, newname);

										// 書き込み準備(各種コード変換)
		#ifdef UNICODE	// UNICODE版は個別に処理する
		if ( (PES->CharCP == VTYPE_UTF8) ||
			 (PES->CharCP == CP_UTF8) ||
			 (PES->CharCP == VTYPE_UNICODE) ||
			 (PES->CharCP == CP__UTF16L) ||
			 (PES->CharCP == VTYPE_UNICODEB) ||
			 (PES->CharCP == CP__UTF16B) ){
			int offset;

			size = GetWindowTextLengthW(PES->hWnd) + 1; // BOM分を加算
			text = HeapAlloc(ProcHeap, 0, TSTROFF(size + 1)); // '\0'分を加算
			if ( text == NULL ){
				ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
				return FALSE;
			}
			// BOM なし
			if ( (PES->CharCP == CP_UTF8) ||
				 (PES->CharCP == CP__UTF16L) ||
				 (PES->CharCP == CP__UTF16B) ){
				offset = 0;
				size--; // BOM分
			}else{ // VTYPE_UNICODE / VTYPE_UNICODEB / VTYPE_UTF8 は BOM あり
				*(WCHAR *)text = 0xfeff; // UCF2HEADER
				offset = 1;
			}

			GetWindowTextW(PES->hWnd, (WCHAR *)text + offset, size + 1);

			if ( PES->CrCode != VTYPE_CRLF ){
				WCHAR *src, *dst;
				WCHAR crcode;

				crcode = (WCHAR)((PES->CrCode == VTYPE_CR) ? '\r' : '\n');
				src = dst = (WCHAR *)text + 1;
				while ( *src ){
					if ( *src == '\r' ){
						src += 2; // '\r\n' 分
						*dst++ = crcode;
						continue;
					}
					*dst++ = *src++;
				}
				size = dst - (WCHAR *)text;
			}
			// CP_UTF8 / VTYPE_UTF8 UCS-2 → UTF-8 変換
			if ( (PES->CharCP == CP_UTF8) || (PES->CharCP == VTYPE_UTF8) ){
				int newsize;
				char *newtext;

				newsize = WideCharToMultiByteU8(CP_UTF8, 0, (WCHAR *)text, size, NULL, 0, NULL, NULL);
				newtext = HeapAlloc(ProcHeap, 0, newsize);
				if ( newtext == NULL ){
					HeapFree(ProcHeap, 0, text);
					ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
					return FALSE;
				}
				size = WideCharToMultiByteU8(CP_UTF8, 0, (WCHAR *)text, size, newtext, newsize, NULL, NULL);
				HeapFree(ProcHeap, 0, text);
				text = newtext;
			}else{ // CP__UTF16L / VTYPE_UNICODE / CP__UTF16B / VTYPE_UNICODEB
				size *= sizeof(WCHAR);
				if ( (PES->CharCP == VTYPE_UNICODEB) || (PES->CharCP == CP__UTF16B) ){
					// バイトオーダ変換
					WCHAR *uptr;
					uptr = (WCHAR *)text;
					while ( *uptr ){
						WCHAR lc;

						lc = *uptr;
						*uptr++ = (WCHAR)((lc >> 8) | (lc << 8));
					}
				}
			}
		}else{
			if ( PES->CharCP == VTYPE_SYSTEMCP ){
				size = GetWindowTextLengthA(PES->hWnd);
				text = HeapAlloc(ProcHeap, 0, size + 1);
				if ( text == NULL ){
					ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
					return FALSE;
				}
				GetWindowTextA(PES->hWnd, text, size + 1);
			}else{
				UINT cp;
				int sizeW;
				WCHAR *textW;

				cp = (PES->CharCP < VTypeToCPlist_max) ? VTypeToCPlist[PES->CharCP] : PES->CharCP;
				sizeW = GetWindowTextLengthW(PES->hWnd) + 1; // '\0'分を加算
				textW = HeapAlloc(ProcHeap, 0, TSTROFF(sizeW));
				if ( textW == NULL ){
					ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
					return FALSE;
				}
				GetWindowTextW(PES->hWnd, textW, sizeW);

				size = WideCharToMultiByteU8(cp, 0, textW, sizeW, NULL, 0, NULL, NULL);
				text = HeapAlloc(ProcHeap, 0, size);
				if ( text == NULL ){
					HeapFree(ProcHeap, 0, textW);
					ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
					return FALSE;
				}
				WideCharToMultiByteU8(cp, 0, textW, sizeW, text, size, NULL, NULL);
				if ( size > 0 ) size--; // '\0'除去
				HeapFree(ProcHeap, 0, textW);
			}

			if ( PES->CrCode != VTYPE_CRLF ){
				char *src, *dst;
				char crcode;

				crcode = (char)((PES->CrCode == VTYPE_CR) ? '\r' : '\n');
				src = dst = text;
				while ( *src ){
					if ( *src == '\r' ){
						src += 2;
						*dst++ = crcode;
						continue;
					}
					*dst++ = *src++;
				}
				size = dst - text;
			}
		}
		#else	// ANSI版は、UNICODEの場合、ある程度まとめて処理する
		size = GetWindowTextLength(PES->hWnd);
		text = HeapAlloc(ProcHeap, 0, size + 1);
		if ( text == NULL ){
			ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
			return FALSE;
		}
		GetWindowText(PES->hWnd, text, size + 1);
		if ( PES->CrCode != VTYPE_CRLF ){
			char *src, *dst;
			char crcode;

			crcode = (char)((PES->CrCode == VTYPE_CR) ? '\r' : '\n');
			src = dst = text;
			while ( *src ){
				if ( *src == '\r' ){
					src += 2;
					*dst++ = crcode;
					continue;
				}
				*dst++ = *src++;
			}
			size = dst - text;
		}
		if ( PES->CharCP != VTYPE_SYSTEMCP ){
			UINT cp;
			WCHAR *newtext;

			cp = (PES->CharCP < VTypeToCPlist_max) ? VTypeToCPlist[PES->CharCP] : PES->CharCP;
			// 一旦 UNICODE へ
			size = MultiByteToWideChar(CP_ACP, 0, text, -1, NULL, 0);
			newtext = HeapAlloc(ProcHeap, 0, size * sizeof(WCHAR));
			if ( newtext != NULL ){
				int offset;

				if ( (PES->CharCP == VTYPE_UTF8) ||
					 (PES->CharCP == VTYPE_UNICODE) ||
					 (PES->CharCP == VTYPE_UNICODEB) ){
					*newtext = 0xfeff; // UCF2HEADER
					offset = 1;
				}else{
					offset = 0;
				}
				size = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,
						text, -1, newtext + offset, size) + offset;
				HeapFree(ProcHeap, 0, text);
				text = (char *)newtext;
				if ( size ) size--; // '\0'除去
			}
			if ( (cp == CP__UTF16L) || (cp == CP__UTF16B) ){ // UNICODE の時は、そのまま
				if ( cp == CP__UTF16B ){ // バイトオーダ変換
					WCHAR *uptr;
					DWORD left;
					uptr = (WCHAR *)text;
					left = size;
					while ( left-- ){
						WCHAR lc;

						lc = *uptr;
						*uptr++ = (WCHAR)((lc >> 8) | (lc << 8));
					}
				}
				size *= sizeof(WCHAR);
			}else{ // UNICODE から該当 codepage へ
				int newsize;
				char *newtext;

				newsize = WideCharToMultiByteU8(cp, 0, (WCHAR *)text, size, NULL, 0, NULL, NULL);
				newtext = HeapAlloc(ProcHeap, 0, newsize);
				if ( newtext == NULL ){
					HeapFree(ProcHeap, 0, text);
					ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
					return FALSE;
				}
				size = WideCharToMultiByteU8(cp, 0, (WCHAR *)text, size, newtext, newsize, NULL, NULL);
				HeapFree(ProcHeap, 0, text);
				text = newtext;
			}
		}
		#endif
		if ( PES->CharCP == VTYPE_EUCJP ) SjisToEUCjp(&text, &size);
										// 書き込み
		if ( mode & EDL_FILEMODE_APPEND ){
			DWORD i = 0;

			SetFilePointer(hFile, 0, (PLONG)&i, FILE_END);
		}
		if ( WriteFile(hFile, text, size, (DWORD *)&size, NULL) == FALSE ){
			ErrorPathBox(PES->hWnd, T("File write error"), newname, PPERROR_GETLASTERROR);
		}else{
			result = TRUE;
			XEditClearModify(PES);
		}
		CloseHandle(hFile);

		HeapFree(ProcHeap, 0, text);
		if ( PES->flags & PPXEDIT_TEXTEDIT ){
			SetWindowText(GetParentCaptionWindow(PES->hWnd), newname);
		}
		if ( IsTrue(result) ) break;
	}
	return result;
}

void OpenMainFromMem(PPxEDSTRUCT *PES, int openmode, const TCHAR *filename, const TCHAR *textimage, DWORD memsize, UINT usecp)
{
	int crcode = VTYPE_CRLF, charcode;
	TCHAR *text;

	charcode = FixTextFileImage(filename, (const char *)textimage, memsize, &text, usecp);
	{									// 改行コードの決定
		int size;
		TCHAR *ptr;

		for ( ptr = text, size = 0x1000 ; *ptr && size ; ptr++, size-- ){
			if ( *ptr == '\r' ){ // CR
				if ( *(ptr + 1) == '\n' ){ // LF
					break;	// CRLF
				}
				crcode = VTYPE_CR;
				break;
			}else if ( *ptr == '\n' ){ // LF
				crcode = VTYPE_LF;
				break;
			}
		}
	}
	if ( openmode != PPE_OPEN_MODE_INSERT ){
		PES->CharCP = (charcode >= 0) ? charcode : -charcode;
		PES->CrCode = crcode;
		if ( filename != NULL ){
			ThSetString(&PES->LocalStringValue, LSV_filename, filename);
			if ( PES->flags & PPXEDIT_TEXTEDIT ){
				SetWindowText(GetParentCaptionWindow(PES->hWnd), filename);
			}
		}
	}
										// 改行コードに合わせて SetText
	if ( crcode != VTYPE_CRLF ){
		TCHAR *p, code;

		if ( openmode != PPE_OPEN_MODE_INSERT ){ // 上書き
			SendMessage(PES->hWnd, EM_SETSEL, 0, EC_LAST);
		}
		p = text;
		code = (TCHAR)((crcode == VTYPE_CR) ? T('\r') : T('\n'));
		while ( *p != '\0' ){
			TCHAR *next;

			next = tstrchr(p, code);
			if ( next == NULL ){
				SendMessage(PES->hWnd, EM_REPLACESEL, 0, (LPARAM)p);
				break;
			}else{
				*next = '\0';
				SendMessage(PES->hWnd, EM_REPLACESEL, 0, (LPARAM)p);
				SendMessage(PES->hWnd, EM_SETSEL, EC_LAST, EC_LAST);
				SendMessage(PES->hWnd, EM_REPLACESEL, 0, (LPARAM)T("\r\n"));
				SendMessage(PES->hWnd, EM_SETSEL, EC_LAST, EC_LAST);
				p = next + 1;
			}
		}
		SendMessage(PES->hWnd, EM_SETSEL, EC_LAST, EC_LAST);
	}else{
		if ( openmode != PPE_OPEN_MODE_INSERT ){ // 上書き
			SetWindowText(PES->hWnd, text);
		}else{
			SendMessage(PES->hWnd, EM_REPLACESEL, 0, (LPARAM)text);
		}
	}
	if ( charcode < 0 ) HeapFree(DLLheap, 0, text);
}

void PPeWindowSizeFix(HWND hParentWnd, int changesize) // ウィンドウサイズ調整
{
	if ( hParentWnd == NULL ) return;

	if ( changesize == PPE_OPEN_MODE_EXCMDOPENnoSIZE ){
		MoveWindowByKey(hParentWnd, 0, 0);
	}else{ // PPE_OPEN_MODE_EXCMDOPENandSIZE
		FixEditWindowRect(hParentWnd);
	}
}

/*-----------------------------------------------------------------------------
	ファイルを開く処理
-----------------------------------------------------------------------------*/
BOOL OpenFromFile(PPxEDSTRUCT *PES, int openmode, const TCHAR *fname)
{
	BOOL newmode = FALSE;
	int changesize = 0;
	DWORD memsize;
	TCHAR filename[VFPS];
	TCHAR oldfilename[VFPS];
	TCHAR *textimage = NULL;
	EDITMODESTRUCT ems = { EDITMODESTRUCT_DEFAULT };

	const TCHAR *exec = NULL, *curdir = NULL;

	TCHAR buf[CMDLINESIZE], code, *more;

	ThGetString(&PES->LocalStringValue, LSV_filename, oldfilename, VFPS);

	if ( openmode >= PPE_OPEN_MODE_CMDOPEN ){

		if ( openmode >= PPE_OPEN_MODE_EXCMDOPEN ){
			curdir = ((PPE_CMDMODEDATA *)fname)->curdir;
			fname = ((PPE_CMDMODEDATA *)fname)->param;
			changesize = openmode;
			openmode = PPE_OPEN_MODE_OPENNEW;
		}else{
			openmode -= PPE_OPEN_MODE_CMDOPEN; // CMDxxx を xxx に変換
		}

		filename[0] = '\0';
		for ( ;; ){
			code = GetOptionParameter(&fname, buf, &more);
			if ( code == '\0' ) break;
			if ( code != '-' ){
				if ( (curdir != NULL) || (oldfilename[0] == '\0') ){
					VFSFixPath(filename, buf, curdir, VFSFIX_FULLPATH | VFSFIX_REALPATH);
				}else{
					// カレントディレクトリを生成
					VFSFullPath(filename, T(".."), oldfilename);
					VFSFixPath(filename, buf, filename, VFSFIX_FULLPATH | VFSFIX_REALPATH);
				}
			}else{
				if ( tstrcmp(buf + 1, T("NEW")) == 0 ){
					newmode = TRUE;
				}else if ( tstrcmp(buf + 1, T("K")) == 0 ){
					exec = fname;
					break;
				}else if ( EditModeParam(&ems, buf + 1, more) == FALSE ){
					XMessage(PES->hWnd, T("File open error"), XM_GrERRld, MES_EUOP, buf);
				}
			}
			NextParameter(&fname);
		}
	}else{
		tstrcpy(filename, fname);
	}
	if ( filename[0] != '\0' ){	// ファイルの読み込み
		ERRORCODE result;

		result = VFSLoadFileImage(filename, 0x40, (char **)&textimage, &memsize, NULL);
		if ( result != NO_ERROR ){
			if ( newmode == FALSE ){
				HWND hParentWnd;

				if ( oldfilename[0] == '\0' ){
					ThSetString(&PES->LocalStringValue, LSV_filename, filename);
					// 設定初期化1
					PPeSetTab(PES, ems.tabwidth);
					if ( ems.crcode >= 0 ) PES->CrCode = ems.crcode;
				}
				hParentWnd = GetParent(PES->hWnd);
				if ( changesize != 0 ){ // ウィンドウサイズ調整
					PPeWindowSizeFix(hParentWnd, changesize);
				}

				if ( openmode >= 0 ){ // PPE_OPEN_MODE_OPEN / PPE_OPEN_MODE_INSERT
					ErrorPathBox(PES->hWnd, NULL, filename, result);
				}else{ // PPE_OPEN_MODE_OPENNEW
					if ( hParentWnd != NULL ){
						SetWindowText(hParentWnd, filename);
					}
				}
				if ( hParentWnd != NULL ){
					SetMessageOnCaption(hParentWnd, MES_EOFI);
				}

				if ( exec != NULL ) EditExtractMacro(PES, exec, NULL, 0);
				return FALSE;
			}
		}
	}
	if ( (textimage == NULL) && (openmode != PPE_OPEN_MODE_INSERT) ){
		textimage = HeapAlloc(ProcHeap, HEAP_ZERO_MEMORY, 4);
		memsize = 4;
	}
	SendMessage(PES->hWnd, WM_SETREDRAW, FALSE, 0);
										// 文字コード判別＆変換
	if ( textimage != NULL ){
		OpenMainFromMem(PES, openmode, filename, textimage, memsize, ems.codepage);
		HeapFree(ProcHeap, 0, textimage);
	}
	// 設定初期化2
	PPeSetTab(PES, ems.tabwidth);
	if ( ems.crcode >= 0 ) PES->CrCode = ems.crcode;

	if ( changesize != 0 ){ // ウィンドウサイズ調整
		PPeWindowSizeFix(GetParent(PES->hWnd), changesize);
	}

	SendMessage(PES->hWnd, EM_SCROLLCARET, 0, 0);
	if ( openmode == PPE_OPEN_MODE_INSERT ){
		XEditSetModify(PES);
	}else{
		XEditClearModify(PES);
	}

	SendMessage(PES->hWnd, WM_SETREDRAW, TRUE, 0);
	InvalidateRect(PES->hWnd, NULL, TRUE);

	SendMessage(PES->hWnd, WM_PPXCOMMAND, K_E_LOAD, 0);

	if ( exec != NULL ) EditExtractMacro(PES, exec, NULL, 0);
	return TRUE;
}


void FileOpen(PPxEDSTRUCT *PES, int mode)
{
	TCHAR name[VFPS], curdir[VFPS];

	name[0] = '\0';
	ThGetString(&PES->LocalStringValue, LSV_filename, name, VFPS);
	VFSFullPath(curdir, T(".."), name);
	if ( tInput(PES->hWnd,
			(mode == 0) ? T("Open Filename") : T("Insert Filename"),
			name, TSIZEOF(name), PPXH_NAME_R, PPXH_PATH) > 0 ){
		VFSFixPath(NULL, name, curdir, VFSFIX_FULLPATH);
		OpenFromFile(PES, mode, name);
	}
}

// 指定行へジャンプ -----------------------------------------------------------
void JumptoLine(HWND hWnd, int line)
{
	DWORD wP, lP;
	int offset;

	SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (WPARAM)&lP);
	offset = SendMessage(hWnd, EM_LINEINDEX, (WPARAM)-1, 0);
	wP -= offset;
	offset = SendMessage(hWnd, EM_LINEINDEX, (WPARAM)line, 0);
	if ( offset != -1 ){
		lP = SendMessage(hWnd, EM_LINELENGTH, (WPARAM)line, 0);
		if ( wP > lP ) wP = lP;
		wP += offset;
		SendMessage(hWnd, EM_SETSEL, wP, wP);
		SendMessage(hWnd, EM_SCROLLCARET, 0, 0);
	}
}

void GetPPePopupPositon(PPxEDSTRUCT *PES, POINT *pos)
{
	if ( PES->mousepos ){
		GetCursorPos(pos);
	}else{
		GetCaretPos(pos);
		pos->y += PES->fontY;
		ClientToScreen(PES->hWnd, pos);
	}
}

HMENU MakePopupMenus(const PPXINMENU *menus, DWORD check)
{
	HMENU hMenu;
	TCHAR strbuf[0x200];

	hMenu = CreatePopupMenu();
	while ( menus->key ){
		if ( menus->str != NULL ){
			const TCHAR *str;

			if ( *menus->str == '%' ){
				PP_ExtractMacro(NULL, NULL, NULL, menus->str, strbuf, 0);
				str = strbuf;
			}else{
				str = MessageText(menus->str);
			}

			AppendMenu(hMenu,
					((menus->key == check) ? MF_CHECKED : 0) |
					((menus->key & INMENU_BREAK) ? MF_ES | MF_MENUBARBREAK : MF_ES),
					menus->key & ~INMENU_BREAK, str);
		}else{
			AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		}
		menus++;
	}
	return hMenu;
}

void PPeMenu(PPxEDSTRUCT *PES, PPECOMMANDPARAM *param, const PPXINMENU *menus)
{
	POINT pos;
	HMENU popup;
	WORD key;

	GetPPePopupPositon(PES, &pos);
	popup = MakePopupMenus(menus, 0);
	if ( menus == amenu ){
		AppendMenu(popup, MF_EPOP, (UINT_PTR)MakePopupMenus(amenu2, 0), amenu2str);
	}
	if ( menus == f2menu ){
		if ( !(PES->style & ES_AUTOHSCROLL) ){
			CheckMenuItem(popup, KE_2p, MF_BYCOMMAND | MF_CHECKED );
		}
		if ( PES->flags & PPXEDIT_SYNTAXCOLOR ){
			CheckMenuItem(popup, K_s | K_c | 'G', MF_BYCOMMAND | MF_CHECKED );
		}
		if ( X_esrx ) CheckMenuItem(popup, KE_2x, MF_BYCOMMAND | MF_CHECKED );
		if ( PES->style & ES_READONLY ){
			CheckMenuItem(popup, KE_qv, MF_BYCOMMAND | MF_CHECKED );
		}
	}
	key = (WORD)TrackPopupMenu(popup, TPM_TDEFAULT, pos.x, pos.y, 0, PES->hWnd, NULL);
	DestroyMenu(popup);
	if ( key != 0 ){
		param->key = (WORD)(key | K_raw);
		PPedCommand(PES, param);
	}
}

int USEFASTCALL PPeGetFileName(PPxEDSTRUCT *PES)
{
	TCHAR buf[VFPS], path[VFPS];
	PPXCMDENUMSTRUCT work;
	HMODULE hCOMDLG32;
	impGetOpenFileName DGetOpenFileName;
	HWND hWnd = PES->hWnd;
	OPENFILENAME ofile = {sizeof(ofile), NULL, NULL, GetFileExtsStr, NULL, 0, 0,
		NULL, VFPS, NULL, 0, NULL, NULL, OFN_HIDEREADONLY | OFN_SHAREAWARE,
		0, 0, WildCard_All, 0, NULL, NULL OPENFILEEXTDEFINE };

	ofile.lpstrTitle = MessageText(MES_TSFN);
	PPxEnumInfoFunc(PES->info, '1', path, &work);
	hCOMDLG32 = LoadSystemDLL(SYSTEMDLL_COMDLG32);
	if ( hCOMDLG32 == NULL ) return 0;
	GETDLLPROCT(hCOMDLG32, GetOpenFileName);
	if ( DGetOpenFileName == NULL ) return 0;
	tstrcpy(buf, NilStr);  // 例えば、d:\\winnt\\*.*
	ofile.hwndOwner = hWnd;
	ofile.lpstrFile = buf;
	ofile.lpstrInitialDir = path;
	if ( !DGetOpenFileName(&ofile) ){
		FreeLibrary(hCOMDLG32);
		return 0;
	}
	FreeLibrary(hCOMDLG32);
	if ( PES->flags & PPXEDIT_SINGLEREF ){
		SendMessage(hWnd, EM_SETSEL, 0, EC_LAST);
	}else{
		DWORD wP, lP;

		SendMessage(hWnd, EM_GETSEL, (WPARAM)&wP, (LPARAM)&lP);
		if ( tstrchr(buf, ' ') != NULL ){ // 空白があるので、「"|"」の|に挿入
			SendMessage(hWnd, EM_REPLACESEL, 0, (LPARAM)T("\"\""));
			SendMessage(hWnd, EM_SETSEL, wP + 1, wP + 1);
			PostMessage(hWnd, WM_KEYDOWN, VK_RIGHT, 0);
			PostMessage(hWnd, WM_KEYUP, VK_RIGHT, 0);
		}
	}
	SendMessage(hWnd, EM_REPLACESEL, 1, (LPARAM)buf);
	return 0;
}
