
#include "stdafx.h"
#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"

// No DES3 lib for 64-bit yet
#ifdef BIT64
#define NO_DES3
#else
#undef NO_DES3
#endif

void WsocksHostImpl::WSHEncrypt(WsocksApp *pmod, char *EncryptBuff,unsigned int *EncryptSize,
								char *DecryptBuff,unsigned int DecryptSize,
								int PortNo,int PortType)
{
#ifdef NO_DES3
#elif defined(WS_ENCRYPTED)
	__try
	{
		if(PortType==WS_CON)
			des_cfb3EEEencode((unsigned char*)DecryptBuff,(unsigned char*)EncryptBuff,DecryptSize,pmod->ConPort[PortNo].Oiv
				,pmod->ConPort[PortNo].OK1,pmod->ConPort[PortNo].OK2,pmod->ConPort[PortNo].OK3);
		else if(PortType==WS_USR)
			des_cfb3EEEencode((unsigned char*)DecryptBuff,(unsigned char*)EncryptBuff,DecryptSize,pmod->UsrPort[PortNo].Oiv
				,pmod->UsrPort[PortNo].OK1,pmod->UsrPort[PortNo].OK2,pmod->UsrPort[PortNo].OK3);
		#ifdef WS_MONITOR
		else if(PortType==WS_UMR)
			des_cfb3EEEencode((unsigned char*)DecryptBuff,(unsigned char*)EncryptBuff,DecryptSize,pmod->UmrPort[PortNo].Oiv
				,pmod->UmrPort[PortNo].OK1,pmod->UmrPort[PortNo].OK2,pmod->UmrPort[PortNo].OK3);
		#endif
		else
			_ASSERT(false);
		*EncryptSize=DecryptSize;
	}
	__except(GetExceptionCode()==0xC06D007E,EXCEPTION_EXECUTE_HANDLER)
	{
		if(PortType==WS_USR)
		{
			int UscPortNo=pmod->UsrPort[PortNo].UscPort;
			WSHLogError(pmod,"DES3.DLL not found for USR%d--Holding USC%d!",PortNo,UscPortNo);
			if((UscPortNo>=0)&&(UscPortNo<pmod->NO_OF_USC_PORTS))
				pmod->UscPort[UscPortNo].ConnectHold=true;
		}
		else if(PortType==WS_CON)
		{
			WSHLogError(pmod,"DES3.DLL not found--Holding CON%d!",PortNo);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CON_PORTS))
				pmod->ConPort[PortNo].ConnectHold=true;
		}
	}
#else
	memcpy(EncryptBuff,DecryptBuff,DecryptSize);
	*EncryptSize=DecryptSize;
#endif
}
void WsocksHostImpl::WSHDecrypt(WsocksApp *pmod, char *DecryptBuff,unsigned int *DecryptSize,
								char *EncryptBuff,unsigned int EncryptSize,
								int PortNo,int PortType)
{
#ifdef NO_DES3
#elif defined(WS_ENCRYPTED)
	__try
	{
		if(PortType==WS_CON)
			des_cfb3EEEdecode((unsigned char*)EncryptBuff,(unsigned char*)DecryptBuff,EncryptSize,pmod->ConPort[PortNo].Iiv
				,pmod->ConPort[PortNo].IK1,pmod->ConPort[PortNo].IK2,pmod->ConPort[PortNo].IK3);
		else if(PortType==WS_USR)
			des_cfb3EEEdecode((unsigned char*)EncryptBuff,(unsigned char*)DecryptBuff,EncryptSize,pmod->UsrPort[PortNo].Iiv
				,pmod->UsrPort[PortNo].IK1,pmod->UsrPort[PortNo].IK2,pmod->UsrPort[PortNo].IK3);
		#ifdef WS_MONITOR
		else if(PortType==WS_UMR)
			des_cfb3EEEdecode((unsigned char*)EncryptBuff,(unsigned char*)DecryptBuff,EncryptSize,pmod->UmrPort[PortNo].Iiv
				,pmod->UmrPort[PortNo].IK1,pmod->UmrPort[PortNo].IK2,pmod->UmrPort[PortNo].IK3);
		#endif
		else
			_ASSERT(false);
		*DecryptSize=EncryptSize;
	}
	__except(GetExceptionCode()==0xC06D007E,EXCEPTION_EXECUTE_HANDLER)
	{
		if(PortType==WS_USR)
		{
			int UscPortNo=pmod->UsrPort[PortNo].UscPort;
			WSHLogError(pmod,"DES3.DLL not found for USR%d--Holding USC%d!",PortNo,UscPortNo);
			if((UscPortNo>=0)&&(UscPortNo<pmod->NO_OF_USC_PORTS))
				pmod->UscPort[UscPortNo].ConnectHold=true;
		}
		else if(PortType==WS_CON)
		{
			WSHLogError(pmod,"DES3.DLL not found--Holding CON%d!",PortNo);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CON_PORTS))
				pmod->ConPort[PortNo].ConnectHold=true;
		}
	}
#else
	memcpy(DecryptBuff,EncryptBuff,EncryptSize);
	*DecryptSize=EncryptSize;
#endif
}

int WsocksHostImpl::WSHResetCryptKeys(WsocksApp *pmod, int PortNo,int PortType)
{
#ifdef NO_DES3
	return -1;
#elif defined(WS_ENCRYPTED)
	__try
	{
		if(PortType==WS_CON)
		{
			des_3EEEinit(pmod->ConPort[PortNo].pw,24,pmod->ConPort[PortNo].IK1,pmod->ConPort[PortNo].IK2,pmod->ConPort[PortNo].IK3);
			des_3EEEinit(pmod->ConPort[PortNo].pw,24,pmod->ConPort[PortNo].OK1,pmod->ConPort[PortNo].OK2,pmod->ConPort[PortNo].OK3);
			return 0;
		}
		else if(PortType==WS_USR)
		{
			des_3EEEinit(pmod->UsrPort[PortNo].pw,24,pmod->UsrPort[PortNo].IK1,pmod->UsrPort[PortNo].IK2,pmod->UsrPort[PortNo].IK3);
			des_3EEEinit(pmod->UsrPort[PortNo].pw,24,pmod->UsrPort[PortNo].OK1,pmod->UsrPort[PortNo].OK2,pmod->UsrPort[PortNo].OK3);
			return 0;
		}
		#ifdef WS_MONITOR
		else if(PortType==WS_UMR)
		{
			des_3EEEinit(pmod->UmrPort[PortNo].pw,24,pmod->UmrPort[PortNo].IK1,pmod->UmrPort[PortNo].IK2,pmod->UmrPort[PortNo].IK3);
			des_3EEEinit(pmod->UmrPort[PortNo].pw,24,pmod->UmrPort[PortNo].OK1,pmod->UmrPort[PortNo].OK2,pmod->UmrPort[PortNo].OK3);
			return 0;
		}
		#endif
		else
		{
			_ASSERT(false);
			return -1;
		}
	}
	// Delayed load DLL not found
	__except(GetExceptionCode()==0xC06D007E,EXCEPTION_EXECUTE_HANDLER)
	{
		if(PortType==WS_USR)
		{
			int UscPortNo=pmod->UsrPort[PortNo].UscPort;
			WSHLogError(pmod,"DES3.DLL not found for USR%d--Holding USC%d!",PortNo,UscPortNo);
			if((UscPortNo>=0)&&(UscPortNo<pmod->NO_OF_USC_PORTS))
				pmod->UscPort[UscPortNo].ConnectHold=true;
		}
		else if(PortType==WS_CON)
		{
			WSHLogError(pmod,"DES3.DLL not found--Holding CON%d!",PortNo);
			if((PortNo>=0)&&(PortNo<pmod->NO_OF_CON_PORTS))
				pmod->ConPort[PortNo].ConnectHold=true;
		}
		return -1;
	}
#endif
}
int WsocksHostImpl::WSHDecompressed(WsocksApp *pmod, int PortType,int PortNo,char *DecompBuff,int DecompSize,char PxySrc[PROXYCOMPRESS_MAX])
{
	if(PortType!=WS_CON)
		return 0;
	if(WSWriteBuff(&pmod->ConPort[PortNo].InBuffer,DecompBuff,DecompSize)<0)
		return 0;
	return 1;
}

// Since zlib isn't thread-safe, compress and uncompress must hold same mutex across all threads.
int WsocksHostImpl::mtcompress(char *dest,unsigned int *destLen,const char *source,unsigned int sourceLen)
{
	// Mutex may not be required for zlib2
	WSHWaitMutex(0x01,INFINITE);
	int rc=compress(dest,destLen,source,sourceLen);
	WSHReleaseMutex(0x01);
	return rc;
}
int WsocksHostImpl::mtuncompress(char *dest,unsigned int *destLen,const char *source,unsigned int sourceLen)
{
	// Mutex may not be required for zlib2
	WSHWaitMutex(0x01,INFINITE);
	int rc=uncompress(dest,destLen,source,sourceLen);
	WSHReleaseMutex(0x01);
	return rc;
}
