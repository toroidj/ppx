/*-----------------------------------------------------------------------------
	Paper Plane file extension fixer
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "PPX.H"
#include "VFS.H"
#pragma hdrstop

const TCHAR StrHelp[] = MES_FIXH;
const TCHAR StrUnknown[] = MES_FIXU;
const TCHAR StrModify[] = MES_FIXM;
const TCHAR StrEqual[] = MES_FIXE;

void SendLogPPc(const TCHAR *text)
{
	TCHAR cmdline[CMDLINESIZE];

	thprintf(cmdline, TSIZEOF(cmdline), T("%%OC *execute C,%%%%OC *linemessage %s"), text);
	PP_ExtractMacro(NULL, NULL, NULL, cmdline, NULL, 0);
}

#pragma warning(suppress:28251) // SDK によって hPrevInstance の属性が異なる
#pragma argsused
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	TCHAR fname[VFPS];
	int CaseLower = FALSE;
	int ForceRename = FALSE;
	int SendPPc = FALSE;
#ifdef UNICODE
	#define CMD cmdptr
	const TCHAR *cmdptr;
	UnUsedParam(hInstance);UnUsedParam(hPrevInstance);UnUsedParam(lpCmdLine);UnUsedParam(nShowCmd);

	cmdptr = GetCommandLine();
	GetLineParamS(&CMD, fname, TSIZEOF(fname));
#else
	#define CMD lpCmdLine
	UnUsedParam(hInstance);UnUsedParam(hPrevInstance);UnUsedParam(nShowCmd);
#endif
	PPxCommonExtCommand(K_UxTheme, KUT_INIT);
	GetCustData(T("X_ffxl"), &CaseLower, sizeof(CaseLower));

	for (;;){
		if ( GetLineParamS((const TCHAR **)&CMD, fname, TSIZEOF(fname)) == '\0' ){
			XMessage(NULL, NULL, XM_INFOld, StrHelp);
			return 0;
		}
		if ( (fname[0] == '-') || (fname[0] == '/') ){
			if ( tstrcmp(fname + 1, T("force")) == 0 ){
				ForceRename = TRUE;
				continue;
			}else if ( tstrcmp(fname + 1, T("lower")) == 0 ){
				CaseLower = TRUE;
				continue;
			}else if ( tstrcmp(fname + 1, T("ppc")) == 0 ){
				SendPPc = TRUE;
				continue;
			}
		}
		break;
	}

	do{
		char *image = NULL;
		SIZE32_T imgsize;
		TCHAR mes[0x800];

		if ( LoadFileImage(fname, 0x100, &image, &imgsize, LFI_ALWAYSLIMIT) ){
			ErrorPathBox(NULL, NULL, fname, PPERROR_GETLASTERROR);
		}else{
			ERRORCODE err;
			VFSFILETYPE vft;

			vft.flags = VFSFT_TYPETEXT | VFSFT_EXT;
			err = VFSGetFileType(fname, image, imgsize, &vft);
			HeapFree(GetProcessHeap(), 0, image);
			if ( err == ERROR_NO_DATA_DETECTED ){
				if ( IsTrue(SendPPc) ){
					SendLogPPc(MessageText(StrUnknown));
				}else{
					XMessage(NULL, fname, XM_RESULTld, StrUnknown);
				}
			}else if ( err != NO_ERROR ){
				ErrorPathBox(NULL, NULL, fname, err);
			}else{
				TCHAR newname[VFPS], *namep, *extp, *extnp;
				const TCHAR *mesp;
				UINT type;

				tstrcpy(newname, fname);
				namep = VFSFindLastEntry(newname);
				if ( *namep == '\\' ) namep++;
				extnp = extp = namep + FindExtSeparator(namep);
				if ( *extnp == '.' ) extnp++;
				if ( tstricmp(extnp, vft.ext) ){ // 変更有り
					mesp = StrModify;
					type = MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION;
				}else{ // 変更不要
					if ( IsTrue(ForceRename) ){
						if ( IsTrue(SendPPc) ){
							thprintf(mes, TSIZEOF(mes), MessageText(StrEqual), vft.typetext, vft.ext);
							SendLogPPc(mes);
						}
						continue;
					}else{
						mesp = StrEqual;
						type = MB_OK | MB_DEFBUTTON1 | MB_ICONINFORMATION;
					}
				}
				if ( IsTrue(CaseLower) ) Strlwr(vft.ext);
				thprintf(mes, TSIZEOF(mes), MessageText(mesp), vft.typetext, vft.ext);
				if ( IsTrue(ForceRename) ||
					 (PMessageBox(NULL, mes, namep, type) == IDYES) ){
					if ( *extp != '.' ) *extp = '.';
					tstrcpy(extp + 1, vft.ext);
					if ( MoveFile(fname, newname) == FALSE ){
						ErrorPathBox(NULL, NULL, fname, err);
					}
				}
			}
		}
	}while( GetLineParamS((const TCHAR **)&CMD, fname, TSIZEOF(fname)) != '\0' );
	return 0;
}
