/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System						「:」処理
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "PPD_DEF.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "VFS_FF.H"
#pragma hdrstop

const TCHAR StrRegTsClient[] = T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SessionInfo");
const TCHAR StrRegTsTarget[] = T("Target");
const TCHAR StrRegWslPath[] = T("SYSTEM\\CurrentControlSet\\Services\\P9NP\\NetworkProvider");
const TCHAR StrRegWslTarget[] = T("TriggerStartPrefix");
const TCHAR StrAuxBase[] = T("base");
const TCHAR StrAuxSep[] = T(" %; ");

void MakeDriveList(FF_MC *mc)
{
	DWORD X_dlf;
	TCHAR textbuf[VFPS + VFPS + 16];
	TCHAR pathbuf[CUST_NAME_LENGTH + 2];

	X_dlf = GetCustDword(StrX_dlf, 0);
							// 変数の初期化
	ThInit(&mc->dirs);
	ThInit(&mc->files);
	mc->d_off = 0;
	mc->f_off = 0;
										// 「#:」==============================
	ThAddString(&mc->dirs, (X_dlf & XDLF_ROOTJUMP) ? T("#:\\") : T("#:"));
	if ( X_dlf & XDLF_DISPDRIVETITLE ){
		mc->dirs.top -= sizeof(TCHAR);
		thprintf(&mc->dirs, THP_ADD, T("> %Ms"), MES_FEXP);
	}
										// 「\\」==============================
	if ( X_dlf & XDLF_DISPDRIVETITLE ){
		thprintf(&mc->dirs, THP_ADD, T("\\\\> %Ms"), MES_FNET);
	}else{
		ThAddString(&mc->dirs, T("\\\\"));
	}
										// 「A:」～「Z:」 =====================
	if ( !(X_dlf & XDLF_NODRIVES) ){
		DWORD drive;
		int i;
		TCHAR name[] = T("A:\\");
		#define DRIVE_OFFLINE B30

		if ( !(X_dlf & XDLF_ROOTJUMP) ) name[2] = '\0';
		drive = GetLogicalDrives();
		for ( i = 0 ; i < 26 ; i++ ){
			if ( !(drive & LSBIT) ){ // ドライブ無し…未接続ネットワークの確認
				pathbuf[0] = '\0';
				thprintf(textbuf, TSIZEOF(textbuf), T("Network\\%c"), (TCHAR)(i + 'A'));
				GetRegString(HKEY_CURRENT_USER, textbuf, RPATHSTR, pathbuf, TSIZEOF(pathbuf));
				if ( pathbuf[0] != '\0' ){
					setflag(drive, LSBIT | DRIVE_OFFLINE);
				}
			}
			if ( drive & LSBIT ){
				if ( X_dlf & XDLF_DISPDRIVETITLE ){
					if ( drive & DRIVE_OFFLINE ){
						thprintf(textbuf, TSIZEOF(textbuf), T("%s> offline(%s)"), name, pathbuf);
					}else{
						TCHAR *p;

						p = thprintf(textbuf, TSIZEOF(textbuf), T("%s> "), name);
						GetDriveNameTitle(p, name[0]);
					}
					ThAddString(&mc->dirs, textbuf);
				}else{
					ThAddString(&mc->dirs, name);
				}
			}
			drive >>= 1;
			name[0]++;
		}
		#undef DRIVE_OFFLINE
	}

	{ // \\tsclient 列挙
		HKEY hSessionInfo, hNameSpace;

		if ( ERROR_SUCCESS == RegOpenKeyEx(HKEY_CURRENT_USER, StrRegTsClient, 0, KEY_READ, &hSessionInfo)){
			int count = 0;
			TCHAR KeyName[128], comid[128], path[MAX_PATH];
			FILETIME write;
			DWORD rsize;

			for ( ; ; count++ ){
				rsize = TSIZEOF(KeyName);
				if ( ERROR_SUCCESS != RegEnumKeyEx(hSessionInfo, count, KeyName, &rsize, NULL, NULL, NULL, &write) ){
					break;
				}
				tstrcat(KeyName, T("\\MyComputer\\Namespace"));

				if ( ERROR_SUCCESS == RegOpenKeyEx(hSessionInfo, KeyName, 0, KEY_READ, &hNameSpace)){
					int count2;

					count2 = 0;
					rsize = 200;
					tstrcpy(comid, T("CLSID\\"));
					if ( ERROR_SUCCESS == RegEnumKeyEx(hNameSpace, count2, comid + 6, &rsize, NULL, NULL, NULL, &write) ){
						tstrcat(comid, T("\\Instance\\InitPropertyBag"));
						GetRegString(HKEY_CLASSES_ROOT, comid, StrRegTsTarget, path, TSIZEOF(path));
						ThAddString(&mc->dirs, path);
					}
					RegCloseKey(hNameSpace);
				}
			}
			RegCloseKey(hSessionInfo);
		}
	}

	// wsl列挙
	pathbuf[2] = '\0';
	GetRegString(HKEY_LOCAL_MACHINE, StrRegWslPath, StrRegWslTarget, pathbuf + 2, TSIZEOF(pathbuf) - sizeof(TCHAR) * 2);
	if ( pathbuf[2] != '\0' ){
		pathbuf[0] = pathbuf[1] = '\\';
		ThAddString(&mc->dirs, pathbuf);
	}

	{ // MTP デバイス列挙
		LPITEMIDLIST idl;
		LPSHELLFOLDER pSF;
		LPENUMIDLIST pEID;
		LPMALLOC pMA;

		pSF = VFPtoIShell(NULL, T("#17:\\"), NULL);
		if ( pSF != NULL ){
			(void)SHGetMalloc(&pMA);

			if ( S_OK == pSF->lpVtbl->EnumObjects(pSF, NULL,
				(OSver.dwMajorVersion >= 6) ?
				ENUMOBJECTSFORFOLDERFLAG_VISTA : ENUMOBJECTSFORFOLDERFLAG_XP,
				&pEID) ){ // S_FALSE のときは、pEID = NULL
				for ( ; ; ){
					DWORD getsi;

					if ( pEID->lpVtbl->Next(pEID, 1, &idl, &getsi) != S_OK ){
						break;
					}
					if ( getsi == 0 ) break;

					if ( IsTrue(PIDL2DisplayName(pathbuf, pSF, idl, SHGDN_INFOLDER | SHGDN_FORPARSING)) ){
						// XMessage(NULL, NULL, XM_DbgLOG, T("%s"),pathbuf);
						// ::{xxx} ライブラリ
						// \\?\usb#vid_… MTP
						// X: drive
						if ( (pathbuf[0] == '\\') && (pathbuf[2] == '?') ){
							if ( IsTrue(PIDL2DisplayNameOf(textbuf, pSF, idl)) ){
								thprintf(&mc->dirs, THP_ADD, T(":<mtp>#17:\\%s"), textbuf);
							}
						}
					}
					pMA->lpVtbl->Free(pMA, idl);
				}
				pEID->lpVtbl->Release(pEID);
			}
			pMA->lpVtbl->Release(pMA);
			pSF->lpVtbl->Release(pSF);
		}
	}

	{ // AUX: 列挙
		int count = 0;
		TCHAR key[CUST_NAME_LENGTH], param[CMDLINESIZE], *sepptr;

		for( ; EnumCustData(count, key, NULL, 0) >= 0; count++ ){
			if ( ((key[0] != 'M') && (key[0] != 'S')) ||
				 (key[1] != '_') ||
				 (key[2] != 'a') ||
				 (key[3] != 'u') ||
				 (key[4] != 'x') ){
				continue;
			}

			param[0] = '\0';
			GetCustTable(key, StrAuxBase, param, sizeof(param));
			if ( param[0] == '\0' ) continue;
			sepptr = tstrstr(param, StrAuxSep);
			if ( sepptr != NULL ){
				*sepptr = '\0';
				thprintf(&mc->dirs, THP_ADD, T(":<%s>%s"), sepptr + TSIZEOFSTR(StrAuxSep), param);
			}else{
				ThAddString(&mc->dirs, param);
			}
		}
	}
								// Net share History ==========================
	if ( !(X_dlf & XDLF_NODISPSHARE) ){
		int index = 0;

		UsePPx();
		for ( ; ; ){
			const TCHAR *hisp;
			TCHAR *p, *lp;
			int mode;

			hisp = EnumHistory(PPXH_PPCPATH, index++);
			if ( hisp == NULL ) break;
										// GNC, #: は無視
			p = VFSGetDriveType(hisp, &mode, NULL);
			if ( (p == NULL) || (mode != VFSPT_UNC) ) continue;

			if ( mode == VFSPT_UNC ){
				p = FindPathSeparator(p);
				if ( (p == NULL) || !*(p + 1) ) continue;	//「\\」「\\xxx」「\\xxx\」無視
										// 「\\xxx\yyy」形式になる末端を検索
				lp = FindPathSeparator(p + 1);
				if ( lp != NULL ){
					p = lp;
				}else{
					p += tstrlen(p);
				}
			}
												// とりあえず、取得
			tstrcpy(textbuf, hisp);
			p = textbuf + (p - hisp);
			if ( (mode == VFSPT_UNC) && !(X_dlf & XDLF_ROOTJUMP) ){
				*p++ = '\\';
				*p++ = ':';
			}
			*p = '\0';
			AddDriveList(&mc->dirs, textbuf);
		}
		FreePPx();
	}
										// Menu ============================
	if ( !(X_dlf & XDLF_NOM_PJUMP) ){
		const TCHAR *MenuName;
		int index = 0;

		MenuName = (X_dlf & XDLF_USEMDRIVES) ? StrMDrives : PathJumpName;

		pathbuf[0] = ':';
		while ( EnumCustTable(index++, MenuName,
				pathbuf + 1, textbuf, sizeof(textbuf)) >= 0 ){
			PP_ExtractMacro(NULL, NULL, NULL, textbuf, textbuf, XEO_DISPONLY);
			if ( textbuf[0] == '\0' ) continue;
			if ( textbuf[0] == ':' ) continue;
			if ( textbuf[0] == '?' ) continue;
			if ( (textbuf[0]=='%') && (textbuf[1]=='M') ) continue; // メニュー

			if ( X_dlf & XDLF_DISPALIAS ){
				AddDriveList(&mc->dirs, pathbuf);
			}else{
				VFSFullPath(NULL, textbuf, NULL);

				if ( !(X_dlf & XDLF_NODRIVES) ){
					if ( X_dlf & XDLF_ROOTJUMP ){
						if ( textbuf[3] == '\0' ) continue;
					}else{
						if ( textbuf[2] == '\0' ) continue;
					}
				}
				AddDriveList(&mc->dirs, textbuf);
			}
		}
	}
	ThAddString(&mc->files, NilStr);
}
