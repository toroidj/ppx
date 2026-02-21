/*-----------------------------------------------------------------------------
	64 bit 整数の操作用関数
-----------------------------------------------------------------------------*/
#include "WINAPI.H"
#include "TOROWIN.H"
#include "PPX_64.H"
#pragma hdrstop

#ifndef _WIN64
/*
	DOUBLE DWORD を 1G 未満で２つの DWORD に分ける
	0 ～ 4,294,967,295,999,999,999(4.3E) まで表現可能(4.3E～18.4E(max)は不可)
	FILETIME なら 西暦15,211年まで
*/
void DDwordToDten(DWORD srcl, DWORD srch, UINTHL *dest)
{
#if _INTEGRAL_MAX_BITS >= 64
	unsigned __int64 a, h;

	if ( srch >= DWORDTEN ){
		dest->HL = DWORDTEN - 1;
		return;
	}
	a = ((unsigned __int64)srch << 32) + (unsigned __int64)srcl;
	#ifndef __BORLANDC__
		dest->s.H = (DWORD)(unsigned __int64)(h = (a / 1000000000uLL));
		dest->s.L = (DWORD)(unsigned __int64)(a - h * 1000000000uLL);
	#else
		dest->s.H = (DWORD)(unsigned __int64)(h = (a / 1000000000i64));
		dest->s.L = (DWORD)(unsigned __int64)(a - h * 1000000000i64);
	#endif
#else
 #if USETASM32
	if ( srch >= DWORDTEN ){
		dest->s.L = dest->s.H = DWORDTEN - 1;
		return;
	}

	asm {
		mov edx, srch
		mov eax, srcl
		mov ebx, DWORDTEN
		div ebx
		mov ebx, dest
		mov [ebx + 4], eax
		mov [ebx], edx
	}
 #else // 精度不足
	#error DDwordToDten double は精度不足のため正しい結果が得られない
	double a;
	DWORD h;

	if ( srch >= DWORDTEN ){
		dest->s.L = dest->s.H = DWORDTEN - 1;
		return;
	}
	a = (double)srch * DWORDPP + (double)srcl;
	dest->s.H = h = (DWORD)(double)(a / 1e9);
	dest->s.L = (DWORD)(double)(a - (double)h * 1e9);
 #endif
#endif
}
#endif
/*-----------------------------------------------
	２つの DWORD を掛算し、DOUBLE DWORD を作成する
-----------------------------------------------*/
void DDmul(DWORD src1, DWORD src2, DWORD *destl, DWORD *desth)
{
#if _INTEGRAL_MAX_BITS >= 64
	unsigned __int64 a;

	a = (unsigned __int64)src1 * (unsigned __int64)src2;
	*desth = (DWORD)(a >> 32);
	*destl = (DWORD)a;
#else
 #if USETASM32
	asm {
		mov eax, src1
		mul src2
		mov ecx, destl
		mov [ecx], eax
		mov eax, desth
		mov [eax], edx
	}
 #else // 53bit なので精度不足
	double a;

	a = (double)src1 * (double)src2;
	*desth = (DWORD)(a / DWORDPP);
	*destl = (DWORD)(a - (double)*desth * DWORDPP);
 #endif
#endif
}

#if !defined(_WIN64) && USETASM32
/*-----------------------------------------------
 FPU の初期化をする
-----------------------------------------------*/
DWORD float_init_ThreadID = 0;

LONG InitFloat(void)
{
	DWORD ID = GetCurrentThreadId();

	if ( float_init_ThreadID == ID ) return EXCEPTION_CONTINUE_SEARCH;
	float_init_ThreadID = ID;
	asm {
		finit
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}
#endif
/*-----------------------------------------------
 migemo の読み出し規約違いを検出・対処する
-----------------------------------------------*/
#if !defined(_WIN64) && USETASM32
typedef struct _migemo{int dummy;} migemo;
migemo *(__stdcall * migemo_open)(const char *dict) = NULL;
// 1.2 以前の旧版
void (__cdecl * migemo_close_cdecl)(migemo *object);
char *(__cdecl * migemo_query_cdecl)(migemo *object, const char *query);
void (__cdecl * migemo_release_cdecl)(migemo *object, char *string);
// 1.3 以降のの新版
void (__stdcall * migemo_close)(migemo *object);
char *(__stdcall * migemo_query)(migemo *object, const char *query);
void (__stdcall * migemo_release)(migemo *object, char *string);

extern migemo * migemo_open_fix(const char *dict);

void __stdcall migemo_close_fix(migemo *object)
{
	migemo_close_cdecl(object);
}
char * __stdcall migemo_query_fix(migemo *object, const char *query)
{
	return migemo_query_cdecl(object, query);
}
void __stdcall migemo_release_fix(migemo *object, char *string)
{
	migemo_release_cdecl(object, string);
}

migemo * migemo_open_fix(const char *dict)
{
	migemo *result;
	DWORD IsCDECL;

	// migemo_open の cdecl() / stdcall() 両対応 & 検出コード
	asm {
		push ebx
		mov  ebx, esp
		push dict
		call migemo_open
		mov  result, eax
		sub  ebx, esp
		mov  IsCDECL, ebx
		add  esp, ebx
		pop  ebx
	};
	if ( IsCDECL > 0 ){ // 旧版向けに用意
		migemo_close_cdecl = (void (__cdecl *)(migemo *))migemo_close;
		migemo_close = migemo_close_fix;

		migemo_query_cdecl = (char *(__cdecl *)(migemo *, const char *))migemo_query;
		migemo_query = migemo_query_fix;

		migemo_release_cdecl = (void (__cdecl *)(migemo *, char *))migemo_release;
		migemo_release = migemo_release_fix;
	}

	return result;
}
#endif
