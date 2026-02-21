/*-----------------------------------------------------------------------------
	Paper Plane xUI	 Virtual File System				～ HTTP処理 ～
-----------------------------------------------------------------------------*/
#define ONVFSDLL		// VFS.H の DLL export 指定
#include "WINAPI.H"
#include <winsock.h>
#include "PPXVER.H"
#include "PPX.H"
#include "PPD_DEF.H"
#include "VFS.H"
#include "VFS_STRU.H"
#include "VFS_FF.H"
#pragma hdrstop

#define MAX_BUF 0x1000

typedef struct impaddrinfo {
	int ai_flags;
	int ai_family;
	int ai_socktype;
	int ai_protocol;
	size_t ai_addrlen;
	char *ai_canonname;
	struct sockaddr *ai_addr;
	struct impaddrinfo *ai_next;
} impADDRINFO;

// Winsock --------------------------

int (WINAPI *Drecv)(SOCKET s, char FAR * buf, int len, int flags);
int (WINAPI *Dsend)(SOCKET s, const char FAR * buf, int len, int flags);
int (WINAPI *DWSAStartup)(WORD wVersionRequested, LPWSADATA lpWSAData);
SOCKET (WINAPI *Dsocket)(int af, int type, int protocol);
unsigned long (WINAPI *Dinet_addr)(const char FAR * cp);
struct hostent FAR *(WINAPI *Dgethostbyname)(const char FAR * name);
//struct servent FAR *(PASCAL FAR *Dgetservbyname)(const char FAR * name, const char FAR * proto);
int (WINAPI *Dconnect)(SOCKET s, const struct sockaddr FAR * name, int namelen);
int (WINAPI *Dclosesocket)(SOCKET s);
int (WINAPI *DWSACleanup)(void);
u_short (WINAPI *Dhtons)(u_short hostshort);
int (WINAPI *DWSAGetLastError)(void);
int (WINAPI *Dshutdown)(SOCKET s, int how);

int (WINAPI *Dselect)(int nfds, fd_set *, fd_set *, fd_set *, const struct timeval *);
int (WINAPI *Dioctlsocket)(SOCKET, long, u_long *);
//DefineWinAPI(int, getaddrinfo, (CTCHAR *pNodeName, CTCHAR *pServiceName, const impADDRINFO *pHints, impADDRINFO **ppResult));
DefineWinAPI(int, getaddrinfo, (const char *pNodeName, const char *pServiceName, const impADDRINFO *pHints, impADDRINFO **ppResult));
DefineWinAPI(void, freeaddrinfo, (impADDRINFO *pAddrInfo));

LOADWINAPISTRUCT WINSOCKDLL[] = {
	LOADWINAPI1(WSAStartup),
	LOADWINAPI1(socket),
	LOADWINAPI1(inet_addr),
	LOADWINAPI1(gethostbyname),
//	LOADWINAPI1(getservbyname),
	LOADWINAPI1(connect),
	LOADWINAPI1(recv),
	LOADWINAPI1(send),
	LOADWINAPI1(closesocket),
	LOADWINAPI1(WSACleanup),
	LOADWINAPI1(htons),
	LOADWINAPI1(WSAGetLastError),
	LOADWINAPI1(shutdown),
	LOADWINAPI1(ioctlsocket),
	LOADWINAPI1(select),
	{NULL, NULL}
};

// SSL -------------------------
#define DefineCdeclAPI(retvar, name, param) typedef retvar (CDECL *imp ## name) param; imp ## name D ## name

typedef void SSL;
typedef void SSL_CTX;
typedef void SSL_METHOD;

DefineCdeclAPI(SSL_CTX *, SSL_CTX_new, (SSL_METHOD *meth));
DefineCdeclAPI(void, SSL_CTX_free, (SSL_CTX *));
DefineCdeclAPI(int, SSL_set_fd, (SSL *s, int fd));
DefineCdeclAPI(SSL *, SSL_new, (SSL_CTX *ctx));
DefineCdeclAPI(void, SSL_free, (SSL *ssl));
DefineCdeclAPI(int, SSL_connect, (SSL *ssl));
DefineCdeclAPI(int, SSL_read, (SSL *ssl, void *buf, int num));
//DefineCdeclAPI(int, SSL_peek, (SSL *ssl, void *buf, int num));
DefineCdeclAPI(int, SSL_write, (SSL *ssl, const void *buf, int num));
DefineCdeclAPI(long, SSL_CTX_ctrl, (SSL_CTX *ctx, int cmd, long larg, void *parg));
DefineCdeclAPI(int, SSL_shutdown, (SSL *s));
DefineCdeclAPI(int, SSL_get_error, (const SSL *s, int ret));

DefineCdeclAPI(int, SSL_set1_host, (SSL *s, const char *host));
DefineCdeclAPI(long, SSL_ctrl, (SSL *ssl, int cmd, long larg, void *parg));
#define SSL_set_tlsext_host_name(s,name) DSSL_ctrl(s, 55 /* SSL_CTRL_SET_TLSEXT_HOSTNAME */, 0 /* TLSEXT_NAMETYPE_host_name */, (void *)name)

// 旧API 1.01 より前
DefineCdeclAPI(int, SSL_library_init, (void));
DefineCdeclAPI(void, SSL_load_error_strings, (void));
DefineCdeclAPI(SSL_METHOD *, SSLv23_client_method, (void));

// 新API 1.01 以降
#ifdef _WIN64
	typedef unsigned long long int OPTS_VALUE;
	#define OPTS_VALUE_INIT 0
	#define OPTS_VALUE_LOAD LOAD_ERROR_STRINGS_OPT
#else
	typedef struct { DWORD dummy[2]; } OPTS_VALUE_struct;
	#define OPTS_VALUE OPTS_VALUE_struct
	#define OPTS_VALUE_INIT {{0, 0}}
	#define OPTS_VALUE_LOAD {{LOAD_ERROR_STRINGS_OPT, 0}}
#endif
// OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS
#define LOAD_ERROR_STRINGS_OPT 0x200002L
DefineCdeclAPI(int, OPENSSL_init_ssl, (OPTS_VALUE, void *));
DefineCdeclAPI(SSL_METHOD *, TLS_client_method, (void));

LOADWINAPISTRUCT SSLEAY32DLL[] = {
	LOADWINAPI1(SSL_CTX_new),
	LOADWINAPI1(SSL_CTX_free),
	LOADWINAPI1(SSL_set_fd),
	LOADWINAPI1(SSL_new),
	LOADWINAPI1(SSL_free),
	LOADWINAPI1(SSL_connect),
	LOADWINAPI1(SSL_read),
//	LOADWINAPI1(SSL_peek),
	LOADWINAPI1(SSL_write),
	LOADWINAPI1(SSL_CTX_ctrl),
	LOADWINAPI1(SSL_shutdown),
	LOADWINAPI1(SSL_get_error),
	{NULL, NULL}
};

#define SSL_CTRL_MODE 33
#define SSL_MODE_AUTO_RETRY 0x00000004L
#define DSSL_CTX_set_mode(ctx, op) DSSL_CTX_ctrl((ctx), SSL_CTRL_MODE, (op), NULL)

// -------------------------
typedef struct {
	SOCKET sock;
	SSL *ssl;
} SSOCKET;

const TCHAR RegProxy[] =T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings");
const TCHAR ProxyEnable[] = T("ProxyEnable");
const char ProxyServer[] = "ProxyServer";
const char DefAgent[] = "PaperPlaneXUI/" FileCfg_Version;
const char HttpResult[] = "=Result=\r\n";
const char SSLerrorstr[] = "\r\n\r\nSocks error\r\n";

void SockErrorMsg(ThSTRUCT *th, ERRORCODE code, HMODULE hModule)
{
	char buf[MAX_BUF];
	char *p;

	strcpy(buf, "\r\n\r\nError:");
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_FROM_HMODULE |
			FORMAT_MESSAGE_IGNORE_INSERTS, hModule, code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT), buf + 10, VFPS, NULL);
	for ( p = buf + 10 ; *p ; p++ ) if ( (BYTE)*p < ' ' ) *p = ' ';
	if ( strlen(buf) < 13 ) wsprintfA(buf + 10, "Unknown(%d)", code);
	ThCatStringA(th, buf);
	return;
}

int GetProxySetting(char *proxyserver)
{
	HKEY HK;
	DWORD size;

	DWORD enable = 0, port = 80;
	char servers[0x800], *p, *q;

	if ( RegOpenKeyEx(HKEY_CURRENT_USER, RegProxy, 0, KEY_READ, &HK) != ERROR_SUCCESS ){
		return 0;
	}
	size = sizeof(DWORD);
	RegQueryValueEx(HK, ProxyEnable, NULL, NULL, (LPBYTE)&enable, &size);
	servers[0] = '\0';
	size = sizeof(servers);
	RegQueryValueExA(HK, ProxyServer, NULL, NULL, (LPBYTE)servers, &size);
	RegCloseKey(HK);

	if ( enable == 0 ) return 0;

	p = strstr(servers, "http=");
	if ( p != NULL ){
		p += 5;
	}else{
		p = strchr(servers, '=');
		if ( p != NULL ) return 0;
		p = servers;
	}
	q = strchr(p, ':');
	if ( q == NULL ){
		q = strchr(p, ';');
		if ( q != NULL ) *q = '\0';
	}else{
		*q++= '\0';
		port = 0;
		while(Isdigit(*q)){
			port = (DWORD)(port * 10 + (BYTE)(*q++ - (BYTE)'0'));
		}
	}
	strcpy(proxyserver, p);
	return port;
}

//１行送信（改行は、内で付加される）
int wsPuts(SSOCKET *ssock, char *str)
{
	char *p;
	int n;
	int len;

	p = str;
	len = strlen32(p);
	while( len != 0 ){
		if ( ssock->ssl == NULL ){
			n = Dsend(ssock->sock, p, len, 0);
			if ( n <= 0 ){
				int errorcode;

				if ( n == 0 ) return NO_ERROR;
				errorcode = DWSAGetLastError();
				if ( errorcode == NO_ERROR ) continue;
				return errorcode;
			}
		}else{
			n = DSSL_write(ssock->ssl, p, len);
			if ( n <= 0 ){
				return DSSL_get_error(ssock->ssl, n);
			}
		}
		p += n;
		len -= n;
	};

	p = "\r\n";
	len = 2;
	while( len != 0 ){
		if ( ssock->ssl == NULL ){
			n = Dsend(ssock->sock, p, len, 0);
			if ( n <= 0 ){
				int errorcode;

				if ( n == 0 ) return NO_ERROR;
				errorcode = DWSAGetLastError();
				if ( errorcode == NO_ERROR ) continue;
				return errorcode;
			}
		}else{
			n = DSSL_write(ssock->ssl, p, len);
			if ( n <= 0 ){
				return DSSL_get_error(ssock->ssl, n);
			}
		}
		p += n;
		len -= n;
	};

	return NO_ERROR;
}

#define INTERNET_SCHEME_HTTPS 2

typedef LPVOID HINTERNET;
#ifndef _WIN64
  #pragma pack(push, 4)
#endif
typedef struct
{
	DWORD dwStructSize;
	LPWSTR lpszScheme;		DWORD dwSchemeLength;
	int nScheme;
	LPWSTR lpszHostName;	DWORD dwHostNameLength;
	INTERNET_PORT nPort;
	LPWSTR lpszUserName;	DWORD dwUserNameLength;
	LPWSTR lpszPassword;	DWORD dwPasswordLength;
	LPWSTR lpszUrlPath;		DWORD dwUrlPathLength;
	LPWSTR lpszExtraInfo;	DWORD dwExtraInfoLength;
} URL_COMPONENTS;
#ifndef _WIN64
  #pragma pack(pop)
#endif

DefineWinAPI(BOOL, WinHttpCrackUrl, (LPCWSTR, DWORD, DWORD, URL_COMPONENTS *));
DefineWinAPI(HINTERNET, WinHttpOpen, (LPCWSTR , DWORD, LPCWSTR, LPCWSTR, DWORD dwFlags));
DefineWinAPI(HINTERNET, WinHttpConnect, (HINTERNET, LPCWSTR, INTERNET_PORT, DWORD));
DefineWinAPI(HINTERNET, WinHttpOpenRequest, (HINTERNET, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR, LPCWSTR *, DWORD));
DefineWinAPI(BOOL, WinHttpSendRequest, (HINTERNET, LPCWSTR, DWORD, LPVOID, DWORD, DWORD, DWORD_PTR));
DefineWinAPI(BOOL, WinHttpReceiveResponse, (HINTERNET, LPVOID));
DefineWinAPI(BOOL, WinHttpQueryDataAvailable, (HINTERNET, LPDWORD));
DefineWinAPI(BOOL, WinHttpReadData, (HINTERNET, LPVOID, DWORD, LPDWORD));
DefineWinAPI(BOOL, WinHttpCloseHandle, (HINTERNET));
//DefineWinAPI(BOOL, WinHttpSetOption, (HINTERNET, DWORD, LPVOID, DWORD));

LOADWINAPISTRUCT WINHTTPDLL[] = {
	LOADWINAPI1(WinHttpCrackUrl),
	LOADWINAPI1(WinHttpOpen),
	LOADWINAPI1(WinHttpConnect),
	LOADWINAPI1(WinHttpOpenRequest),
	LOADWINAPI1(WinHttpSendRequest),
	LOADWINAPI1(WinHttpReceiveResponse),
	LOADWINAPI1(WinHttpQueryDataAvailable),
	LOADWINAPI1(WinHttpReadData),
	LOADWINAPI1(WinHttpCloseHandle),
//	LOADWINAPI1(WinHttpSetOption),
	{NULL, NULL}
};

#define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY 0
#define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#define WINHTTP_FLAG_SECURE 0x00800000

#define WINHTTP_OPTION_CONNECT_TIMEOUT  3
#define WINHTTP_OPTION_SECURE_PROTOCOLS  84
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0x00002000
#define WINHTTP_FLAG_SECURE_DEFAULTS 0x30000000

BOOL GetImageByWinhttp(const TCHAR *urladr, ThSTRUCT *th)
{
	HANDLE hWINHTTP;
	HINTERNET hSession, hConnect, hRequest = NULL;
	URL_COMPONENTS uc;
	WCHAR Agent[MAX_BUF], urladrW[0x800], sep;

	hWINHTTP = LoadWinAPI("winhttp.dll", NULL, WINHTTPDLL, LOADWINAPI_LOAD_ERRMSG);
	if ( hWINHTTP == NULL ) return FALSE;

	#ifdef UNICODE
		strcpyW(urladrW, urladr);
	#else
		AnsiToUnicode(urladr, urladrW, TSIZEOF(urladrW));
	#endif

	memset(&uc, 0, sizeof(uc));
	uc.dwStructSize = sizeof(uc);
	uc.dwHostNameLength = 1;
	uc.dwUserNameLength = 1;
	uc.dwPasswordLength = 1;
	uc.dwUrlPathLength = 1;

	Agent[0] = '\0';
	#ifdef UNICODE
	GetCustData(T("V_httpa"), &Agent, sizeof(Agent));
	#else
	{
		char AgentBufA[MAX_BUF];
		AgentBufA[0] = '\0';
		GetCustData(T("V_httpa"), &AgentBufA, sizeof(AgentBufA));
		AnsiToUnicode(AgentBufA, Agent, TSIZEOF(Agent));
	}
	#endif
	if ( Agent[0] == '\0' ) AnsiToUnicode(DefAgent, Agent, TSIZEOF(Agent));

	if ( DWinHttpCrackUrl(urladrW, 0, 0, &uc) ){
//		DWORD option;

		hSession = DWinHttpOpen( Agent,
				(WinType >= WINTYPE_81) ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY :
				WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL,
				(uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE_DEFAULTS : 0);

//		option = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
//		DWinHttpSetOption(hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &option, sizeof(option));

		sep = *uc.lpszUrlPath;
		*(uc.lpszHostName + uc.dwHostNameLength) = '\0';
		*uc.lpszUrlPath = '\0';
		hConnect = DWinHttpConnect(hSession, uc.lpszHostName, uc.nPort, 0);
		*uc.lpszUrlPath = sep;

		if ( hConnect != NULL ){
			hRequest = DWinHttpOpenRequest(hConnect, L"GET", uc.lpszUrlPath,
					NULL, NULL, NULL,
					(uc.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
		}

		if ( hRequest != NULL ){
			BOOL result;

			result = DWinHttpSendRequest(hRequest, NULL, 0, NULL, 0, 0, 0);
			if ( IsTrue(result) ){
			#ifdef UNICODE
				char urladrA[1040];

				UnicodeToAnsi(urladr, urladrA, sizeof(urladrA));
				urladrA[sizeof(urladrA) - 1] = '\0';
				ThCatStringA(th, "http: winhttpW\r\nGET ");
				ThCatStringA(th, urladrA);
				ThCatStringA(th, "\r\n");
				ThCatStringA(th, HttpResult);
			#else
				thprintf(th, 0, "http: winhttpA\r\nGET %s\r\n%s", urladr, HttpResult);
			#endif
				result = DWinHttpReceiveResponse(hRequest, NULL);

				if ( IsTrue(result) ){
					// ●WinHttpQueryHeaders でステータスコードを取得する予定 2.05+4
					ThCatStringA(th, "HTTP/WINHTTP 200\r\n\r\n");
				}
			}
			if ( IsTrue(result) ){
				for(;;){
					DWORD size, readsize;

					size = 0;
					if ( DWinHttpQueryDataAvailable(hRequest, &size) == FALSE ){
						break;
					}
					if ( size == 0 ) break;

					ThSize(th, size);
					if( DWinHttpReadData(hRequest,
						(LPVOID)(char *)(th->bottom + th->top), size, &readsize) ){
						th->top += readsize;
					}
				}
			}
			DWinHttpCloseHandle(hRequest);
		}
		if ( hConnect != NULL ) DWinHttpCloseHandle(hConnect);
		DWinHttpCloseHandle(hSession);
	}
	#undef urladrW
	FreeLibrary(hWINHTTP);
	ThAppend(th, "", 1);
	return TRUE;
}

ERRORCODE GetImageByWinsock(const TCHAR *urladr, ThSTRUCT *th)
{
	HANDLE hWSOCK32;
	HANDLE hSsleay32 = NULL;

	SSOCKET ssock;
	impADDRINFO ai_default, *adr_info = NULL, *useadr;
	WSADATA wsad;
	SSL_CTX *ctx;

	SOCKADDR_IN addr_in;

	char url[MAX_BUF];
	char host[VFPS];
	char buf[MAX_BUF];
	const char *Agent = DefAgent;
	char AgentBuf[MAX_BUF];
	char AuthorizationBuf[MAX_BUF];
//	DWORD resultoffset;
	ERRORCODE result = NO_ERROR;

	int cache = 0;
	int errorcode;
	int getport = 0, connectport = 80;

	#ifdef UNICODE
		TCHAR bufW[VFPS];
		#define bufT bufW
	#else
		#define bufT buf
	#endif


	HWND hFocusWnd;
	DWORD FocusPID;
	DWORD tickcount;

	char *getdir, *urldir;

#ifdef _WIN64
	#define reqwsver 0x202 // 必ず2.2
#else
	WORD reqwsver;
#endif

#ifdef UNICODE
	hWSOCK32 = LoadWinAPI("WS2_32.dll", NULL, WINSOCKDLL, LOADWINAPI_LOAD_ERRMSG);
#else
	hWSOCK32 = LoadWinAPI((WinType >= WINTYPE_2000) ?
		"WS2_32.dll" : "WSOCK32.DLL", NULL, WINSOCKDLL, LOADWINAPI_LOAD_ERRMSG);
#endif
	if ( hWSOCK32 == NULL ) return ERROR_MOD_NOT_FOUND;
	GETDLLPROC(hWSOCK32, getaddrinfo);
	GETDLLPROC(hWSOCK32, freeaddrinfo);

	if ( urladr[4] == 's' ){ // https://
		if ( hSsleay32 == NULL ){
			tstrcpy(bufT, T("libssl.dll"));
			GetCustTable(StrCustOthers, T("libssl"), bufT, sizeof(bufT));
			hSsleay32 = LoadLibrary(bufT);
			if ( hSsleay32 == NULL ) hSsleay32 = LoadLibrary(T("SSLEAY32.DLL"));
			if ( hSsleay32 != NULL ){
				if ( LoadWinAPI(NULL, hSsleay32, SSLEAY32DLL, 0) == NULL ){
					FreeLibrary(hSsleay32);
					hSsleay32 = NULL;
				}
			}
			if ( hSsleay32 == NULL ){
				FreeLibrary(hWSOCK32);
				return ERROR_MOD_NOT_FOUND;
			}
			GETDLLPROC(hSsleay32, OPENSSL_init_ssl);
			if ( DOPENSSL_init_ssl != NULL ){ // 1.01 以降
				OPTS_VALUE opt_init = OPTS_VALUE_INIT;
				OPTS_VALUE opt_load = OPTS_VALUE_LOAD;

				GETDLLPROC(hSsleay32, TLS_client_method);
				GETDLLPROC(hSsleay32, SSL_ctrl);
				GETDLLPROC(hSsleay32, SSL_set1_host);

				DOPENSSL_init_ssl(opt_init, NULL); // SSL_library_init 相当。今は不要
				DOPENSSL_init_ssl(opt_load, NULL); // SSL_load_error_strings 相当。今は不要
			}else{ // 0.98
				GETDLLPROC(hSsleay32, SSL_library_init);
				GETDLLPROC(hSsleay32, SSL_load_error_strings);
				DTLS_client_method = (impTLS_client_method)GetProcAddress(hSsleay32, "SSLv23_client_method");
//				GETDLLPROC(hSsleay32, SSLv23_client_method);
				if ( DTLS_client_method == NULL ){
					FreeLibrary(hSsleay32);
					FreeLibrary(hWSOCK32);
					return ERROR_INVALID_FUNCTION;
				}
				DSSL_library_init();
				DSSL_load_error_strings();
			}
		}
		//	RAND_seed(seed, sizeof(seed) - 1);
		//	SSLeay_add_ssl_algorithms();
		ctx = DSSL_CTX_new(DTLS_client_method());
		//	DSSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
		//	DSSL_CTX_set_default_verify_paths(ctx);
		//SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION)) {
		DSSL_CTX_set_mode(ctx, SSL_MODE_AUTO_RETRY);
	}
#ifndef _WIN64
	if ( WinType >= WINTYPE_XP ){
		reqwsver = 0x202; // XP以降は2.2
	}else{
		reqwsver = 0x101; // 2000以前は1.1
	}
#endif
	if ( DWSAStartup(reqwsver, &wsad) != 0 ) return ERROR_NOT_READY;

	AuthorizationBuf[0] = '\0';
	hFocusWnd = GetFocus();
	GetWindowThreadProcessId(hFocusWnd, &FocusPID);
	if ( FocusPID != GetCurrentProcessId() ) hFocusWnd = NULL;

	{
		char domain[MAX_PATH];
		TCHAR portbuf[32];
		char *q, *r;
												// コード変換 -----------------
		#ifdef UNICODE
			UnicodeToAnsi(urladr, url, sizeof(url));
		#else
			strcpy(url, urladr);
		#endif
		q = url;
		for ( ; ; ){
			r = strchr(q, '&');
			if ( r == NULL ) break;
			q = r + 1;
			if ( !memcmp( q, "amp;", 4) ){
				memmove( q, q + 4, strlen(q) - 3);
			}
		}
/*
		q = url;
		for ( ; ; ){
			r = strchr(q, '%');
			if ( r == NULL ) break;
			q = r + 1;
			if ( IsxdigitA(*q) && IsxdigitA(*(q + 1)) ){
				*r = (BYTE)(GetHexCharA(&q) * 16);
				*r |= (BYTE)GetHexCharA(&q);
				memmove( r + 1, q, strlen(q) + 1);
				q = r + 1;
			}
		}
*/
												// ドメイン部分を切り出し -----
		getdir = url + sizeof("http://") - 1;
		if ( url[4] == 's' ) getdir++;
		urldir = strchr(getdir, '/');
		if ( urldir == NULL ){
			urldir = getdir + strlen(getdir);
			strcpy(urldir, "/index.html");
		}
		memcpy(host, getdir, urldir - getdir);
		*(host + (urldir - getdir)) = '\0';
		{										// ポートの指定 ---------------
			char *pt;

			#pragma warning(suppress:6054) //*(host～でNil team
			pt = strchr(host, ':');
			if ( pt != NULL ){
				*pt++ = '\0';
#ifdef UNICODE
				getport = (int)GetNumberA((const char **)&pt);
#else
				getport = (int)GetNumber((const char **)&pt);
#endif
			}
			if ( getport == 0 ) getport = (hSsleay32 == NULL) ? 80 : 443;
//			addr_in.sin_port = Dhtons((WORD)port);	//ポートの指定
			#if 0
				struct servent *service;
				service = Dgetservbyname("http", "tcp");
				if ( service ){
					addr_in.sin_port = service->s_port;
				}else{
					addr_in.sin_port = Dhtons((u_short)80);	//ポートの指定
				}
			#endif
		}

		#ifdef UNICODE
			bufW[0] = '\0';
			GetCustData(T("V_proxy"), &bufW, sizeof(bufW));
			UnicodeToAnsi(bufW, domain, sizeof(domain));
		#else
			domain[0] = '\0';
			GetCustData(T("V_proxy"), &domain, sizeof(domain));
		#endif

		if ( domain[0] == '\0' ){
			connectport = GetProxySetting(domain);
		}
		if ( domain[0] != '\0' ){
			getdir = url;
			ThCatStringA(th, "Domain(proxy):");

			portbuf[0] = '\0';
			GetCustData(T("V_http"), &portbuf, sizeof(portbuf));
			if ( portbuf[0] != '\0' ){
				const TCHAR *tp;

				tp = portbuf;
				connectport = (int)GetNumber(&tp);
			}
		}else{
			connectport = getport;
			strcpy(domain, host);
			getdir = urldir;
			ThCatStringA(th, "Domain:");
		}
		ThCatStringA(th, domain);
		ThCatStringA(th, "\r\n");

		if ( hFocusWnd != NULL ){
			if ( SendMessage(hFocusWnd, WM_PPXCOMMAND, K_SETPOPLINENOLOG,
					(LPARAM)T("Resolving")) != 1 ){
				hFocusWnd = NULL;
			}
		}
		memset(&ai_default, 0, sizeof(ai_default));
		ai_default.ai_socktype = SOCK_STREAM;
		ssock.ssl = NULL;
		ssock.sock = INVALID_SOCKET;

		if ( Dgetaddrinfo == NULL ){ // 古い形式
			DWORD ip;

			useadr = &ai_default;
			addr_in.sin_family = AF_INET;
			addr_in.sin_port = Dhtons((u_short)connectport);
			ai_default.ai_addrlen = sizeof(addr_in);
			ai_default.ai_addr = (struct sockaddr *)&addr_in;
									//ドット形式のアドレスをIPアドレスへ変換
			ip = Dinet_addr(domain);
			if ( ip == INADDR_NONE ){ //ドット形式でないとき
				struct hostent *pHost;

				pHost = Dgethostbyname(domain);//ホスト名->IPアドレス変換
				if ( pHost != NULL ) ip = *((DWORD*)(pHost->h_addr));
			}
			if ( ip != INADDR_NONE ){
				addr_in.sin_addr.s_addr = ip;
				ssock.sock = Dsocket(PF_INET, SOCK_STREAM, 0);
			}
		}else{ // IPv6対応版
			wsprintfA(buf, "%d", connectport);
			ai_default.ai_family = PF_UNSPEC;
			if ( Dgetaddrinfo(domain, buf, &ai_default, &adr_info) == 0 ){
				for ( useadr = adr_info ; useadr != NULL ; useadr = useadr->ai_next ){
					ssock.sock = Dsocket(useadr->ai_family, useadr->ai_socktype, useadr->ai_protocol);
					if ( ssock.sock != INVALID_SOCKET ) break;
				}
			}
		}
	}

	AgentBuf[0] = '\0';
	#ifdef UNICODE
	bufW[0] = '\0';
	GetCustData(T("V_httpa"), &bufW, sizeof(bufW));
	UnicodeToAnsi(bufW, AgentBuf, sizeof(AgentBuf));
	#else
	GetCustData(T("V_httpa"), &AgentBuf, sizeof(AgentBuf));
	#endif
	if ( AgentBuf[0] != '\0' ) Agent = AgentBuf;

													//ソケットのオープン
	if ( ssock.sock != INVALID_SOCKET ) for(;;){
		int retry = 3;

		if ( hFocusWnd != NULL ){
			SendMessage(hFocusWnd, WM_PPXCOMMAND,
					K_SETPOPLINENOLOG, (LPARAM)T("Connecting"));
		}
													//接続
		if ( Dconnect(ssock.sock, useadr->ai_addr, useadr->ai_addrlen) ) break;
		if ( hSsleay32 != NULL ){
			if ( getdir == url ){ // proxy
				int getlen;

				wsprintfA(buf, "CONNECT %s:%d HTTP/1.1\r\n", host, getport);
				ThCatStringA(th, buf);
				errorcode = wsPuts(&ssock, buf);
				if ( errorcode != NO_ERROR ){
					SockErrorMsg(th, errorcode, hWSOCK32);
					break;
				}
				getlen = Drecv(ssock.sock, buf, sizeof(buf) - 1, 0);
				if ( getlen <= 0 ) break;

				buf[getlen] = '\0';
				ThCatStringA(th, buf);
				if ( strstr(buf, " 200") == NULL ) break; // 200 以外は失敗
				if ( th->top > 2 ) th->top -= 2;
				getdir = urldir;
			}
			ssock.ssl = DSSL_new(ctx);
			DSSL_set_fd(ssock.ssl, (int)ssock.sock);	// 注意 !x64! ssock.sock で警告

			if ( DSSL_ctrl != NULL ) SSL_set_tlsext_host_name(ssock.ssl, host);
			if ( DSSL_set1_host != NULL ) DSSL_set1_host(ssock.ssl, host);
			if ( DSSL_connect(ssock.ssl) != 1 ){
				result = ERROR_MOD_NOT_FOUND;
				break;
			}
		}

		GetCustData(T("V_httpc"), &cache, sizeof(cache));

		wsprintfA(buf,
			"GET %s HTTP/1.0\r\n"
			"Accept: text/html, */*\r\n"
			"Accept-Language: ja-JP\r\n"
			"User-Agent: %s\r\n"
			"Connection: close\r\n"
//			"Authorization: \r\n"
//			"Proxy-Authorization: \r\n"
//			"Referer: \r\n"
//			"From: tester@hogehoge\r\n"
			"Host: %s\r\n"
			, getdir, Agent, host);
		if ( cache != 0 ){
			strcat(buf, "Pragma: no-cache\r\nCache-Control: no-cache\r\n");
		}
		errorcode = wsPuts(&ssock, buf);
		if ( errorcode != NO_ERROR ){
			SockErrorMsg(th, errorcode, hWSOCK32);
			break;
		}
		ThCatStringA(th, buf);
		ThCatStringA(th, HttpResult);
//		resultoffset = th->top;

		if ( hFocusWnd != NULL ){
			SendMessage(hFocusWnd, WM_PPXCOMMAND, K_SETPOPLINENOLOG, (LPARAM)T("Reading"));
			tickcount = GetTickCount();
		}
		for ( ; ; ){
			int getlen;

			if ( (WinType >= WINTYPE_VISTA) && (th->size >= (32 * KB)) ){
				getlen = (th->size / 2) & ~MAX_BUF;
			}else{
				getlen = MAX_BUF;
			}
			ThSize(th, getlen);
			if ( ssock.ssl == NULL ){
				getlen = Drecv(ssock.sock,
						th->bottom + th->top, th->size - th->top - 1, 0);
			}else{
				getlen = DSSL_read(ssock.ssl,
						th->bottom + th->top, th->size - th->top - 1);
			}
			*(th->bottom + th->top + getlen) = '\0';

			if ( getlen <= 0 ){ // 失敗・エラー
				if ( ssock.ssl != NULL ){ // SSL
					int sslresult;

					sslresult = DSSL_get_error(ssock.ssl, getlen);
					if ( sslresult != 6 ){ // 6:SSL_ERROR_ZERO_RETURN(ポートが閉じた)
						thprintf(bufT, 32, T("SSL error %d"), sslresult);
						ThCatStringA(th, buf);
					}
					break;
				}else if ( getlen == 0 ){ // WinSocks
					retry--;
					if ( retry == 0 ) break;
				}else if ( getlen == SOCKET_ERROR ){
					int wsaerrorcode;

					wsaerrorcode = DWSAGetLastError();
					if ( wsaerrorcode == NO_ERROR ) continue;
					SockErrorMsg(th, wsaerrorcode, hWSOCK32);
					break;
				}
			}else{
				retry = 3;
			}

			th->top += getlen;
			if ( hFocusWnd != NULL ){
				DWORD newtickcount;

				newtickcount = GetTickCount();
				if ( (newtickcount - tickcount) >= 200 ){
					tickcount = newtickcount;
					thprintf(bufT, 32, T("Reading %d"), th->top);
					SendMessage(hFocusWnd, WM_PPXCOMMAND, K_SETPOPLINENOLOG, (LPARAM)bufT);
				}
			}
		}

		ThAppend(th, "", 1);
		if ( ssock.ssl != NULL ){
			DSSL_shutdown(ssock.ssl);
			DSSL_free(ssock.ssl);
			DSSL_CTX_free(ctx);
		}
		Dshutdown(ssock.sock, 0);	// recv を無効に
#if 0
		{
			char *p;

			p = th->bottom + resultoffset;
			p = strchr(p, ' ');
			if ( p != NULL ){
				p++;
				if ( !memcmp(p, "401", 3) ){
				// Authorization: Basic base64==
				}
			}
		}
#endif
		break;
	}

	if ( hFocusWnd != NULL ){
		SendMessage(hFocusWnd, WM_PPXCOMMAND, K_SETPOPLINENOLOG, (LPARAM)NULL);
	}

	Dclosesocket(ssock.sock);
	if ( adr_info != NULL ) Dfreeaddrinfo(adr_info);
	DWSACleanup();
	FreeLibrary(hWSOCK32);
	if ( result == ERROR_MOD_NOT_FOUND ) ThFree(th);

	return result;
}

VFSDLL BOOL PPXAPI GetImageByHttp(const TCHAR *urladr, ThSTRUCT *th)
{
	ERRORCODE result;

	ThInit(th);

	result = GetImageByWinsock(urladr, th);
	if ( result == NO_ERROR ) return TRUE;
	if ( result == ERROR_MOD_NOT_FOUND ){
		th->top = 0;
		return GetImageByWinhttp(urladr, th);
	}
	return FALSE;
}

BOOL MakeWebListSub(ThSTRUCT *th, char *p, char *name)
{
	char *q, buf[MAX_PATH];

	while ( (p = strstr(p, name)) != NULL ){
		p += strlen(name);

		while ( (*p == ' ') || (*p == '\t') ) p++;
		if ( *p != '=' ) continue;
		p++;
		while ( (*p == ' ') || (*p == '\t') ) p++;
		if ( *p != '\"' ){
			q = p;
			while ( (BYTE)*q > ' ' ) q++;
		}else{
			p++;
			q = strchr(p, '\"');
			if ( !q ) continue;
		}
		if ( (DWORD)(q - p) >= MAX_PATH ) continue;
		if ( (DWORD)(q - p) == '\0' ) continue;
		if ( *p == '#' ) continue; // アンカーはリスト不要
		memcpy(buf, p, q - p);
		buf[q - p] = '\0';
		p = q;

		// 重複チェック
		q = th->bottom;
		while ( (DWORD)(q - th->bottom) < th->top ){
			if ( strcmp(q, buf) == 0 ) break;
			q += strlen(q) + 1;
		}
		if ( (DWORD)(q - th->bottom) >= th->top ){
			#ifdef UNICODE
				TCHAR nameA[MAX_PATH];

				AnsiToUnicode(buf, nameA, MAX_PATH);
				ThAddString(th, nameA);
			#else
				ThAddString(th, buf);
			#endif
		}
	}
	return TRUE;
}

ERRORCODE MakeWebList(FF_MC *mc, const TCHAR *filename, BOOL file)
{
	ThSTRUCT th;
	char *bottom;

	ThInit(&mc->dirs);
	ThInit(&mc->files);
	mc->d_off = 0;
	mc->f_off = 0;
										// 「\\」==============================
	if ( file == FALSE ){
		char *p;
		char statusF;

		ThAddString(&mc->dirs, T(":"));

		if ( GetImageByHttp(filename, &th) == FALSE ) return ERROR_READ_FAULT;
		bottom = (char *)th.bottom;
		// ステータスを取得する
		p = strstr(bottom, HttpResult);
		if ( p != NULL ){
			p += sizeof(HttpResult) - 1;
			for (;;){
				char c = *p;

				if ( IsalnumA(c) || (c == '/') || (c == '.') ){
					p++;
					continue;
				}
				break;
			}
			for (;;){
				statusF = *p;
				if ( statusF == ' ' ){
					p++;
					continue;
				}
				break;
			}
		}else{
			p = bottom;
			statusF = '\0';
		}
		if ( statusF != '2' ){
			if ( IsdigitA(statusF) ){
				char *plf;

				plf = strchr(p, '\r');
				if ( plf != NULL ) *plf = '\0';
				if ( strlen(p) > 200 ) *(p + 200) = '\0';

				#ifdef UNICODE
				{
					TCHAR nameA[MAX_PATH];

					AnsiToUnicode(p, nameA, MAX_PATH);
					ThAddString(&mc->files, nameA);
				}
				#else
					ThAddString(&mc->files, p);
				#endif
			}else{
				ThAddString(&mc->files, T("HTTP unknown error"));
			}
			bottom = NULL;
		}else{
			p = strstr(bottom, "\r\n\r\n");
			if ( p && *(p + 4) ){
				bottom = p + 4;
			}
		}
	}else{
		bottom = NULL;
		if ( LoadFileImage(filename, 0x40, &bottom, NULL, NULL) ){
			return GetLastError();
		}

		ThAddString(&mc->dirs, T("."));
		ThAddString(&mc->dirs, T(".."));
	}
	if ( bottom != NULL ){
		MakeWebListSub(&mc->dirs, bottom, "HREF");
		MakeWebListSub(&mc->dirs, bottom, "href");
		MakeWebListSub(&mc->files, bottom, "SRC");
		MakeWebListSub(&mc->files, bottom, "src");
	}
	ThAddString(&mc->dirs, NilStr);
	ThAddString(&mc->files, NilStr);

	if ( file ) HeapFree(DLLheap, 0, bottom);
	return NO_ERROR;
}

#ifndef IF_MAX_PHYS_ADDRESS_LENGTH
#define IF_MAX_PHYS_ADDRESS_LENGTH 32
#endif

#define AF_UNSPEC 0
#define AF_INET   2
#define AF_INET6  23

#ifndef _WIN64
  #pragma pack(push, 4)
#endif

typedef ULONG xIN_ADDR;
typedef struct {
	UCHAR       Byte[16];
} xIN6_ADDR;

typedef struct
{
	/* ADDRESS_FAMILY */ USHORT sin_family;
	USHORT sin_port;
	xIN_ADDR sin_addr;
	CHAR sin_zero[8];
} xSOCKADDR_IN;

typedef struct
{
	/* ADDRESS_FAMILY */ USHORT sin6_family;
	USHORT sin6_port;
	ULONG  sin6_flowinfo;
	xIN6_ADDR sin6_addr;
	ULONG sin6_scope_id;
} xSOCKADDR_IN6;

typedef union {
	xSOCKADDR_IN Ipv4;
	xSOCKADDR_IN6 Ipv6;
	/* ADDRESS_FAMILY */ USHORT si_family;
} xSOCKADDR_INET;

struct xNET_LUID { DWORD Value, Info; };

typedef struct {
	xSOCKADDR_INET Address;
	/* NET_IFINDEX */ ULONG InterfaceIndex;
	LARGE_INTEGER InterfaceLuid;
	UCHAR PhysicalAddress[IF_MAX_PHYS_ADDRESS_LENGTH];
	ULONG PhysicalAddressLength;
	/*NL_NEIGHBOR_STATE*/ int State;
	UCHAR Flags;
	ULONG ReachabilityTime;
} xMIB_IPNET_ROW2;

typedef struct {
	ULONG NumEntries;
#ifndef _WIN64
	int dummy;
#endif
	xMIB_IPNET_ROW2 Table[1];
} xMIB_IPNET_TABLE2;
#ifndef _WIN64
  #pragma pack(pop)
#endif

typedef struct {
	int port;
	DWORD flag;
} CheclPortListStruct;
CheclPortListStruct CheclPortList[] = {
//	{22, B0}, // TCP 22  ssh
//	{5353, B0}, // UDP 5353  mdns / Bonjour
	{445, B0}, // TCP 445  SMB
	{443, B1}, // TCP 443  https
	{80, B2}, // TCP 80  http
	{0, 0}
};

extern DWORD CheckPorts(xMIB_IPNET_ROW2 *);
DWORD CheckPorts(xMIB_IPNET_ROW2 *target)
{
	HANDLE hWSOCK32;
	CheclPortListStruct *list = CheclPortList;

	SSOCKET ssock;
	impADDRINFO ai_default, *useadr;
	WSADATA wsad;
	DWORD result = 0;

	SOCKADDR_IN addr_in;

	#define reqwsver 0x202 // 必ず2.2

	if ( WinType < WINTYPE_XP ) return 0;

#ifdef UNICODE
	hWSOCK32 = LoadWinAPI("WS2_32.dll", NULL, WINSOCKDLL, LOADWINAPI_LOAD_ERRMSG);
#else
	hWSOCK32 = LoadWinAPI((WinType >= WINTYPE_2000) ?
		"WS2_32.dll" : "WSOCK32.DLL", NULL, WINSOCKDLL, LOADWINAPI_LOAD_ERRMSG);
#endif
	if ( hWSOCK32 == NULL ) return 0;

	if ( DWSAStartup(reqwsver, &wsad) != 0 ) return 0;

	useadr = &ai_default;
	memset(&ai_default, 0, sizeof(ai_default));
	ai_default.ai_socktype = SOCK_STREAM;
	ai_default.ai_addrlen = sizeof(addr_in);
	ai_default.ai_addr = (struct sockaddr *)&addr_in;

	for (;;){
		addr_in = *(SOCKADDR_IN *)&target->Address.Ipv4;
		addr_in.sin_family = AF_INET;
		addr_in.sin_port = Dhtons((u_short)list->port);

		ssock.ssl = NULL;
		ssock.sock = Dsocket(PF_INET, SOCK_STREAM, 0);
													//ソケットのオープン
		if ( ssock.sock != INVALID_SOCKET ){
			u_long blockmode = 1;
			int s_result;

			Dioctlsocket(ssock.sock, FIONBIO, &blockmode); // 非ブロッキング
													//接続
			s_result = Dconnect(ssock.sock, useadr->ai_addr, useadr->ai_addrlen);
			// connect 完了を待つ
			if ( (s_result != 0) && (DWSAGetLastError() == WSAEWOULDBLOCK) ){
				fd_set readFd, writeFd, errFd;
				const struct timeval timeout = {0, 20 * 1000};

				FD_ZERO(&readFd);
				FD_ZERO(&writeFd);
				FD_ZERO(&errFd);
				FD_SET(ssock.sock, &readFd);
				FD_SET(ssock.sock, &writeFd);
				FD_SET(ssock.sock, &errFd);
				s_result = Dselect(0, &readFd, &writeFd, &errFd, &timeout);
				s_result = (s_result != 0) && (s_result != SOCKET_ERROR) ? 0 : SOCKET_ERROR;
			}

			if ( s_result == 0 ) setflag(result, list->flag);

			Dshutdown(ssock.sock, 0);
			Dclosesocket(ssock.sock);
		}
		list++;
		if ( list->port == 0 ) break;
	}

	DWSACleanup();
	FreeLibrary(hWSOCK32);
	return result;
}
