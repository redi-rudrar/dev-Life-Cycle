
#ifndef _WSPORTIMPL_H
#define _WSPORTIMPL_H

#pragma pack(push,1)

struct CONIMPL_2_0
{
	#ifdef WS_DECLINING_RECONNECT
	WORD ReconnectDelay;
	DWORD ReconnectTime;
	DWORD ConnectTime;
	#endif
	int PortWasActiveBeforeSuspend;
	WSRECORDING Recording;
	HWND setupDlg;
	char MonitorGroups[8][16];
	HANDLE tmutex;
	int activeThread;
	#ifdef WS_OIO
	WSOVERLAPPED rovls[WS_OVERLAP_MAX];
	#endif
	char S5Status;	// 0	= offline
					// 10	= Login into Proxy
					// 20	= Connecting to Remote
					// 30	= Binding to Remote
					// 100+	= Online ready for data
	char EncryptionOn;
	unsigned char pw[24];
	DES_KS IK1;
	DES_KS IK2;
	DES_KS IK3;
	unsigned char Iiv[8];
	DES_KS OK1;
	DES_KS OK2;
	DES_KS OK3;
	unsigned char Oiv[8];
	long ReconCount;
	char Status[80];
	int SendTimeOut;
	int SinceLastBeatCount;
	SOCKET_T Sock;
	int MonPortDataSize;
	struct tdMonPortData *MonPortData;
	BUFFER InCompBuffer;
	BUFFER OutCompBuffer;

	// Metrics
	unsigned long Seconds;
	unsigned long BytesIn;
	unsigned long BytesLastIn;
	unsigned long BytesPerSecIn;
	unsigned long BytesPerSecMaxIn;
	unsigned long BytesPerSecAvgIn;
	unsigned long BytesOut;
	unsigned long BytesLastOut;
	unsigned long BytesPerSecOut;
	unsigned long BytesPerSecMaxOut;
	unsigned long BytesPerSecAvgOut;
	unsigned long PacketsIn;
	unsigned long PacketsOut;
	unsigned long BlocksIn;
	unsigned long BlocksOut;
	unsigned long BeatsIn;
	unsigned long BeatsOut;
	unsigned long LastChokeSize;
	#ifdef WS_OIO
	unsigned long IOCPSendBytes; //Send I/O completion-pending bytes
	#endif
	DWORD LastDataTime;		//Last GetTickCount data arrived
	DWORD lastReadyStart;	//Begin GetTickCount this port serviced
};

#pragma pack(pop)

#endif//_WSPORTIMPL_H

