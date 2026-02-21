/*-----------------------------------------------------------------------------
		ctype.h 互換ヘッダ	Version 1.13					(c)TORO 1995-2024

	isXXX 系は真の時が 0 以外であって 1 ではない
-----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

#define T_IS_KNJ 2			/* 漢字     */
#define T_IS_DIG 4			/* １０進数 */
#define T_IS_HEX 8			/* １６進数 */
#define T_IS_UPP 16			/* 英大文字 */
#define T_IS_LOW 32			/* 英小文字 */
#define T_IS_CTL 64			/* 制御文字 */

extern char T_CHRTYPE[256];

										/* ctype.h 互換関数群 */
/* ANSI */
#define IsalnumA(c) (T_CHRTYPE[(unsigned char)(c)] & (T_IS_DIG | T_IS_UPP | T_IS_LOW))
#define IsalphaA(c) (T_CHRTYPE[(unsigned char)(c)] & (T_IS_UPP | T_IS_LOW))
#define IsasciiA(c) ((unsigned char)(c) < 128)
#define IscntrlA(c) (T_CHRTYPE[(unsigned char)(c)] & T_IS_CTL)
#define IsdigitA(c) (T_CHRTYPE[(unsigned char)(c)] & T_IS_DIG)
#define IsgraphA(c) ((unsigned char)(c) >= 0x21 && (unsigned char)(c) <= 0x7e)
#define IslowerA(c) (T_CHRTYPE[(unsigned char)(c)] & T_IS_LOW)
#define IsprintA(c) ((unsigned char)(c) >= 0x20 && (unsigned char)(c) <= 0x7e)
#define IspunctA(c) (T_CHRTYPE[(unsigned char)(c)] & T_IS_PUN)
#define IsupperA(c) (T_CHRTYPE[(unsigned char)(c)] & T_IS_UPP)
#define IsxdigitA(c) (T_CHRTYPE[(unsigned char)(c)] & (T_IS_DIG | T_IS_HEX))

/* UNICODE */
#define IsalnumW(c) (((c) > 'z') ? 0 : (T_CHRTYPE[(unsigned char)(c)] & (T_IS_DIG | T_IS_UPP | T_IS_LOW)))
#define IsalphaW(c) (((c) > 'z') ? 0 : (T_CHRTYPE[(unsigned char)(c)] & (T_IS_UPP | T_IS_LOW)))
#define IsasciiW(c) ((c) < 128)
#define IscntrlW(c) (((c) > 0x80) ? 0 : (T_CHRTYPE[(unsigned char)(c)] & T_IS_CTL))
#define IsdigitW(c) ((c) >= '0' && (c) <= '9')
#define IsgraphW(c) ((c) >= 0x21 && (c) <= 0x7e)
#define IslowerW(c) ((c) >= 'a' && (c) <= 'z')
#define IsprintW(c) ((c) >= 0x20 && (c) <= 0x7e)
#define IspunctW(c) (T_CHRTYPE[(unsigned char)(c)] & T_IS_PUN)
#define IsupperW(c) ((c) >= 'A' && (c) <= 'Z')
#define IsxdigitW(c) (((c) > 'z') ? 0 : (T_CHRTYPE[(unsigned char)(c)] & (T_IS_DIG | T_IS_HEX)))

#ifdef UNICODE
	#define Isalnum IsalnumW
	#define Isalpha IsalphaW
	#define Isascii IsasciiW
	#define Iscntrl IscntrlW
	#define Isdigit IsdigitW
	#define Isgraph IsgraphW
	#define Islower IslowerW
	#define Isprint IsprintW
	#define Ispunct IspunctW
	#define Isupper IsupperW
	#define Isxdigit IsxdigitW
#else
	#define Isalnum IsalnumA
	#define Isalpha IsalphaA
	#define Isascii IsasciiA
	#define Iscntrl IscntrlA
	#define Isdigit IsdigitA
	#define Isgraph IsgraphA
	#define Islower IslowerA
	#define Isprint IsprintA
	#define Ispunct IspunctA
	#define Isupper IsupperA
	#define Isxdigit IsxdigitA
#endif
										/* 追加された関数 */
/* 漢字の１バイト目か？(ANSI専用) */
#define IskanjiA(c)	(T_CHRTYPE[(unsigned char)(c)] & T_IS_KNJ)
/* 文字のバイト数を得る(NULL=0, ANSI専用) */
#define ChrlenA(c)	(T_CHRTYPE[(unsigned char)(c)] & 3)

#define IsmultiA(c) IskanjiA(c)
#define IsmultiW(c) (((c) >= 0xd800) && ((c) <= 0xdfff))

#ifdef UNICODE
	#define Ismulti(c) IsmultiW(c)
	#define ChrlenW(c) 1
	#define Chrlen(c) ChrlenW(c)
#else
	#define Ismulti(c) IsmultiA(c)
	#define Iskanji(c) IskanjiA(c)
	#define Chrlen(c) ChrlenA(c)
#endif

#ifdef __cplusplus
}
#endif // __cplusplus
