/*-----------------------------------------------------------------------------
	Paper Plane commandUI												Main
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include <shlobj.h>
#include "PPX.H"
#include "VFS.H"
#include "PPC_STRU.H"
#include "PPC_FUNC.H"
#include "PPC_DD.H"
#include "PPCOMBO.H"
#pragma hdrstop
#include "PPC_GVAR.C"	// グローバルの実体定義

int runcheck = 0;
BOOL ComboFix(PPCSTARTPARAM *psp);

const TCHAR RunAlone[] = T("-alone");

#define H_GestureConfig_count 4
GESTURECONFIG H_GestureConfig[] = {
	{ GID_ZOOM, GC_ZOOM, 0 },
	{ GID_PAN, GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY /* | GC_PAN_WITH_INTERTIA */, GC_PAN_WITH_SINGLE_FINGER_VERTICALLY},
//	{ GID_ROTATE, GC_ROTATE, 0},
	{ GID_TWOFINGERTAP, GC_TWOFINGERTAP, 0},
	{ GID_PRESSANDTAP, GC_PRESSANDTAP, 0},
};

#pragma warning(suppress: 28251) // SDK によって hPrevInstance の属性が異なる
#pragma argsused
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	int result;
	PPCSTARTPARAM psp;
	UnUsedParam(hPrevInstance);UnUsedParam(lpCmdLine);

									// グローバル初期化
	hInst = hInstance;
	MainThreadID = GetCurrentThreadId();
	(void)CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	InitPPcGlobal();
	psp.show = nShowCmd;
	#ifdef UNICODE
	if ( LoadParam(&psp, NULL, TRUE) == FALSE ){
		result = EXIT_FAILURE;
		goto fin;
	}
	#else
	if ( LoadParam(&psp, lpCmdLine, TRUE) == FALSE ){
		result = EXIT_FAILURE;
		goto fin;
	}
	#endif
#if 0
	XMessage(NULL, NULL, XM_DbgLOG, T("PSP SingleProcess:%d Reuse:%d UseCmd:%d AllocCmd:%d alone:%d show:%d Focus:%c"),
		psp.SingleProcess, psp.Reuse, psp.UseCmd, psp.AllocCmd, psp.alone, psp.show, psp.Focus);
	{
		PSPONE *next;

		next = psp.next;
		while ( next != NULL ){
			if ( next->id.RegID[0] == '\0' ) break;
			XMessage(NULL, NULL, XM_DbgLOG, T("PSPone RegID:%s RegMode:%d Pair:%d Combo:%d Lock:%d Pane:%d Path:%s"),
				next->id.RegID, next->id.RegMode, next->id.Pair,
				next->combo.use, next->combo.dirlock, next->combo.pane,
				next->path);
			next = PSPONE_next(next);
		}
	}
#endif

	if ( X_ChooseMode != CHOOSEMODE_NONE ){
		runcheck = 1;
		X_sps = psp.SingleProcess = FALSE;
	}else{
		runcheck = 2;
		if ( X_combo != COMBO_OFF ){
			X_sps = psp.SingleProcess = TRUE;
			runcheck = 3;
			if ( ComboFix(&psp) == FALSE ){
				result = EXIT_SUCCESS;
				goto fin;
			}
			runcheck = 4;
		}

		 if ( IsTrue(psp.SingleProcess) ){
			if ( psp.ComboID == '\0' ){
				runcheck = 5;
				if ( IsTrue(CallPPc(&psp, NULL)) ){
					result = EXIT_SUCCESS;
					goto fin;
				}
			}
		}
	}
	runcheck = 7;
	if ( IsTrue(psp.Reuse) ) ReuseFix(&psp);
	runcheck = 10;

	PreloadMigemo();

	result = PPcMain(&psp);
	// これ以降はメッセージボックスを表示することができない
fin:
	runcheck = 11;
#ifndef RELEASE
//	XMessage(NULL, NULL, XM_DbgLOG, T("Main Msg Loop quit"));
#endif
	PPxWaitExitThread();	// /sps指定時、ここで待機
	runcheck = 12;
#ifndef RELEASE
//	XMessage(NULL, NULL, XM_DbgLOG, T("PPxWaitExitThread end"));
#endif
									// 終了処理
	if ( hPreviewWnd != NULL ) DestroyWindow(hPreviewWnd);

	if ( Combo.Report.hBrush != NULL ) DeleteObject(Combo.Report.hBrush);
	if ( CacheIcon.hImage != NULL ){
		DImageList_Destroy(CacheIcon.hImage);
	}
	FreeOverlayCom();
	DeleteCriticalSection(&SHGetFileInfoSection);
	DeleteCriticalSection(&FindFirstAsyncSection);
	PPxCommonCommand(NULL, 0, K_CLEANUP);
	CoUninitialize();
#ifndef RELEASE
//	XMessage(NULL, NULL, XM_DbgLOG, T("Return WinMain"));
#endif
	return result;
}

TCHAR *SetTabGroupToPsp(PPCSTARTPARAM *psp, int showpane, TCHAR *listptr)
{
	PSPONE newpspo;
	TCHAR *dest;

	tstrcpy(newpspo.id.RegID, T("GRP"));
	newpspo.id.RegMode = PPXREGIST_IDASSIGN;
	newpspo.id.SubID = 0;
	newpspo.id.Pair = FALSE;
	newpspo.combo.use = X_combo;
	newpspo.combo.dirlock = 0;
	newpspo.combo.pane = showpane;

	dest = newpspo.path;
	for (;;){
		TCHAR chr;

		chr = *listptr;
		if ( (UTCHAR)chr < ' ' ) return NULL;
		if ( chr == '?' ){
			listptr++;
			break;
		}
		if ( dest < (newpspo.path + VFPS - 1) ){
			if ( chr == '%' ){
				chr = *(++listptr);
				if ( chr == '%' ){

				}else if ( chr == '&' ){
					chr = '?';
				}else if ( Islower(chr) ){
					chr = TinyCharUpper(chr);
				}
				#ifndef UNICODE
				else if ( Isdigit(chr) ){
					chr -= (TCHAR)'0';
					for (;;){
						TCHAR chr2;
						chr2 = *(listptr + 1);
						if ( !Isdigit(chr2) ){
							if ( Islower(chr2) ){
								*dest++ = chr;
								chr = TinyCharUpper(chr2);
								listptr++;
							}else{
								if ( (UTCHAR)chr2 < ' ' ) return NULL;
								chr = chr2;
							}
							break;
						}
						chr = (TCHAR)((chr * 10) + (chr2 - '0'));
						listptr++;
					}
				}
				#endif
			}
			*dest++ = chr;
		}
		listptr++;
	}
	*dest = '\0';
	if ( newpspo.path[0] == '\0' ) tstrcpy(newpspo.path, T("?"));
	if ( !Isupper(*listptr) ) return listptr; // このグループはIDがないので作成不要

	if ( psp->next != NULL ){
		psp->th.top -= TSTROFF(1); // 末端を除去
	}

	ThAppend(&psp->th, &newpspo, PSPONE_size(&newpspo));
	ThAppend(&psp->th, NilStr, TSTROFF(1));

	psp->next = (PSPONE *)psp->th.bottom;
	return listptr;
}

void FindParamComboTarget(PPCSTARTPARAM *psp, TCHAR ID, int subid, int dirlock, int showpane)
{
	PSPONE newpspo, *pspo, *foundPspo = NULL;

	pspo = psp->next;
	if ( pspo != NULL ){
		while ( pspo->id.RegID[0] != '\0' ){
			if ( pspo->id.RegMode == PPXREGIST_IDASSIGN ){
				if ( (pspo->id.RegID[2] == ID) &&
					  ((ID != 'Z') || (pspo->id.SubID == subid)) ){	// 既に同じID指定のがある→特に設定すること無し
					if ( pspo->combo.dirlock == 0 ){
						pspo->combo.dirlock = dirlock;
					}
					return;
				}
			}else{ // 未定義IDの窓があれば利用
				if ( pspo->combo.use != 0 ){
					foundPspo = pspo;
					// PPXREGIST_IDASSIGN 指定のが有るかもしれないので続行
				}
			}
			pspo = PSPONE_next(pspo);
		}
		if ( foundPspo != NULL ){
			foundPspo->id.RegID[0] = 'C';
			foundPspo->id.RegID[1] = '_';
			foundPspo->id.RegID[2] = ID;
			foundPspo->id.RegID[3] = '\0';
			foundPspo->id.RegMode = PPXREGIST_IDASSIGN;
			foundPspo->id.SubID = subid;
			if ( foundPspo->combo.dirlock == 0 ){
				foundPspo->combo.dirlock = dirlock;
			}
			return;
		}
		psp->th.top -= TSTROFF(1); // 末端を除去
	}
	// 該当がないので追加登録
	// pspone をセット*
	newpspo.id.RegID[0] = 'C';
	newpspo.id.RegID[1] = '_';
	newpspo.id.RegID[2] = ID;
	newpspo.id.RegID[3] = '\0';
	newpspo.id.RegMode = PPXREGIST_IDASSIGN;
	newpspo.id.SubID = subid;
	newpspo.id.Pair = FALSE;
	newpspo.combo.use = X_combo;
	newpspo.combo.dirlock = dirlock;
	newpspo.combo.pane = showpane;
	newpspo.path[0] = '\0';
	ThAppend(&psp->th, &newpspo, ToSIZE32_T((char *)&newpspo.path[0] - (char *)&newpspo.id.RegID + sizeof(TCHAR)));
	ThAppend(&psp->th, NilStr, TSTROFF(1));

	psp->next = (PSPONE *)psp->th.bottom;
}

BOOL ComboFix(PPCSTARTPARAM *psp)
{
	TCHAR comboid[] = T("CBA");
	TCHAR *listptr;
	int showpane = PSPONE_PANE_DEFAULT;
	HWND hComboWnd;
	DWORD size;

	if ( psp->ComboID != '\0' ){
		comboid[2] = (TCHAR)psp->ComboID;
		if ( comboid[2] == '@' ){
			// ID を確保しないが、未使用 ID を得る
			PPxRegist(NULL, comboid, PPXREGIST_COMBO_IDASSIGN);
			psp->ComboID = comboid[2];
			hComboWnd = NULL;
		}else{
			hComboWnd = PPcGetWindow(psp->ComboID - 'A', CGETW_GETCOMBOHWND);
		}
		// 既存 Combo 有り
		if ( hComboWnd != NULL ){
			CallPPc(psp, hComboWnd);
			if ( (psp->show != SW_SHOWNOACTIVATE) &&
				 (psp->show != SW_SHOWMINNOACTIVE) ){
				SetForegroundWindow(hComboWnd);
			}
			return FALSE;
		}
	}else if ( PPxCombo(BADHWND) != NULL ){
		// 普通の一体化時で、既に一体化窓があるときは、
		// 既存一体化窓への追加なので何もしない
		return TRUE;
	}else if ( IsTrue(psp->Reuse) ){
		// -r 時、既に別のPPcがあるなら、CallPPc をさせる
		if ( PPcGetWindow(0, CGETW_GETFOCUS) != NULL ) return TRUE;
	}

	if ( psp->RestoreTab != 0 ){
		if ( psp->RestoreTab < 0 ) return TRUE;
	}else{
		if ( X_combos[1] & CMBS1_NORESTORETAB ) return TRUE;
		if ( psp->usealone ) return TRUE; // alone 時はペインの再現を行わない
	}

	if ( IsTrue(psp->UseCmd) && (psp->next == NULL) ) psp->UseCmd = 2;

	// 元の状態を取得
	ComboPaneLayout = GetCustValue(T("_Path"), comboid, &size, 0);
	if ( ComboPaneLayout == NULL ) return TRUE;
	if ( (size == 0) ||
		 (ComboPaneLayout[0] == '\0') ){
		ProcHeapFree(ComboPaneLayout);
		ComboPaneLayout = NULL;
		return TRUE;
	}
	// 左側ペインの処理
	if ( Isupper(ComboPaneLayout[0]) ) psp->Focus = ComboPaneLayout[0];
	listptr = ComboPaneLayout + 1;
	while ( Islower(*listptr) ) listptr++; // Z小文字部分をスキップ

	// 右側ペインの処理
	if ( *listptr == '?' ){
		listptr++;
	}else{
		if ( Isupper(*listptr) ) listptr++;
		while ( Islower(*listptr) ) listptr++; // Z小文字部分をスキップ
	}
	// listptr は、ペイン・タブ並びの先頭を示す

	{ // Z が複数ある場合、無効にする
		TCHAR *Zfirst = tstrchr(listptr, 'Z');

		if ( Zfirst != NULL ){
			TCHAR *Znext;

			Znext = Zfirst;
			for ( ;; ) {
				int i;

				Znext = tstrchr(Znext + 1, 'Z');
				if ( Znext == NULL ) break;
				for ( i = 1 ; ; i++ ){
					if ( Islower(*(Zfirst + i)) ){
						if ( *(Zfirst + i) == *(Znext + i) ){
							continue; // 同じsubid...継続
						}else{ // subidがちがう...次へ
							break;
						}
					}else{
						if ( !Islower(*(Znext + i)) ){
							*Znext = ' '; // 同時に終わり... 重複なので無効
							break;
						}else{ // subidがちがう...次へ
							break;
						}
					}
				}
			}
		}
	}
	// CMBS_TABEACHITEM の時は "-" が付いているかを確認。無ければ設定を破棄
	if ( X_combos[0] & CMBS_TABEACHITEM ){
		showpane = PSPONE_PANE_SETPANE; // ※1
		if ( *listptr == '-' ){ // CMBS_TABEACHITEM 用の印を発見
			listptr++;
		}else{ // CMBS_TABEACHITEM 用の設定でないので並び順を廃棄
			*listptr = '\0';
		}
	}else{
		if ( *listptr == '-' ){ // CMBS_TABEACHITEM 用の設定なので並び順を廃棄
			*listptr = '\0';
		}
	}

	if ( psp->next != NULL ){ // IDを割り振っていないコマンドラインパラメータにIDを割り当てる
		int sindex = 0;
		TCHAR *tmplist;
		PSPONE *pspo = psp->next;

		while ( pspo->id.RegID[0] != '\0' ){
			if ( (pspo->id.RegMode <= PPXREGIST_IDASSIGN) &&
				 (pspo->id.RegID[2] == '\0') ){ // ID を割り振っていない
				tmplist = listptr;
				for ( ; *tmplist != '\0' ; ){
					TCHAR ID;
					int panesubid;

					ID = *tmplist++;
					if ( !Isupper(ID) ) continue;

					if ( Islower(*tmplist) ){ // subid(ComboID)有り
						panesubid = 0;
						tmplist++; // ComboID skip
						while ( Islower(*tmplist) ){
							panesubid = (panesubid * 26) + (*tmplist++ - 'a');
						}
					}else{
						panesubid = -1;
					}

					if ( *tmplist == '$' ) tmplist++;

					if ( Isdigit(*tmplist) ){
						int showid;

						showid = 0;
						while ( Isdigit(*tmplist) ){
							showid = (showid * 10) + (*tmplist++ - '0');
						}
						if ( sindex == showid ){
							pspo->id.RegID[2] = ID;
							pspo->id.RegID[3] = '\0';
							pspo->id.SubID = panesubid;
							pspo->id.RegMode = PPXREGIST_IDASSIGN;
							break;
						}
					}
					while ( *tmplist && !Isupper(*tmplist) ) tmplist++;
				}
				sindex++;
			}
			pspo = PSPONE_next(pspo);
		}
	}

	// 元の状態を再現する為の一覧を作成する
	for ( ; *listptr != '\0' ; ){
		TCHAR ID;
		int dirlock;
		int panesubid;

		ID = *listptr++;

		if ( ID == '?' ){
			listptr = SetTabGroupToPsp(psp, showpane, listptr);
			if ( listptr == NULL ) break;
			continue;
		}

		if ( !Isupper(ID) ) continue; // 認識できない内容をスキップ

		if ( Islower(*listptr) ){ // subid有り
			panesubid = 0;
			listptr++; // ComboID部分をスキップ
			while ( Islower(*listptr) ){
				panesubid = (panesubid * 26) + (*listptr++ - 'a');
			}
		}else{
			panesubid = -1;
		}

		if ( *listptr == '$' ){
			listptr++;
			dirlock = 1;
		}else{
			dirlock = 0;
		}
		while ( Isdigit(*listptr) ) listptr++;

		FindParamComboTarget(psp, ID, panesubid, dirlock, showpane);

		if ( *listptr == '-' ){
			listptr++;
			if ( X_combos[0] & CMBS_TABEACHITEM ) showpane++; // ※1 参照
		}
	}
#if 0 // 整形済み PSPONE一覧表示
	{
		PSPONE *pspo;
		int sindex;

		pspo = psp->next;
		sindex = 0;
		while ( (pspo != NULL) && pspo->id.RegID[0] ){
			XMessage(NULL, NULL, XM_DbgLOG, T("index=%d ID=%s.%d Pane=%d path:%s"), sindex++, pspo->id.RegID, pspo->id.SubID, pspo->combo.pane, pspo->path);
			pspo = PSPONE_next(pspo);
		}
	}
#endif
	return TRUE;
}

void USEFASTCALL PPCuiWithPathForLock(PPC_APPINFO *cinfo, const TCHAR *path)
{
	TCHAR cmdline[CMDLINESIZE];

	if ( X_combos[0] & CMBS_REUSETLOCK ){
		if ( SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_pathfocus, (LPARAM)path) == (SENDCOMBO_OK + 1) ){
			return;
		}
	}

	if ( cinfo->combo ){
		if ( cinfo->info.hWnd != hComboFocusWnd ){
			thprintf(cmdline, TSIZEOF(cmdline), T("\"%s\" -noactive"), path);
		}else{
			thprintf(cmdline, TSIZEOF(cmdline), T("\"%s\""), path);
		}
		CallPPcParam(Combo.hWnd, cmdline);
		return;
	}
	PPCuiWithPath(cinfo->info.hWnd, path);
}

void RunNewPPc(PPCSTARTPARAM *psp, MAINWINDOWSTRUCT *mws)
{
	DWORD tmp;
#ifndef _WIN64 // ユーザ空間が狭くなったら警告(Win64ではユーザ空間がWin8:8T/Win8.1:128Tなので実質警告不要)
	MEMORYSTATUS mstate;

	mstate.dwLength = sizeof(MEMORYSTATUS);
	GlobalMemoryStatus(&mstate);

	if ( mstate.dwAvailVirtual < (500 * MB) ){
		XMessage(NULL, NULL, XM_ImWRNld, MemWarnStr);
	}
#endif
	if ( X_MultiThread == 0 ){
		if ( IsTrue(PPxRegisterThread(NULL)) ){
			CreatePPcWindow(psp, mws);
			return;
		}
		{
		// PPcMain を通さずに初期化されている状態？ 将来削除可能。
			TCHAR buf[100];

			thprintf(buf, TSIZEOF(buf), T("New PPc before initialize %d %d %d"),runcheck, MainThreadID, GetCurrentThreadId());
			PPxCommonExtCommand(K_SENDREPORT, (WPARAM)buf);
		}
		return;
	}

	CloseHandle(t_beginthreadex(NULL, 0, (THREADEXFUNC)PPcMain, psp, 0, &tmp));
}

int GetPPcSubID(const TCHAR *idstr, TCHAR *dest)
{
	const TCHAR *strptr;

	strptr = idstr;
	if ( Isupper(*strptr) ){
		*dest++ = *strptr++;
		while ( Islower(*strptr) ) *dest++ = *strptr++;
	}
	*dest = '\0';
	return strptr - idstr;
}

#if DRAWMODE == DRAWMODE_GDI
	#define FirstReadEntry(cinfo) read_entry(cinfo, RENTRY_READ)
#else
	void FirstReadEntry(PPC_APPINFO *cinfo)
	{
		read_entry(cinfo, RENTRY_READ);
		if ( ((cinfo->cel.Area.cx * cinfo->cel.Area.cy) < cinfo->e.cellIMax) &&
			 !(cinfo->X_win & XWIN_HIDESCROLL) ){
			SetWindowPos(cinfo->hScrollBarWnd, HWND_TOP, 0, 0, 0, 0,
					SWP_SHOWWINDOW | SWP_NOACTIVATE);
		}
	}
#endif


BOOL CreatePPcWindow(PPCSTARTPARAM *psp, MAINWINDOWSTRUCT *mws)
{
	static int firstfocus = 0; // コンボ起動時のフォーカス窓の指定位置
	BOOL usepath = FALSE;
	TCHAR focusID[REGEXTIDSIZE];
	BOOL lastinit = TRUE;
	PPC_APPINFO *cinfo;
	int showpane = PSPONE_PANE_DEFAULT;
	int select = FALSE;
	HMODULE hUser32;
	DWORD tmp;

	hUser32 = GetModuleHandle(StrUser32DLL);

	focusID[0] = '\0';
	if ( (psp != NULL) && (psp->next != NULL) ){
		showpane = psp->next->combo.pane;
		select = psp->next->combo.select;
	}

	cinfo = (PPC_APPINFO *)HeapAlloc(hProcessHeap, HEAP_ZERO_MEMORY, sizeof(PPC_APPINFO));
	if ( cinfo == NULL ){
		XMessage(NULL, NULL, XM_GrERRld, T("mem alloc error"));
		return FALSE;
	}
	// cinfo は 0 fill 済み
	if ( psp != NULL ){
		cinfo->combo = ( psp->next != NULL ) ? psp->next->combo.use : X_combo;
		cinfo->WinPos.show = (BYTE)psp->show;
	}else{
		cinfo->combo = X_combo;
		cinfo->WinPos.show = SW_SHOWDEFAULT;
	}

	if ( cinfo->combo ){
		cinfo->combo = -1; // Size 変更通知を無効にする
		if ( Combo.hWnd == NULL ){ // Combo を新規起動→左端をフォーカス指定
			firstfocus = 1;
		}
		cinfo->hComboWnd = InitCombo(psp);
		if ( cinfo->hComboWnd == NULL ) return FALSE;
	}

	//********** RegisterID 内で psp->next は次の pspo を示すようになる
	if ( RegisterID(cinfo, psp, &usepath) == FALSE ){
		ProcHeapFree(cinfo);
		return FALSE;
	}

	InitPPcWindow(cinfo, usepath);
	if ( cinfo->swin & SWIN_WBOOT ){
		if ( !(psp && psp->next && psp->next->id.RegID[0]) ){
			// 後続の指定が無ければ、連結対象を開く
			BootPairPPc(cinfo);
		}
	}
	InitCli(cinfo);

	if ( cinfo->combo == 0 ){
		if ( cinfo->WinPos.pos.left == (int)CW_USEDEFAULT ){ // 幅が未定義なら調節
			// ShowWindow より前に指定する必要有り
			PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND, K_raw | K_a | K_F6, 0);
		}
		ShowWindow(cinfo->info.hWnd, cinfo->WinPos.show);
		FirstReadEntry(cinfo);
	}
	// X_IME == 1 のときは、WM_SETFOCUS で IMEOFF
	if ( X_IME == 2 ) PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND, K_IMEOFF, 0);
										// メインウインドウの表示を更新 -------
	PPxRegist(cinfo->info.hWnd, cinfo->RegID, PPXREGIST_SETHWND);	// 正式登録

	cinfo->hSubThread = t_beginthreadex(NULL, 0,
			(THREADEXFUNC)PPcSubThread, cinfo, 0, &tmp);

	dd_init(cinfo);
										// フォーカスの再設定
	if ( cinfo->combo ){
	// 一体時
		WPARAM keycmd;

		if ( cinfo->ChdirLock < 0 ){ // 強制 off
			cinfo->ChdirLock = 0;
		}else if ( (X_combos[0] & CMBS_DEFAULTLOCK) && (ComboInit == 0) ){
			cinfo->ChdirLock = 1;
		}

		keycmd = ((cinfo->WinPos.show != SW_SHOWNOACTIVATE) ?
						KCW_entry : (KCW_entry + KCW_entry_NOACTIVE)) +
				(showpane + 1) * KCW_entry_DEFPANE;
		if ( select ) setflag(keycmd, KCW_entry_SELECTNA);

		//↓２枚目以降は計算されていないことがあるので対策
		WmWindowPosChanged(cinfo);
		SendMessage(cinfo->hComboWnd, WM_PPXCOMMAND,
				keycmd, (LPARAM)cinfo->info.hWnd); // 内部でFocusいじるのでSend
	}else{
		// 強制offを反映
		if ( cinfo->ChdirLock < 0 ) cinfo->ChdirLock = 0;

		if ( (cinfo->swin & SWIN_WBOOT) && (cinfo->WinPos.show != SW_SHOWNOACTIVATE) ){ // 連結時
			HWND PairHWnd;
			DWORD flag;

			flag = (cinfo->swin & SWIN_BFOCUES) | (cinfo->RegID[2] & PAIRBIT);
			// アクティブA..現在B/アクティブB..現在A なら反対へ
			if ( (flag == 0) || (flag == (PAIRBIT | SWIN_BFOCUES)) ){
				PairHWnd = GetJoinWnd(cinfo);
				if ( PairHWnd != NULL ) ForceSetForegroundWindow(PairHWnd);
			}else{
				ForceSetForegroundWindow(cinfo->info.hWnd);
			}
		}
	}
										// 初期化が完了したのでFixを有効
	resetflag(cinfo->swin, SWIN_BUSY);
	IOX_win(cinfo, TRUE);

	if ( (!cinfo->combo || !(X_combos[0] & CMBS_COMMONTREE)) && cinfo->XC_tree.mode ){
		PPC_Tree(cinfo, cinfo->XC_tree.mode, FALSE);
	}

	if ( !(psp && psp->next && psp->next->id.RegID[0]) && // ←別のを呼び出しなし
			(cinfo->swin & SWIN_JOIN) && CheckReady(cinfo) ){	// 連結
		if ( GetJoinWnd(cinfo) != NULL ) JoinWindow(cinfo);
	}
	// 次のPPcを起動 / psp を解放
	if ( psp != NULL ){
		if ( psp->Focus != 0 ){
			focusID[0] = 'C';
			if ( ComboPaneLayout == NULL ){
				focusID[1] = psp->Focus;
				focusID[2] = '\0';
			}else{
				GetPPcSubID(ComboPaneLayout, focusID + 1);
			}
		}
		if ( psp->next != NULL ){
			if ( psp->next->id.RegID[0] ){	// 次を起動
				lastinit = FALSE;
				RunNewPPc(psp, mws);
			}else{ // 終わり / cmd 用のメモリを確保してなければここで解放
				psp->next = NULL;
				if ( psp->AllocCmd == FALSE ){ // cmd は静的なので PSPONE 解放
					ThFree(&psp->th);
				}
			}
		}
	}
	// これ以降は psp の使用禁止 **********************************************
	if ( (cinfo->docks.t.hWnd != NULL) ||
		 (cinfo->docks.b.hWnd != NULL) ||
		 (cinfo->hHeaderWnd != NULL) /*|| (cinfo.hToolBarWnd != NULL)*/ ){
		WmWindowPosChanged(cinfo); // 位置を再調整する
	}

	if ( cinfo->combo ){	// 一体化完了後読み込みをしたいがまだ効果が薄すぎ
		FirstReadEntry(cinfo);
	}

	if ( CountCustTable(T("_Delayed")) > 0 ){
		PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND, KC_DODO, 0);
	}
	if ( cinfo->FirstCommand != NULL ){
		PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND, K_FIRSTCMD, 0);
	}
	if ( IsExistCustTable(StrKC_main, T("FIRSTEVENT")) ){
		PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND, K_E_FIRST, 0);
	}
	if ( lastinit && firstinit ){
		if ( cinfo->combo && (ComboThreadID != 0) ){
			if ( Combo.BaseCount < X_mpane.first ){
				MorePPc(NULL, &cinfo->mws);
			}else{
				HWND hWnd;

				hWnd = firstfocus ? NULL : cinfo->info.hWnd;
				firstfocus = 0;
				firstinit = 0;
				if ( focusID[0] != '\0' ){
					HWND hFocusWnd;

					hFocusWnd = GetHwndFromIDCombo(focusID);
					if ( hFocusWnd != NULL ) hWnd = hFocusWnd;
				}
				PostMessage(cinfo->hComboWnd, WM_PPXCOMMAND, KCW_ready, (LPARAM)hWnd);
			}
		}
	}

	cinfo->mws.ThreadID = GetCurrentThreadId();
	cinfo->mws.state = MWS_STATE_WORK;
	cinfo->mws.cinfo = cinfo;
	cinfo->mws.type = cinfo->combo ? MWS_TYPE_COMBOCHILD : MWS_TYPE_SINGLE;
	UsePPx();
	cinfo->mws.next = mws->next;
	mws->next = &cinfo->mws;
	FreePPx();
	DxSetMotion(cinfo->DxDraw, DXMOTION_NewWindow);

#if XTOUCH
	{
		GETDLLPROC(hUser32, RegisterTouchWindow);
		if ( DRegisterTouchWindow != NULL ){
			if ( DRegisterTouchWindow(cinfo->info.hWnd, 0) ){
				SetPopMsg(cinfo, POPMSG_MSG, MES_TOCH);
			}
			GETDLLPROC(hUser32, GetTouchInputInfo);
			GETDLLPROC(hUser32, CloseTouchInputHandle);
		}
	}
#endif
	GETDLLPROC(hUser32, GetGestureInfo);

	if ( !cinfo->list.scroll ){
		GETDLLPROC(hUser32, SetGestureConfig);
		if ( DSetGestureConfig != NULL ){
			DSetGestureConfig(cinfo->info.hWnd, 0, H_GestureConfig_count, H_GestureConfig, sizeof(GESTURECONFIG));
		}
	}

	if ( cinfo->combo ){ // サイズ通知を有効にする
		PostMessage(cinfo->info.hWnd, WM_PPXCOMMAND, KCW_ready, 0);
	}
	return TRUE;
}

void PPCui(HWND hWnd, const TCHAR *cmdline)
{
	TCHAR param[CMDLINESIZE], dir[VFPS];
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if ( cmdline != RunAlone ){
		if ( X_sps || (X_combo != COMBO_OFF) ){
			MorePPc(cmdline, &MainWindows);
			return;
		}
	}

	si.cb			= sizeof(si);
	si.lpReserved	= NULL;
	si.lpDesktop	= NULL;
	si.lpTitle		= NULL;
	si.dwFlags		= 0;
	si.cbReserved2	= 0;
	si.lpReserved2	= NULL;

	GetModuleFileName(hInst, dir, VFPS);
#ifdef WINEGCC
	tstrcpy(tstrrchr(dir, '\\') + 1, T(PPCEXE)); //Z:\...\PPC.EXE を Z:\...\ppc に修正
#endif
	if ( cmdline ){
		thprintf(param, TSIZEOF(param), T("\"%s\" %s"), dir, cmdline);
	}else{
		thprintf(param, TSIZEOF(param), T("\"%s\""), dir);
	}
											// カレントディレクトリを作成
	*tstrrchr(dir, '\\') = '\0'; // 最後は「\PPC.EXE」なので、漢字対策せず

	if ( IsTrue(CreateProcess(NULL, param, NULL, NULL, FALSE,
			CREATE_DEFAULT_ERROR_MODE, NULL, dir, &si, &pi)) ){
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}else{
		ErrorPathBox(hWnd, NULL, param, PPERROR_GETLASTERROR);
	}
}

void MorePPc(const TCHAR *cmdline, MAINWINDOWSTRUCT *mws)
{
	DWORD nexttop C4701CHECK, cmdtop C4701CHECK;
	PPCSTARTPARAM *psp = NULL, defpsp;
	ThSTRUCT th;

	if ( (cmdline == NULL) && (X_combo != COMBO_OFF) && (X_combos[0] & CMBS_TABMAXALONE) && (Combo.ShowCount >= X_mpane.limit) ){
		PPCui(NULL, RunAlone);
		return;
	}

	if ( cmdline != NULL ){
		defpsp.show = SW_SHOW;

		LoadParam(&defpsp, cmdline, FALSE);

//		if ( defpsp.alone ){ // 新規プロセスが必要なオプション
//			return FALSE;
//		}

		ThInit(&th);
		ThAppend(&th, &defpsp, sizeof(PPCSTARTPARAM));
		if ( defpsp.next != NULL ){
			nexttop = th.top;
			ThAppend(&th, defpsp.next, defpsp.th.top);
			ThFree(&defpsp.th);
		}
		if ( IsTrue(defpsp.UseCmd) ){
			cmdtop = th.top;
			((PPCSTARTPARAM *)th.bottom)->AllocCmd = TRUE;
			((PPCSTARTPARAM *)th.bottom)->cmd = (const TCHAR *)(DWORD_PTR)th.top;
			ThAddString(&th, defpsp.cmd);
		}
		psp = (PPCSTARTPARAM *)th.bottom;
		psp->th = th;
		if ( defpsp.next != NULL ){
			psp->next = (PSPONE *)ThPointer(&th, nexttop);  // C4701ok
		}
		if ( IsTrue(defpsp.UseCmd) ){
			psp->cmd = ThPointerT(&th, cmdtop);  // C4701ok
		}
	}
	RunNewPPc(psp, mws);
}

// WM_COPYDATA 経由で PPc 操作を行う
void SendCallPPc(COPYDATASTRUCT *copydata)
{
#define PSP ((PPCSTARTPARAM *)th.bottom)
	ThSTRUCT th;

	ThInit(&th);
	ThAppend(&th, copydata->lpData, copydata->cbData);
	ReplyMessage(TRUE);

	PSP->th = th;
	// ポインタをオフセットから実際の値に変換
	if ( IsTrue(PSP->UseCmd) ){
		PSP->AllocCmd = TRUE;
		PSP->cmd = ThPointerT(&th, (size_t)(PSP->cmd));
	}
	if ( PSP->next != NULL ){
		PSP->next = (PSPONE *)ThPointer(&th, sizeof(PPCSTARTPARAM));
		if ( IsTrue(PSP->Reuse) ) ReuseFix(PSP); // /R なら ID割当て
	}
	RunNewPPc(PSP, &MainWindows);
#undef PSP
}
