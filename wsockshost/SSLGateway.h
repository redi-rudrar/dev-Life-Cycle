#ifndef _SSLGATEWAY_H_
#define _SSLGATEWAY_H_

#include <openssl\bio.h>
#include <openssl\engine.h>
#include <openssl\err.h>
#include <openssl\rand.h>
#include <openssl\ssl.h>
#include <openssl\x509v3.h>

#include "wsocksapi.h"

struct SSLInfo
{
    int         fd;
    SSL*        ssl;
    bool        activated;

    SSLInfo(int _fd, SSL* _ssl) : fd(_fd),ssl(_ssl),activated(false) {}
};

class SSLGatewayNotify
{
public:
	virtual void SSLLogError(const char *format, ...)=0;
	virtual void SSLLogEvent(const char *format, ...)=0;
};

class SSLGateway
{
public:
	static SSLGatewayNotify *pNotify;

	static inline SSLGateway* instance(){return _instance;}

    SSLGateway();
    bool SSLInit(SSLGatewayNotify *pNotify, const char* Passwd, const char* CAFILE, const char* CERTFILE);
	void SSLCleanup();

    SSLInfo *ServerSetup(int fd);
    SSLInfo *ClientSetup(int fd);

    bool IsActivated(SSLInfo *sslInfo);

    bool SSLAccept(SSLInfo *sslInfo);
    bool SSLConnect(SSLInfo *sslInfo);
    int  SSLSend(SSLInfo *sslInfo, const char* buf, int size);
    int  SSLRecv(SSLInfo *sslInfo, char* buf, int size);
    bool SSLRemove(SSLInfo *pSSLInfo);
	void SSLCleanErrorState();

	void SSLLock(int n);
	void SSLUnlock(int n);

private:
	static SSLGateway* _instance;

    char PemPasswd[128];

    bool ServerInitialized;
    bool ClientInitialized;

    SSL_CTX *ServerCtx;
    SSL_CTX *ClientCtx;

    static int SSL_pem_passwd_cb(char *buf, int size, int rwflag, void *userdata);

    DH * SetDHE1024();
	DH* tls_dhe1024;
	HANDLE *pMutexes;

    bool SSLGetError(int r, SSLInfo* sslInfo);
    bool SSLInitialize(const char* CAFILE, const char* CERTFILE, bool IsServer);
    SSLInfo *SSLSetup(int fd, bool IsServer);
};

#endif