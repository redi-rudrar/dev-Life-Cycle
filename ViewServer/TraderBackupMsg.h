//  FILE
//    TraderBackupMsg.h
//
//  WARNING:  Shared file
//
//  DESCRIPTION
//    This file contains the constants and structures associated with message communication among applications (TBACKUP, TCENTRAL, etc.)
//    This is a shared file.  Verify that all applications that depend on this file build.
//  
//  Copyright 2006 Banc of America Securities, LLC


#ifndef _TRADERBACKUPMSG_H
#define _TRADERBACKUPMSG_H

// Constants
#define TCENTRAL_SENDER_LEN     20


// Version response
typedef struct tdMsg473Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
} MSG473REC;

// File transfer Request
typedef struct tdMsg474Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
	char FileName[MAX_PATH];		
} MSG474REC;

// New executable message 
typedef struct tdMsg475Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
	char FileName[MAX_PATH];		
	unsigned char digest[16];		// MD5 hash value.
} MSG475REC;


// New executable ACK message
typedef struct tdMsg476Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
	bool Ready;
	char FileName[MAX_PATH];
} MSG476REC;


// File transfer done message
typedef struct tdMsg477Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
	char FileName[MAX_PATH];		
	unsigned char digest[16];		// MD5 hash value.
} MSG477REC;


typedef enum _FILE_XFER_RESULT
{
	COMPLETE=0,
	ERROR_STARTING_EXE=1,
	FAILED_HASH_CALC=2,
	UNKNOWN=99
} FILE_XFER_RESULT;


// File transfer done ACK message 
typedef struct tdMsg478Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
	char FileName[MAX_PATH];
	int Result;
} MSG478REC;


// Trigger retrieval of Error and Event logs message 
typedef struct tdMsg479Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
} MSG479REC;


// Retrieve a file from TBackup
typedef struct tdMsg480Rec
{
	short MsgVersionMajor;
	short MsgVersionMinor;
	char Sender[TCENTRAL_SENDER_LEN];
	char FileName[MAX_PATH];		
} MSG480REC;

typedef struct tdMsg495Rec
{
	char baseDirectory[MAX_PATH+1]; // Base directory for backup application
} MSG495REC;


typedef enum _MSG500ACTION
{
	MSG500_ACTION_TR_INFORM_HASH=1,
	MSG500_ACTION_TR_INFORM_NOT_FOUND=2,
	MSG500_ACTION_TB_REQUEST_FILE=3
} MSG500ACTION;

typedef enum _MSG500TYPE
{
	MSG500_TYPE_WHOLE=101,	        // Set the end of file after receiving
	MSG500_TYPE_PARTIAL=102         // Do not set the end of file after receiving
} MSG500TYPE;


typedef struct tdMsg500Rec
{
	char relativeFilePath[256];		// Ex: FullLog\20060419.txt
	unsigned char digest[16];		// MD5 hash value.
	MSG500ACTION action;
	int type;						// int to match the 822 message.
	long size;
	bool Executable;                // Tells the backup app that this file is the executable
	FILETIME lastWriteDate;         // Last modified date for this file
} MSG500REC;

typedef struct tdMsg822Block
{
	MSG822REC Msg822Rec;
	tdMsg822Block *NextMsg822Block;
} MSG822BLOCK;

typedef struct tdMsg1822Block
{
	MSG1822REC Msg1822Rec;
	tdMsg1822Block *NextMsg1822Block;
} MSG1822BLOCK;						//DT5122

typedef enum _MSG822TYPE
{
	MSG822TYPE_UNKNOWN = 0,
	MSG822TYPE_EXECUTABLE = 1,
	MSG822TYPE_SETUP_FILE = 2,
	MSG822TYPE_BACKUP = 3
} MSG822TYPE;


typedef struct tdMsg822File
{
	int PortNo;
	int BlockNo;
	MSG822TYPE Type;
	char FileDesc[80];
	MSG822BLOCK *TopMsg822Block;
	tdMsg822File *NextMsg822File;
} MSG822FILE;

typedef struct tdMsg1822File
{
	int PortNo;
	int BlockNo;
	MSG822TYPE Type;
	char FileDesc[MAX_PATH];
	MSG1822BLOCK *TopMsg1822Block;
	tdMsg1822File *NextMsg1822File;
} MSG1822FILE;						//DT5122

#endif//_TRADERBACKUPMSG_H

