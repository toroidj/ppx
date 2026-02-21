/*-----------------------------------------------------------------------------
	Paper Plane cUI									各種コマンド - *command
-----------------------------------------------------------------------------*/
#define ONPPXDLL		// PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "PPXVER.H"
#include "VFS.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#include "VFS_STRU.H"
#include "CALC.H"
#include "PPX_64.H"
#include "PPXCMDS.C"
#pragma hdrstop

const TCHAR Str1[] = T("1");

const int KeysShift[] = { VK_SHIFT, VK_LSHIFT, VK_RSHIFT };
const int KeysCtrl[] = { VK_CONTROL, VK_LCONTROL, VK_RCONTROL };
const int KeysAlt[] = { VK_MENU, VK_LMENU, VK_RMENU };

#if defined(WINEGCC) && (WINEGCC >= 500) && (WINEGCC < 900) // Wine5 で GVAR が認識されないので仮に用意
const TCHAR HistTypes[HIST_TYPE_COUNT + 1] = T("gnhdcfsmpveuUxX");
#endif

#define SKEY_BOTH 0
#define SKEY_LEFT 1
#define SKEY_RIGHT 2
#define SKEY_MASK 3
#define SKEY_FIX B8

struct PACKINFO {
	PPXAPPINFO info, *parent;
	TCHAR *path; // 作成する書庫ファイル名
	TCHAR *files; // 書庫に入れるファイル・ディレクトリ(indiv mode)
	DWORD attr; // files の属性(indiv mode)
};


#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((LONG)0x00000000L)
#endif

#ifndef BCRYPT_SHA1_ALGORITHM
#define BCRYPT_MD5_ALGORITHM  L"MD5"
#define BCRYPT_SHA1_ALGORITHM L"SHA1"
#define BCRYPT_OBJECT_LENGTH L"ObjectLength"
#define BCRYPT_HASH_LENGTH L"HashDigestLength"
#endif

#ifndef BCRYPT_ECCPUBLIC_BLOB
#define BCRYPT_ECDSA_P256_ALGORITHM L"ECDSA_P256"
#define BCRYPT_ECCPUBLIC_BLOB L"ECCPUBLICBLOB"
#endif

#define UseHashAlgorithm	BCRYPT_SHA1_ALGORITHM
#define UseKeyAlgorithm		BCRYPT_ECDSA_P256_ALGORITHM

typedef PVOID BCRYPT_HANDLE;
typedef PVOID BCRYPT_ALG_HANDLE;
typedef PVOID BCRYPT_KEY_HANDLE;
typedef PVOID BCRYPT_HASH_HANDLE;

typedef struct {
  LPWSTR pszName;
  ULONG  dwClass;
  ULONG  dwFlags;
} xBCRYPT_ALGORITHM_IDENTIFIER;

DefineWinAPI(LONG, BCryptOpenAlgorithmProvider, (BCRYPT_ALG_HANDLE *, LPCWSTR, LPCWSTR, ULONG));
DefineWinAPI(LONG, BCryptGetProperty, (BCRYPT_HANDLE, LPCWSTR, PUCHAR, ULONG, ULONG *, ULONG));
DefineWinAPI(LONG, BCryptCreateHash, (BCRYPT_ALG_HANDLE, BCRYPT_HASH_HANDLE *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG));
DefineWinAPI(LONG, BCryptHashData, (BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG));
DefineWinAPI(LONG, BCryptFinishHash, (BCRYPT_HASH_HANDLE, PUCHAR, ULONG, ULONG));
DefineWinAPI(LONG, BCryptDestroyHash, (BCRYPT_HASH_HANDLE));
DefineWinAPI(LONG, BCryptCloseAlgorithmProvider, (BCRYPT_ALG_HANDLE, ULONG));
DefineWinAPI(LONG, BCryptImportKeyPair, (BCRYPT_ALG_HANDLE, BCRYPT_KEY_HANDLE, LPCWSTR, BCRYPT_KEY_HANDLE *, PUCHAR, ULONG, ULONG));
DefineWinAPI(LONG, BCryptVerifySignature, (BCRYPT_KEY_HANDLE, VOID *, PUCHAR, ULONG, PUCHAR, ULONG, ULONG));
DefineWinAPI(LONG, BCryptDestroyKey, (BCRYPT_KEY_HANDLE));
DefineWinAPI(LONG, BCryptEnumAlgorithms, (ULONG, ULONG *, xBCRYPT_ALGORITHM_IDENTIFIER **, ULONG));
DefineWinAPI(VOID, BCryptFreeBuffer, (PVOID));

LOADWINAPISTRUCT BCRYPTDLL[] = {
	LOADWINAPI1(BCryptOpenAlgorithmProvider),
	LOADWINAPI1(BCryptGetProperty),
	LOADWINAPI1(BCryptCreateHash),
	LOADWINAPI1(BCryptHashData),
	LOADWINAPI1(BCryptFinishHash),
	LOADWINAPI1(BCryptDestroyHash),
	LOADWINAPI1(BCryptCloseAlgorithmProvider),
	LOADWINAPI1(BCryptImportKeyPair),
	LOADWINAPI1(BCryptVerifySignature),
	LOADWINAPI1(BCryptDestroyKey),
	LOADWINAPI1(BCryptEnumAlgorithms),
	LOADWINAPI1(BCryptFreeBuffer),
	{NULL, NULL}
};

BOOL GetImageHash(BYTE *Image, DWORD ImageSize, BYTE **HashData, DWORD *HashSize, const WCHAR *hashname)
{
	BCRYPT_ALG_HANDLE hAlg;
	BCRYPT_HASH_HANDLE hHash;
	DWORD resultsize;
	DWORD worksize;
	BYTE *hashwork;

	if ( STATUS_SUCCESS != DBCryptOpenAlgorithmProvider(&hAlg, hashname, NULL, 0) ){
		return FALSE;
	}

	DBCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (LPBYTE)&worksize, sizeof(worksize), &resultsize, 0);
	hashwork = (BYTE *)HeapAlloc(ProcHeap, 0, worksize);
	if ( STATUS_SUCCESS != DBCryptCreateHash(hAlg, &hHash, hashwork, worksize, NULL, 0, 0) ){
		HeapFree(ProcHeap, 0, hashwork);
		DBCryptCloseAlgorithmProvider(hAlg, 0);
		return FALSE;
	}
	DBCryptHashData(hHash, Image, ImageSize, 0);
	DBCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (LPBYTE)HashSize, sizeof(DWORD), &resultsize, 0);
	*HashData = (LPBYTE)HeapAlloc(ProcHeap, 0, *HashSize);
	DBCryptFinishHash(hHash, *HashData, *HashSize, 0);

	DBCryptDestroyHash(hHash);
	HeapFree(ProcHeap, 0, hashwork);
	DBCryptCloseAlgorithmProvider(hAlg, 0);
	return TRUE;
}

BYTE DefaultPublicKey[] = {
 0x45, 0x43, 0x53, 0x31, 0x20, 0x00, 0x00, 0x00,
 0x1D, 0xA8, 0x8F, 0xA9, 0xB1, 0xD7, 0x80, 0xAD,
 0x4B, 0x72, 0x1E, 0x76, 0x60, 0xD3, 0x8C, 0xD7,
 0xB1, 0x6A, 0x65, 0x74, 0x72, 0x37, 0x93, 0x06,
 0x43, 0x90, 0x92, 0x8A, 0x1D, 0x4A, 0x76, 0x9D,
 0xA8, 0x09, 0xAB, 0x13, 0x92, 0x54, 0xEA, 0x36,
 0x3C, 0x97, 0x1C, 0xEB, 0x53, 0xA9, 0x7C, 0xD6,
 0xFB, 0x72, 0x84, 0x24, 0xFB, 0x3A, 0x80, 0x86,
 0x6D, 0x1F, 0xFF, 0xC1, 0xB5, 0xD9, 0x84, 0x3F,
};
#define DefaultPublicKeySize 0x48

PPXDLL BOOL PPXAPI GetFileHash(const TCHAR *filename, const TCHAR *hashtype, TCHAR *hashtext)
{
	BOOL result = TRUE;
	BYTE *fileimage = NULL;
	DWORD filesize;
	BYTE *HashData;
	DWORD HashDataSize;
	HANDLE hBCRYPT = LoadWinAPI("BCRYPT.DLL", NULL, BCRYPTDLL, LOADWINAPI_LOAD);
	#ifdef UNICODE
		#define HASHTYPENAME hashtype
	#else
		WCHAR HASHTYPENAME[256];
		strcpyToW(HASHTYPENAME, hashtype, 256);
	#endif

	if ( hBCRYPT == NULL ) return FALSE;

	if ( NO_ERROR != LoadFileImage(filename, 0x1000, (char **)&fileimage, NULL, &filesize) ){
		result = FALSE;
	}else{
		if ( GetImageHash(fileimage, filesize, &HashData, &HashDataSize, HASHTYPENAME) == FALSE ){
			result = FALSE;
		}else{
			int i;

			for ( i = 0; i < (int)HashDataSize; i++ ){
				hashtext = thprintf(hashtext, 4, T("%02x"), HashData[i]);
			}
			HeapFree(ProcHeap, 0, HashData);
		}
		HeapFree(ProcHeap, 0, fileimage);
	}
	FreeLibrary(hBCRYPT);
	return result;
}

PPXDLL int PPXAPI VerifyImage(BYTE *image, DWORD imagesize)
{
	int success = VERIFYZIP_FAILED;

	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_KEY_HANDLE hKey = NULL;

	HANDLE hBCRYPT = LoadWinAPI("BCRYPT.DLL", NULL, BCRYPTDLL, LOADWINAPI_LOAD);

	if ( hBCRYPT == NULL ) return VERIFYZIP_NOSUPPORTVERIFY;

	if ( STATUS_SUCCESS != DBCryptOpenAlgorithmProvider(&hAlg, UseKeyAlgorithm, NULL, 0) ){
		success = VERIFYZIP_NOSUPPORTVERIFY;
	}

	if ( success ){
		if ( STATUS_SUCCESS != DBCryptImportKeyPair(hAlg, NULL, BCRYPT_ECCPUBLIC_BLOB, &hKey, DefaultPublicKey, DefaultPublicKeySize, 0) ){
			success = VERIFYZIP_NOSUPPORTVERIFY;
		}
	}

	if ( success ) for(;;){
		DWORD DataSize;
		DWORD SignatureSize;
		DWORD HashDataSize;
		BYTE *HashData;
		BYTE *Signature;

		success = VERIFYZIP_FAILED;
		if ( imagesize < (DWORD)0x100 ) break;
		SignatureSize = ((*(image + imagesize - 4) - 'a') << 8) + ((*(image + imagesize - 3) - 'a') << 4) + (*(image + imagesize - 2) - 'a');
		if ( SignatureSize >= 0x100 ) break;;

		DataSize = imagesize - SignatureSize;
		if ( *image == 'P' ) DataSize -= sizeof(WORD);
		Signature = image + imagesize - SignatureSize;
		SignatureSize = (SignatureSize - 3 - 1) / 2;

		{
			BYTE *src, *dest;
			DWORD size = SignatureSize;

			src = dest = Signature;
			while ( size ){
				*dest++ = (BYTE)(((*src - 'a') << 4) + (*(src + 1) - 'a'));
				src += 2;
				size--;
			}
		}

		if ( GetImageHash(image, DataSize, &HashData, &HashDataSize, UseHashAlgorithm) == FALSE ){
			break;
		}

		if ( STATUS_SUCCESS == DBCryptVerifySignature(hKey, NULL, HashData, HashDataSize, Signature, SignatureSize, 0) ){
			success = VERIFYZIP_SUCCEDD;
		}
		HeapFree(ProcHeap, 0, HashData);
		break;
	}
	if ( hKey != NULL ) DBCryptDestroyKey(hKey);
	if ( hAlg != NULL ) DBCryptCloseAlgorithmProvider(hAlg, 0);
	FreeLibrary(hBCRYPT);
	return success;
}

int VerifyZipImage(const TCHAR *SignedFileName)
{
	BYTE *fileimage = NULL;
	DWORD filesize;
	int success;

	if ( WinType < WINTYPE_VISTA ) return VERIFYZIP_NOSUPPORTVERIFY;

	if ( NO_ERROR != LoadFileImage(SignedFileName, 0x1000, (char **)&fileimage, NULL, &filesize) ){
		return VERIFYZIP_NOSUPPORTVERIFY;
	}

	success = VerifyImage(fileimage, filesize);
	HeapFree(ProcHeap, 0, fileimage);
	return success;
}

void GetWindowHashList(HMENU hMenuDest, DWORD *PopupID)
{
	HANDLE hBCRYPT = LoadWinAPI("BCRYPT.DLL", NULL, BCRYPTDLL, LOADWINAPI_LOAD);
	ULONG algcount;
	xBCRYPT_ALGORITHM_IDENTIFIER *alglist;
#ifndef UNICODE
	char bufA[256];
	#define AlgName bufA
#else
	#define AlgName alglist[i].pszName
#endif
	if ( hBCRYPT == NULL ) return;

	// BCRYPT_HASH_OPERATION 2
	if ( NO_ERROR == DBCryptEnumAlgorithms(2, &algcount, &alglist, 0) ){
		ULONG i;

		for ( i = 0 ; i < algcount ; i++ ){
			#ifndef UNICODE
				UnicodeToAnsi(alglist[i].pszName, AlgName, 256);
			#endif
			if ( tstrstr(AlgName, T("MAC")) != NULL ) continue;
			AppendMenuString(hMenuDest, (*PopupID)++, AlgName);
		}
		DBCryptFreeBuffer(alglist);
	}
	FreeLibrary(hBCRYPT);
	return;
}

typedef struct {
	PPXAPPINFO info, *parent;
	EXECSTRUCT *Z;
	const TCHAR *cmdname;
	const TCHAR *arg;
} USERCOMMANDSTRUCT;

DWORD_PTR USECDECL UserCommandInfo(USERCOMMANDSTRUCT *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch(cmdID){
		case PPXCMDID_GETEXTRACTVARIABLESTRUCT: {
			DWORD_PTR result;

			result = info->parent->Function(info->parent, PPXCMDID_GETEXTRACTVARIABLESTRUCT, NULL);
			return (result != PPXA_INVALID_FUNCTION) ? result : (DWORD_PTR)&info->Z->StringVariable;
		}

		case PPXCMDID_FUNCTION:
			if ( !tstrcmp(uptr->funcparam.param, T("ARG")) ){
				const TCHAR *ptr;
				int index;

				ptr = uptr->funcparam.optparam;
				if ( !Isdigit(SkipSpace(&ptr)) ){
					ptr = info->arg;
					if ( info->cmdname == NULL ) ptr += tstrlen(ptr) + 1;
					tstrcpy(uptr->funcparam.dest, ptr);
					return PPXA_NO_ERROR;
				}
				index = GetDigitNumber32(&ptr);
				if ( index <= 0 ){
					if ( index == 0 ){ // arg(0) コマンド名
						tstrcpy(uptr->funcparam.dest,
								(info->cmdname != NULL) ? info->cmdname : info->arg);
					}else{
						uptr->funcparam.dest[0] = '\0';
					}
				}else{ // arg(1...)
					ptr = info->arg;
					if ( info->cmdname == NULL ) ptr += tstrlen(ptr) + 1;
					for (;;){
						const TCHAR *oldptr;

						oldptr = ptr;
						GetCommandParameter(&ptr, uptr->funcparam.dest, CMDLINESIZE);
						if ( --index == 0 ){
							size_t len = ptr - oldptr;

							if ( len >= CMDLINESIZE ){
								TCHAR *longbuf;

								len += 1; // Nil分
								longbuf = HeapAlloc(ProcHeap, 0, TSTROFF(len));
								if ( longbuf != NULL ){
									ptr = oldptr;
									GetCommandParameter(&ptr, longbuf, len);
									uptr->funcparam.dest = longbuf;
								}
							}
							break;
						}
						*uptr->funcparam.dest = '\0';
						if ( NextParameter(&ptr) == FALSE ) break;
					}
				}
				return PPXA_NO_ERROR;
			}
			// default へ

		default:
			return info->parent->Function(info->parent, cmdID, uptr);
	}
}

void UserCommand(EXECSTRUCT *Z, const TCHAR *cmdname, const TCHAR *args, const TCHAR *cmdline, TCHAR *dest)
{
	USERCOMMANDSTRUCT ucs;

	ucs.info = *Z->Info;
	ucs.info.Function = (PPXAPPINFOFUNCTION)UserCommandInfo;
	ucs.parent = Z->Info;
	ucs.Z = Z;
	ucs.cmdname = cmdname;
	ucs.arg = args;
	Z->result = PP_ExtractMacro(ucs.info.hWnd, &ucs.info, NULL, cmdline, dest,
			(dest == NULL) ? XEO_EXTRACTEXEC : XEO_EXTRACTEXEC | XEO_EXTRACTLONG);
	if ( Z->result == ERROR_EX_RETURN ) Z->result = NO_ERROR;
}

// *command 処理内で、残った文字列をパラメータとして取得する
// dest == param 可
void GetLineString(const TCHAR *param, TCHAR *dest, size_t destlen)
{
	TCHAR separator, *destptr;
	TCHAR code;
	const TCHAR *parammax;

	separator = SkipSpace(&param);
	if ( (separator == '\"') || (separator == '\'') ){
		param++;
	}else{
		separator = '\0';
	}

	parammax = param + destlen - 1;

	destptr = dest;
	while( param < parammax ){
		code = *param++;
		if ( code == '\0' ) break;
		if ( (separator != '\0') && (code == separator) && (*param == separator) ){
			param++;
		}
		*destptr++ = code;
	}
	while( destptr > dest ){
		code = *(destptr - 1);
		if ( code == separator ){
			destptr--;
			break;
		}
		if ( code != ' ' ) break;
		destptr--;
	}
	*destptr = '\0';
}

void USEFASTCALL ExecOnConsole(EXECSTRUCT *Z, const TCHAR *param, const TCHAR *curdir)
{
	PPXCMD_EXEC ppbe;

	ppbe.name = param;
	ppbe.flag = Z->flags;
	ppbe.path = curdir; // CID_FILE_EXEC - ZSetCurrentDir で取得済み
	Z->ExitCode = PPxInfoFunc32u(Z->Info, PPXCMDID_EXEC, &ppbe);
}

TCHAR * USEFASTCALL ZGetFilePathParam(EXECSTRUCT *Z, const TCHAR **ptr, TCHAR *path)
{
	GetCommandParameter(ptr, path, VFPS);
	if ( *path == '\0' ) return path;
	return VFSFixPath(NULL, path, GetZCurDir(Z), VFSFIX_PATH | VFSFIX_NOFIXEDGE);
}

void ZapMain(EXECSTRUCT *Z, const TCHAR *command, const TCHAR *exepath, const TCHAR *param, const TCHAR *path)
{
	TCHAR msg[VFPS];
	HANDLE hProcess;

	if ( NULL != (hProcess = PPxShellExecute(Z->hWnd, command, exepath,
			param, path, Z->flags, msg)) ){
		if ( Z->flags & XEO_WAITIDLE ){
			WaitForInputIdle(hProcess, 30*60*1000);
		}
		if ( Z->flags & XEO_SEQUENTIAL ){
			if ( WaitJobDialog(Z->hWnd, hProcess, exepath, Z->flags & XEO_WAITQUIET) == FALSE ){
				Z->result = ERROR_CANCELLED;
			}
		}
		if ( Z->flags & (XEO_WAITIDLE | XEO_SEQUENTIAL) ){
			CloseHandle(hProcess);
		}
	}else{
		PopupErrorMessage(Z->hWnd, command, msg);
		Z->result = ERROR_CANCELLED;
	}
}

void ZExtractLog(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR cmdname[128];
	size_t paramlen;

	if ( Z->command < CID_MINID ){
		tstrcpy(cmdname, T("execute"));
	}else if ( Z->command < CID_MAXID ){
		thprintf(cmdname, TSIZEOF(cmdname), T("*%s"), CmdName[Z->command - CID_MINID]);
		Strlwr(cmdname);
	}else{
		thprintf(cmdname, TSIZEOF(cmdname), T("unknown execute %d"), Z->command);
	}
	paramlen = tstrlen(param) + 128 + 2; // 区切り + \0
	if ( paramlen >= 1020 ){
		TCHAR *bufptr, *ptr;

		bufptr = HeapAlloc(DLLheap, 0, TSTROFF(paramlen));
		if ( bufptr != NULL ){
			ptr = tstpcpy(bufptr, cmdname);
			*ptr++ = ' ';
			tstrcpy(ptr, param);
			XMessage(NULL, NULL, XM_DbgLOG, T("%s"), bufptr);
			HeapFree(DLLheap, 0, bufptr);
			return;
		}
	}
	XMessage(NULL, NULL, XM_DbgLOG, T("%s %s"), cmdname, param);
}

void ComZExecPPx(EXECSTRUCT *Z, const TCHAR *ppxname, TCHAR *param)
{
	TCHAR buf[CMDLINESIZE], pathbuf[MAX_PATH];
	const TCHAR *ptr = param, *path;
	DWORD attr;
	int addlen;
	EXEC_EXTRA extra;

	path = GetZCurDir(Z);
	attr = GetFileAttributesL(path);
	if ( ((attr != BADATTR) && !(attr & FILE_ATTRIBUTE_DIRECTORY)) ||
		 !memcmp(path, T("\\\\.\\"), sizeof(TCHAR) * 4) ){
		MakeTempEntry(MAX_PATH, pathbuf, FILE_ATTRIBUTE_DIRECTORY);
		path = pathbuf;
	}
	if ( (GetOptionParameter(&ptr, buf, NULL) == '-') && !tstrcmp(buf + 1, T("RUNAS")) ){
		CatPath(buf, DLLpath, ppxname);
		if ( PPxShellExecute(Z->hWnd, T("RUNAS"), buf, ptr, path, 0, pathbuf) == NULL ){
			PopupErrorMessage(Z->hWnd, buf, pathbuf);
			Z->result = ERROR_CANCELLED;
		}
		return;
	}

	// param の前に PPx の実行パスを記載
	addlen = thprintf(buf, TSIZEOF(buf), T("\"%s\\%s\" "), DLLpath, ppxname) - buf;
	if ( (param >= Z->DstBuf) && (param < (Z->DstBuf + TSIZEOF(Z->DstBuf))) ){
		// DstBuf は CMDLINESIZE * 2 なので、追記する余裕あり
	}else{
		if ( ThSize(&Z->ExtendDst, TSTROFF(tstrlen(param) + (param - (const TCHAR *)Z->ExtendDst.bottom) - Z->ExtendDst.top + addlen + 4) ) == FALSE ){
			Z->result = RPC_S_STRING_TOO_LONG;
			return;
		}
		param = (TCHAR *)Z->ExtendDst.bottom; // バッファの位置が変わった時用
	}
	tstrgap(param, addlen);
	memcpy(param, buf, TSTROFF(addlen));

	extra.mask = 0;
	extra.ExitCode = &Z->ExitCode;
	ComExecSelf(Z->hWnd, param, path, Z->flags, &extra);
}

int USECDECL SureDialog(const TCHAR *message)
{
	return PMessageBox(NULL, message, NilStr, MB_YESNO) == IDYES;
}

void CustCmdSub(EXECSTRUCT *Z, TCHAR *text, TCHAR *textmax, BOOL reload)
{
	TCHAR *log = NULL;
	int result;

	result = PPcustCStore(text, textmax, PPXCUSTMODE_APPEND, &log, SureDialog);
	if ( log ){
		if ( reload ){
			TCHAR *logp = log;

			for ( ;; logp++ ){
				TCHAR c;

				c = *logp;
				if ( (c == ' ') || (c == '\r') || (c == '\n') ) continue;
				break;
			}
			PPxInfoFunc(Z->Info, PPXCMDID_SETPOPLINE, (void *)logp);
		}
		HeapFree(ProcHeap, 0, log);
	}
	if ( reload && ((result & (PPCUSTRESULT_RELOAD | 1)) == 1) ){
		PPxPostMessage(WM_PPXCOMMAND, K_Lcust, GetTickCount());
	}
}

void CustFile(EXECSTRUCT *Z, const TCHAR *filename, BOOL reload)
{
	TCHAR *mem, *text, *textmax;		// カスタマイズ解析位置

	PPxSendMessage(WM_PPXCOMMAND, K_Scust, 0);
										// ファイル読み込み処理 ---------------
	if ( LoadTextData(filename, &mem, &text, &textmax, 0) != NO_ERROR ){
		ErrorPathBox(Z->hWnd, NULL, filename, PPERROR_GETLASTERROR);
		return;
	}
	CustCmdSub(Z, text, textmax, reload);
	HeapFree(ProcHeap, 0, mem);
}

void CustLine(EXECSTRUCT *Z, const TCHAR *line, BOOL reload)
{
	#define MAXLEN_WSPRINTF (1024 - 1) // Win95等は1023, WinNT等は1024
	TCHAR buf[MAXLEN_WSPRINTF + 5]; // wsprintf の最大サイズ(1024) と '\0' 分
	TCHAR *param, *param2, *subkey, separator;
	int len;

	PPxSendMessage(WM_PPXCOMMAND, K_Scust, 0);
	param = tstrchr(line, '=');
	param2 = tstrchr(line, ',');
	if ( param == NULL ){
		if ( param2 == NULL ) return;
		param = param2;
	}else{
		if ( (param2 != NULL) && (param > param2) ) param = param2;
	}
	separator = *param;
	*param++ = '\0';

	subkey = tstrchr(line, ':');
	if ( subkey != NULL ){
		*subkey = '\0';
		len = wsprintf(buf, T("%s = {\n%s %c%s\n}"), line, subkey + 1, separator, param);
	}else{
		len = wsprintf(buf, T("%s %c%s"), line, separator, param);
	}
	if ( len < MAXLEN_WSPRINTF ){
		CustCmdSub(Z, buf, buf + tstrlen(buf), reload);
	}else{ // wsprintf では無理だった
		ThSTRUCT th;

		ThInit(&th);
		if ( subkey != NULL ){
			thprintf(&th, 0, T("%s = {\n%s %c%s\n}"), line, subkey + 1, separator, param);
		}else{
			thprintf(&th, 0, T("%s %c%s"), line, separator, param);
		}
		CustCmdSub(Z, (TCHAR *)th.bottom, (TCHAR *)(char *)(th.bottom + th.top), reload);
		ThFree(&th);
	}
}

BOOL GetSetParams(TCHAR **name, TCHAR **param)
{
	UTCHAR *p;
														// 指定無し
	if ( (UTCHAR)SkipSpace((const TCHAR **)name) < (UTCHAR)' ' ) return FALSE;
	p = (UTCHAR *)*name;
	for ( ; ; ){
		if ( *p <= (UTCHAR)' ' ){
			*p++ = '\0';
			if ( SkipSpace((const TCHAR **)&p) == '=' ){
				p++;
			}
			break;
		}
		if ( *p == '=' ){
			*p++ = '\0';
			break;
		}
		p++;
	}
	if ( (UTCHAR)SkipSpace((const TCHAR **)&p) < (UTCHAR)' ' ){
		*param = NULL;
	}else{
		*param = (TCHAR *)p;
	}
	return TRUE;
}

void ZSetCurrentDir(EXECSTRUCT *Z, TCHAR *olddir)
{
	GetCurrentDirectory(VFPS, olddir);
	SetCurrentDirectory(GetZCurDir(Z));
}

//-----------------------------------------------------------------------------
int USEFASTCALL SaveShiftKeys(const int *keysID, int requirekey)
{
	int oldkey, type = SKEY_BOTH;

	oldkey = GetAsyncKeyState(keysID[SKEY_BOTH]) & KEYSTATE_PUSH;
	if ( oldkey ^ requirekey ){ // シフト状態が異なる
		If_WinNT_Block {
			if ( (GetAsyncKeyState(keysID[SKEY_LEFT]) & KEYSTATE_PUSH) ^ requirekey ){
				type = SKEY_LEFT;
			}else if ( (GetAsyncKeyState(keysID[SKEY_RIGHT]) & KEYSTATE_PUSH) ^ requirekey ){
				type = SKEY_RIGHT;
			}
		}
		keybd_event((BYTE)keysID[type], 0, requirekey ? 0 : KEYEVENTF_KEYUP, 0);
		oldkey |= type | SKEY_FIX;
	}
	return oldkey;
}

void USEFASTCALL RestoreShiftKeys(const int *keysID, int oldkey)
{
	if ( oldkey & SKEY_FIX ){ // シフト状態が異なる
		// 右シフトキーはうまく戻せないので省略する
		if ( (oldkey & (KEYSTATE_PUSH | SKEY_MASK)) == (KEYSTATE_PUSH | SKEY_RIGHT) ){
			return;
		}
		keybd_event((BYTE)keysID[oldkey & SKEY_MASK], 0, (oldkey & KEYSTATE_PUSH) ? 0 : KEYEVENTF_KEYUP, 0);
	}
}
// キーエミュレート
void CmdEmurateKeyInput(EXECSTRUCT *Z, const TCHAR *ptr) // %k, *sendkey
{
	int oldctrl, oldalt, oldshift;
	const TCHAR *p;

	if ( SkipSpace(&ptr) != '\"' ){
		XMessage(Z->hWnd, T("%K"), XM_GrERRld, MES_EPRM);
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}
	p = ptr + 1;
	for ( ; ; ){
		UTCHAR c;
		BOOL winshift;
		int key, exflag;

		c = SkipSpace(&p);
		if ( c < ' ' ) break;
		if ( c == '\"' ){
			p++;
			break;
		}
		if ( SkipSpace(&p) == '`' ){
			p++;
			winshift = TRUE;
		}else{
			winshift = FALSE;
		}
		key = GetKeyCode(&p);
		if ( key < 0 ){
			XMessage(Z->hWnd, T("%K"), XM_GrERRld, MES_EPRM);
			Z->result = ERROR_INVALID_PARAMETER;
			break;
		}
		if ( key == K_NULL ){
			Sleep(100);
			continue;
		}
		if ( !(key & K_v) && !IsalnumA(key) ){
			int vkey;

			vkey = (int)VkKeyScan((TCHAR)(key & 0xff));
			key = (vkey & 0xff) | (key & 0xff00);
			if ( vkey & B8 ) setflag(key, K_s);
		}
		oldshift = SaveShiftKeys(KeysShift, (key & K_s) << 5);
		oldctrl  = SaveShiftKeys(KeysCtrl , (key & K_c) << 4);
		oldalt   = SaveShiftKeys(KeysAlt  , (key & K_a) << 3);
		if ( IsTrue(winshift) ) keybd_event(VK_RWIN, 0, 0, 0);

		exflag = 0;
		if ( key & K_v ){
			int vkey = key & 0xff;

			if ( ((vkey >= VK_PRIOR) && (vkey <= VK_DOWN)) ||
				 ((vkey >= VK_MULTIPLY) && (vkey <= VK_DIVIDE)) ){
				exflag = KEYEVENTF_EXTENDEDKEY;
			}
		}
		keybd_event((BYTE)key, 0, exflag, 0);
		keybd_event((BYTE)key, 0, exflag | KEYEVENTF_KEYUP, 0);

		if ( IsTrue(winshift) ) keybd_event(VK_RWIN, 0, KEYEVENTF_KEYUP, 0);
		RestoreShiftKeys(KeysAlt  , oldalt);
		RestoreShiftKeys(KeysCtrl , oldctrl);
		RestoreShiftKeys(KeysShift, oldshift);
	}
}

void BreakAction(EXECSTRUCT *Z)
{
	MSG msg;

	Z->result = ERROR_CANCELLED;
	// 先行入力を除去
	while ( PeekMessage(&msg, NULL, WM_KEYFIRST, 0x10f, PM_REMOVE) );
	while ( PeekMessage(&msg, NULL, WM_MOUSEFIRST, 0x20f, PM_REMOVE) );
	PPxInfoFunc(Z->Info, PPXCMDID_SETPOPLINE, (void *)MessageText(MES_BRAK));
}

BOOL WaitBreakCheck(EXECSTRUCT *Z)
{
	MSG msg;

	while ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ){
		if ( msg.message == WM_QUIT ) break;

		if ( (msg.message == WM_KEYDOWN) &&
			 ((int)msg.wParam == VK_PAUSE) ){
			BreakAction(Z);
			return TRUE;
		}
		if ( DialogKeyProc(&msg) ) continue;
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return FALSE;
}

#pragma argsused
VOID CALLBACK WaitCoundDown(HWND hWnd, UINT msg, UINT_PTR id, DWORD time)
{
	int count;
	TCHAR buf[64];

	UnUsedParam(msg);UnUsedParam(id);UnUsedParam(time);

	count = ((WAITDLGSTRUCT *)GetWindowLongPtr(hWnd, DWLP_USER))->count / 1000;
	if ( count >= 60 ){
		thprintf(buf, TSIZEOF(buf), T("%d:%02d"), count / 60, count % 60);
	}else{
		thprintf(buf, TSIZEOF(buf), T("%d s"), count);
	}
	SetDlgItemText(hWnd, IDS_MSGBOX_TEXT, buf);
}

#define WAIT_STEP_TIME 100
void CmdWaitExtract(EXECSTRUCT *Z, const TCHAR *param) // *wait
{
	int sleepmode = 0;
	TCHAR *more;
	DWORD pid = 0;
	const TCHAR *ptr = param;
	TCHAR buf[VFPS];
	HWND hDlg = NULL;
	WAITDLGSTRUCT wds;
	BOOL PreventSleep = FALSE;

	wds.choose = WDS_UNCHOOSE;
	wds.count = 0;
	for (;; ){
		if ( GetOptionParameter(&ptr, buf, &more) != '-' ){ // 待機時間
			int timevalue;
			if ( buf[0] == '\0' ) break;
			more = buf;
			timevalue = GetIntNumber((const TCHAR **)&more);
			if ( (*more > ' ') && (*more != ',') ){
				for (;;){
					if ( *more == 's' ){ // 秒単位
						timevalue *= 1000;
						more++;
					}else if ( *more == 'm' ){ // 分単位
						timevalue *= 60 * 1000;
						more++;
					}else if ( *more == 'h' ){ // 時間単位
						timevalue *= 60 * 60 * 1000;
						more++;
					}
					if ( !Isdigit(*more) ) break;
					wds.count += timevalue;
					timevalue = GetIntNumber((const TCHAR **)&more);
				}
			}
			wds.count += timevalue;

			if ( IsTrue(NextParameter((const TCHAR **)&more)) ){
				sleepmode = GetIntNumber((const TCHAR **)&more);
			}else if ( IsTrue(NextParameter((const TCHAR **)&ptr)) && Isdigit(*ptr) ){
				sleepmode = GetIntNumber(&ptr);
			}
		}else if ( tstrcmp(buf + 1, T("PREVENTSLEEP")) == 0 ){
			PreventSleep = TRUE;
			StartPreventSleep(Z->hWnd, FALSE);
		}else if ( tstrcmp(buf + 1, T("DIALOG")) == 0 ){
			wds.md.title = T("wait dialog");
			hDlg = CreateDialogParam(DLLhInst, MAKEINTRESOURCE(IDD_NULLMIN), NULL, WaitDlgBox, (LPARAM)&wds);
			MoveCenterWindow(hDlg, Z->hWnd);
		}else if ( (tstrcmp(buf + 1, T("RUN")) == 0) || (tstrcmp(buf + 1, T("PID")) == 0) ){
			if ( *more == '\0' ){
				ThGetString(&Z->StringVariable, T("PID"), buf, TSIZEOF(buf));
				more = buf;
			}
			pid = GetIntNumber((const TCHAR **)&more);
			if ( pid == 0 ){
				XMessage(Z->hWnd, T("*wait -run / -pid"), XM_GrERRld, MES_ENFT);
				goto end;
			}
		}else{
			XMessage(Z->hWnd, NULL, XM_GrERRld, MES_EUOP, buf);
			Z->result = ERROR_INVALID_PARAMETER;
			goto end;
		}
	}

	if ( (hDlg != NULL) && (wds.count > 0) ){
		SetTimer(hDlg, TIMERID_MSGBOX_TIMER, TIME_MSGBOX_TIMER, WaitCoundDown);
	}

	if ( pid != 0 ){ // pid か run
		HANDLE hProcess;
		DWORD starttick;
		ERRORCODE ecode;

		starttick = GetTickCount();
		Z->ExitCode = (DWORD)-1;

		for (;;){
			if ( (wds.count != 0) && ( (GetTickCount() - starttick) > (DWORD)wds.count) ){
				goto end; // タイムアウト
			}

			hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
			if ( hProcess != NULL ){
				for (;;){
					DWORD result;

					result = WaitForSingleObject(hProcess, WAIT_STEP_TIME);
					if ( result != WAIT_TIMEOUT ) break;
					if ( (wds.count != 0) && ( (GetTickCount() - starttick) > (DWORD)wds.count) ){
						goto end; // タイムアウト
					}

					if ( hDlg != NULL ){
						if ( PeekDialogMessageLoop(NULL, hDlg) == FALSE ) break;
						if ( IsChooseWDS(wds.choose) ){
							if ( IsTrue(wds.choose) ) break; // Ok→続行
							Z->result = ERROR_CANCELLED;
							goto end; // cancel →中止
						}
					}
					if ( sleepmode && IsTrue(WaitBreakCheck(Z)) ) break;
				}
				GetExitCodeProcess(hProcess, &Z->ExitCode);
				CloseHandle(hProcess);
			}else{
				ecode = GetLastError();
				if ( ecode != ERROR_ACCESS_DENIED ){
					// 通常 ERROR_INVALID_PARAMETER (87)
					goto end;
				}
				// ERROR_ACCESS_DENIED は、権限等の関係で開けなかったとき→プロセス有り
			}
			// プロセスが生きているので待機する
			if ( hDlg != NULL ){
				if ( PeekDialogMessageLoop(NULL, hDlg) == FALSE ) break;
				if ( IsChooseWDS(wds.choose) ){
					if ( IsTrue(wds.choose) ) break; // Ok→続行
					Z->result = ERROR_CANCELLED;
					goto end; // cancel →中止
				}
			}else{
				Sleep(WAIT_STEP_TIME);
			}
			if ( sleepmode && IsTrue(WaitBreakCheck(Z)) ) break;
		}
	}

	if ( hDlg != NULL ){
		while( wds.count >= 0 ){
			DWORD tick, result;

			tick = GetTickCount();
			result = MsgWaitForMultipleObjects(0, NULL, TRUE, wds.count, QS_ALLEVENTS);
			tick = GetTickCount() - tick;
			if ( tick > (DWORD)wds.count ) break;
			wds.count -= tick;
			if ( result == WAIT_OBJECT_0 ){
				if ( PeekDialogMessageLoop(NULL, hDlg) == FALSE ) break;
				if ( IsChooseWDS(wds.choose) ){
					if ( IsTrue(wds.choose) ) break; // Ok→続行
					Z->result = ERROR_CANCELLED;
					goto end; // cancel →中止
				}
			}
		}
	}else{
		if ( sleepmode ){
			while( wds.count >= 0 ){
				Sleep( (sleepmode == 1) || (wds.count <= WAIT_STEP_TIME) ?
						wds.count : WAIT_STEP_TIME );
				if ( IsTrue(WaitBreakCheck(Z)) ) break;
				if ( sleepmode == 1 ) goto end;
				wds.count -= WAIT_STEP_TIME;
			}
			goto end;
		}
		// sleepmode == 0
		GetAsyncKeyState(VK_PAUSE); // 読み捨て(最下位bit対策)
		Sleep(wds.count);
	}
	if ( GetAsyncKeyState(VK_PAUSE) & KEYSTATE_FULLPUSH ) BreakAction(Z);
end:
	if ( PreventSleep ) EndPreventSleep(Z->hWnd);
	if ( hDlg != NULL ){
		KillTimer(hDlg, TIMERID_MSGBOX_TIMER);
		DestroyWindow(hDlg);
	}
}

void CmdShowTip(EXECSTRUCT *Z, const TCHAR *param)	// *tip
{
	TCHAR text[CMDLINESIZE];
	TOOLINFO ti;

	GetLineString(param, text, TSIZEOF(text));

	LoadCommonControls(ICC_TAB_CLASSES);
	if ( hTipWnd != NULL ){
		PostMessage(hTipWnd, WM_CLOSE, 0, 0);
		hTipWnd = NULL;
	}
	if ( text[0] == '\0' ) return;

	hTipWnd = CreateWindow(TOOLTIPS_CLASS, NULL, TTS_ALWAYSTIP | TTS_NOPREFIX,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			Z->hWnd, NULL, DLLhInst, NULL);

	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
	ti.hwnd = Z->hWnd;
	ti.hinst = DLLhInst;
	ti.uId = (UINT_PTR)Z->hWnd;
	ti.lpszText = text;
	ti.rect.left = 0;
	ti.rect.top = 0;
	ti.rect.right = 10000;
	ti.rect.bottom = 10000;
	SendMessage(hTipWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
}

void CmdDoForfile(EXECSTRUCT *Z, TCHAR *param, const TCHAR *cmdline)
{
	TCHAR mask[VFPS], cmd[CMDLINESIZE], dir[VFPS], *p;
	HANDLE hFF;
	WIN32_FIND_DATA ff;
	FN_REGEXP fn;

	p = VFSFindLastEntry(param);
	tstrcpy(mask, ((*p == '\\') || (*p == '/')) ? p + 1 : p);
	MakeFN_REGEXP(&fn, mask);

	*p = '\0';
	VFSFullPath(NULL, param, GetZCurDir(Z));
	CatPath(dir, param, WildCard_All);

	tstrcpy(cmd, cmdline);
	p = cmd + tstrlen(cmd);

	hFF = VFSFindFirst(dir, &ff);
	if ( hFF == INVALID_HANDLE_VALUE ) return;
	do{
		if ( IsRelativeDirectory(ff.cFileName) ) continue;
		if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
		if ( FinddataRegularExpression(&ff, &fn) ){
			VFSFullPath(p, ff.cFileName, param);
			PP_ExtractMacro(Z->hWnd, Z->Info, NULL, cmd, NULL, 0);
		}

	}while( IsTrue(VFSFindNext(hFF, &ff)) );
	VFSFindClose(hFF);
	FreeFN_REGEXP(&fn);
	return;
}

BOOL CheckAndMinimize(EXECSTRUCT *Z, BOOL toggle, HWND hTargetWnd)
{
	HWND hForegroundWnd;

	if ( IsTrue(toggle) ){
		// ↓PPtray は ForegroundWnd を別途取得する
		hForegroundWnd = (HWND)PPxInfoFunc(Z->Info, PPXCMDID_GETFGWND, NULL);
		if ( hForegroundWnd == NULL ){
			hForegroundWnd = GetForegroundWindow();
		}
		if ( hForegroundWnd == hTargetWnd ){
			ShowWindow(hTargetWnd, SW_MINIMIZE);
			return TRUE;
		}
	}
	return FALSE;
}

void PPcSetForeground(HWND hParentWnd, HWND hTargetWnd, BOOL combo)
{
	if ( GetWindowLongPtr(hTargetWnd, GWL_STYLE) & WS_POPUP ){
		ForceSetForegroundWindow(hTargetWnd);
		return;
	}

	if ( combo ?
			((hParentWnd != NULL) && (GetParent(hTargetWnd) != NULL)) :
			(hParentWnd != hTargetWnd) ){
		if ( combo ) ForceSetForegroundWindow(hParentWnd);
		hParentWnd = GetParent(hTargetWnd);
		if ( (hParentWnd != NULL) && (SENDCOMBO_OK != SendMessage(hParentWnd, WM_PPXCOMMAND, KCW_setforeground, (LPARAM)hTargetWnd)) ){
			// ↑で失敗した時は hTargetWnd のフォーカスセットをする
			if ( GetForegroundWindow() != hParentWnd ) return;
		}
	}

	ForceSetForegroundWindow(hTargetWnd);
	SetFocus(hTargetWnd); //一体化対策
	WindowZPosition(hTargetWnd, HWND_TOP);
}

void CmdFocusPPx(EXECSTRUCT *Z, const TCHAR *paramptr) // *focus
{
	TCHAR param[VFPS];
	BOOL toggle = FALSE;
	HWND hPPcWnd;

	if ( SkipSpace(&paramptr) == '!' ){
		toggle = TRUE;
		paramptr++;
	}
	GetCommandParameter(&paramptr, param, TSIZEOF(param));
	// パラメータ指定があり、"C" 以外→通常／PPxウィンドウが対象
	if ( (param[0] != '\0') && !((param[0] == 'C') && (param[1] == '\0')) ){
		HWND hTargetWnd;
		EXEC_EXTRA extra;

		hTargetWnd = GetWindowHandleByText(Z->Info, param);
		if ( hTargetWnd != NULL ){
			HWND hParentWnd, hWnd;

			if ( IsTrue(CheckAndMinimize(Z, toggle, hTargetWnd)) ) return;
			hParentWnd = GetParent(hTargetWnd);
			if ( hParentWnd != NULL ){
				int index;

				while ( (hWnd = GetParent(hParentWnd)) != NULL ){
					hParentWnd = hWnd;
				}
				for ( index = 0; index < X_MaxComboID; index++ ){
					if ( Sm->ppc.hComboWnd[index] == hParentWnd ){
						PPcSetForeground(Sm->ppc.hComboWnd[index], hTargetWnd, TRUE);
						return;
					}
				}
			}
			PPcSetForeground(hParentWnd, hTargetWnd, FALSE);
			return;
		}
		if ( NextParameter(&paramptr) == FALSE ) return;
		extra.mask = 0;
		extra.ExitCode = &Z->ExitCode;
		if ( ComExecEx(Z->hWnd, paramptr, GetZCurDir(Z), &Z->useppb, Z->flags, &extra) == FALSE ){
			Z->result = ERROR_PATH_NOT_FOUND;
		}
		return;
	}
	// パラメータ指定無し or 「C」→アクティブ PPc が対象
	hPPcWnd = PPcGetWindow(0, CGETW_GETFOCUS);
	if ( hPPcWnd == NULL ){
		ComZExecPPx(Z, PPcExeName, NilStrNC); // PPc 起動
	}else{
		HWND hParentWnd;

		hParentWnd = GetParent(hPPcWnd);
		if ( hParentWnd == NULL ) hParentWnd = hPPcWnd;
		if ( IsTrue(CheckAndMinimize(Z, toggle, hParentWnd)) ) return;
		PPcSetForeground(hParentWnd, hPPcWnd, FALSE);
	}
}

void CmdSelectPPx(EXECSTRUCT *Z, const TCHAR *param)
{
	HWND hTargetWnd;
	DWORD menuid = 1;
	TCHAR buf[VFPS + 16];
	int comboID = -1;
									// 指定あり？
	SkipSpace(&param);
	hTargetWnd = GetPPxhWndFromID(Z->Info, &param, NULL);
	if ( hTargetWnd == BADHWND ){
		Z->result = ERROR_INVALID_PARAMETER;
		return; // 該当無し
	}
	if ( hTargetWnd == NULL ){ // 指定無し...メニュー表示
		HMENU hPopupMenu;
		MENUITEMINFO minfo;

		hPopupMenu = CreatePopupMenu();
		if ( GetPPxList(hPopupMenu, GetPPxList_hWnd, NULL, &menuid) ){
			int index;

			index = TTrackPopupMenu(Z, hPopupMenu, NULL);
			if ( index > 0 ){
				minfo.cbSize = sizeof(minfo);
				minfo.fMask = (WinType < WINTYPE_2000) ?
						(MIIM_STATE | MIIM_TYPE | MIIM_ID | MIIM_DATA) :
						(MIIM_STATE | MIIM_FTYPE | MIIM_STRING | MIIM_ID | MIIM_DATA);
				minfo.cch = VFPS + 8;
				minfo.dwTypeData = buf;
				if ( GetMenuItemInfo(hPopupMenu, index, MF_BYCOMMAND, &minfo) == FALSE ){
					Z->result = ERROR_CANCELLED;
				}else{
					hTargetWnd = (HWND)minfo.dwItemData;
					if ( hTargetWnd == NULL ){ // 一体化PPc→hwndを問い合わせて取得する
						TCHAR *idptr, *sep;
						idptr = tstrchr(buf, '&');
						if ( idptr != NULL ){
							*idptr = 'C';
							comboID = idptr[2]; // CZx
							sep = tstrchr(idptr, ':');
							if ( sep != NULL ) *sep = '\0';
							hTargetWnd = (HWND)PPxInfoFunc(Z->Info, PPXCMDID_COMBOGETHWNDEX, idptr);
							if ( hTargetWnd == NULL ){ // 別プロセスPPc
								hTargetWnd = GetPPxhWndFromID(Z->Info, (const TCHAR **)&idptr, NULL);
							}
						}
					}
				}
			}else{
				Z->result = ERROR_CANCELLED;
			}
		}
		DestroyMenu(hPopupMenu);
	}
	if ( (hTargetWnd != NULL) && (hTargetWnd != BADHWND) ){
		HWND hParentWnd;
		if ( (comboID >= 'a') && (comboID <= 'z') &&
			 (Sm->ppc.hComboWnd[comboID - 'a'] != NULL) ){
			hParentWnd = Sm->ppc.hComboWnd[comboID - 'a'];
		}else{
			comboID = -1;
			hParentWnd = hTargetWnd;
		}
		PPcSetForeground(hParentWnd, hTargetWnd, (comboID >= 0) );
	}
}

BOOL CmdPack_Edit(EXECSTRUCT *Z, struct PACKINFO *pinfo, ThSTRUCT *StringValue, TCHAR *packname, TCHAR *packcmd)
{
	TINPUT tinput;

	if ( packcmd[0] == '\0' ){
		tstrcpy(packname, StrPackZipFolderTitle);
		tstrcpy(packcmd, StrPackZipFolderCommand);
	}

	ThSetString(StringValue, T("Edit_PackName"), packname);

	if ( *pinfo->path == '!' ){
		pinfo->path++;
		return TRUE;
	}

	ThSetString(StringValue, T("Edit_PackCmd"), packcmd);

	tinput.title	= packname;
	tinput.buff		= pinfo->path;
	tinput.size		= VFPS;
	tinput.flag		= TIEX_USEINFO | TIEX_USEOPTBTN | TIEX_INSTRSEL | TIEX_USESELECT;
	tinput.firstC	= EC_LAST;
	tinput.lastC	= EC_LAST;

	if ( !(Z->status & ST_EDITHIST) ){
		tinput.hWtype = PPXH_FILENAME;
		tinput.hRtype = PPXH_GENERAL | PPXH_FILENAME | PPXH_PATH | PPXH_PPCPATH | PPXH_PPVNAME;
		setflag(tinput.flag, TIEX_Z_HIST_SETTINGED);
	}

	if ( ZTinput(Z, &tinput) == FALSE ){
		Z->result = ERROR_CANCELLED;
		return FALSE;
	}

	ThGetString(StringValue, T("Edit_PackCmd"), packcmd, CMDLINESIZE);
	return TRUE;
}

void CmdClosePPx(const TCHAR *param) // *closeppx
{
	TCHAR text[MAX_PATH], cmd[MAX_PATH + 32];
	int index;
	HWND hWnd;
	FN_REGEXP fn;
	const TCHAR *id;

	GetCommandParameter(&param, text, TSIZEOF(text));
	if ( text[0] == '\0' ){
		PPxSendMessage(WM_CLOSE, 0, 0);
		return;
	}
	tstrreplace(text, T("_"), NilStr); // _ を除去
	if ( MakeFN_REGEXP(&fn, text) & REGEXPF_ERROR ) return;

	FixTask();
	if ( tstrchr(text, '#') == NULL ){ // 独立窓
		for ( index = 0 ; index < X_MaxComboID ; index++ ){ // 一体化窓上のを X
			HWND hComboWnd;
			COPYDATASTRUCT copydata;

			hComboWnd = Sm->ppc.hComboWnd[index];
			if ( (hComboWnd == NULL) || (hComboWnd == BADHWND) ) continue;

			thprintf(cmd, TSIZEOF(cmd), T("*idclose \"%s\""), text);
			copydata.dwData = 'H';
			copydata.cbData = TSTRSIZE32(cmd);
			copydata.lpData = (PVOID)cmd;
			SendMessage(hComboWnd, WM_COPYDATA, 0, (LPARAM)&copydata);
		}

		for ( index = 0 ; index < X_Mtask ; index++ ){ // 独立窓を閉じる
			if ( Sm->P[index].ID[0] == '\0' ) continue;
			hWnd = Sm->P[index].hWnd;
			if ( (hWnd == NULL) ||
				 (((LONG_PTR)hWnd & (LONG_PTR)HWND_BROADCAST) == (LONG_PTR)HWND_BROADCAST) ){
				continue;
			}
			id = Sm->P[index].ID;
			if ( id[1] == '_' ){
				text[0] = id[0];
				text[1] = id[2];
				text[2] = '\0';
				if ( (text[0] == 'C') && (text[1] == 'Z') &&
					 (GetParent(hWnd) != NULL) ){
					continue; // 一体化窓上のC_Z はここで処理しない(CZxx 回避)
				}
				if ( FilenameRegularExpression(text, &fn) ){
					ClosePPxOne(hWnd, FALSE);
				}
			}else if ( FilenameRegularExpression(id, &fn) ){
				ClosePPxOne(hWnd, FALSE);
			}
		}
	}else{ // 一体化窓
		text[0] = 'C';
		text[1] = 'B';
		text[3] = '#';
		text[4] = '\0';

		for ( index = 0 ; index < X_MaxComboID ; index++ ){
			HWND hComboWnd;

			hComboWnd = Sm->ppc.hComboWnd[index];
			if ( (hComboWnd == NULL) || (hComboWnd == BADHWND) ) continue;
			text[2] = (TCHAR)('A' + index);
			if ( FilenameRegularExpression(text, &fn) ){
				PostMessage(hComboWnd, WM_CLOSE, 0, 0);
			}
		}
	}
	FreeFN_REGEXP(&fn);
}

void CmdHttpGet(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR uri[CMDLINESIZE], name[VFPS];
	ThSTRUCT th;
	const char *bottom, *body;
	DWORD size;
	HANDLE hFile;
	int inheader = 0;

	GetCommandParameter(&param, uri, TSIZEOF(uri));
	if ( (*uri == '/') && ((*(uri+1) == 'h') || (*(uri+1) == 'H')) ){
		inheader = 1;
		NextParameter(&param);
		GetCommandParameter(&param, uri, TSIZEOF(uri));
	}
	if ( NextParameter(&param) == FALSE ){
		XMessage(Z->hWnd, T("*httpget"), XM_GrERRld, MES_EPRM);
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}
	ZGetFilePathParam(Z, &param, name);
								// メモリ上に取得 --------------------
	GetImageByHttp(uri, &th);
	bottom = (char *)th.bottom;
	if ( bottom == NULL ){
		Z->result = ERROR_FILE_NOT_FOUND;
		ErrorPathBox(Z->hWnd, T("*httpget"), uri, Z->result);
		return;
	}
	size = th.top - 1;
	body = strstr(bottom, "\r\n\r\n");
	if ( !inheader && (body != NULL) ){
		size -= body - bottom + 4;
		bottom = body + 4;
	}
								// 書き込み ---------------------------
	hFile = CreateFileL(name, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile != INVALID_HANDLE_VALUE ){
		WriteFile(hFile, bottom, size, &size, NULL);
		CloseHandle(hFile);
	}else{
		Z->result = ErrorPathBox(Z->hWnd, T("*httpget"), name, PPERROR_GETLASTERROR);
	}
	ThFree(&th);
}

/* ●(UNICODE/64bit版)書庫内文字コードを指定するコマンド(*setarchivecp)
  現在、PPcによる書庫内の一覧表示にしか対応していないため、隠し機能扱い。
  例) *setarchivecp 65001 // codepage をUTF8
  他) 932 SJIS, 20932 EUC-JP
*/
#ifdef UNICODE
void CmdSetArchiveCP(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR buf[0x40];

	UnDllCodepage = GetDigitNumber32(&param);
	if ( UnDllCodepage >= 0xffff ) UnDllCodepage = CP_ACP;
	if ( UnDllCodepage == CP_ACP ){
		tstrcpy(buf, T("Codepage:default"));
	}else{
		thprintf(buf, TSIZEOF(buf), T("Codepage:%d"), UnDllCodepage);
	}
	SendMessage(Z->hWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_SETPOPMSG, POPMSG_MSG), (LPARAM)buf);
}
#endif

void CmdChopDirectory(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR path[VFPS], edir[VFPS], tdir[VFPS], *path_sep;
	WIN32_FIND_DATA ff;
	HANDLE hFile;
	int entries = 0;

	if ( VFSFixPath(path, (TCHAR *)param, GetZCurDir(Z), (VFSFIX_NOBLANK | VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH)) == NULL ){
		return;
	}
	CatPath(NULL, path, WildCard_All);
	path_sep = path + tstrlen(path) - 2;
	hFile = FindFirstFileL(path, &ff);
	if ( hFile == INVALID_HANDLE_VALUE ) return;
	tdir[0] = '\0';
	for (;;){
		if ( !IsRelativeDirectory(ff.cFileName) ){
			if ( ff.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ){
				TCHAR *p;

				entries++;
				if ( entries >= 2 ) break;

				*path_sep = '\0';
				// 仮dir名が書庫内dir名と被ったら、名前を変えて移動可能にする
				if ( tstricmp(ff.cFileName, FindLastEntryPoint(path)) == 0 ){
					tstrcpy(tdir, path);
					*(path_sep - 1) = '~';
				}
				tstrcpy(edir, path);
				*path_sep = '\\';
				tstrcpy(path_sep + 1, ff.cFileName); // 移動元
				p = FindLastEntryPoint(edir);
				if ( *p == '\0' ){
					entries = 2;
					break;
				}
				tstrcpy(p, ff.cFileName); // 移動先
			}else{
				entries = 0; // fileだった…chop不要
				break;
			}
		}
		if ( FindNextFile(hFile, &ff) == FALSE ) break;
	}
	FindClose(hFile);
	if ( entries != 1 ) return; // dir数が1以外は処理不要
	if ( tdir[0] != '\0' ){
		*path_sep = '\0';
		MoveFileL(tdir, path);
		*path_sep = '\\';
	}
	MoveFileL(path, edir);
	*path_sep = '\0';
	if ( IsTrue(RemoveDirectoryL(path)) ){
		FolderNotifyToShell(SHCNE_RMDIR, path, NULL);
	}
}

#ifdef UNICODE
	#ifndef _M_ARM64
		#ifndef USEDIRECTWRITE
			#define UHNAME ValueX3264(T("ppw"), T("ppx64"))
		#else
			#define UHNAME ValueX3264(T("ppxdw"), T("ppxdw64"))
		#endif
	#else
		#define UHNAME T("ppxarm64")
	#endif
#else
	#define UHNAME T("ppx")
#endif
const TCHAR CheckUpdateURL[] = CHECKVERSIONURL T("?exe=PPx&ver=") T(FileProp_Version) T("&name=") UHNAME T("&lcid=%04X&lrev=%s");
const TCHAR CheckUpdateURL2[] = T("http://toro.d.dooo.jp/checkver/checkver.cgi") T("?exe=PPx&ver=") T(FileProp_Version) T("&name=") UHNAME T("&lcid=%04X&lrev=%s");

const TCHAR CheckUpdateErrorMsg[] = MES_XUUE;
const TCHAR CheckUpdateNoupdMsg[] = MES_XUUN;
const TCHAR CheckUpdateQueryMsg[] = MES_XUUQ;
const TCHAR CheckUpdateBeginMsg[] = T("Download address '%s' is trust ?");

#define FTHOUR_L 0x61c46800
#define FTHOUR_H 8

BOOL IntervalUpdateCheck(const TCHAR *ptr)
{
	int itime;
	FILETIME nowtime;
	TCHAR buf[64];
	DWORD timeL, timeH, tmpH, tmpT;

	// 確認間隔を取得
	ptr++;
	itime = GetDigitNumber32(&ptr);
	if ( itime <= 0 ) itime = 7;
	if ( *ptr != 'h' ) itime *= 24; // i123[d] ... 日数→時間

	// 基準時間
	DDmul(itime, FTHOUR_L, &timeL, &timeH);
	DDmul(itime, FTHOUR_H, &tmpH, &tmpT);
	timeH += tmpH;
	// 以前の確認時刻を取得
	buf[0] = '\0';
	GetCustTable(StrCustSetup, T("CheckFT"), buf, sizeof(buf));
	ptr = buf;
	tmpT = GetDigitNumber32(&ptr);
	if ( *ptr != '.' ) return TRUE; // データ内→初めてのチェック
	ptr++;
	tmpH = GetDigitNumber32(&ptr);
	AddDD(timeL, timeH, tmpT, tmpH);
	// 比較
	GetSystemTimeAsFileTime(&nowtime);
	// high
	if ( nowtime.dwHighDateTime > timeH ) return TRUE;
	if ( nowtime.dwHighDateTime < timeH ) return FALSE;
	// low
	return (nowtime.dwLowDateTime >= timeL ) ? TRUE : FALSE;
}

void Update_CheckUpdateInterval(void)
{
	TCHAR buf[64];
	FILETIME nowtime;

	GetSystemTimeAsFileTime(&nowtime);
	thprintf(buf, TSIZEOF(buf), T("%u.%u"), nowtime.dwLowDateTime, nowtime.dwHighDateTime);
	SetCustStringTable(StrCustSetup, T("CheckFT"), buf, 0);
}

void CmdCheckUpdateFile(EXECSTRUCT *Z, const char *dataA, const TCHAR *param)
{
	TCHAR ver[VFPS];
	TCHAR url[VFPS];
	TCHAR text[VFPS * 3];
#ifdef UNICODE
	WCHAR bufW[0x400];
	const WCHAR *DATA;

	AnsiToUnicode(dataA, bufW, 0x400);
	bufW[0x400 - 1] = '\0';
	DATA = bufW;
#else
	#define DATA dataA
#endif
	GetLineParamS(&DATA, ver, TSIZEOF(ver));
	GetLineParamS(&DATA, url, TSIZEOF(url));

	if ( tstrchr(param, 'y') == NULL ){
		thprintf(text, TSIZEOF(text), MessageText(CheckUpdateQueryMsg), ver, T(FileProp_Version));
		if ( PMessageBox(Z->hWnd, text, NULL, MB_YESNO | MB_PPX_PLAIN_TEXT) != IDYES ) return;
	}
	if ( (memcmp(url, T("http://toro.d.dooo.jp/"), TSTROFF(22)) != 0) &&
		 (memcmp(url, T("https://toro.d.dooo.jp/"), TSTROFF(23)) != 0) ){
		thprintf(text, TSIZEOF(text), MessageText(CheckUpdateBeginMsg), url);
		if ( PMessageBox(Z->hWnd, text, NULL, MB_YESNO | MB_DEFBUTTON2 | MB_PPX_PLAIN_TEXT) != IDYES ){
			return;
		}
	}
	Get_X_save_widthUI(ver);
	CatPath(NULL, ver, tstrrchr(url, '/') + 1);
	{ // 保存できるか確認する
		HANDLE hFile;

		hFile = CreateFileL(ver, GENERIC_WRITE,
				FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS,
				FILE_FLAG_SEQUENTIAL_SCAN, NULL);
		if ( hFile != INVALID_HANDLE_VALUE ){ // 成功
			CloseHandle(hFile);
		}else{ // 失敗したので temp に保存
			GetTempPath(MAX_PATH, ver);
			CatPath(NULL, ver, tstrrchr(url, '/') + 1);
		}
	}
	thprintf(text, TSIZEOF(text), T("\"%s\",\"%s\""), url, ver);
	CmdHttpGet(Z, text);
	if ( Z->result == NO_ERROR ){
		int VerifyResult = VerifyZipImage(ver);

		if ( VerifyResult != VERIFYZIP_NOSUPPORTVERIFY ){
			if ( VerifyResult != VERIFYZIP_SUCCEDD ){
				XMessage(NULL, NULL, XM_GrERRld, MES_EBSG, ver);
				Z->result = ERROR_INVALID_DATA;
				return;
			}
		}
		thprintf(text, TSIZEOF(text), T("setup /sq \"%s\""), ver);

		url[0] = '\0';
		GetCustTable(StrCustSetup, T("elevate"), url, sizeof(url));
		if ( url[0] == '1' ){ // 昇格
			text[5] = '\0';
			text[8] = ' ';
			PPxShellExecute(Z->hWnd, T("RUNAS"), text, text + 6, DLLpath, XEO_NOUSEPPB, text);
		}else{
		#ifndef _WIN64
			#define COMPATNAME T("__COMPAT_LAYER")
			TCHAR oldvaule[0x2000];

			GetEnvironmentVariable(COMPATNAME, oldvaule, TSIZEOF(oldvaule));
			SetEnvironmentVariable(COMPATNAME, T("RunAsInvoker"));
		#endif
			ComExecSelf(Z->hWnd, text, DLLpath, XEO_NOUSEPPB, NULL);
		#ifndef _WIN64
			SetEnvironmentVariable(COMPATNAME, oldvaule);
		#endif
		}
		// ●とりあえずもっと前の段階で時刻更新
		// if ( tstrchr(param, 'i') != NULL) Update_CheckUpdateInterval();
	}
}

void zipunpack(EXECSTRUCT *Z, const TCHAR *param)
{
	IMAGEGETEXINFO exinfo;
	TCHAR src[VFPS];
	BYTE *mem;
	DWORD type = VFSDT_ZIPFOLDER;
	ERRORCODE result;

	GetLineParamS(&param, src, TSIZEOF(src));
	VFSFullPath(NULL, src, GetZCurDir(Z));
	GetLineParamS(&param, exinfo.dest, TSIZEOF(exinfo.dest));

	exinfo.Progress = (LPPROGRESS_ROUTINE)NULL;
	exinfo.lpData = NULL;
	exinfo.Cancel = NULL;
	mem = (BYTE *)&exinfo;

	result = VFSGetArchivefileImage(INVALID_HANDLE_VALUE, NULL, src, NilStr, NULL, &type, NULL, &mem);
	if ( result != MAX32 ){
		Z->result = result;
		ErrorPathBox(Z->hWnd, T("Unarc"), src, result);
	}
}

void CmdCheckUpdate(EXECSTRUCT *Z, const TCHAR *param)	// *checkupdate
{
	ThSTRUCT th;
	TCHAR url[MAX_PATH], mesname[16], lrev[32];
	const char *bottom;
	const TCHAR *msg = CheckUpdateNoupdMsg, *force, *ptr;
	int lcid;

	if ( *param == 'u' ){
		zipunpack(Z, param + 1);
		return;
	}
	if ( tstrchr(param, 'n') != NULL ) msg = NULL; // 最新版表示無し

	ptr = tstrchr(param, 'i');
	if ( ptr != NULL ){ // チェック間隔指定
		if ( IntervalUpdateCheck(ptr) == FALSE ) return;
		Update_CheckUpdateInterval(); // ●とりあえずここに
	}

	lcid = GetCustDword(T("X_LANG"), 0);
	if ( lcid == 0 ) lcid = LOWORD(GetUserDefaultLCID());
	thprintf(mesname, TSIZEOF(mesname), T("Mes%04X"), lcid);
	lrev[0] = '\0';
	GetCustTable(mesname, T(PPxLangRevItemID), &lrev, sizeof(lrev));
	thprintf(url, TSIZEOF(url), CheckUpdateURL, lcid, lrev);
	GetImageByHttp(url, &th);
	if ( th.bottom == NULL ){
		bottom = NULL;
	}else{
		bottom = strstr(th.bottom, "\r\n\r\n#");
		if ( bottom == NULL ){
			thprintf(url, TSIZEOF(url), CheckUpdateURL2, lcid, lrev);
			GetImageByHttp(url, &th);
		}

		force = tstrchr(param, 'f');
		bottom = strstr(th.bottom, "\n>");
		if ( bottom != NULL ){
			MessageBoxA(Z->hWnd, bottom + 2, NULL, MB_ICONINFORMATION | MB_OK);
		}
		bottom = strstr(th.bottom, "\r\n\r\n#");
	}
	if ( bottom == NULL ){
		msg = CheckUpdateErrorMsg;
	}else{ // チェック可能
		const TCHAR *pluscheck;
		const char *plusbottom;

		pluscheck = tstrchr(param, 'p');
		plusbottom = strstr(bottom + 5, "\r\n#");
		if ( (*(bottom + 5) == '+') || // 正式版の更新有り
					// 強制更新有り→
					//  試験公開版指定なしか、指定あり時に試験版がない
			 ((force != NULL) && ((pluscheck == NULL) || (plusbottom == NULL)))
			 ){
			CmdCheckUpdateFile(Z, bottom + 6, param);
			msg = NULL;
		}
		#if VersionP // 試験公開版は、常に試験公開版の更新チェックをする
		else
		#else // 正式版は、'p' 指定があるときのみ更新チェック
		else if ( pluscheck != NULL )
		#endif
		{
			if ( (plusbottom != NULL) &&
				 ((*(plusbottom + 3) == '+') || (force != NULL)) ){ // 試験公開版更新有
				CmdCheckUpdateFile(Z, plusbottom + 4, param);
				msg = NULL;
			}
		}
	}
	ThFree(&th);
	if ( msg != NULL ){
		if ( FALSE == PPxInfoFunc(Z->Info, PPXCMDID_SETPOPLINE, (void *)msg) ){
			XMessage(Z->hWnd, T("PPx ") T(FileProp_Version), XM_RESULTld, T("%s"), msg);
		}
		Z->result = ERROR_CANCELLED;
	}
}

void CmdCheckSignature(EXECSTRUCT *Z, TCHAR *line)
{
	TCHAR param[CMDLINESIZE];
	int VerifyResult;
	BOOL showsuccessresult = TRUE;

	GetModuleFileName(DLLhInst, param, MAX_PATH);
	VerifyResult = VerifyZipImage(param);
	if ( VerifyResult != VERIFYZIP_SUCCEDD ){
		XMessage(NULL, NULL, XM_GrERRld,
			(VerifyResult != VERIFYZIP_NOSUPPORTVERIFY) ? MES_EBSG : MES_EBRV,
			param);
		Z->result = ERROR_INVALID_DATA;
		return;
	}

	if ( *line == '!' ){
		showsuccessresult = FALSE;
		line++;
	}
	ZGetFilePathParam(Z, (const TCHAR **)&line, param);
	if ( param[0] != '\0' ){
		VerifyResult = VerifyZipImage(param);
	}else{
		GetModuleFileName(DLLhInst, param, MAX_PATH);
	}

	if ( VerifyResult != VERIFYZIP_SUCCEDD ){
		XMessage(NULL, NULL, XM_GrERRld, MES_EBSG, param);
		Z->result = ERROR_INVALID_DATA;
	}else{
		if ( showsuccessresult ) XMessage(NULL, NULL, XM_RESULTld, MES_GSIG);
	}
}

/*
			*togglecustword Custname:subname, word,
	又は、  *togglecustword Custname:subname, word:subword,
	"word" が一致すれば、削除・追加、subwordがあってもwordが一致すればよい
*/
void CmdToggleCustWord(const TCHAR *param)
{
	TCHAR parambuf[CMDLINESIZE], *sub, *optname, *optptr, *optnamesub;
	TCHAR custbuf[CMDLINESIZE];

	tstrcpy(parambuf, param);
	sub = tstrchr(parambuf, ':');
	if ( sub == NULL ) return;
	*sub++ = '\0';
	optname = tstrchr(sub, ',');
	if ( optname == NULL ) return;
	*optname++ = '\0';
	optnamesub = tstrchr(optname, ':');
	if ( optnamesub != NULL ) *optnamesub = '\0';
	custbuf[0] = '\0';
	GetCustTable(parambuf, sub, custbuf, sizeof(custbuf));
	optptr = tstrstr(custbuf, optname);
	if ( optptr != NULL ){ // 存在…削除
		TCHAR *ptr = optptr + tstrlen(optname);

		if ( optnamesub != NULL ){ // 別名有り…入れ替え
			int add;

			*optnamesub = ':';
			for (;;){
				if ( *ptr == '\0' ) break;
				if ( *ptr++ == ',' ) break;
			}
			add = (tstrstr(optptr, optname) != optptr);
			memmove(optptr, ptr, TSTRSIZE(ptr));
			if ( add ) tstrcat(custbuf, optname);
		}else{
			memmove(optptr, ptr, TSTRSIZE(ptr));
		}
	}else{ // 追加
		if ( optnamesub != NULL ) *optnamesub = ':';
		tstrcat(custbuf, optname);
	}
	SetCustStringTable(parambuf, sub, custbuf, 0);
}

void CmdMakeDirectory(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR path[CMDLINESIZE];
	ERRORCODE result;

	if ( VFSFixPath(path, (TCHAR *)param, GetZCurDir(Z), (VFSFIX_NOBLANK | VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH)) == NULL ){
		return;
	}
	result = MakeDirectories(path, NULL);
	if ( (result != NO_ERROR) && (result != ERROR_ALREADY_EXISTS) ){
		Z->result = ErrorPathBox(Z->hWnd, T("*makedir"), path, result);
	}
}

void CmdAddHistory(const TCHAR *param)
{
	TCHAR *ptr, buf[CMDLINESIZE];

	GetCommandParameter(&param, buf, TSIZEOF(buf));
	ptr = tstrchr(HistTypes, buf[0]);
	if ( (ptr == NULL) || (*ptr == '\0') || (tstrchr(T("pv"), buf[0]) != NULL) ){
		XMessage(NULL, NULL, XM_GrERRld, MES_EPRM);
		return;
	}

	NextParameter(&param);
	GetCommandParameter(&param, buf, TSIZEOF(buf));

	if ( buf[0] != '\0' ){
		WriteHistory(HistWriteTypeflag[ptr - HistTypes], buf, 0, NULL);
	}
}

void CmdSignal(EXECSTRUCT *Z, const TCHAR *param) // *signal
{
	TCHAR command[CMDLINESIZE];
	int useID;

	command[0] = '>';

	useID = GetPPxRegID(&param);
	if ( SkipSpace(&param) == ',' ) param++;
	GetCommandParameterDual(&param, command + 1, TSIZEOF(command) - 2);

	if ( (command[1] == '\0') || (tstrcmp(command + 1,T("c")) == 0) ){
		tstrcpy(command + 1, T("Gc"));
	}else if( tstrcmp(command + 1,T("break")) == 0 ){
		tstrcpy(command + 1, T("Gb"));
	}else if( tstrcmp(command + 1,T("execute")) == 0 ){
		NextParameter(&param);
		thprintf(command + 1, TSIZEOF(command) - 1, T("K%s"), param);
	}else if( tstrcmp(command + 1,T("focus")) == 0 ){
		tstrcpy(command + 1, T("Gf"));
	}else if( tstrcmp(command + 1,T("kill")) == 0 ){
		tstrcpy(command + 1, T("Gk"));
	}else if( tstrcmp(command + 1,T("killone")) == 0 ){
		tstrcpy(command + 1, T("GK"));
	}else if( tstrcmp(command + 1,T("x")) == 0 ){
		tstrcpy(command + 1, T("Gx"));
	}else{
		XMessage(NULL, NULL, XM_GrERRld, T("bad *signal command: %s"), command);
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}

	if ( useID < 0 ) useID = UsePPb(Z->hWnd); // もし、空いている PPb があれば確保
	if ( useID >= 0 ){
		if ( (Sm->P[useID].ID[0] == 'B') &&
			 (Sm->P[useID].ID[1] == '_') ){
			Sm->P[useID].UsehWnd = Z->hWnd;
			PPbSpecialMessage(Z->hWnd, useID, command);
			Sm->P[useID].UsehWnd = BADHWND;
		}
		return;
	}
	XMessage(NULL, NULL, XM_GrERRld, T("no PPb or busy"));
	Z->result = ERROR_BAD_UNIT;
}

void CmdAux(EXECSTRUCT *Z, const TCHAR *param) // *aux
{
	TCHAR *more;
	const TCHAR *ptr = param;
	TCHAR buf[VFPS], src[VFPS], dest[VFPS], id[64], path[VFPS];
	TCHAR pathbuf[VFPS];
	int flags = AUXOP_ASYNC;

	src[0] = dest[0] = id[0] = path[0] = '\0';
	for (;; ){
		if ( GetOptionParameter(&ptr, buf, &more) != '-' ){
			if ( buf[0] == '\0' ) break;
			if ( id[0] == '\0' ){
				tstplimcpy(id, buf, 64);
			}else{
				tstrcpy(path,
						(tstrcmppart(buf, T("aux:"), 4) == 0) ? buf + 4 : buf);
			}
		}else if ( tstrcmp(buf + 1, T("SRC")) == 0 ){
			tstrcpy(src, more);
		}else if ( tstrcmp(buf + 1, T("DEST")) == 0 ){
			tstrcpy(dest, more);
		}else if ( tstrcmp(buf + 1, T("SYNC")) == 0 ){
			setflag(flags, AUXOP_SYNC);
		}else{
			XMessage(Z->hWnd, NULL, XM_GrERRld, T("*aux id path [-src:][-dest:]"));
			return;
		}
	}

	tstrcpy(pathbuf, path);
	Z->result = AuxOperation(Z->Info, id, pathbuf, src, dest, NULL, flags);
}

const TCHAR TouchCmdStr[] = T("off\0") T("touch\0") T("mouse\0") T("pen\0");
void CmdTouch(EXECSTRUCT *Z, const TCHAR *param) // *touch
{
	TCHAR buf[VFPS];
	int pmc;

	GetCommandParameter((const TCHAR **)&param, buf, VFPS);
	if ( buf[0] == '\0' ){
		pmc = pmc_touch;
	}else{
		pmc = GetStringListCommand(buf, TouchCmdStr);
		if ( pmc < pmc_mode ) return;
	}
	if ( pmc == 0 ) pmc = pmc_mouse;
	if ( pmc > pmc_pen ) pmc = pmc_touch;
	PPxCommonCommand(Z->hWnd, pmc, K_E_TABLET);
	SendMessage(Z->hWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_E_TABLET, pmc), 0);
}

void CmdReportDialog(const TCHAR *param) // *reportdialog
{
	TCHAR command[CMDLINESIZE], c;

	c = SkipSpace((const TCHAR **)&param);
	GetCommandParameterDual(&param, command, TSIZEOF(command));

	if ( command[1] == 's' ) Sm->NowShutdown = TRUE;
	if ( c == '\"' ){
		PPxSendReport(command);
	}else if ( command[0] == 'r' ){
		command[0] = *(TCHAR *)0;
	}else if ( command[0] == 'w' ){
		*(char *)0 = 1;
	}else{
		XMessage(NULL, NULL, XM_GrERRld, T("r/w/\"text\""));
	}
}

void CmdDeleteHistory(const TCHAR *param)
{
	TCHAR *ptr, buf[CMDLINESIZE];
	WORD htype;

	GetCommandParameter(&param, buf, TSIZEOF(buf));
	ptr = tstrchr(HistTypes, buf[0]);
	if ( (ptr == NULL) || (*ptr == '\0') ){
		XMessage(NULL, NULL, XM_GrERRld, MES_EPRM);
		return;
	}
	htype = HistWriteTypeflag[ptr - HistTypes];

	NextParameter(&param);
	if ( Isdigit(*param) ){
		int index = GetDigitNumber32(&param);
		const TCHAR *hptr;

		UsePPx();
		hptr = EnumHistory(htype, index++);
		if ( hptr == NULL ) hptr = NilStr;
		tstrcpy(buf, hptr);
		FreePPx();
	}else{
		GetCommandParameter(&param, buf, TSIZEOF(buf));
	}
	if ( buf[0] != '\0' ) DeleteHistory(htype, buf);
}

void CmdMakeFile(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR path[CMDLINESIZE];
	HANDLE hFile;

	if ( VFSFixPath(path, (TCHAR *)param, GetZCurDir(Z), (VFSFIX_NOBLANK | VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH)) == NULL ){
		return;
	}
	hFile = CreateFileL(path, GENERIC_WRITE,
			FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_NEW,
			FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if ( hFile == INVALID_HANDLE_VALUE ){
		Z->result = ErrorPathBox(Z->hWnd, T("*makefile"), path, PPERROR_GETLASTERROR);
		return;
	}
	CloseHandle(hFile);
}

void CmdDeleteEntry(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR path[CMDLINESIZE];
	DWORD attr;
	//BOOL result = FALSE;

	if ( VFSFixPath(path, (TCHAR *)param, GetZCurDir(Z), (VFSFIX_NOBLANK | VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH)) == NULL ){
		return;
	}
	attr = GetFileAttributesL(path);
	if ( attr != BADATTR ){
		if ( attr & FILE_ATTRIBUTE_DIRECTORY ){
			/*result = */DeleteDirectories(path, TRUE);
		}else{
			/*result = */DeleteFileL(path);
		}
	}
	/*
	if ( result == FALSE ){
		Z->result = ErrorPathBox(Z->hWnd, T("*delete"), path, PPERROR_GETLASTERROR);
	}
	*/
}

void CmdRenameEntry(EXECSTRUCT *Z, const TCHAR *param)
{
	TCHAR path1[CMDLINESIZE], path2[CMDLINESIZE], *curdir;

	GetCommandParameter(&param, path1, TSIZEOF(path1));
	if ( NextParameter(&param) == FALSE ){
		Z->result = PPErrorBox(Z->hWnd, T("*rename"), ERROR_INVALID_PARAMETER);
		return;
	}
	curdir = GetZCurDir(Z);
	if ( VFSFixPath(NULL, path1, curdir, (VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH | VFSFIX_NOFIXEDGE)) == NULL ){
		return;
	}
	if ( VFSFixPath(path2, (TCHAR *)param, curdir, (VFSFIX_SEPARATOR | VFSFIX_FULLPATH | VFSFIX_REALPATH)) == NULL ){
		return;
	}
	if ( MoveFileL(path1, path2) == FALSE ){
		Z->result = ErrorPathBox(Z->hWnd, T("*rename"), path1, PPERROR_GETLASTERROR);
	}
}

void CmdStart(EXECSTRUCT *Z, const TCHAR *param) // *start
{
	BOOL launch = FALSE;
	TCHAR cmdbuf[VFPS], exepath[VFPS], pathbuf[VFPS];
	TCHAR *command = NULL, *path;

	path = GetZCurDir(Z);
	exepath[0] = '\0';

	for (;;){
		const TCHAR *ptr;
		TCHAR c;

		ptr = param;
		c = GetOptionParameter(&ptr, pathbuf, NULL);
		if ( c == '\0' ) break;
		if ( c == '-' ){
			if ( tstrcmp(pathbuf + 1, T("LAUNCH")) == 0 ){
				launch = TRUE;
			}else{
				tstrcpy(cmdbuf, pathbuf + 1);
				command = cmdbuf;
			}
			param = ptr;
		}else{
			if ( GetExecType(&param, exepath, path) == GTYPE_ERROR ){
				tstrcpy(exepath, pathbuf);
			}
			break;
		}
	}

	if ( launch ){
		tstrcpy(pathbuf, exepath);
		*VFSFindLastEntry(pathbuf) = '\0';
		path = pathbuf;
		if ( *path == '\"' ) path++; // 末尾の \" は２行前で削除済み
	}
	ZapMain(Z, command, exepath, param, path);
}

#ifndef ABOVE_NORMAL_PRIORITY_CLASS
	#define BELOW_NORMAL_PRIORITY_CLASS 0x4000 // スケジュール
	#define ABOVE_NORMAL_PRIORITY_CLASS 0x8000 // スケジュール
#endif
#ifndef CREATE_BREAKAWAY_FROM_JOB
	#define CREATE_BREAKAWAY_FROM_JOB 0x01000000  // ジョブを分離する
#endif
#ifndef PROCESS_MODE_BACKGROUND_BEGIN
	#define PROCESS_MODE_BACKGROUND_BEGIN 0x00100000 // 低いIO優先度
#endif

#if 0
// __compat_layer の例。全部で500位有る。スペースで区切る
RunAsInvoker 昇格無効
RunAsHighest 管理者権限がある場合にのみUACをトリガー、
			 管理者権限がない場合は管理者権限を付与しない
RunAsAdmin	常にUACをトリガー
DisableThemes	テーマ無効
256Color 640x480	画面解像度
Win95 Win98 NT4SP5 VistaRTM 等

// CreateProcess flags
#define CREATE_SUSPENDED                  0x00000004 // 起動待機
#define DETACHED_PROCESS                  0x00000008 // console/コンソールWindowを持たない。コンソールハンドルあり？

#define INHERIT_PARENT_AFFINITY           0x00010000 // 親のPARENT_AFFINITY
#define INHERIT_CALLER_PRIORITY           0x00020000 // 廃止。親のスケジュール
#define CREATE_PROTECTED_PROCESS          0x00040000 // 著作権保護プロセス
#define EXTENDED_STARTUPINFO_PRESENT      0x00080000 // STARTUPINFOEX を使用(affinity とか)

#define PROCESS_MODE_BACKGROUND_END       0x00200000 // 低いIO優先度

#define CREATE_PRESERVE_CODE_AUTHZ_LEVEL  0x02000000 // プロセス制限をバイパス?
#define CREATE_NO_WINDOW                  0x08000000 // console/コンソールWindowを持たない。コンソールハンドルなし

#define PROFILE_USER                      0x10000000 // ?
#define PROFILE_KERNEL                    0x20000000 // ?
#define PROFILE_SERVER                    0x40000000 // ?
#define CREATE_IGNORE_SYSTEM_DEFAULT      0x80000000 // ?
#endif

void CmdRun_io_send(EXEC_EXTRA *extra, const TCHAR **more, BOOL setret)
{
	TCHAR buf[CMDLINESIZE];

	setflag(extra->mask, EEM_STDIN_value);
	if ( SkipSpace(more) == ',' ) (*more)++;
	GetCommandParameter(more, buf, TSIZEOF(buf));
	if ( setret ) tstrcat(buf, T("\r\n"));
	ThCatString(&extra->stdio.thReceive, buf);
}

void CmdRunFunction(EXECSTRUCT *Z, const TCHAR *param, int runmode) // *run / *launch / %*run
{
	DWORD flags;
	const TCHAR *ptr = param;
	TCHAR *more, *curdir = NULL;
	TCHAR buf[CMDLINESIZE], curdirbuf[VFPS];
	EXEC_EXTRA extra;
	ThSTRUCT parambuf;
	BOOL launch;

	extra.mask = 0;
	extra.ExtraCreateFlags = 0;
	extra.ExtraStartup.flags = 0;
	parambuf.bottom = NULL;
	flags = Z->flags;
	launch = (runmode == Run_LAUNCH);
	if ( runmode == Run_FUNCTION ){
		setflag(flags, XEO_NOUSEPPB | XEO_SEQUENTIAL);
		ThInit(&extra.stdio.thReceive);
		setflag(extra.mask, EEM_UseESF | EEM_STDOUT_receive);
		setflag(extra.ExtraStartup.flags, STARTF_USESTDHANDLES);
		extra.info = Z->Info;
		extra.stdio.hPPeWnd = NULL;
		extra.stdio.codepage = CP_ACP;
	}
	while ( GetOptionParameter(&ptr, buf, &more) == '-' ){
		if ( tstrcmp(buf + 1, T("LAUNCH")) == 0 ){ // PPx
			launch = TRUE;
		}else if ( tstrcmp(buf + 1, T("D")) == 0 ){ // CMD カレントディレクトリ
			VFSFixPath(curdirbuf, more, GetZCurDir(Z), VFSFIX_FULLPATH);
			curdir = curdirbuf;
		}else if ( tstrcmp(buf + 1, T("ONPPB")) == 0 ){ // %OB
			setflag(flags, XEO_USEPPB);
		}else if ( tstrcmp(buf + 1, T("NOPPB")) == 0 ){ // %Ob
			setflag(flags, XEO_NOUSEPPB);
		}else if ( tstrcmp(buf + 1, T("CMD")) == 0 ){ // %Oc
			setflag(flags, XEO_USECMD);
		}else if ( tstrcmp(buf + 1, T("MIN")) == 0 ){ // CMD %On
			setflag(flags, XEO_MIN);
		}else if ( tstrcmp(buf + 1, T("MAX")) == 0 ){ // CMD %Ox
			setflag(flags, XEO_MAX);
		}else if ( tstrcmp(buf + 1, T("HIDE")) == 0 ){ // %Od
			setflag(flags, XEO_HIDE);
		}else if ( tstrcmp(buf + 1, T("NOACTIVE")) == 0 ){ // %Oa
			setflag(flags, XEO_NOACTIVE);
		}else if ( tstrcmp(buf + 1, T("LOW")) == 0 ){ // CMD %Ol
			setflag(flags, XEO_LOW);
		}else if ( tstrcmp(buf + 1, T("NORMAL")) == 0 ){ // CMD %Ol-
			resetflag(flags, XEO_LOW);
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, NORMAL_PRIORITY_CLASS);
		}else if ( tstrcmp(buf + 1, T("HIGH")) == 0 ){ // CMD
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, HIGH_PRIORITY_CLASS);
		}else if ( tstrcmp(buf + 1, T("REALTIME")) == 0 ){ // CMD
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, REALTIME_PRIORITY_CLASS);
		}else if ( tstrcmp(buf + 1, T("ABOVENORMAL")) == 0 ){ // CMD
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, ABOVE_NORMAL_PRIORITY_CLASS);
		}else if ( tstrcmp(buf + 1, T("BELOWNORMAL")) == 0 ){ // CMD
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, BELOW_NORMAL_PRIORITY_CLASS);
		}else if ( tstrcmp(buf + 1, T("WAIT")) == 0 ){ // CMD %Os
			if ( tstrcmp(more, T("idle")) == 0 ){
				setflag(flags, XEO_WAITIDLE);
			}else if ( tstrcmp(more, T("no")) == 0 ){
				resetflag(flags, XEO_WAITIDLE | XEO_SEQUENTIAL);
			}else if ( tstrcmp(more, T("later")) == 0 ){
				setflag(extra.mask, EEM_GETID);
				setflag(flags, XEO_NOUSEPPB);
			}else{
				setflag(flags, XEO_SEQUENTIAL);
			}
			// WAIT:quiet
		}else if ( tstrcmp(buf + 1, T("BREAKJOB")) == 0 ){ // PPx ジョブを分離する
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_BREAKAWAY_FROM_JOB);
		}else if ( tstrcmp(buf + 1, T("NEWGROUP")) == 0 ){ // プロセスグループを分離 ^C/^BREAK の影響
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_NEW_PROCESS_GROUP);
		}else if ( tstrcmp(buf + 1, T("NOWINDOW")) == 0 ){ // CREATE_NO_WINDOW // コンソールウィンドウ無し CREATE_NEW_CONSOLE,DETACHED_PROCESS併用不可
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_NO_WINDOW);
		}else if ( tstrcmp(buf + 1, T("PID")) == 0 ){ // プロセスIDを記憶
			setflag(flags, XEO_NOUSEPPB);
			setflag(extra.mask, EEM_GETID);
		}else if ( tstrcmp(buf + 1, T("TID")) == 0 ){ // スレッドIDを記憶
			setflag(flags, XEO_NOUSEPPB);
			setflag(extra.mask, EEM_GETID);
		}else if ( tstrcmp(buf + 1, T("MONITOR")) == 0 ){
			if ( NULL != (extra.ExtraStartup.hStdOut = GetMonitorHandle(Z->hWnd, GMH_INWINDOW)) ){
				setflag(extra.mask, EEM_UseESF);
				setflag(extra.ExtraStartup.flags, STARTF_MONITOR);
			}
		}else if ( tstrcmp(buf + 1, T("POS")) == 0 ){ // STARTUPINFO dwX, dwY
			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_USEPOSITION);
			extra.ExtraStartup.x = GetIntNumber((const TCHAR **)&more);
			NextParameter((const TCHAR **)&more);
			extra.ExtraStartup.y = GetIntNumber((const TCHAR **)&more);
 		}else if ( tstrcmp(buf + 1, T("SIZE")) == 0 ){ // STARTUPINFO dwXSize, dwYSize
			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_USESIZE);
			extra.ExtraStartup.width = GetIntNumber((const TCHAR **)&more);
			NextParameter((const TCHAR **)&more);
			extra.ExtraStartup.height = GetIntNumber((const TCHAR **)&more);
		}else if ( (tstrcmp(buf + 1, T("IO")) == 0) ||
				   (tstrcmp(buf + 1, T("LOG")) == 0) ){ // ログ窓に送信
			if ( buf[1] == 'L' ) setflag(extra.mask, EEM_STDOUT_log);
			if ( ! (extra.ExtraStartup.flags & STARTF_USESTDHANDLES) ){
				setflag(flags, XEO_HIDE | XEO_NOUSEPPB | XEO_SEQUENTIAL);
				setflag(extra.mask, EEM_UseESF);
				setflag(extra.ExtraStartup.flags, STARTF_USESTDHANDLES);
				extra.info = Z->Info;
				extra.stdio.hPPeWnd = NULL;
				extra.stdio.codepage = CP_ACP;
				ThInit(&extra.stdio.thReceive);
			}
			if ( *more != '\0' ){
				// ※more は buf 上のポインタであることに注意
				for(;;){
					TCHAR chr;
					chr = GetCommandParameter((const TCHAR **)&more, buf, TSIZEOF(buf));
					if ( chr == '\0' ) break;
					if ( chr == ',' ){
						more++;
						continue;
					}
					if ( tstrcmp(buf, T("utf8")) == 0 ){
						extra.stdio.codepage = CP_UTF8;
					}else if ( tstrcmp(buf, T("system")) == 0 ){
						extra.stdio.codepage = GetACP();
					}else if ( (tstrcmp(buf, T("unicode")) == 0) || (tstrcmp(buf, T("utf16")) == 0) ){
						extra.stdio.codepage = CP__UTF16L;
					}else if ( (tstrcmp(buf, T("ibm")) == 0) || (tstrcmp(buf, T("us")) == 0) ){
						extra.stdio.codepage = CP__US;
					}else if ( (tstrcmp(buf, T("ansi")) == 0) || (tstrcmp(buf, T("latin1")) == 0) ){
						extra.stdio.codepage = CP__LATIN1;
					}else if ( tstrcmp(buf, T("sjis")) == 0 ){
						extra.stdio.codepage = (GetACP() == CP__SJIS) ? VTYPE_SYSTEMCP : CP__SJIS;
					}else if ( tstrcmp(buf, T("log")) == 0 ){
						setflag(extra.mask, EEM_STDOUT_log);
					}else if ( tstrcmp(buf, T("ppe")) == 0 ){
						setflag(extra.mask, EEM_STDOUT_PPe);
					}else if ( (tstrcmp(buf, T("value")) == 0) || (tstrcmp(buf, T(">>>")) == 0) ){
						setflag(extra.mask, EEM_STDOUT_value);
					}else if ( tstrcmp(buf, T("string")) == 0 ){
						CmdRun_io_send(&extra, (const TCHAR **)&more, FALSE);
					}else if ( (tstrcmp(buf, T("send")) == 0) || (tstrcmp(buf, T("<<<")) == 0) ){
						CmdRun_io_send(&extra, (const TCHAR **)&more, TRUE);
					}else if ( Isdigit(buf[0]) ){
						extra.stdio.codepage = GetIntNumber((const TCHAR **)&more);
					}else{
						XMessage(Z->hWnd, NULL, XM_GrERRld, MES_EUOP, buf);
						Z->result = ERROR_INVALID_PARAMETER;
					}
				}
			}
		}else if ( tstrcmp(buf + 1, T("DESKTOPLEVEL")) == 0 ){
			setflag(extra.mask, EEM_DesktopLevel);
		}else if ( tstrcmp(buf + 1, T("NOSTARTMSG")) == 0 ){
			setflag(extra.mask, EEM_NoStartMsg);
#if 0
		}else if ( tstrcmp(buf + 1, T("I")) == 0 ){ // CMD lpEnvironment ?
		}else if ( tstrcmp(buf + 1, T("B")) == 0 ){ // CMD !CREATE_NEW_CONSOLE ?

		}else if ( tstrcmp(buf + 1, T("DOS")) == 0 ){ // OS/2 の DOS部分を実行
			// Win32 の DOS部分を実行できるわけではない
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_FORCEDOS);

		}else if ( tstrcmp(buf + 1, T("SEPARATE")) == 0 ){ // CMD
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_SEPARATE_WOW_VDM);

		}else if ( tstrcmp(buf + 1, T("SHARED")) == 0 ){ // CMD
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_SHARED_WOW_VDM);

		}else if ( tstrcmp(buf + 1, T("NODE")) == 0 ){ // CMD

		}else if ( tstrcmp(buf + 1, T("AFFINITY")) == 0 ){ // CMD

		}else if ( tstrcmp(buf + 1, T("TITLE")) == 0 ){ // CMD? キャプション指定

		}else if ( tstrcmp(buf + 1, T("COMP")) == 0 ){ // 互換性設定

		}else if ( tstrcmp(buf + 1, T("FULLSCREEN")) == 0 ){ // PPx console をフルスクリーン ※ x86 のみ
			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_RUNFULLSCREEN);

		}else if ( tstrcmp(buf + 1, T("NEWCONSOLE")) == 0 ){ // 新しいコンソール。DETACHED_PROCESS併用不可
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_NEW_CONSOLE);

		}else if ( tstrcmp(buf + 1, T("BGIO")) == 0 ){ // 使えないようだ
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, PROCESS_MODE_BACKGROUND_BEGIN);
		}else if ( tstrcmp(buf + 1, T("SECURE")) == 0 ){ // 各種条件が必要
			setflag(extra.mask, EEM_UseECF);
			setflag(extra.ExtraCreateFlags, CREATE_SECURE_PROCESS);

		}else if ( tstrcmp(buf + 1, T("HOTKEY")) == 0 ){
//			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_USEHOTKEY);
//#define STARTF_USEHOTKEY           0x00000200 Use the hot-key DWORD specified in the hStdInput member.
//		STARTF_TITLESHORTCUT	0x800	Program was started through a shortcut. The lpTitle contains the shortcut path.
//		STARTF_SCREENSAVER	0x80000000	Start the program with NORMAL_PRIORITY, then drop to IDLE_PRIORITY.
//#define STARTF_TITLEISLINKNAME     0x00000800
//#define STARTF_TITLEISAPPID        0x00001000
//#define STARTF_PREVENTPINNING      0x00002000
//		}else if ( tstrcmp(buf + 1, T("NOPIN")) == 0 ){ // PPx
//			setflag(extra.mask, EEM_UseESF);
//			setflag(extra.ExtraStartup.flags, STARTF_PREVENTPINNING | STARTF_TITLEISAPPID);
//			lpTitle 設定

		}else if ( tstrcmp(buf + 1, T("DESKTOP")) == 0 ){ // STARTUPINFO lpDesktop
//			setflag(extra.mask, EEM_UseESF);
//			setflag(extra.ExtraStartup.flags,
		}else if ( tstrcmp(buf + 1, T("CONSOLESIZE")) == 0 ){ // STARTUPINFO dwXCountChars, dwYCountChars
//			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_USECOUNTCHARS);
		}else if ( tstrcmp(buf + 1, T("COLOR")) == 0 ){ // STARTUPINFO dwFillAttribute
//			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_USEFILLATTRIBUTE);
		}else if ( tstrcmp(buf + 1, T("UNTRUSTED")) == 0 ){ // PPx コマンドラインが信頼できないことを伝える。受け手側は、GetStartupInfo で確認する
			setflag(extra.mask, EEM_UseESF);
			setflag(extra.ExtraStartup.flags, STARTF_UNTRUSTEDSOURCE);
#endif

		}else{
//			tstrcpy(cmdbuf, buf + 1);
//			command = cmdbuf;
			XMessage(Z->hWnd, NULL, XM_GrERRld, MES_EUOP, buf);
			Z->result = ERROR_INVALID_PARAMETER;
		}
		param = ptr;
	}
	if ( curdir == NULL ){
		curdir = GetZCurDir(Z);

		if ( launch ){
			const TCHAR *ptr = param;

			if ( GTYPE_ERROR != GetExecType(&ptr, buf, curdir) ){
				tstrcpy(curdirbuf, buf);
				*VFSFindLastEntry(curdirbuf) = '\0';
				curdir = curdirbuf;
				if ( *curdir == '\"' ) curdir++; // 末尾の \" は２行前で削除済み
			}
		}
	}

	if ( (Z->result != NO_ERROR) || (SkipSpace(&param) == '\0') ) return;

	if ( Isalnum(*param) && (*(param + 1) != ':') ){ // エイリアス展開
		const TCHAR *ptr = param;

		GetLineParamS(&ptr, buf, CmdFuncMaxLen);
		if ( NO_ERROR == GetCustTable(T("A_exec"), buf, buf, TSTROFF(CMDLINESIZE)) ){ // エイリアス有り
			Z->result = PP_ExtractMacro(Z->hWnd, Z->Info, NULL, buf, buf, 0);
			if ( Z->result != NO_ERROR ) return;
			parambuf.top = 0;
			ThCatString(&parambuf, buf);
			ThCatString(&parambuf, T(" "));
			ThCatString(&parambuf, ptr);
			param = (TCHAR *)parambuf.bottom;
		}
	}
	if ( (Z->flags & XEO_CONSOLE) && !(extra.mask & (EEM_GETID | EEM_UseSTDIO)) ){
		ExecOnConsole(Z, param, curdir);
	}else{
		extra.ExitCode = &Z->ExitCode;
		if ( ComExecEx(Z->hWnd, param, curdir, &Z->useppb, flags, &extra) == FALSE ){
			Z->result = GetLastError();
			if ( Z->result == NO_ERROR ) Z->result = ERROR_PATH_NOT_FOUND;
		}else{
			if ( extra.mask & EEM_GETID ){
				Numer32ToString(buf, extra.ProcessID);
				ThSetString(&Z->StringVariable, T("PID"), buf);
				Numer32ToString(buf, extra.ThreadID);
				ThSetString(&Z->StringVariable, T("TID"), buf);
			}
		}
		if ( extra.mask & EEM_UseSTDIO ){
			if ( extra.mask & EEM_STDOUT_receive ){
				SetLongDestMain(Z, (extra.stdio.thReceive.bottom == NULL) ? NilStr : (TCHAR *)extra.stdio.thReceive.bottom, 0);
			}
			if ( extra.mask & EEM_STDOUT_value ){
				ThSetString(&Z->StringVariable, T("stdout"),
					(extra.stdio.thReceive.bottom == NULL) ? NilStr : (TCHAR *)extra.stdio.thReceive.bottom );
			}
			ThFree(&extra.stdio.thReceive);
		}
	}
	ThFree(&parambuf);
}

DWORD_PTR USECDECL CmdPackInfo(struct PACKINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch(cmdID){
		case '2': // %2 展開先
			tstrcpy(uptr->enums.buffer, info->path);
			break;

		default:
			return info->parent->Function(info->parent, cmdID, uptr);
	}
	return 1;
}

DWORD_PTR USECDECL CmdPackInfoIndiv(struct PACKINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch(cmdID){
		case PPXCMDID_STARTENUM:	// 検索開始(マーク無しもあり)
		case PPXCMDID_STARTNOENUM:	// 検索開始(マーク無しはなし)
			uptr->enums.enumID = 1;
			break;

		case PPXCMDID_NEXTENUM:		// 次へ
		case PPXCMDID_ENDENUM:		//列挙終了
			uptr->enums.enumID = -1;
			return 0;

		case '2': // %2 展開先
			tstrcpy(uptr->enums.buffer, info->path);
			break;

		case '3': // %3 書庫対象ファイル(フルパス、zipfldr.dl)
			info->parent->Function(info->parent, '1', uptr);
			VFSFullPath(uptr->enums.buffer, info->files, uptr->enums.buffer);
			break;

		case '4': // %4 書庫対象ファイル
			tstrcpy(uptr->enums.buffer, info->files);
			if ( info->attr & FILE_ATTRIBUTE_DIRECTORY ){
				tstrcat(uptr->enums.buffer, T("\\*"));
			}
			break;

		case PPXCMDID_ENUMATTR:
			*(DWORD *)uptr->enums.buffer = info->attr;
			break;

		default:
			return info->parent->Function(info->parent, cmdID, uptr);
	}
	return PPXA_NO_ERROR;
}

void CmdPack(EXECSTRUCT *Z, const TCHAR *param)  // *pack 書庫作成
{
	struct PACKINFO pinfo;
	TCHAR packname[VFPS], packcmd[CMDLINESIZE], packtype[MAX_PATH];
	TCHAR pathbuf[VFPS], buf[VFPS], *pp;
	ThSTRUCT *thStringValue;
	BOOL indivmode = FALSE, AddExt = TRUE;

	buf[0] = '1';
	GetCustTable(StrCustOthers, T("PackAddExt"), buf, sizeof(buf));
	if ( buf[0] == '0' ) AddExt = FALSE;

	thStringValue = (ThSTRUCT *)PPxInfoFunc(Z->Info, PPXCMDID_GETWNDVARIABLESTRUCT, NULL);
	if ( thStringValue == NULL ) thStringValue = &ProcessStringValue;

	// 1st param : 編集無し"!" + ファイル名
	GetCommandParameter(&param, pathbuf, TSIZEOF(pathbuf));
	pinfo.path = pathbuf;

	if ( pathbuf[0] == '\0' ){
		GetValue(Z, '2', pathbuf);
		if ( pathbuf[0] != '\0' ){
			DWORD attr;

			attr = GetFileAttributesL(pathbuf);
			if ( (attr != BADATTR) && !(attr & FILE_ATTRIBUTE_DIRECTORY) ){
				tstrcat(packname, T("|"));
			}else{
				CatPath(NULL, pathbuf, T("|"));
				GetValue(Z, 'X', packname);
				tstrcat(packname, T("|"));
				tstrcat(pathbuf, packname);
			}
		}
	}

	// 2nd param : dllname
	if ( NextParameter(&param) == FALSE ){
		packtype[0] = '\0';
	}else{
		GetCommandParameter(&param, packtype, TSIZEOF(packtype));
	}

	// another 2nd param : indiv ?
	if ( tstrcmp(packtype, T("indiv")) == 0 ){
		indivmode = TRUE;
		if ( NextParameter(&param) == FALSE ){
			packtype[0] = '\0';
		}else{
			GetCommandParameter(&param, packtype, TSIZEOF(packtype));
		}
	}

	SetCustTable(StrCustOthers, T("PackIndiv"), indivmode ? Str1 : NilStr , sizeof(Str1));

	// another 2nd param : AddExt ?
	if ( tstrcmp(packtype, T("addext")) == 0 ){
		AddExt = TRUE;
		if ( NextParameter(&param) == FALSE ){
			packtype[0] = '\0';
		}else{
			GetCommandParameter(&param, packtype, TSIZEOF(packtype));
		}
	}

	// 3rd param : type
	NextParameter(&param);
	GetCommandParameter(&param, packname + 2, TSIZEOF(packname) - 2);

	if ( packtype[0] != '\0' ){ // 圧縮種類を指定
		const TCHAR *tpac;

		packname[0] = 'p';
		packname[1] = ':';
		if ( FindPackType(packtype, packname, packcmd) == FALSE ){
			XMessage(Z->hWnd, NULL, XM_GrERRld, T("unmatch name %s - %s"), packtype, packname + 2);
			Z->result = ERROR_INVALID_PARAMETER;
			return;
		}
		ThSetString(&ProcessStringValue, T("Edit_PackMode"), T("t"));
		tpac = MessageText(MES_TPAC);
		tstrcpy(buf, packtype);
		pp = tstrrchr(buf, '.');
		if ( pp != NULL ) *pp = '\0';
		thprintf(packtype, TSIZEOF(packtype), T("%s %s - %s"), tpac, packname + 2, buf);
		thprintf(packname, TSIZEOF(packname), T("*string i,Edit_PackCmd=%%M&M:?packlist %%: *string i,Edit_PackName=%%si\"Menu_Index\" %%: *string i,Edit_PackCmd=%%si\"Edit_PackCmd\" %: *setcaption %s %%si\"Menu_Index\""), tpac);
		ThSetString(thStringValue, T("Edit_OptionCmd"), packname);


		if ( FALSE == CmdPack_Edit(Z, &pinfo, thStringValue, packtype, packcmd) ) return;

		ThGetString(thStringValue, T("Edit_PackName"), packtype, TSIZEOF(packtype));
		pp = tstrchr(packtype, ':');
		if ( pp != NULL ){
			pp++;
			if ( *pp == ' ' ) pp++;
			memmove(packtype, pp, TSTRSIZE(pp));
		}
	}else{ // 圧縮種類の指定なし
		const TCHAR *tpac;

		packname[0] = '\0';
		packcmd[0] = '\0';
		GetCustTable(StrCustOthers, T("PackName"), packname, sizeof(packname));
		GetCustTable(StrCustOthers, T("PackCmd"), packcmd, sizeof(packcmd));

		ThSetString(&ProcessStringValue, T("Edit_PackMode"), T("g"));
		tpac = MessageText(MES_TPAC);
		thprintf(packtype, TSIZEOF(packtype), T("%s %s"), tpac, packname);

		thprintf(packname, TSIZEOF(packname), T("*string i,Edit_PackCmd=%%M&M:?packlist %%: *setcust _others:PackName=%%si\"Menu_Index\" %%: *setcust _others:PackCmd=%%si\"Edit_PackCmd\" %%: *setcaption %s %%si\"Menu_Index\""), tpac);
		ThSetString(thStringValue, T("Edit_OptionCmd"), packname);

		if ( FALSE == CmdPack_Edit(Z, &pinfo, thStringValue, packtype, packcmd) ) return;

		GetCustTable(StrCustOthers, T("PackName"), packtype, sizeof(packtype));

	}

	packname[0] = '0';
	GetCustTable(StrCustOthers, T("PackIndiv"), packname, sizeof(packname));
	indivmode = packname[0] == '1';

	// オプションがあれば、 <option1, option2...< で展開する
	packname[0] = '<';
	GetCustTable(StrCustOthers, T("PackOption"), packname + 1, sizeof(packname) - sizeof(TCHAR));

	while ( NextParameter(&param) != FALSE ){
		GetCommandParameter(&param, buf, TSIZEOF(buf));
		if ( buf[0] != '\0' ){
			if ( buf[0] == '-' ){ // option 削除
				tstrreplace(packname, buf + 1, NilStr);
			}else{
				TCHAR *ptr = tstrchr(buf, ':');

				if ( ptr != NULL ){ // 種別指定有り
					TCHAR back = *(ptr + 1), *oldptr;

					*(ptr + 1) = '\0';
					oldptr = tstrstr(packname, buf);
					*(ptr + 1) = back;
					if ( oldptr != NULL ){ // 同じ種別があったら除去する
						TCHAR *oldptrlast = oldptr;

						for (;;){
							if ( *oldptrlast == '\0' ) break;
							if ( *oldptrlast++ == ',' ) break;
						}
						memmove(oldptr, oldptrlast, TSTRSIZE(oldptrlast));
					}
				}

				tstrcat(packname, buf);
				tstrcat(packname, T(","));
			}
		}
	}

	if ( packname[1] != '\0' ){
		tstrcat(packname, T("<"));
	}else{
		packname[0] = '\0';
	}

	tstrreplace(packcmd, T(">"), packname);
	tstrreplace(packcmd, T("%2%\\"), T("%2")); // %2にファイル名が入っているから

	pinfo.info = *Z->Info;
	pinfo.parent = Z->Info;
	if ( indivmode == FALSE ){ // 一括
		if ( AddExt ){ // 書庫拡張子がなければ追加する
			tstrcpy(buf, packtype);
			pp = tstrchr(buf, ' ');
			if ( pp != NULL ) *pp = '\0';

			if ( buf[0] != '\0' ){
				pp = tstrchr(buf, ':');
				if ( pp != NULL ) *pp = '\0';

				pp = tstrchr(pathbuf, '.');
				if ( (pp == NULL) || (tstricmp(pp + 1, buf) != 0) ){
					pp = pathbuf + tstrlen(pathbuf);
					thprintf(pp, TSIZEOF(pathbuf) - (pp - pathbuf), T(".%s"), buf);
				}
			}
		}

		pinfo.info.Function = (PPXAPPINFOFUNCTION)CmdPackInfo;
		PP_ExtractMacro(Z->hWnd, &pinfo.info, NULL, packcmd, NULL, XEO_NOEDIT);
	}else{ // 個別(indiv)
		TCHAR *pathLast, *lp;
		ThSTRUCT th;
		BOOL addext; // %2 に拡張子を追加するか

		{ // packtype を作成書庫の拡張子名に加工する
			TCHAR *p;

			p = tstrstr(packtype, T(" - "));
			if ( p != NULL ) *p = '\0';
		}

		pinfo.info.Function = (PPXAPPINFOFUNCTION)CmdPackInfoIndiv;
		pinfo.files = buf;
		if ( tstrstr(packcmd, T("%u") VFS_TYPEID_zipfldr) != NULL ){
			tstrcpy(packcmd, T("%u") VFS_TYPEID_zipfldr T(",A \"%2\" /src:\"%3\""));
		}else{
			tstrreplace(packcmd, T("%@"), T("\"%4\""));
		}
		// pack command が %2.tar とかの形になっているなら、拡張子付与はしない
		addext = (tstrstr(packcmd, T("%2.")) == NULL) ? TRUE : FALSE;
		CatPath(NULL, pinfo.path, NilStr);
		pathLast = pinfo.path + tstrlen(pinfo.path);
		ThInit(&th);
		for ( ; ; ){
			GetValue(Z, 'C', buf);
			if ( buf[0] == '\0' ) break;
			PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMATTR, (TCHAR *)&pinfo.attr, &Z->EnumInfo);
			tstrcpy(pathLast, buf);
			if ( pinfo.attr & FILE_ATTRIBUTE_DIRECTORY ){
				lp = pathLast + tstrlen(pathLast);
			}else{
				lp = pathLast + FindExtSeparator(pathLast);
			}
			if ( addext ){
				*lp++ = '.';
				tstrcpy(lp, packtype);
			}else{
				*lp = '\0';
			}
			ThSize(&th, CMDLINESIZE);
			ThCatString(&th, T("%u/"));
			PP_ExtractMacro(Z->hWnd, &pinfo.info, NULL, packcmd, ThStrLastT(&th), XEO_NOEDIT);
			if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_NEXTENUM, buf, &Z->EnumInfo) == 0 ){
				break;
			}
			th.top += TSTRLENGTH32(ThStrLastT(&th));
			ThCatString(&th, T("\r\n"));
			ThCatString(&th, T("%: *wait 0,1\r\n")); //★溜まったメッセージを処理
		}
		if ( th.top > 0 ){
			PP_ExtractMacro(Z->hWnd, Z->Info, NULL, (TCHAR *)th.bottom, NULL, XEO_NOEDIT);
		}
		ThFree(&th);
	}
}

void CmdNextItem(EXECSTRUCT *Z, const TCHAR *param) // *nextitem
{
	int skipcount = 1;
	TCHAR buf[64];

	if ( *param != '\0' ) CalcString(&param, &skipcount);
	while ( skipcount > 0 ){
		if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_NEXTENUM, buf, &Z->EnumInfo) == 0 ){
			XMessage(Z->hWnd, NULL, XM_GrERRld, T("empty next item"));
			Z->result = ERROR_INVALID_PARAMETER;
			break;
		}
		skipcount--;
	}
}

void CmdStop(EXECSTRUCT *Z, const TCHAR *param) // *stop
{
	int result = 1;
	BOOL nextitem = FALSE;

	if ( ((param[0] == '-') || (param[0] == '/')) && // nextitem option
		 (param[1] == 'n') &&
		 ((UTCHAR)param[2] <= ' ') ){
		param += 2;
		nextitem = TRUE;
	}

	if ( (*param == '\0') || (CalcString(&param, &result) == CALC_NOERROR) ){
		if ( result ){
			if ( nextitem == FALSE ){ // 実行中止
				Z->result = ERROR_CANCELLED;
			}else{ // 次へ
				Z->src += tstrlen(Z->src);
			}
		}
	}else{
		XMessage(Z->hWnd, T("*stop"), XM_GrERRld, MES_EPRM);
		Z->result = ERROR_INVALID_PARAMETER;
	}
}

void CmdIf(EXECSTRUCT *Z, const TCHAR *param)
{
	const TCHAR *paramback;
	int result;

	paramback = param;
	if ( CalcString(&param, &result) != CALC_NOERROR ){
		XMessage(NULL, NULL, XM_GrERRld, T("*if error"), paramback);
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}

	if ( result ) return;

	for (;;){ // 行末までスキップ
		TCHAR c;

		c = *Z->src;
		if ( (c == '\0') || (c == '\r') || (c == '\n') ) return;
		Z->src++;
	}
}

void CmdIfMatch(EXECSTRUCT *Z, TCHAR *param) // *ifmatch
{
	TCHAR wildcard[CMDLINESIZE], defparam[VFPS];
	DWORD MakeResult;
	int fnresult;
	FN_REGEXP fn;

	GetCommandParameter((const TCHAR **)&param, wildcard, TSIZEOF(wildcard));
	if ( NextParameter((const TCHAR **)&param) == FALSE ){
		GetValue(Z, 'C', defparam);
		param = defparam;
	}
	MakeResult = MakeFN_REGEXP(&fn, wildcard);
	if ( MakeResult & REGEXPF_ERROR ){
		XMessage(Z->hWnd, T("*ifmatch"), XM_GrERRld, MES_EWLD);
		Z->result = ERROR_CANCELLED;
		return;
	}

	if ( MakeResult & (REGEXPF_REQ_ATTR | REGEXPF_REQ_SIZE | REGEXPF_REQ_TIME) ){
		WIN32_FIND_DATA ff;

		if ( (param != defparam) ||
			 (PPxEnumInfoFunc(Z->Info, PPXCMDID_ENUMFINDDATA, (TCHAR *)&ff, &Z->EnumInfo) == 0) ){
			HANDLE hFF;

			VFSFixPath(wildcard, (TCHAR *)param, GetZCurDir(Z), VFSFIX_FULLPATH);
			hFF = FindFirstFileL(wildcard, &ff);
			if ( hFF == INVALID_HANDLE_VALUE ){
				DWORD fileattr;

				Z->result = GetLastError();
				if ( (Z->result == ERROR_FILE_NOT_FOUND) &&
					 (wildcard[3] == '\0') &&
					 (wildcard[2] == '\\') &&
					 ((fileattr = GetFileAttributesL(wildcard)) != BADATTR) ){
					// root dir.(C:\) をディレクトリ扱いにする
					memset(&ff, 0, sizeof(ff));
					ff.dwFileAttributes = fileattr;
					Z->result = NO_ERROR;
				}else{
					FreeFN_REGEXP(&fn);
					if ( MakeResult & REGEXPF_SILENTERROR ){
						Z->result = NO_ERROR;
						goto fin;
					}else{
						ErrorPathBox(Z->hWnd, T("*ifmatch"), wildcard, Z->result);
					}
					return;
				}
			}else{
				FindClose(hFF);
			}
		}
		fnresult = FinddataRegularExpression(&ff, &fn);
	}else{
		if ( param != defparam ){ // 文字列パラメータの整形
		// ※保存先は自身であり、必ず必要分存在するので保存先サイズは仮に32kとしている。
			TCHAR *bufptr;

			bufptr = param;
			if ( SkipSpace((const TCHAR **)&param) == '\"' ){
				GetLfGetParam((const TCHAR **)&param, bufptr, 0x8000);
			}else{
				TCHAR *dst, *dstlast;

				dst = bufptr;
				dstlast = dst;
				for (;;){
					TCHAR code;

					code = *param++;
					*dst++ = code;
					if ( (UTCHAR)code > ' ' ) dstlast = dst;
					if ( code == '\0' ) break;
				}
				*dstlast = '\0';
			}
			param = bufptr;
		}
		fnresult = FilenameRegularExpression(param, &fn);
	}
	FreeFN_REGEXP(&fn);
	if ( fnresult ) return; // 一致したのでそのまま実行
fin:
	for (;;){ // 行末までスキップ
		TCHAR c;

		c = *Z->src;
		if ( (c == '\0') || (c == '\r') || (c == '\n') ) return;
		Z->src++;
	}
}

void CmdCust(EXECSTRUCT *Z, TCHAR *line, BOOL reload) // *setcust / *customize
{
	TCHAR param[CMDLINESIZE];

	if ( SkipSpace((const TCHAR **)&line) == '@' ){ // filename mode
		line++;
		ZGetFilePathParam(Z, (const TCHAR **)&line, param);
		CustFile(Z, param, reload);
	}else{
		CustLine(Z, line, reload);
	}
}
/* test code
*linemessage 1: %*getcust("_User:a")
*linecust 1,_User:a=data1
*linecust 2,_User:a=data2
*linecust 3,_User:a=data3
*linecust 4,_User:a=data4
*linecust 5,_User:a=data5
*linemessage 2: %*getcust("_User:a")
*linecust 1,_User:a=data1+
*linemessage 3: %*getcust("_User:a")
*linecust 1,_User:a=a1-
*linemessage 4: %*getcust("_User:a")
*linecust 3,_User:a=data3+
*linemessage 5: %*getcust("_User:a")
*linecust 3,_User:a=a3-
*linemessage 6: %*getcust("_User:a")
*linecust 5,_User:a=a5-
*linemessage 7: %*getcust("_User:a")
*/
void CmdLineCust(EXECSTRUCT *Z, TCHAR *line) // *linecust
{
	TCHAR id[MAX_PATH], *idsep;
	TCHAR linebuf[CMDLINESIZE], *org_top, *orgline;
	TCHAR *custtext, separator, *sub, orgseparator;
	DWORD idsize;
	BOOL delmode = FALSE;
	TCHAR *nextline, *org_end = NilStrNC;
	ThSTRUCT thmakedata;
	DWORD top1st;

	if ( SkipSpace((const TCHAR **)&line) == '^' ){ // toggle
		delmode = TRUE;
		line++;
	}

	id[0] = '%';
	id[1] = 'm';
	GetCommandParameter((const TCHAR **)&line, id + 2, TSIZEOF(id) - 2);
	idsize = TSTRLENGTH32(id);
	NextParameter((const TCHAR **)&line);

	custtext = tstrchr(line, '=');
	idsep = tstrchr(line, ',');
	if ( custtext == NULL ){
		if ( idsep == NULL ) return;
		custtext = idsep;
	}else{
		if ( (idsep != NULL) && (custtext > idsep) ) custtext = idsep;
	}
	separator = *custtext;
	*custtext++ = '\0';

	sub = tstrchr(line, ':');
	if ( sub != NULL ) *sub++ = '\0';
	orgline = linebuf;
	PPcustCDumpText(line, sub, &orgline);

	orgseparator = '\0';
	org_top = orgline;
	if ( *org_top == '\t' ) org_top++;
	if ( (*org_top == '=') || (*org_top == ',') ) orgseparator = *org_top++;
	if ( *org_top == ' ' ) org_top++;
	nextline = org_top;
	for (;;){
		TCHAR c, *p1, *p;

		if ( *nextline == '\0' ) break;
		// 次の行頭を探す
		p = nextline;
		for (;;){
			c = *nextline;
			if ( c == '\0' ) break;
			if ( c == '\n' ){
				nextline++;
				while ( *nextline == '\n' ) nextline++;
				break;
			}
			nextline++;
		}
		p1 = p;
		if ( *p1 == '\t' ) p1++;
		if ( memcmp(p1, id, idsize) == 0 ){ // 登録済み？
			if ( p > org_top ) --p;
			*p = '\0';
			org_end = nextline;
			if ( delmode ) custtext = NilStrNC; // 削除する
			break;
		}
	}
	ThInit(&thmakedata);

	if ( sub != NULL ){
		thprintf(&thmakedata, 0, T("%s = {\n%s %c"), line, sub, separator);
	}else{
		thprintf(&thmakedata, 0, T("%s %c"), line, separator);
	}
	top1st = thmakedata.top;
//	data1st = destp;
	// 前部
	if ( *org_top != '\0' ){
		if ( ((*line == 'K') || (*line == 'E')) && (orgseparator == '=') ){
			ThCatString(&thmakedata, T("%%K\""));
		}
		ThCatString(&thmakedata, org_top);
	}
	// 同一行
	if ( *custtext != '\0' ){
		if ( *org_top != '\0' ) ThCatString(&thmakedata, T("\n\t"));
		thprintf(&thmakedata, 0, T("%s %s"), id, custtext);
	}
	// 後部
	if ( *org_end != '\0' ){
		if ( top1st != thmakedata.top ) ThCatString(&thmakedata, T("\n"));
		ThCatString(&thmakedata, org_end);
	}
	CustCmdSub(Z, ThStrT(&thmakedata), ThStrLastT(&thmakedata), FALSE);
	ThFree(&thmakedata);

	if ( orgline != linebuf ) HeapFree(ProcHeap, 0, orgline);
}

void CmdDeleteCust(const TCHAR *param)
{
	TCHAR *ptr, key[CMDLINESIZE], name[CMDLINESIZE], first;
	int index = -1;

	first = SkipSpace(&param);
	GetCommandParameter(&param, key, TSIZEOF(key));
	if ( key[0] == '\0' ) return;

	ptr = tstrchr(key, ':');
	if ( ptr != NULL ){ // key:name 形式
		*ptr = '\0';
		param = ptr + 1;
		if ( *param == '\0' ) return;
		tstrcpy(name, param);
	}else if ( NextParameter(&param) == FALSE ){ // "key" のみ
		if ( first != '\"' ) return;
		name[0] = '\0';
	}else if ( Isdigit(*param) ){ // key,index
		index = GetDigitNumber32(&param);
		if ( index < 0 ) return;
	}else if ( *param == '\"' ){ // key,"name"
		GetCommandParameter(&param, name, TSIZEOF(name));
		if ( name[0] == '\0' ) return;
	}else{
		return;
	}
	if ( index >= 0 ){
		DeleteCustTable(key, NULL, index);
	}else if ( name[0] != '\0' ){
		DeleteCustTable(key, name, 0);
	}else{
		DeleteCustData(key);
	}
}

void CmdAlias(EXECSTRUCT *Z, TCHAR *param)
{
	TCHAR *data;

	if ( GetSetParams(&param, &data) == FALSE ){
		HMENU hMenu;
		DWORD id = 1;
		int menupos;
		PPXMENUINFO xminfo;

		ThInit(&xminfo.th);
		xminfo.info = Z->Info;
		xminfo.commandID = 0;
		hMenu = PP_AddMenu(Z->Info, Z->hWnd, NULL, &id, T("A_exec"), &xminfo.th);
		PPxSetMenuInfo(hMenu, &xminfo);

		if ( hMenu != NULL ){
			menupos = TTrackPopupMenu(Z, hMenu, &xminfo);
			DestroyMenu(hMenu);
			if ( menupos ){
				const TCHAR *newsrc;

				newsrc = GetMenuDataString(&xminfo.th, menupos - 1);
				PP_ExtractMacro(Z->hWnd, Z->Info, Z->posptr, newsrc, NULL, Z->flags);
			}else{
				Z->result = ERROR_CANCELLED;
			}
		}
		ThFree(&xminfo.th);
	}else{
		if ( data != NULL ){
			SetCustStringTable(T("A_exec"), param, data, 0);
		}else{
			DeleteCustTable(T("A_exec"), param, 0);
		}
	}
}

void CmdPPbSet(EXECSTRUCT *Z, const TCHAR *param, const TCHAR *data)
{
	TCHAR fixbuf[CMDLINESIZE * 2], *butptr = fixbuf, *allocbuf;
	size_t len;

	len = tstrlen(param) + 16;
	if ( data != NULL ) len += tstrlen(data);
	if ( len >= TSIZEOF(fixbuf) ){
		allocbuf = HeapAlloc(DLLheap, 0, TSTROFF(len));
		if ( allocbuf == NULL ) return;
		butptr = allocbuf;
	}

	tstrcpy(butptr, T(">*set "));
	tstrcpy(butptr + 6, param);
	tstrcat(butptr + 6, T("="));
	if ( data != NULL ) tstrcat(butptr + 6, data);
	ComExecEx(Z->hWnd, butptr, GetZCurDir(Z), &Z->useppb, Z->flags, NULL);
	#pragma warning(suppress: 6001) // if 内で変更
	if ( butptr != fixbuf ) HeapFree(DLLheap, 0, allocbuf);
}

// *set
void CmdSet(EXECSTRUCT *Z, TCHAR *param)
{
	TCHAR *data;

	if ( IsTrue(GetSetParams(&param, &data)) ){
		TCHAR *paramlp, *allocbuf = NULL;

		paramlp = param + tstrlen(param) - 1;
		if ( *paramlp == '+' ){
			TCHAR fixbuf[CMDLINESIZE], *bufptr;
			DWORD esize, buflen;
			BOOL tail = FALSE;

			if ( (paramlp > param) && (*(paramlp - 1) == '+') ){
				paramlp--;
				tail = TRUE;
			}

			*paramlp = '\0';
			bufptr = fixbuf;
			esize = GetEnvironmentVariable(param, bufptr, TSIZEOF(fixbuf));
			buflen = esize + ((data != NULL) ? (tstrlen32(data) + 32) : 32);
			if ( buflen >= TSIZEOF(fixbuf) ){
				allocbuf = HeapAlloc(DLLheap, 0, TSTROFF(buflen));
				if ( allocbuf == NULL ){
					PPErrorBox(Z->hWnd, NULL, PPERROR_GETLASTERROR);
					return;
				}
				bufptr = allocbuf;
				esize = GetEnvironmentVariable(param, bufptr, buflen);
			}
			if ( esize && (data != NULL) ){ // 既存有り→追加するか判断
				TCHAR *ptr, *lp;
				size_t len;

				len = tstrlen(data);
				ptr = tstrstr(bufptr, data);
				if ( ptr != NULL ){
					lp = ptr + len;
					if ( !( ((ptr == bufptr) || (*(ptr - 1) == ';') ) &&
							((*lp == '\0') || (*lp == ';')) ) ){ // 未記載？
						ptr = NULL;
					}
				}
				if ( ptr != NULL ){
					if ( allocbuf != NULL ) HeapFree(DLLheap, 0, allocbuf);
					return; // 記載済みのため、処理せず
				}
				// 未記載なら、追加
				if ( tail ){ // 末尾追加
					size_t taillen;

					taillen = tstrlen(bufptr);
					if ( (taillen > 0) && (bufptr[taillen - 1] != ';') ){
						bufptr[taillen++] = ';';
					}
					tstrcpy(bufptr + taillen, data);
				}else{ // 先頭追加
					#pragma warning(suppress: 6385) // bufptr は Heap で増える
					memmove(bufptr + len + 1, bufptr, TSTROFF(esize + 1));
					memcpy(bufptr, data, TSTROFF(len));
					*(bufptr + len) = ';';
				}
				#ifndef _MSC_VER
					data = bufptr;
				#else // VS2008 の場合、data = pathbuf で、上の memcpy が省略される(data==pathbuf前提に処理する)最適化バグがあるので、回避する。
					if ( SetEnvironmentVariable(param, bufptr) == FALSE ){
						ErrorPathBox(Z->hWnd, NULL, param, PPERROR_GETLASTERROR);
					}
					if ( ((Z->flags & XEO_USEPPB) || (Z->useppb != -1)) &&
						 !(Z->flags & (XEO_NOUSEPPB | XEO_CONSOLE | XEO_USECMD)) ){
						CmdPPbSet(Z, param, bufptr);
					}
					if ( allocbuf != NULL ) HeapFree(DLLheap, 0, allocbuf);
					return;
				#endif
			}
		}
		if ( SetEnvironmentVariable(param, data) == FALSE ){
			ErrorPathBox(Z->hWnd, NULL, param, PPERROR_GETLASTERROR);
		}
		if ( ((Z->flags & XEO_USEPPB) || (Z->useppb != -1)) &&
			 !(Z->flags & (XEO_NOUSEPPB | XEO_CONSOLE | XEO_USECMD)) ){
			CmdPPbSet(Z, param, data);
		}
		if ( allocbuf != NULL ) HeapFree(DLLheap, 0, allocbuf);
	}else{ // 一覧表示
		HMENU hMenu;
		TCHAR *envptr;
		const TCHAR *p;

		hMenu = CreatePopupMenu();
		p = envptr = GetEnvironmentStrings();
		while ( *p != '\0' ){
			AppendMenuString(hMenu, 0, p);
			p += tstrlen(p) + 1;
		}
		FreeEnvironmentStrings(envptr);
		TTrackPopupMenu(Z, hMenu, NULL);
		DestroyMenu(hMenu);
	}
}

void USEFASTCALL CmdCursor(EXECSTRUCT *Z, const TCHAR *param) // *cursor
{
	int ibuf[CURSOR_PARAM_ITEMS + 1], *ip;

	ip = ibuf;
	memset(ibuf, 0, sizeof(ibuf));
	for (;;){
		TCHAR numbuf[100], chr;
		const TCHAR *numptr;

		numbuf[0] = '\0';
		GetCommandParameter(&param, numbuf, TSIZEOF(numbuf));

		numptr = numbuf;
		*ip++ = GetIntNumber(&numptr);
		ibuf[CURSOR_PARAM_ITEMS]++;
		chr = SkipSpace(&param);
		if ( (chr == '\0') || (ip >= (ibuf + CURSOR_PARAM_ITEMS)) ) break;
		if ( chr != ',' ) break;
		param++;
	}
	PPxInfoFunc(Z->Info, PPXCMDID_MOVECSR, &ibuf);
}

void USEFASTCALL CmdPPeEdit(EXECSTRUCT *Z, const TCHAR *param)
{
	HWND hEwnd;
	MSG msg;
	PPE_CMDMODEDATA pc;

	if ( ConsoleMode >= ConsoleMode_ConsoleOnly ){
		PrintToConsole(T("** PPe not support on console **\r\n"));
		Z->result = ERROR_CANCELLED;
		return;
	}

	pc.dummy = NULL;
	pc.param = param;
	pc.curdir = GetZCurDir(Z);

	hEwnd = PPEui(Z->hWnd, (const TCHAR *)&pc, PPE_TEXT_CMDMODE);

	if ( Z->command == CID_EDIT ){ // *edit は PPe を閉じるまで待つ
		while ( IsWindow(hEwnd) ){
			if ( (int)GetMessage(&msg, NULL, 0, 0) <= 0 ) break;
			if ( DialogKeyProc(&msg) ) continue;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

// %I / %Q / *msgbox / *messagebox
void USEFASTCALL CmdMessageBox(EXECSTRUCT *Z, TCHAR *param)
{
	DWORD style = MB_ICONINFORMATION | MB_PPX_PLAIN_TEXT;

	if ( Z->command == 'Q' ){
		// 繰り返し実行中なら、前回に既に ok を押しているので確認しない
	 	if ( Z->status & ST_2ndLOOP ) return;
		if ( (*param == 'N') || (*param == 'C') ){
			param++;
			style = MB_ICONQUESTION | MB_OKCANCEL | MB_DEFBUTTON2 | MB_PPX_PLAIN_TEXT;
		}else{
			if ( (*param == 'Y') || (*param == 'O') ) param++;
			style = MB_ICONQUESTION | MB_OKCANCEL | MB_PPX_PLAIN_TEXT;
		}
	}
	if ( SkipSpace((const TCHAR **)&param) == '\"' ){
		TCHAR *src;

		src = param;
		GetLfGetParam((const TCHAR **)&param, param, tstrlen32(param) + 1 );
		param = src;
	}
	if ( IDOK != PMessageBox(Z->hWnd, param, ZGetTitleName(Z), style) ){
		Z->result = ERROR_CANCELLED;
	}
}

void USEFASTCALL CmdKeyCommand(EXECSTRUCT *Z, const TCHAR *param) // %K, *key
{
	HWND hWnd;

	hWnd = GetPPxhWndFromID(Z->Info, &param, NULL);
	if ( hWnd == BADHWND ) return; // 該当無し
	if ( hWnd != NULL ){ // 指定あり
		if ( SkipSpace(&param) == ',' ) param++;
	}

	if ( *param != '\"' ){
		XMessage(Z->hWnd, T("%K"), XM_GrERRld, MES_EPRM);
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}
	param++;

	for ( ; ; ){
		UTCHAR c;
		int key;

		c = SkipSpace(&param);
		if ( c < ' ' ) break;
		if ( c == '\"' ){
			param++;
			break;
		}
		key = GetKeyCode(&param);
		if ( key < 0 ){
			XMessage(Z->hWnd, T("%K"), XM_GrERRld, MES_EPRM);
			Z->result = ERROR_INVALID_PARAMETER;
			break;
		}
		if ( key == K_NULL ){
			Sleep(100);
			continue;
		}
		if ( hWnd == NULL ){ // 自分自身で実行
			ERRORCODE result;

			result = PPxInfoFunc32u(Z->Info, PPXCMDID_PPXCOMMAD, &key);
			if ( result > 1 ){ // NO_ERROR, ERROR_INVALID_FUNCTION 以外
				if ( (result == ERROR_CANCELLED) || !(Z->flags & XEO_IGNOREERR) ){
					Z->result = result;
					break;
				}
			}
		}else{ // 他のPPxに送信
			PostMessage(hWnd, WM_PPXCOMMAND, (WPARAM)key, 0);
		}
	}
}

void CmdPPv(EXECSTRUCT *Z, const TCHAR *paramptr, TCHAR *param) // %v
{
	TCHAR param2[CMDLINESIZE];
	PPXCMD_F pcmdf;

	pcmdf.dest[0] = '\0';
	GetCommandParameter(&paramptr, param, VFPS);
	if ( *param != '\0' ){
		pcmdf.source = T("VDN");
		Get_F_MacroData(Z->Info, &pcmdf, &Z->EnumInfo);
		VFSFixPath(param2, param, pcmdf.dest, VFSFIX_PATH | VFSFIX_NOFIXEDGE);
		PPxView(Z->hWnd, param2, 0);
	}else{
		pcmdf.source = T("VCDN");
		Get_F_MacroData(Z->Info, &pcmdf, &Z->EnumInfo);
		PPxView(Z->hWnd, pcmdf.dest, 0);
	}
}

void CmdVFS(const TCHAR *paramptr) // *vfs
{
	if ( CheckParamOnOff(&paramptr) ){
		VFSOn(VFS_DIRECTORY);
	}else{
		VFSOff();
	}
}

void CmdZap(EXECSTRUCT *Z, const TCHAR *paramptr, TCHAR *param)
{
	TCHAR param2[CMDLINESIZE];
	const TCHAR *command;
	POINT pos;

	GetCommandParameter(&paramptr, param, CMDLINESIZE);
	if ( *paramptr == ',' ){
		paramptr++;
		GetCommandParameter(&paramptr, param2, CMDLINESIZE);
		command = *param2 ? param2 : NilStr;
	}else{
		command = NULL;
	}
	if ( *param == '\0' ){	// ファイル名が省略されていた
		GetValue(Z, 'C', param);
	}
	if ( Z->command == 'Z' ){	// %Z
		ZapMain(Z, command, param, NilStr, GetZCurDir(Z));
	}else{	// %z
		GetPopupPoint(Z, &pos);
		VFSSHContextMenu(Z->hWnd, &pos, GetZCurDir(Z), param, command);
	}
}

void CreateFileList(EXECSTRUCT *Z, ThSTRUCT *thFiles)
{
	PPXCMD_F buf;

	for ( ; ; ){
		buf.source = FullPathMacroStr;
		buf.dest[0] = '\0';
		Get_F_MacroData(Z->Info, &buf, &Z->EnumInfo);
		ThAddString(thFiles, buf.dest);
		if ( PPxEnumInfoFunc(Z->Info, PPXCMDID_NEXTENUM, buf.dest, &Z->EnumInfo)== 0){
			setflag(Z->status, ST_LOOPEND);
			break;
		}
	}
}

void CmdTaskProgress(EXECSTRUCT *Z, const TCHAR *param, TCHAR *buf)
{
	int nowcount = 50, maxcount = 100;
	TCHAR title[CMDLINESIZE], *p, *more;
	if ( WinType < WINTYPE_7 ) return; // 対応していない

	title[0] = '\0';
	for (;;){
		if ( GetOptionParameter(&param, buf, &more) != '-' ){
			if ( buf[0] == '\0' ) break;
			p = buf;
		}else{
			p = buf + 1;
		}
		if ( tstricmp(p, T("CLOSE")) == 0 ){
			if ( hTaskProgressWnd != NULL ){
				DestroyWindow(hTaskProgressWnd);
				hTaskProgressWnd = NULL;
			}
			return;
		}else if ( tstricmp(p, T("TITLE")) == 0 ){
			tstrcpy(title, more);
		}else if ( tstricmp(p, T("OFF")) == 0 ){
			nowcount = TBPF_NOPROGRESS;
			maxcount = 0;
		}else if ( tstricmp(p, T("LOOP")) == 0 ){
			nowcount = TBPF_INDETERMINATE;
			maxcount = 0;
		}else if ( tstricmp(p, T("NORMAL")) == 0 ){
			nowcount = TBPF_NORMAL;
			maxcount = 0;
		}else if ( tstricmp(p, T("ERROR")) == 0 ){
			nowcount = TBPF_ERROR;
			maxcount = 0;
		}else if ( tstricmp(p, T("PAUSE")) == 0 ){
			nowcount = TBPF_PAUSED;
			maxcount = 0;
		}

		if ( Isdigit(*p) ){ // key,index
			nowcount = GetDigitNumber32((const TCHAR **)&p);
			if ( SkipSpace((const TCHAR **)&p) != '\0' ){
				if ( !Isdigit(*p) ) p++;
				if ( Isdigit(*p) ){
					maxcount = GetDigitNumber32((const TCHAR **)&p);
				}
			}
		}
	}

	if ( hTaskProgressWnd == NULL ){
		hTaskProgressWnd = CreateDummyWindow((Z->Info->hWnd != NULL) ? Z->Info->hWnd : GetForegroundWindow(), title);
		if ( hTaskProgressWnd == NULL ) return;
	}else if ( title[0] != '\0' ){
		SetWindowText(hTaskProgressWnd, title);
	}
	SetTaskBarButtonProgress(hTaskProgressWnd, nowcount, maxcount);
}

void CmdShellMenu(EXECSTRUCT *Z, const TCHAR *param, TCHAR *buf)
{
	TCHAR verb[CMDLINESIZE];
	POINT pos;
	LPITEMIDLIST idl = NULL, *pidl;
	LPSHELLFOLDER pSF;
	int flags_pieces = SHCM_NOSHIFTKEY;
	BOOL multi = FALSE;
	ThSTRUCT thFiles;
	int params = 0;

	ThInit(&thFiles);
	verb[0] = '\1';
	for (;;){
		if ( GetOptionParameter(&param, buf, NULL) != '-' ){
			if ( buf[0] == '\0' ) break;
			ThAddString(&thFiles, buf);
			params++;
		// 動作オプション
		}else if ( tstrcmp(buf + 1, T("MARKS")) == 0 ){
			multi = TRUE;
		}else if ( tstrcmp(buf + 1, T("EXTEND")) == 0 ){
			setflag(flags_pieces, SHCM_EXTENDED);
		}else if ( tstrcmp(buf + 1, T("ITEMMENU")) == 0 ){
			setflag(flags_pieces, SHCM_ITEMMENU);
		}else if ( tstrcmp(buf + 1, T("VIEWPANE")) == 0 ){
			setflag(flags_pieces, SHCM_VIEWPANE);
		}else if ( tstrcmp(buf + 1, T("SHIFTKEY")) == 0 ){
			resetflag(flags_pieces, SHCM_NOSHIFTKEY);
		// 各種動詞
		}else{
			tstrcpy(verb, buf + 1);
		}
	}

	if ( multi ){
		CreateFileList(Z, &thFiles);
		if ( thFiles.bottom == NULL ) return; // 該当無し
		params = 2;
	}
	if ( (thFiles.bottom == NULL) || (params == 0) ){	// ファイル名が省略されていた
		GetValue(Z, 'C', buf);
		ThAddString(&thFiles, buf);
		params = 1;
	}
	if ( params == 1 ){
		if ( VFSMakeIDL(GetZCurDir(Z), &pSF, &idl, (const TCHAR *)thFiles.bottom) == FALSE ){
			params = 0;
		}
		pidl = &idl;
		flags_pieces++;
	}else{
		params = MakePIDLTableFromFileZ(GetZCurDir(Z), (const TCHAR *)thFiles.bottom, &pidl, &pSF);
	}
	ThFree(&thFiles);
	if ( params <= 0 ) return;

	GetPopupPoint(Z, &pos);
	SHContextMenu(Z->hWnd, &pos, pSF, pidl, flags_pieces | params,
			(*verb == '\1') ? NULL : verb);
	FreePIDLS(pidl, params);
	pSF->lpVtbl->Release(pSF);
}

struct ENUMEXECUTEINFO {
	PPXAPPINFO info, *parent;
	HANDLE hFF;
	WIN32_FIND_DATA ff;
	EXECSTRUCT *Z;
	FN_REGEXP fn;
	BOOL detail; // ff を全て手動で埋めたか
};

DWORD_PTR USECDECL CmdEnumExecuteInfo(struct ENUMEXECUTEINFO *info, DWORD cmdID, PPXAPPINFOUNION *uptr)
{
	switch(cmdID){
		case PPXCMDID_STARTENUM:
		case PPXCMDID_STARTNOENUM:
		case PPXCMDID_ENDENUM:
			return PPXA_NO_ERROR;
		case PPXCMDID_TRIMENUM:
			return PPXA_INVALID_FUNCTION;

		case '1':
			tstrcpy(uptr->enums.buffer, GetZCurDir(info->Z));
			break;

		case 'C':
			tstrcpy(uptr->enums.buffer, info->ff.cFileName);
			break;

		case 'F':
		case 'R':
		case 'X':
		case 'Y':
		case 'T':
		case 't':
			*uptr->enums.buffer = '\0';
			return PPXA_INVALID_FUNCTION;

		case PPXCMDID_NEXTENUM:
			for (; VFSFindNext(info->hFF, &info->ff); ){
				if ( !IsRelativeDir(info->ff.cFileName) ){
					if ( FinddataRegularExpression(&info->ff, &info->fn) ){
						return 1;
					}
				}
			}
			return 0;

		case PPXCMDID_ENUMFINDDATA:
			if ( info->ff.dwFileAttributes & FILE_ATTRIBUTEX_NODETAIL ){
				return PPXA_INVALID_FUNCTION;
			}
			*(WIN32_FIND_DATA *)uptr->enums.buffer = info->ff;
			break;

		case PPXCMDID_ENUMATTR:
			if ( info->ff.dwFileAttributes & FILE_ATTRIBUTEX_NODETAIL ){
				TCHAR *last;

				*(DWORD *)uptr->enums.buffer = 0;
				last = VFSFindLastEntry(info->ff.cFileName);
				if ( (*last == '\\') || (*last == '/') ){
					if ( (*(last + 1) == '\0') || (*(last + 1) == '*') ){
						*(DWORD *)uptr->enums.buffer = FILE_ATTRIBUTE_DIRECTORY;
					}
				}
			}else{
				*(DWORD *)uptr->enums.buffer = info->ff.dwFileAttributes;
			}
			break;

		default:
			return info->parent->Function(info->parent, cmdID, uptr);
	}
	return PPXA_NO_ERROR;
}

void CmdEnumExecute(EXECSTRUCT *Z, const TCHAR *param) // *execute @(列挙実行)
{
	struct ENUMEXECUTEINFO eei;
	TCHAR filename[VFPS + 20], optbuf[CMDLINESIZE];
	DWORD attr;
	BOOL use_separator;

	GetCommandParameter(&param, filename, TSIZEOF(filename));
	VFSFullPath(NULL, filename, GetZCurDir(Z));
	use_separator = tstrstr(filename, T("::")) != NULL;

	if ( !use_separator ){
		attr = GetFileAttributesL(filename);
		if ( attr == BADATTR ){
			ErrorPathBox(Z->hWnd, NULL, filename, PPERROR_GETLASTERROR);
			return;
		}
	}

	MakeFN_REGEXP(&eei.fn, NilStr);

	if ( SkipSpace(&param) == ',' ){
		param++;
	}else{
		if ( GetOptionParameter(&param, optbuf, NULL) != '-' ){
			param = T("%OC *msgbox \"empty command\n%#nFC\"");
		}else if ( tstrcmp(optbuf + 1, T("MASK")) == 0 ){
			MakeFN_REGEXP(&eei.fn, optbuf);
		}
	}
	tstrcat(filename, (use_separator || (attr & FILE_ATTRIBUTE_DIRECTORY)) ? T("\\*") :T("::listfile\\*"));

	eei.hFF = VFSFindFirst(filename, &eei.ff);
	if ( eei.hFF == INVALID_HANDLE_VALUE ){
		ERRORCODE err;

		err = GetLastError();
		if ( err != ERROR_NO_MORE_FILES ){
			ErrorPathBox(Z->hWnd, NULL, filename, err);
		}
		FreeFN_REGEXP(&eei.fn);
		return;
	}

	eei.info.Function = (PPXAPPINFOFUNCTION)CmdEnumExecuteInfo;
	eei.info.Name = T("*execute @");
	eei.info.RegID = Z->Info->RegID;
	eei.info.hWnd = Z->Info->hWnd;
	eei.parent = Z->Info;
	eei.Z = Z;

	do {
		if ( IsRelativeDir(eei.ff.cFileName) ) continue;
		if ( FinddataRegularExpression(&eei.ff, &eei.fn) == FALSE ) continue;
		Z->result = PP_ExtractMacro(Z->hWnd, &eei.info, NULL, param, NULL, 0);
		break;
	} while ( IsTrue(VFSFindNext(eei.hFF, &eei.ff)) );
	VFSFindClose(eei.hFF);
	FreeFN_REGEXP(&eei.fn);
}

void CmdPPbExecute(EXECSTRUCT *Z, const TCHAR *param) // *execute B(PPbで実行)
{
	int useID;

	useID = GetPPxRegID(&param);
	param = tstrchr(param, ',');
	if ( (useID >= 0) && (param != NULL) ){
		if ( (Sm->P[useID].ID[0] == 'B') &&
			 (Sm->P[useID].ID[1] == '_') &&
			 ((Sm->P[useID].UsehWnd == Z->hWnd) ||
			  (Sm->P[useID].UsehWnd == BADHWND)) ){
			Sm->P[useID].UsehWnd = Z->hWnd;

			if ( LongSendPPB(Z->hWnd, GetZCurDir(Z), 'L', useID) >= 0 ){ // >L
				LongSendPPB(Z->hWnd, param + 1, 'K', useID); // >K
				FreePPb(Z->hWnd, useID);
				return;
			}
			FreePPb(Z->hWnd, useID);
		}
	}
	XMessage(NULL, NULL, XM_GrERRld, T("no PPb or busy"));
	Z->result = ERROR_BAD_UNIT;
}

void CmdExecute(EXECSTRUCT *Z, const TCHAR *param) // *execute
{
	HWND hWnd;
	COPYDATASTRUCT copydata;
	TCHAR topchr;

	topchr = SkipSpace(&param);
	if ( topchr == '@' ){
		CmdEnumExecute(Z, param + 1);
		return;
	}
	if ( TinyCharLower(topchr) == 'b' ){
		CmdPPbExecute(Z, param);
		return;
	}
	hWnd = GetPPxhWndFromID(Z->Info, &param, NULL);
	if ( hWnd == BADHWND ) return; // 該当無し

	if ( SkipSpace(&param) == ',' ) param++;
	if ( hWnd == NULL ){ // 指定がない→自前実行
		Z->result = PP_ExtractMacro(Z->hWnd, Z->Info, NULL, param, NULL, 0);
	}else{ // 指定有り→指定PPxで実行
		copydata.dwData = 'H';
		copydata.cbData = TSTRSIZE32(param);
		copydata.lpData = (PVOID)param;
		SendMessage(hWnd, WM_COPYDATA, 0, (LPARAM)&copydata);
	}
}

// *file
void CmdFile(EXECSTRUCT *Z, TCHAR *olddir, const TCHAR *pptr, TCHAR *param)
{
	VFSFILEOPERATION fileop;
	TCHAR param2[CMDLINESIZE];

	fileop.src		= NULL;
	fileop.dest		= NULL;
	fileop.files	= NULL;
	fileop.option	= NULL;
	fileop.dtype	= VFSDT_UNKNOWN;
	fileop.info		= Z->Info;
	fileop.flags	= VFSFOP_FREEFILES;
	fileop.hReturnWnd = Z->hWnd;
								// 第１パラメータ：種類
	if ( SkipSpace(&pptr) == '!' ){ // autorun
		pptr++;
		setflag(fileop.flags, VFSFOP_AUTOSTART);
	}
	if ( SkipSpace(&pptr) == '\"' ){ // actionname
		GetQuotedParameter(&pptr, param, param + VFPS - 1);
	}else{
		TCHAR *dest, code;

		dest = param;
		for ( ;; ){
			code = *pptr;
			if ( (code == '\0') || (code == ',') ||
				 (code == ' ')  || (code == '\t') ){
				break;
			}
			*dest++ = code;
			pptr++;
		}
		*dest = '\0';
	}
	fileop.action = param;
	if ( SkipSpace(&pptr) == ',' ){	// 旧形式 // 第２パラメータ：コピー元
		pptr++;
		GetCommandParameter(&pptr, param2, VFPS);
		if ( param2[0] != '\0' ){
			ZSetCurrentDir(Z, olddir);
			fileop.src = Z->curdir; // ZSetCurrentDir で取得済み
			fileop.files = MakeFOPlistFromParam(param2, fileop.src, &fileop.flags);

			if ( fileop.files == NULL ){
				XMessage(Z->hWnd, NULL, XM_FaERRd, MES_ELTA);
				Z->result = ERROR_INVALID_PARAMETER;
				return;
			}
		}
								// 第３パラメータ：コピー先
		if ( *pptr == ',' ){
			pptr++;
			GetCommandParameter(&pptr, param2, VFPS);
			fileop.dest = param2;
							// 第４パラメータ：オプション
			SkipSpace(&pptr);
			if ( *pptr == ',' ) pptr++;
			if ( *pptr != '\0' ) fileop.option = pptr;
		}
	}else if ( *pptr != '\0' ){			// 新形式
		fileop.option = pptr;
	}
	if ( PPxFileOperation(NULL, &fileop) == FALSE ){
		Z->result = ERROR_CANCELLED;
	}
}

// １コマンド文を実行する -----------------------------------------------------
void ZExec(EXECSTRUCT *Z)
{
	TCHAR *param;
	TCHAR *lp;
	TCHAR linebuf[CMDLINESIZE];
	TCHAR olddir[VFPS];
	EXEC_EXTRA extra;

	olddir[0] = '\0';
	param = lp = Z->DstBuf;
	if ( Z->ExtendDst.top != 0 ){
		ThCatString(&Z->ExtendDst, Z->DstBuf);
		param = lp = (TCHAR *)Z->ExtendDst.bottom;
		lp += Z->ExtendDst.top / sizeof(TCHAR);
		Z->ExtendDst.top = 0;
		resetflag(Z->status, ST_EXTENDDST);
	}else{
		lp += tstrlen(lp);
	}
	while ( lp > param ){ // 末尾の空白を除去
		if ( *(lp - 1) != ' ' ) break;
		*(--lp) = '\0';
	}

	if ( Z->func.off != 0 ){
		XMessage(Z->hWnd, NULL, XM_GrERRld, MES_EMUT, param + Z->func.off - 1);
		Z->result = ERROR_INVALID_PARAMETER;
		return;
	}

	SkipSpace((const TCHAR **)&param);
	if ( Z->flags & XEO_EXTRACTLOG ) ZExtractLog(Z, param);
	switch( Z->command ){
		case CID_FILE_EXEC:				// 外部プロセスを実行
			if ( *param == '\0' ) return;
			ZSetCurrentDir(Z, olddir);
			if ( Z->flags & XEO_CONSOLE ){
				ExecOnConsole(Z, param, Z->curdir);
				break;
			}
			extra.mask = 0;
			extra.ExitCode = &Z->ExitCode;
			// Z->curdir ... ZSetCurrentDir で取得済み
			if ( ComExecEx(Z->hWnd, param, Z->curdir, &Z->useppb, Z->flags, &extra) == FALSE ){
				Z->result = GetLastError();
				if ( Z->result == NO_ERROR ) Z->result = ERROR_PATH_NOT_FOUND;
			}
			break;

		case CID_CLOSEPPX:
			CmdClosePPx(param);
			break;
										// *screensaver スクリーンセーバを起動
		case CID_SCREENSAVER:
			PPxCommonCommand(Z->hWnd, 0, K_SSav);
			break;
										// *logoff
		case CID_LOGOFF:
			ExitSession(Z->hWnd, EWX_LOGOFF);
			break;
										// *poweroff
		case CID_POWEROFF:
			ExitSession(Z->hWnd, EWX_POWEROFF);
			break;
										// *reboot
		case CID_REBOOT:
			ExitSession(Z->hWnd, EWX_REBOOT);
			break;
										// *shutdown
		case CID_SHUTDOWN:
			ExitSession(Z->hWnd, EWX_SHUTDOWN);
			break;
										// *terminate
		case CID_TERMINATE:
			ExitSession(Z->hWnd, EWX_FORCE);
			break;
										// *suspend
		case CID_SUSPEND:
			ExitSession(Z->hWnd, EWX_EX_SUSPEND);
			break;
										// *hibernate
		case CID_HIBERNATE:
			ExitSession(Z->hWnd, EWX_EX_HIBERNATE);
			break;
										// *lockpc
		case CID_LOCKPC:
			LockPC();
			break;
										// *httpget
		case CID_HTTPGET:
			CmdHttpGet(Z, param);
			break;
										// *cliptext
		case CID_CLIPTEXT:
			ClipTextData(Z->hWnd, param);
			break;
										// *selectppx
		case CID_SELECTPPX:
			CmdSelectPPx(Z, param);
			break;

		case CID_STOP:
			CmdStop(Z, param);
			break;
										// *nextitem
		case CID_NEXTITEM:
			CmdNextItem(Z, param);
			break;
										// *cd
		case CID_CD:
			ZGetFilePathParam(Z, (const TCHAR **)&param, GetZCurDir(Z));
			break;
										// *cursor
		case CID_CURSOR:
			CmdCursor(Z, param);
			break;
										// *ppe / *edit
		case CID_PPE:
		case CID_EDIT:
			CmdPPeEdit(Z, param);
			break;

		case CID_FILE:
			CmdFile(Z, olddir, param, linebuf);
			break;
										// *insert, *insertsel
		case CID_INSERT:
		case CID_INSERTSEL:
			GetLineString(param, param, lp - param + 1);
			if ( Z->flags & XEO_CONSOLE ){
				Z->result = PPxInfoFunc32u(Z->Info,
						(Z->command == CID_INSERT) ?
							PPXCMDID_INSERT : PPXCMDID_INSERTSEL,
						param);
			}else{
				SendMessage(Z->hWnd, WM_PPXCOMMAND,
						(Z->command == CID_INSERT) ? KE_insert : KE_insertsel,
						(LPARAM)param);
			}
			break;
										// *replace
		case CID_REPLACE:
			GetLineString(param, param, lp - param + 1);
			if ( Z->flags & XEO_CONSOLE ){
				Z->result = PPxInfoFunc32u(Z->Info, PPXCMDID_REPLACE, param);
			}else{
				SendMessage(Z->hWnd, WM_PPXCOMMAND, KE_replace, (LPARAM)param);
			}
			break;

		case CID_FOCUS:
			CmdFocusPPx(Z, param);
			break;
										// *set
		case CID_SET:
			CmdSet(Z, param);
			break;
										// *forfile
		case CID_FORFILE:
			GetCommandParameter((const TCHAR **)&param, linebuf, TSIZEOF(linebuf));
			if ( *param == ',' ) param++;
			CmdDoForfile(Z, linebuf, param);
			break;
										// *alias
		case CID_ALIAS:
			CmdAlias(Z, param);
			break;

		case CID_CUSTOMIZE:
			CmdCust(Z, param, TRUE);
			break;

		case CID_SETCUST:
			CmdCust(Z, param, FALSE);
			break;

		case CID_LINECUST:
			CmdLineCust(Z, param);
			break;
										// *deletecust
		case CID_DELETECUST:
			CmdDeleteCust(param);
			break;
										// *monitoroff
		case CID_MONITOROFF:
			SendWmSysyemMessage(SC_MONITORPOWER, 2);
			break;

		case CID_EXECUTE:
			CmdExecute(Z, param);
			break;
										// *ppc
		case CID_PPC:
			ComZExecPPx(Z, PPcExeName, param);
			break;
										// *ppv
		case CID_PPV:
			ComZExecPPx(Z, PPvExeName, param);
			break;
										// *ppb
		case CID_PPB:
			ComZExecPPx(Z, T(PPBEXE), param);
			break;
										// *pptray
		case CID_PPTRAY:
			ComZExecPPx(Z, T(PPTRAYEXE), param);
			break;
										// *ppffix
		case CID_PPFFIX:
			ComZExecPPx(Z, T(PPFFIXEXE), param);
			break;
										// *ppcust
		case CID_PPCUST:
			ComZExecPPx(Z, T(PPCUSTEXE), param);
			break;
										// *freediruse
		case CID_FREEDRIVEUSE:
			PPxSendMessage(WM_PPXCOMMAND, K_FREEDRIVEUSE, upper(*param));
			Sleep(500); // 完全に開放されるまで少し待機
			break;
										// *launch
		case CID_LAUNCH:
			CmdRunFunction(Z, param, Run_LAUNCH);
			break;

		case CID_START:
			CmdStart(Z, param);
			break;
										// *job
		case CID_JOB:
			if ( JobListMenu(Z, SkipSpace((const TCHAR **)&param)) == FALSE ){
				Z->result = ERROR_CANCELLED;
			}
			break;

		case CID_TIP:
			CmdShowTip(Z, param);
			break;
										// *help
		case CID_HELP:
			GetCommandParameter((const TCHAR **)&param, linebuf, VFPS);
			if ( linebuf[0] != '\0' ){
				PPxHelp(Z->hWnd, HELP_KEY, (DWORD_PTR)linebuf);
			}else{
				PPxHelp(Z->hWnd, HELP_FINDER, 0);
			}
			break;
										// *makedir
		case CID_MAKEDIR:
			CmdMakeDirectory(Z, param);
			break;
										// *makefile
		case CID_MAKEFILE:
			CmdMakeFile(Z, param);
			break;
										// *delete
		case CID_DELETE:
			CmdDeleteEntry(Z, param);
			break;
										// *rename
		case CID_RENAME:
			CmdRenameEntry(Z, param);
			break;
										// *linemessage
		case CID_LINEMESSAGE:
			if ( PPxInfoFunc(Z->Info, PPXCMDID_SETPOPLINE, (void *)param) ){
				break;
			}
			// 表示できなかったとき
			XMessage(Z->hWnd, NULL, XM_INFOld, T("%s"), param);
			break;
										// *commandhash
		case CID_COMMANDHASH: {
			DWORD hash;

			GetCommandParameter((const TCHAR **)&param, linebuf, TSIZEOF(linebuf));
			hash = GetModuleNameHash(linebuf, linebuf);
			thprintf(linebuf + tstrlen(linebuf), 64, T(" is 0x%x"), hash);
			tInput(Z->hWnd, T("COMMANDHASH"), linebuf, TSIZEOF(linebuf), PPXH_GENERAL, PPXH_GENERAL);
			break;
		}
										// *setarchivecp
		case CID_SETARCHIVECP:
			#ifdef UNICODE
				CmdSetArchiveCP(Z, param);
			#else
				SendMessage(Z->hWnd, WM_PPXCOMMAND, TMAKEWPARAM(K_SETPOPMSG, POPMSG_MSG), (LPARAM)"Codepage change not support");
			#endif
			break;

		case 'I':						// %I 情報メッセージボックス
		case 'Q':						// %Q 確認メッセージボックス
		case CID_MSGBOX:
		case CID_MESSAGEBOX:
			CmdMessageBox(Z, param);
			break;

		case CID_KEY:
		// case 'K' へ
		case 'K':
			CmdKeyCommand(Z, param);
			break;

		/* *NOIME コマンド
			実行したプロセスでIMEを完全に使用できないようにする。
			ただし、ウィンドウを作成する前に実行しないと機能しないので、
			コマンドの公開はしていない。*/
		case CID_NOIME:
			PPxCommonExtCommand(K_IMEDISABLE, (WPARAM)-1);
			break;

		case CID_IME:
			SetIMEStatus(Z->hWnd, GetIntNumber((const TCHAR **)&param) > 0);
			break;
										// *flashwindow
		case CID_FLASHWINDOW: {
			HWND hWnd;

			hWnd = GetWindowHandleByText(Z->Info, param);
			if ( hWnd == NULL ) hWnd = Z->hWnd;
			PPxFlashWindow(hWnd, PPXFLASH_FLASH);
			break;
		}
										// *sound
		case CID_SOUND:
			ZGetFilePathParam(Z, (const TCHAR **)&param, linebuf);
			PlayWave(linebuf);
			break;
										// *wait
		case CID_WAIT:
			CmdWaitExtract(Z, param);
			break;

		case CID_SENDKEY:
//		case 'k': へ
		case 'k':
			CmdEmurateKeyInput(Z, param);
			break;
										// %j  パスジャンプ
		case 'j':
			GetCommandParameter((const TCHAR **)&param, linebuf, VFPS);
			PPxInfoFunc(Z->Info, PPXCMDID_CHDIR, linebuf);
			break;
										// *string  特殊環境変数
		case CID_STRING:
			ZStringVariable(Z, (const TCHAR **)&param, StringVariable_command);
			break;
										// *chopdir  直下のディレクトリを削除
		case CID_CHOPDIR:
			CmdChopDirectory(Z, param);
			break;
										// *if  条件を満たしたら、この行を実行
		case CID_IF:
			CmdIf(Z, param);
			break;
										// *ifmatch  条件を満たしたら、この行を実行
		case CID_IFMATCH:
			CmdIfMatch(Z, param);
			break;

		case CID_PACK:
			CmdPack(Z, param);
			break;
										// *togglecustword
		case CID_TOGGLECUSTWORD:
			CmdToggleCustWord(param);
			break;

		case CID_CHECKUPDATE:
			CmdCheckUpdate(Z, param);
			break;
										// *checksignature
		case CID_CHECKSIGNATURE:
			CmdCheckSignature(Z, param);
			break;
										// *addhistory
		case CID_ADDHISTORY:
			CmdAddHistory(param);
			break;
										// *deletehistory
		case CID_DELETEHISTORY:
			CmdDeleteHistory(param);
			break;
										// *trimmark
		case CID_TRIMMARK:
			PPxEnumInfoFunc(Z->Info, PPXCMDID_TRIMENUM, Z->DstBuf, &Z->EnumInfo);
			break;
										// *psinfo
		case CID_PSINFO:
			ProcessInfo();
			break;
										// *clearauth
		case CID_CLEARAUTH:
			AuthHostCache[0] = '\0';
			break;
										// *return
		case CID_RETURN:
			GetCommandParameter((const TCHAR **)&param, Z->DstBuf, CMDLINESIZE);
			Z->result = ERROR_EX_RETURN;
			Z->dst = Z->DstBuf + tstrlen(Z->dst);
			Z->src += tstrlen(Z->src);
			break;
										// *maxlength
		case CID_MAXLENGTH:
			Z->LongResultLen = GetDigitNumber32u((const TCHAR **)&param);
			if ( Z->LongResultLen == 0 ) Z->LongResultLen = 31 * KB;
			setflag(Z->status, ST_LONGRESULT);
			break;
/*
		case CID_GOTO: // *goto は PP_ExtractMacro で処理済み
			CmdGoto(Z, param);
			break;
*/
		case CID_RUN:
			CmdRunFunction(Z, param, Run_COMMAND);
			break;

		case CID_SIGNAL:
			CmdSignal(Z, param);
			break;

		case CID_REPORTDIALOG:
			CmdReportDialog(param);
			break;

		case CID_AUX:
			CmdAux(Z, param);
			break;

		case CID_TOUCH:
			CmdTouch(Z, param);
			break;
										// *jumpentry
		case CID_JUMPENTRY:
//		case 'J': へ
		case 'J':						// %J  エントリ移動
			GetCommandParameter((const TCHAR **)&param, linebuf, VFPS);
			PPxInfoFunc(Z->Info, PPXCMDID_PATHJUMP, linebuf);
			break;
									// %u [/]DllName,[!all,]param  UnXXX を実行
		case 'u':
			GetCommandParameter((const TCHAR **)&param, linebuf, VFPS);
			if ( *param != ',' ){
				XMessage(Z->hWnd, T("%u"), XM_GrERRld, MES_EPRM);
				Z->result = ERROR_INVALID_PARAMETER;
			}else{
				tstrreplace((TCHAR *)param + 1, T(">"), T(" "));
				ZSetCurrentDir(Z, olddir);
				if ( DoUnarc(Z->Info, linebuf, Z->hWnd, param + 1) != 0 ){
					Z->result = ERROR_BAD_COMMAND;
				}
			}
			break;
										// %v  PPV で表示
		case 'v':
			CmdPPv(Z, param, linebuf);
			break;

		case 'z':						// %z  ShellContextMenu で実行
		case 'Z':						// %Z  ShellExecute で実行
			CmdZap(Z, param, linebuf);
			break;

		case CID_SHELLMENU:
			CmdShellMenu(Z, param, linebuf);
			break;

		case CID_TASKPROGRESS:
			CmdTaskProgress(Z, param, linebuf);
			break;

		case CID_VFS:
			CmdVFS(param);
			break;

		default:
			if ( Z->command >= CID_USERNAME ){
				ERRORCODE result;
				TCHAR *cmdptr;

				// 各PPx固有機能
				if ( PPXA_INVALID_FUNCTION != (result = PPxInfoFunc32u(Z->Info, PPXCMDID_COMMAND, (void *)param)) ){
					Z->result = result  ^ 1; // PPXARESULT を戻す
					break;
				}

				// コマンドモジュール
				ZSetCurrentDir(Z, olddir);
				result = CommandModule(Z, param);
				if ( result == (ERRORCODE)PPXMRESULT_STOP ){
					Z->result = ERROR_CANCELLED;
					break;
				}
				if ( result != PPXMRESULT_SKIP ) break;

				// user コマンド
				cmdptr = GetCustValue(StrUserCommand, param, linebuf, sizeof(linebuf));
				if ( cmdptr != NULL ){
					UserCommand(Z, NULL, param, cmdptr, NULL);
					if ( cmdptr != linebuf ) HeapFree(ProcHeap, 0, cmdptr);
				}else{
					XMessage(Z->hWnd, NULL, XM_GrERRld, MES_EUXC, param);
					Z->result = ERROR_BAD_COMMAND;
				}
				break;
			}
			XMessage(Z->hWnd, NULL, XM_GrERRld, T("ZExec:Not command"));
			Z->result = ERROR_CANCELLED;
	}
	if ( olddir[0] != '\0' ) SetCurrentDirectory(olddir);
}
