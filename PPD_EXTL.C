/*-----------------------------------------------------------------------------
	Paper Plane xUI	 commom library						拡張子自前判別
-----------------------------------------------------------------------------*/
#define ONPPXDLL // PPCOMMON.H の DLL 定義指定
#include "WINAPI.H"
#include "PPX.H"
#include "PPCOMMON.RH"
#include "PPD_DEF.H"
#pragma hdrstop

#ifdef UNICODE
DefineWinAPI(HRESULT, SHLoadIndirectString, (LPCWSTR pszSource, LPWSTR pszOutBuf, UINT cchOutBuf, void **ppvReserved)) = NULL;
#endif

const TCHAR MUIVerbStr[] = T("MUIVerb");
const TCHAR openstr[] = MES_MCOP;
const TCHAR playstr[] = MES_MCPL;
const TCHAR printstr[] = MES_MCPR;
const TCHAR runasV5str[] = MES_MCRK;
const TCHAR runasV6str[] = MES_MCRV;
const TCHAR ExtsChoiseStr[] = T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\FileExts\\%s");
const TCHAR ProgIDstr[] = T("ProgId");
const TCHAR LegacyDisablestr[] = T("LegacyDisable");
const TCHAR PAOstr[] = T("ProgrammaticAccessOnly");
const TCHAR ShellStr[] = T("\\shell");
const TCHAR UnknownExtStr[] = T("Unknown");
const TCHAR ChoisePath[] = T("UserChoice");
const TCHAR OpenWithProgids[] = T("OpenWithProgids");
const TCHAR MRUList[] = T("MRUList");
// const TCHAR OpenWithList[] = T("OpenWithList");
const TCHAR StrApplication[] = T("application");

/*
	HKEY_CLASSES_ROOT ( HKEY_CURRENT_USER\Software\Classes )
	.拡張子名
		(規定) アプリケーション名 → HKEY_CLASSES_ROOT\アプリケーション名
		(key) OpenWithList
			(key) 実行ファイル名 →  HKEY_CLASSES_ROOT\Applications\実行ファイル名
		(key) OpenWithProgids
			アプリケーション名 → HKEY_CLASSES_ROOT\アプリケーション名
		(key) PersistentHandler
		(key) ShellEx

	HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\FileExts
	.拡張子名
		(key) OpenWithList  ※手動選択のみ使用
			id=実行ファイル名 →  HKEY_CLASSES_ROOT\Applications\実行ファイル名
			MRUList=id列
		(key) OpenWithProgids
			アプリケーション名 → HKEY_CLASSES_ROOT\アプリケーション名
		(key) UserChoice
			ProgId=アプリケーション名 → HKEY_CLASSES_ROOT\アプリケーション名

	HKEY_CLASSES_ROOT ( HKEY_CURRENT_USER\Software\Classes )
	Applications\実行ファイル名
	アプリケーション名
		(規定) コメント
		(key) CLSID
		(key) CurVer
			(規定) アプリケーション名 → HKEY_CLASSES_ROOT\アプリケーション名
		(key) DefaultIcon
		(key) Shell
			(key) 動詞
				(key) command
					DelegateExecute アプリ実行
		(key) ShellEx
*/

HKEY GetExtShellHandle(TCHAR *appN)
{
	HKEY hAppShellKey;
	int len;

	len = tstrlen(appN);
	tstrcpy(appN + len, ShellStr);
	if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, appN, 0, KEY_READ, &hAppShellKey)){
		return hAppShellKey;
	}

	tstrcpy(appN + len, T("\\CurVer"));
	if ( FALSE != GetRegString(HKEY_CLASSES_ROOT, appN, NilStr, appN, TSTROFF(MAX_PATH)) ){
		tstrcat(appN, ShellStr);
		if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, appN, 0, KEY_READ, &hAppShellKey)){
			return hAppShellKey;
		}
	}
	return NULL;
}

HKEY GetExtRegHandle(HKEY keyH, const TCHAR *extpath, TCHAR *appN)
{
	HKEY hExtKey;
	HKEY hAppShellKey = NULL;

	DWORD size;
	TCHAR buf[MAX_PATH];

	// 拡張子を開く
	if ( RegOpenKeyEx(keyH, extpath, 0, KEY_READ, &hExtKey) != ERROR_SUCCESS ){
		return NULL;
	}

	// 規定設定を読む
	if ( keyH == HKEY_CLASSES_ROOT ){
		// 規定のアプリケーション名
		size = TSTROFF(MAX_PATH);
		if ( RegQueryValueEx(hExtKey, NilStr, NULL, NULL, (LPBYTE)appN, &size) == ERROR_SUCCESS ){
			hAppShellKey = GetExtShellHandle(appN);
		}
	}else if ( WinType >= WINTYPE_VISTA ){ // UserChoice ( FileExts 限定 )
		if ( IsTrue(GetRegString(hExtKey, ChoisePath, ProgIDstr, appN, MAX_PATH) ) ){
			hAppShellKey = GetExtShellHandle(appN);
		}
	}else{ // applications (XP 以前, FileExts 限定 )
		if ( IsTrue(GetRegString(keyH, extpath, StrApplication, buf, MAX_PATH) ) ){
			thprintf(appN, MAX_PATH, T("applications\\%s"), buf);
			hAppShellKey = GetExtShellHandle(appN);
		}
	}
#if 0 // 既定実行には使わない
	// OpenWithList を読む
	if ( hAppShellKey == NULL ){
		HKEY hListKey;

		if ( ERROR_SUCCESS == RegOpenKeyEx(hExtKey, OpenWithList, 0, KEY_READ, &hListKey)){
			size = sizeof(buf);
			if ( RegQueryValueEx(hListKey, MRUList, NULL, NULL, (LPBYTE)buf, &size) == ERROR_SUCCESS ){
				appN[0] = buf[0];
				appN[1] = '\0';
				size = MAX_PATH * sizeof(TCHAR);
				if ( RegQueryValueEx(hListKey, appN, NULL, NULL, (LPBYTE)buf, &size) == ERROR_SUCCESS ){
					thprintf(appN, MAX_PATH, T("applications\\%s"), buf);
					hAppShellKey = GetExtShellHandle(appN);
				}
			}
			RegCloseKey(hListKey);
		}
	}
#endif
	// OpenWithProgids を読む
	if ( hAppShellKey == NULL ){
		HKEY hListKey;

		if ( ERROR_SUCCESS == RegOpenKeyEx(hExtKey, OpenWithProgids, 0, KEY_READ, &hListKey)){
			int cnt = 0;

			// Value 一覧 (FileExts)
			for ( ; ; cnt++ ){
				size = TSTROFF(MAX_PATH);
				if ( ERROR_SUCCESS != RegEnumValue(hListKey, cnt, appN, &size, NULL, NULL, NULL, NULL) ){
					break;
				}
				hAppShellKey = GetExtShellHandle(appN);
				if ( hAppShellKey != NULL ) break;
			}

			// Key 一覧 (ROOT)
#if 0
			int cnt = 0;
			for ( ; ; cnt++ ){
				TCHAR xkeyN[MAX_PATH];

				size = TSTROFF(MAX_PATH);
				if ( ERROR_SUCCESS != RegEnumValue(hAppShellKey, cnt, xkeyN, &size, NULL, NULL, NULL, NULL) ){
					break;
				}
				tstrcat(xkeyN, ShellStr);
				RegCloseKey(hAppShellKey);
				if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CLASSES_ROOT, xkeyN, 0, KEY_READ, &hAppShellKey) ){
					return hAppShellKey;
				}
				break;
			}
#endif
			RegCloseKey(hListKey);
		}
	}
	RegCloseKey(hExtKey);
	return hAppShellKey;
}

// 指定拡張子のアクション一覧をメニューに登録する -----------------------------
PPXDLL int PPXAPI GetExtentionMenu(HMENU hSubMenu, const TCHAR *ext, PPXMENUDATAINFO *pmdi)
{
	DWORD Bsize; // バッファサイズ指定用

	FILETIME write;
	DWORD keyS;

	TCHAR appN[MAX_PATH]; // アプリケーションのキー
	TCHAR keyN[MAX_PATH]; // レジストリのキー名称
	TCHAR comN[MAX_PATH]; // ...\\command を示す
	TCHAR defN[MAX_PATH]; // デフォルトのアクション
	HKEY hAppShellKey = NULL, hCommandKey;

	MENUITEMINFO minfo;

	int cnt = 0;

	minfo.cbSize = sizeof(minfo);
	minfo.fMask = MIIM_STATE | MIIM_TYPE | MIIM_ID;
	minfo.fType = MFT_STRING;
//	minfo.fState = MFS_ENABLED;
	minfo.wID = pmdi->id;

	if ( ext[0] == '.' ){				// 拡張子からキーを求める -------------
		// Windows8以降
		thprintf(keyN, TSIZEOF(keyN), ExtsChoiseStr, ext);
		if ( (hAppShellKey = GetExtRegHandle(HKEY_CURRENT_USER, keyN, appN)) == NULL ){
			// 従来
			if ( (hAppShellKey = GetExtRegHandle(HKEY_CLASSES_ROOT, ext, appN)) == NULL ){
				// 全種
				if ( (hAppShellKey = GetExtRegHandle(HKEY_CLASSES_ROOT, WildCard_All, appN)) == NULL ){
					tstrcpy(appN, UnknownExtStr); // 該当無し
				}
			}
		}
	}else if ( ext[0] != '\0' ){
		tstrcpy(appN, ext);
	}else{
		tstrcpy(appN, UnknownExtStr); // 拡張子無し
	}
	if ( hAppShellKey == NULL ){
		tstrcat(appN, ShellStr);
										// アプリケーションのシェル -----------
		if ( ERROR_SUCCESS != RegOpenKeyEx(HKEY_CLASSES_ROOT, appN, 0, KEY_READ, &hAppShellKey)){
			goto nomenuitem;
		}
	}

	Bsize = sizeof(defN);
	defN[0] = '\0';
	RegQueryValueEx(hAppShellKey, NilStr, NULL, NULL, (LPBYTE)defN, &Bsize);
	if ( defN[0] == '\0' ) tstrcpy(defN, ShellVerb_open);	// デフォルトの指定が無い

	for ( ; ; cnt++ ){					// 設定を取り出す ---------------------
		keyS = TSIZEOF(keyN);
		if ( ERROR_SUCCESS != RegEnumKeyEx(hAppShellKey, cnt, keyN, &keyS, NULL, NULL, NULL, &write) ){
			break;
		}

		if ( ERROR_SUCCESS == RegOpenKeyEx(hAppShellKey, keyN, 0, KEY_READ, &hCommandKey) ){
			Bsize = sizeof(appN);
			if ( ERROR_SUCCESS == RegQueryValueEx(hCommandKey, LegacyDisablestr, NULL, NULL, NULL, &Bsize) ){
				RegCloseKey(hCommandKey);
				continue;
			}
			RegCloseKey(hCommandKey);
		}

		// keyがprintto(指定プリンタに印刷)以外  && shell\key name\command が開けるなら登録
		tstrcpy(comN, keyN);
		tstrcat(comN, T("\\command"));
		if ( (tstricmp(keyN, T("printto")) != 0) && (ERROR_SUCCESS == RegOpenKeyEx(hAppShellKey, comN, 0, KEY_READ, &hCommandKey)) ){
			RegCloseKey(hCommandKey);
			minfo.dwTypeData = keyN;
							// デフォルトは太字にする
			minfo.fState = (tstricmp(defN, keyN) == 0) ?
					(MFS_ENABLED | MFS_DEFAULT) : MFS_ENABLED;

			if ( pmdi->th.top != MAX32 ){
				HKEY hAppItemKey;

				RegOpenKeyEx(hAppShellKey, keyN, 0, KEY_READ, &hAppItemKey);
				// 表示用の文字列があれば取得
				Bsize = sizeof(appN);
				appN[0] = '\0';
#ifdef UNICODE
				RegQueryValueEx(hAppItemKey, MUIVerbStr, NULL, NULL, (LPBYTE)appN, &Bsize);
				if ( appN[0] == '\0' ){
#endif
					Bsize = sizeof(appN);
					RegQueryValueEx(hAppItemKey, NilStr, NULL, NULL, (LPBYTE)appN, &Bsize);
#ifdef UNICODE
				}
#endif
				if ( appN[0] == '@' ){
#ifdef UNICODE
					if ( DSHLoadIndirectString == NULL ){
						GETDLLPROC(GetModuleHandle(T("shlwapi.dll")), SHLoadIndirectString);
					}
					if ( DSHLoadIndirectString != NULL ){
						DSHLoadIndirectString(appN, appN, MAX_PATH, NULL);
					}
#else
					appN[0] = '\0';
#endif
				}

				if ( appN[0] == '\0' ){ // 表示文字列が無いとき。
					Bsize = sizeof(appN);
					if ( RegQueryValueEx(hAppItemKey, PAOstr, NULL, NULL, (LPBYTE)appN, &Bsize) == ERROR_SUCCESS ){
						// ProgrammaticAccessOnly があるので表示しない
						RegCloseKey(hAppItemKey);
						continue;
					}

					if ( tstricmp(keyN, ShellVerb_open) == 0 ){
						minfo.dwTypeData = (TCHAR *)MessageText(openstr);
					}else if ( tstricmp(keyN, T("print")) == 0 ){
						minfo.dwTypeData = (TCHAR *)MessageText(printstr);
					}else if ( tstricmp(keyN, T("play")) == 0 ){
						minfo.dwTypeData = (TCHAR *)MessageText(playstr);
					}else if ( tstrcmp(keyN, T("runas")) == 0 ){
						minfo.dwTypeData = (TCHAR *)MessageText(
								(WinType >= WINTYPE_VISTA) ?
								runasV6str : runasV5str);
					}
				}else{
					minfo.dwTypeData = appN;
				}
				RegCloseKey(hAppItemKey);
				ThAddString(&pmdi->th, keyN);
			}
			InsertMenuItem(hSubMenu, 0xffff, TRUE, &minfo);
			minfo.wID++;
		}
	}
	RegCloseKey(hAppShellKey);
	if ( pmdi->id == minfo.wID ) goto nomenuitem;
	pmdi->id = minfo.wID;
	return cnt;

nomenuitem:
	minfo.dwTypeData = T("*open");
	minfo.fState = MFS_ENABLED;
	InsertMenuItem(hSubMenu, 0xffff, TRUE, &minfo);
	pmdi->id++;
	return 0;
}

// 1.20 時点で未使用
PPXDLL int PPXAPI PP_GetContextMenu(HMENU hSubMenu, const TCHAR *ext, DWORD *ID)
{
	PPXMENUDATAINFO pmdi;
	int result;

	pmdi.th.top = MAX32;
	pmdi.id = *ID;
	result = GetExtentionMenu(hSubMenu, ext, &pmdi);
	*ID = pmdi.id;
	return result;
}
