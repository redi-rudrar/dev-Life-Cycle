#include "stdafx.h"
#include "SSLGateway.h"

SSLGateway* SSLGateway::_instance = NULL;
SSLGatewayNotify *SSLGateway::pNotify = NULL;

void locking_function(int mode, int n, const char *file, int line)
{
	if(mode & CRYPTO_LOCK)
		SSLGateway::instance()->SSLLock(n);
	else
		SSLGateway::instance()->SSLUnlock(n);
}

int MY_verify_callback(int ok, X509_STORE_CTX *store)
{
	char data[256];
	if (!ok)
	{
		X509 *cert = X509_STORE_CTX_get_current_cert(store);
		int depth = X509_STORE_CTX_get_error_depth(store);
		int err = X509_STORE_CTX_get_error(store);
		if(SSLGateway::pNotify)
			SSLGateway::pNotify->SSLLogError("WSSL -Error with certificate at depth: %i\n", depth);
		X509_NAME_oneline(X509_get_issuer_name(cert), data, 256);
		if(SSLGateway::pNotify)
			SSLGateway::pNotify->SSLLogError("issuer = %s\n", data);
		X509_NAME_oneline(X509_get_subject_name(cert), data, 256);
		if(SSLGateway::pNotify)
		{
			SSLGateway::pNotify->SSLLogError(" subject = %s\n", data);
			SSLGateway::pNotify->SSLLogError(" err %i:%s\n", err, X509_verify_cert_error_string(err));
		}
	}
	return ok;
}

SSLGateway::SSLGateway()
	:ServerCtx(0)
	,ClientCtx(0)
	,ServerInitialized(false)
	,ClientInitialized(false)
	,tls_dhe1024(0)
	,pMutexes(0)
{
	SSLGateway::_instance=this;
    CRYPTO_malloc_init();           // Initialize malloc, free, etc for OpenSSL's use
    SSL_library_init();             // Initialize OpenSSL's SSL libraries
    SSL_load_error_strings();       // Load SSL error strings
    ERR_load_BIO_strings();         // Load BIO error strings
    OpenSSL_add_all_algorithms();   // Load all available encryption algorithms

	if(!pMutexes)
	{
		int n = CRYPTO_num_locks();
		pMutexes = new HANDLE[n + 1];
		for(int i=0;i<=n;i++)
		{
			pMutexes[i] = CreateMutex(0,false,0);
		}
	}
}

void SSLGateway::SSLCleanup()
{
	if(!SSLGateway::_instance)
		return;

	if(ServerCtx)
	{
		SSL_CTX_free(ServerCtx);
		ServerCtx = 0;
	}

	if(ClientCtx)
	{
		SSL_CTX_free(ClientCtx);
		ServerCtx = 0;
	}

	if(tls_dhe1024)
	{
		DH_free(tls_dhe1024);
		tls_dhe1024 = NULL;
	}

	ERR_remove_state(0);
	ENGINE_cleanup();
	CONF_modules_finish();
	CONF_modules_unload(1);
	CONF_modules_free();
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();

	OBJ_cleanup();
	COMP_zlib_cleanup();
	RAND_cleanup();

	if(pMutexes)
	{
		delete[] pMutexes;
		pMutexes = NULL;
	}

	//delete SSLGateway::_instance;
	SSLGateway::_instance = NULL;
}

DH * SSLGateway::SetDHE1024()
{
    DSA *dsaparams;
    DH *dhparams;
    /* random parameters (may take a while) */
    dsaparams = DSA_generate_parameters(1024, NULL, 0, NULL, NULL, 0, NULL);

    if (dsaparams == NULL) 
    {
	    return NULL;
    }
    dhparams = DSA_dup_DH(dsaparams);
    DSA_free(dsaparams);
    if (dhparams == NULL) 
    {
	    return NULL;
    }
    return dhparams;
}

int SSLGateway::SSL_pem_passwd_cb(char *buf, int size, int, void *)
{
    memset(buf, 0, size);
    strncpy_s(buf, size, SSLGateway::instance()->PemPasswd, size-1);
    return (int)strlen(SSLGateway::instance()->PemPasswd);
}

bool SSLGateway::SSLInit(SSLGatewayNotify *pNotify, const char* Passwd, const char* CAFILE, const char* CERTFILE)
{
	this->pNotify=pNotify;
    strcpy_s(PemPasswd, Passwd);
    if((*CAFILE)&&(*CERTFILE)&&(!SSLInitialize(CAFILE, CERTFILE, true)))  //Initialize server component
		return false;
    if(!SSLInitialize(0, 0, false))  //Initialize client component
		return false;
	return true;
}

SSLInfo *SSLGateway::ServerSetup(int fd)
{
    return SSLSetup(fd, true);
}

SSLInfo *SSLGateway::ClientSetup(int fd)
{
    return SSLSetup(fd, false);
}

SSLInfo *SSLGateway::SSLSetup(int fd, bool IsServer)
{
    SSL_CTX * ctx;
    if(IsServer)
        ctx = ServerCtx;
    else
        ctx = ClientCtx;

    SSL* ssl = SSL_new(ctx);
    if(!ssl)
    {
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error creating SSL context: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
 
    if (!SSL_set_fd(ssl, fd))
    {
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error setting client fd to ssl: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }

    if(IsServer)
        SSL_set_accept_state(ssl);
    else
	    SSL_set_connect_state(ssl);

    SSLInfo *pSSLInfo = new SSLInfo(fd, ssl);
	return pSSLInfo;
}

bool SSLGateway::SSLRemove(SSLInfo *pSSLInfo)
{
	int returnval = 0;
	do
	{
		returnval = SSL_shutdown(pSSLInfo->ssl);
		Sleep(1);
	} while(returnval == 0);

	SSL_free(pSSLInfo->ssl);
    return true;
}

bool SSLGateway::SSLInitialize(const char* CAFILE, const char* CERTFILE, bool IsServer)
{
    SSL_CTX * ctx = NULL;
    int (*pem_passwd_cb)(char*,int, int, void*) = SSL_pem_passwd_cb;
    int (*verify_callback)(int, X509_STORE_CTX*);
    if(IsServer)
    {
        if(ServerCtx == NULL)
            ServerCtx = SSL_CTX_new(SSLv23_server_method( ));
        ctx = ServerCtx;
        verify_callback = MY_verify_callback;
    }
    else
    {
        if(ClientCtx == NULL)
            ClientCtx = SSL_CTX_new(SSLv23_client_method( ));
        ctx = ClientCtx;
        verify_callback = MY_verify_callback;
    }

    SSL_CTX_set_mode(ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);

	if (CAFILE && SSL_CTX_load_verify_locations(ctx, CAFILE, NULL) != 1)
    {
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error loading CA file and/or directory: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
	if (SSL_CTX_set_default_verify_paths(ctx) != 1)
    {
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error when calling SSL_CTX_set_default_verify_paths: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }

	if(CERTFILE && SSL_CTX_use_certificate_chain_file(ctx, CERTFILE) != 1)
	{
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error when calling SSL_CTX_set_default_verify_paths: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
	}

    SSL_CTX_set_default_passwd_cb(ctx, pem_passwd_cb);

	if (CERTFILE && SSL_CTX_use_PrivateKey_file(ctx, CERTFILE, SSL_FILETYPE_PEM) != 1)
    {
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error loading private key from file: %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }
	SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, verify_callback);
	SSL_CTX_set_verify_depth(ctx, 4);
	SSL_CTX_set_options(ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_SINGLE_DH_USE);
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

    if(IsServer)
    {
        tls_dhe1024 = SetDHE1024();
        if(!tls_dhe1024)
        {
			if(pNotify)
				pNotify->SSLLogError("WSSSL: Error creating DH: %s\n", ERR_error_string(ERR_get_error(), NULL));
            return false;
        }
        if (!SSL_CTX_set_tmp_dh(ctx, tls_dhe1024))
        {
			if(pNotify)
				pNotify->SSLLogError("WSSSL: Error setting DH: %s\n", ERR_error_string(ERR_get_error(), NULL));
            return false;
        }
    }

	if (SSL_CTX_set_cipher_list(ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH") != 1)
    {
		if(pNotify)
			pNotify->SSLLogError("WSSSL: Error setting cipher list (no valid ciphers): %s\n", ERR_error_string(ERR_get_error(), NULL));
        return false;
    }

    if(IsServer)
        ServerInitialized = true;
    else
        ClientInitialized = true;

	CRYPTO_set_locking_callback(locking_function);
	SSL_CTX_set_session_cache_mode(ctx, SSL_SESS_CACHE_OFF);

	return true;
}

bool SSLGateway::SSLGetError(int r, SSLInfo* sslInfo)
{
    int err = SSL_get_error(sslInfo->ssl, r);

    switch (err) 
    {
    case SSL_ERROR_NONE:
	case SSL_ERROR_SSL: // should this be ignored?  we still seemed ot make a good connection...
    case SSL_ERROR_WANT_WRITE:
    case SSL_ERROR_WANT_READ:
	    return true;

    case SSL_ERROR_ZERO_RETURN:
	    return false;

    case SSL_ERROR_SYSCALL: // This is usually a socket error
		#ifdef _DEBUG // Spam warning
		if(pNotify)
			pNotify->SSLLogEvent("SSL_ERROR_SYSCALL: %d",  (int)GetLastError());
		#endif
	    return false;

    default:
		if(pNotify)
			pNotify->SSLLogEvent("SSL Error=%d, %s", err, ERR_error_string(ERR_get_error(), NULL));
        break;
    }

    return false;
}

bool SSLGateway::SSLAccept(SSLInfo *sslInfo)
{
    if(sslInfo == NULL)
	{
        return false;
	}

    if(sslInfo->activated) 
	{
        return true;
	}

    int n = SSL_accept(sslInfo->ssl);
    bool r = SSLGetError(n, sslInfo);

    if (r == false) 
    {
        return false;
    }
    if (!SSL_in_init(sslInfo->ssl)) 
    {
        sslInfo->activated = true;
    }
	// BAO: For non-blocking sockets, we need to wait a little and try accept again
	else
    {
		int err = SSL_get_error(sslInfo->ssl, n);
		// This block of code if we don't want to return until SSL handshake completed
		//DWORD tstart=GetTickCount();
		//while((err==SSL_ERROR_WANT_READ)||(err==SSL_ERROR_WANT_WRITE))
		//{
		//#ifndef _DEBUG
		//	// To prevent holding the server too long, set a timeout for release build
		//	DWORD tnow=GetTickCount();
		//	if(tnow -tstart>=5000)
		//		break;
		//#endif
		//	SleepEx(10,true);
		//	n = SSL_accept(sslInfo->ssl);
		//	err = SSL_get_error(sslInfo->ssl, n);
		//}
		//if (!SSL_in_init(sslInfo->ssl)) 
		//{
		//	sslInfo->activated = true;
		//}
		//else
		//	return false;

		// This block of code if we want to let the app continue and call SSLAccept again
		if((err!=SSL_ERROR_WANT_READ)&&(err!=SSL_ERROR_WANT_WRITE))
			return false;
    }

    return true;
}

bool SSLGateway::SSLConnect(SSLInfo *sslInfo)
{
    if(sslInfo == NULL)
	{
        return false;
	}

    if(sslInfo->activated) 
	{
        return true;
	}

    int n = SSL_connect(sslInfo->ssl);

    bool r = SSLGetError(n, sslInfo);
    if (r == false) 
    {
        return false;
    }

    if (!SSL_in_init(sslInfo->ssl)) 
    {
        sslInfo->activated = true;
    }
	// BAO: For non-blocking sockets, we need to wait a little and try connect again
	else
    {
		int err = SSL_get_error(sslInfo->ssl, n);
		// This block of code if we want to let the app continue and call SSLConnect again
		if((err!=SSL_ERROR_WANT_READ)&&(err!=SSL_ERROR_WANT_WRITE))
			return false;
    }
       
    return true;
}

int SSLGateway::SSLSend(SSLInfo *sslInfo, const char* msg, int size)
{
    if(sslInfo == NULL)
	{
        return 0;
	}

    int n = SSL_write(sslInfo->ssl, msg, size);

    bool r = SSLGetError(n, sslInfo);

    if (r == false)
    {
        return -1;
    }

    return n;
}

int SSLGateway::SSLRecv(SSLInfo *sslInfo, char* buf, int size)
{
    if(sslInfo == NULL)
	{
        return 0;
	}

    int n = SSL_read(sslInfo->ssl, buf, size);

    bool r = SSLGetError(n, sslInfo);

    if (r == false)
    {
        return -1;
    }

    return n;
}

bool SSLGateway::IsActivated(SSLInfo *sslInfo)
{
    if(sslInfo == NULL)
	{
        return false;
	}

    return sslInfo->activated;
}

void SSLGateway::SSLLock(int n)
{
	if((pMutexes)&&(pMutexes[n]))
		WaitForSingleObject(pMutexes[n],INFINITE);
}

void SSLGateway::SSLUnlock(int n)
{
	if((pMutexes)&&(pMutexes[n]))
		ReleaseMutex(pMutexes[n]);
}

void SSLGateway::SSLCleanErrorState()
{
	ERR_remove_state(0);
}
