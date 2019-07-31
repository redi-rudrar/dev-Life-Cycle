
#include "stdafx.h"
#include "setsocks.h"

#include "wsocksapi.h"
#include "wsockshost.h"
#include "wsocksimpl.h"
#include "wsockshostimpl.h"
#include "wstring.h"
#ifdef WIN32
#include <shlwapi.h>
#include <Iphlpapi.h>
#endif

#ifdef WS_GUARANTEED

FILEINDEX *WsocksHostImpl::WSHCreateFileIndex(WsocksApp *pmod, const char *fpath, int initsize , FILE *fGDLog)
{
    if ( initsize < 0 )
        return 0;
    FILEINDEX *findex = (FILEINDEX *)calloc(1,sizeof(FILEINDEX));
    if ( !findex )
        return 0;
	if(fpath)
		strncpy(findex->fpath, fpath, MAX_PATH -1);
    if ( initsize <= 0 )
        initsize = 8;
    findex->Indexes = (FIDX *)calloc(initsize, sizeof(FIDX));
    if( !findex->Indexes )
    {
        WSHLogError(pmod,"!WSOCKS: WSCreateFileIndex Creating Buffer");
        free(findex);
        return 0;
    }
    findex->Size = initsize;
    findex->Count = 1;
    findex->fhnd = INVALID_FILE_VALUE;
	findex->fGDLog = fGDLog;
    return findex;
}
void WsocksHostImpl::WSHCloseFileIndex(WsocksApp *pmod, FILEINDEX *findex)
{
    if ( findex )
    {
        if ( findex->Indexes )
            free(findex->Indexes);
        free(findex);
    }
}

FIDX *WsocksHostImpl::WSHAddToFileIndex(WsocksApp *pmod, FILEINDEX *findex, unsigned index, unsigned long offset, unsigned int len)
{
    if ( !findex )
        return 0;
	if (findex->Count != index)
		return 0;
    if ( findex->Count >= findex->Size )
    {
        unsigned int nsize = findex->Size *2;
        findex->Indexes = (FIDX *)realloc(findex->Indexes, nsize *sizeof(FIDX));
        if ( !findex->Indexes )
        {
            WSHLogError(pmod,"!WSOCKS: WSAddFileIndex Realloc Buffer");
            return 0;
        }
        findex->Size = nsize;
    }

    FIDX *fidx = &findex->Indexes[findex->Count];
    fidx->Offset = offset;
    fidx->Length = len;
/*    if ( !findex->Count )
        findex->MinOffset = findex->MaxOffset = offset;
    else
    {
        if ( offset < findex->MinOffset )
            findex->MinOffset = offset;
        if ( offset > findex->MaxOffset )
            findex->MaxOffset = offset;
        // Enforce add by increasing offset only,
        // but there may be gaps between index blocks
        else
        {
            WSHLogError(pmod,"!WSOCKS: WSHAddToFileIndex offset %d less than max %d!", 
                offset, findex->MaxOffset);
            return  0;
        }
    }
*/    findex->Count ++;
    return fidx;
}
/*
FIDX *WsocksHostImpl::WSHNextFileIndex(WsocksApp *pmod, FILEINDEX *findex, FIDX *fidx)
{
    if ( !findex || findex->Count<=0 )
        return 0;
    if ( !fidx )
        return &findex->Indexes[0];
    unsigned int ioffset = (DWORD)fidx -(DWORD)findex->Indexes;
    if ( ioffset < 0 )
        return 0;
    unsigned int iidx = ioffset /sizeof(FIDX);
    if ( findex->Count <= iidx +1 )
        return 0;
    return &findex->Indexes[iidx +1];
}
static int __cdecl SortFileIndexes(const void *e1, const void *e2)
{
    FIDX *fidx1 = (FIDX *)e1;
    FIDX *fidx2 = (FIDX *)e2;
    return fidx1->Offset -fidx2->Offset;
}
BOOL WSSortFileIndexes(FILEINDEX *findex)
{
    if ( !findex || findex->Count<0 )
        return FALSE;
    qsort(findex->Indexes, findex->Count, sizeof(FIDX), SortFileIndexes);
    return TRUE;
}
*/
// Open an indexed file, and make sure indexes don't exceed file size.
BOOL WsocksHostImpl::WSHOpenIndexFile(WsocksApp *pmod, FILEINDEX *findex)
{
    if ( findex->fhnd == INVALID_FILE_VALUE )
    {
	#ifdef WIN32
        findex->fhnd = CreateFile(findex->fpath, GENERIC_READ, 
            FILE_SHARE_READ |FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
	#else
        findex->fhnd = fopen(findex->fpath, "rb");
	#endif
        if ( findex->fhnd == INVALID_FILE_VALUE )
        {
            int err = GetLastError();
            WSHLogError(pmod,"!WSOCKS: WSReadFileBlock failed to open \"%s\" with Error %d!", 
                findex->fpath, err);
            return FALSE;
        }
        // Dump all indexes past end of the file
	#ifdef WIN32
        DWORD fsize = GetFileSize(findex->fhnd, 0);
	#else
		__int64 fsize = ftello(findex->fhnd);
	#endif
        for ( DWORD i=0; i<findex->Count; i++ )
        {
            FIDX *fidx = &findex->Indexes[i];
            // Drop all from this one
            if ( fidx->Offset > fsize )
            {
                WSHLogError(pmod,"!WSOCKS: WSReadFileBlock dropping all from index %d/%d at %d past end of file \"%s\" (%d bytes)!",
                    i, findex->Count, fidx->Offset, findex->fpath, fsize);
                findex->Count = i -1;
                break;
            }
            // Truncate, then drop the rest
            else if ( fidx->Offset +fidx->Length > fsize )
            {
                WSHLogError(pmod,"!WSOCKS: WSReadFileBlock truncating index %d/%d from [%d, %d) past end of file \"%s\" to [%d, %d)!",
                    i, findex->Count, fidx->Offset, fidx->Offset +fidx->Length, 
                    findex->fpath, fidx->Offset, fsize);
                fidx->Length = (DWORD)(fsize -fidx->Offset);
                findex->Count = i +1;
                break;
            }
        }
        findex->ReadIdx = 0;
    }
    return TRUE;
}

// Read all blocks the caller's buffer can accommodate
BOOL WsocksHostImpl::WSHReadFILEBlocks(WsocksApp *pmod, FILEINDEX *findex, char *pBuffer, unsigned int *pSize, unsigned int FromIndex, unsigned int *pCount)
{
    if ( !findex || !pBuffer || !pSize || !pCount)
        return FALSE;
    DWORD msize = *pSize;
	DWORD mCount = *pCount;
	*pSize = 0;
	*pCount = 0;

    if ( !findex->fGDLog)
        return FALSE;
	if ( FromIndex==0)
		return FALSE;
    // We've read all the blocks, so close
    if ( FromIndex >= findex->Count )
    {
        return FALSE;
    }
	findex->ReadIdx=FromIndex;
    // Read till the caller's buffer is full
    // Allow gaps between indexes
	DWORD i;
    for ( i=findex->ReadIdx; (i<FromIndex+mCount)&&(i<findex->Count); i++ )
    {
        FIDX *fidx = &findex->Indexes[i];
        DWORD rsize = fidx->Length;
        // No partial block reads
        if ( (*pSize) +rsize > msize )
			break;
		fseek(findex->fGDLog,fidx->Offset,SEEK_SET);
        if (!fread(pBuffer +(*pSize), 1, rsize,findex->fGDLog))
            break;
		(*pCount)++;
        (*pSize)+= rsize;
        // Save next block to be read
    }
    findex->ReadIdx = i;
    return TRUE;       
}

BOOL WsocksHostImpl::WSHReadFileBlocks(WsocksApp *pmod, FILEINDEX *findex, char *pBuffer, unsigned int *pSize)
{
    if ( !findex || !pBuffer || !pSize )
        return FALSE;
    DWORD msize = *pSize; *pSize = 0;

	//use other funtionc if fopen was Used
    if ( !WSHOpenIndexFile(pmod,findex) )
        return FALSE;
    // We've read all the blocks, so close
    if ( findex->ReadIdx >= findex->Count )
    {
	#ifdef WIN32
        CloseHandle(findex->fhnd);
	#else
		fclose(findex->fhnd);
	#endif
		findex->fhnd = INVALID_FILE_VALUE;
        return FALSE;
    }
    // Read till the caller's buffer is full
    // Allow gaps between indexes
    DWORD rtot = 0;
	DWORD i;
    for ( i=findex->ReadIdx; i<findex->Count; i++ )
    {
        FIDX *fidx = &findex->Indexes[i];
        DWORD rsize = fidx->Length;
        // No partial block reads
        if ( rtot +rsize > msize )
        {
            // Return number of bytes needed
            if ( i == findex->ReadIdx )
            {
                *pSize = rsize;
                return FALSE;
            }
            break;
        }
	#ifdef WIN32
        SetFilePointer(findex->fhnd, fidx->Offset, 0, FILE_BEGIN);
        DWORD rbytes = 0;
        if ( !ReadFile(findex->fhnd, pBuffer +rtot, rsize, &rbytes, 0) || rbytes < rsize )
        {
		#ifdef WIN32
            CloseHandle(findex->fhnd);
		#else
			fclose(findex->fhnd);
		#endif
			findex->fhnd = INVALID_FILE_VALUE;
            break;
        }
	#else
		fseeko(findex->fhnd, fidx->Offset, SEEK_SET);
		DWORD rbytes = (DWORD)fread(pBuffer +rtot, 1, rsize, findex->fhnd);
        if( rbytes != rsize)
        {
			fclose(findex->fhnd);
			findex->fhnd = INVALID_FILE_VALUE;
            break;
        }
	#endif
        rtot += rbytes;
        // Save next block to be read
    }
    *pSize = rtot;
    findex->ReadIdx = i;
    return TRUE;       
}

// Read just one block
BOOL WsocksHostImpl::WSHReadOneFileBlock(WsocksApp *pmod, FILEINDEX *findex, int bidx, char *pBuffer, unsigned int *pSize)
{
    if ( !findex || bidx<0 || !pBuffer || !pSize )
        return FALSE;
    DWORD msize = *pSize; *pSize = 0;
    if ( !WSHOpenIndexFile(pmod,findex) )
        return FALSE;
    // We've read all the blocks, so close
    if ( (DWORD)bidx >= findex->Count )
    {
	#ifdef WIN32
        CloseHandle(findex->fhnd);
	#else
		fclose(findex->fhnd);
	#endif
		findex->fhnd = INVALID_FILE_VALUE;
        return FALSE;
    }
    FIDX *fidx = &findex->Indexes[bidx];
    // No partial block reads
    // Return number of bytes nedded
    DWORD rsize = fidx->Length;
    if ( rsize > msize )
    {
        *pSize = rsize;
        return FALSE;
    }
#ifdef WIN32
    SetFilePointer(findex->fhnd, fidx->Offset, 0, FILE_BEGIN);
    DWORD rbytes = 0;
    if ( !ReadFile(findex->fhnd, pBuffer, rsize, &rbytes, 0) )
    {
        CloseHandle(findex->fhnd); 
		findex->fhnd = INVALID_FILE_VALUE;
        return FALSE;
    }
#else
    fseeko(findex->fhnd, fidx->Offset, SEEK_SET);
    DWORD rbytes = (DWORD)fread(pBuffer, 1, rsize, findex->fhnd);
    if ( rbytes != rsize )
    {
		fclose(findex->fhnd);
		findex->fhnd = INVALID_FILE_VALUE;
        return FALSE;
    }
#endif
    *pSize = rbytes;
    return TRUE;
}

// Rolls over last day's guaranteed delivery messages that weren't sent.
// In case of HISINTRA and HISDAILY, we need this functionality.
int WsocksHostImpl::WSHRolloverCGD(WsocksApp *pmod, int PortNo,const char *NewSessionId)
{
	if ( pmod->CgdPort[PortNo].InUse && pmod->CgdPort[PortNo].fGDOutLog )
	{
		if(!pmod->CgdPort[PortNo].SockConnected)
		{
			WSHLogError(pmod,"WSRolloverCGD CGD%d not connected to determine rollover.",PortNo);
			return 0;
		}
		char NewFileName[MAX_PATH];
		sprintf(NewFileName,"DGLogs/CgdLogs/OutLog/%d.%s.%s.%s.dat",pmod->CgdPort[PortNo].GDId.LineId,pmod->CgdPort[PortNo].GDId.LineName,pmod->CgdPort[PortNo].GDId.ClientId,NewSessionId);
		if(!_stricmp(NewFileName,pmod->CgdPort[PortNo].fGDOutLogName))
			return 0;
		WSHMakeLocalDirs(pmod,NewFileName);
		FILE *NewfGDOutLog=0;
		if(!(NewfGDOutLog=fopen(NewFileName,"rb+")))
		{
			if(!(NewfGDOutLog=fopen(NewFileName,"wb+")))
			{
				WSHLogError(pmod,"Error Creating %s",NewFileName);
				return FALSE;
			}
		}			
#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupOpen(NewfGDOutLog, NewFileName);
#endif

		char *Block;
		int MaxBlockSize=pmod->CgdPort[PortNo].BlockSize -1;
		if(MaxBlockSize>63)
			MaxBlockSize=63;
		MaxBlockSize*=1024;
		unsigned int BlockSize;
		unsigned int BlockCount;
		unsigned int Count=0;
		unsigned int LastRolloverId=pmod->CgdPort[PortNo].LastGDSendLogId;
		if (pmod->CgdPort[PortNo].NextGDOutLogId-1>LastRolloverId)
			Count=pmod->CgdPort[PortNo].NextGDOutLogId-1-LastRolloverId;
		if(Count>0)
			WSHLogEvent(pmod,"WSRolloverCGD PortNo %d rolling over [%d,%d)...",PortNo,LastRolloverId,LastRolloverId+Count);
		else
			WSHLogEvent(pmod,"WSRolloverCGD PortNo %d not needed.",PortNo);
		Block=(char*)malloc(MaxBlockSize);
		while(Count > 0)
		{
			BlockSize=MaxBlockSize;
			BlockCount=Count;
			if(WSHReadFILEBlocks(pmod,pmod->CgdPort[PortNo].GDOutLogFileIndex, Block, &BlockSize,LastRolloverId+1,&BlockCount))
			{
				if(BlockCount)
				{
					LastRolloverId+=BlockCount;
					Count-=BlockCount;
					if(BlockSize)
					{
						unsigned long Pos=ftell(NewfGDOutLog);
						fwrite(Block,1,BlockSize,NewfGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
					    WSGDBackupWrite(NewfGDOutLog,Pos,Block,BlockSize);
#endif
					}
				}
				else
				{
					WSHLogError(pmod,"WSRolloverCGD PortNo %d failed at %d; %d remaining!",PortNo,LastRolloverId,Count);
					break;
				}
			}
		}
		free(Block);

		fclose(NewfGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(NewfGDOutLog);
#endif
	}
	return 0;
}

int WsocksHostImpl::WSHReadUgcOutLog(WsocksApp *pmod, int PortNo,GDLINE *GDLine)
{
	char Msg[10240];
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;
	MSGHEADER MsgHeader;
	if(GDLine->fGDOutLog)
	{
		fclose(GDLine->fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(GDLine->fGDOutLog);
#endif
		GDLine->fGDOutLog=NULL;
	}
	GDLine->NextGDOutLogId=1;
	GDLine->NextGDOutLogOffset=0;
	sprintf(GDLine->fGDOutLogName,"DGLogs/UgcLogs/OutLog/%d.%s.%s.dat",GDLine->LineId,GDLine->LineName,pmod->UgcPort[PortNo].SessionId);
    char *FileName=GDLine->fGDOutLogName;
	WSHMakeLocalDirs(pmod,FileName);
	if(!(GDLine->fGDOutLog=fopen(FileName,"rb+")))
	{
		if(!(GDLine->fGDOutLog=fopen(FileName,"wb+")))
		{
			WSHLogError(pmod,"Error Creating %s",FileName);
			return FALSE;
		}
	}			
#ifdef WS_GUARANTEED_BACKUP
	WSGDBackupOpen(GDLine->fGDOutLog, FileName);
#endif
	GDLine->GDOutLogFileIndex=WSHCreateFileIndex(pmod,NULL,1000,GDLine->fGDOutLog);

	while(fread(&GDMsgHeader,1,sizeof(MSGHEADER),GDLine->fGDOutLog))
	{
		if(!fread(&GDHeader,1,sizeof(GDHEADER),GDLine->fGDOutLog))
		{
			WSHLogError(pmod,"Error Reading %s",FileName);
			fclose(GDLine->fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
			WSGDBackupClose(GDLine->fGDOutLog);
#endif
			GDLine->fGDOutLog=NULL;
			GDLine->NextGDOutLogId=1;
			GDLine->NextGDOutLogOffset=0;
			return FALSE;
		}
        if(GDHeader.Type!=GD_TYPE_GAP)
        {
		    if(!fread(&MsgHeader,1,sizeof(MSGHEADER),GDLine->fGDOutLog))
		    {
			    WSHLogError(pmod,"Error Reading %s",FileName);
			    fclose(GDLine->fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupClose(GDLine->fGDOutLog);
#endif
			    GDLine->fGDOutLog=NULL;
			    GDLine->NextGDOutLogId=1;
			    GDLine->NextGDOutLogOffset=0;
			    return FALSE;
		    }
		    if(!fread(&Msg,1,MsgHeader.MsgLen,GDLine->fGDOutLog))
		    {
			    WSHLogError(pmod,"Error Reading %s",FileName);
			    fclose(GDLine->fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupClose(GDLine->fGDOutLog);
#endif
			    GDLine->fGDOutLog=NULL;
			    GDLine->NextGDOutLogId=1;
			    GDLine->NextGDOutLogOffset=0;
			    return FALSE;
		    }
        }
		WSHAddToFileIndex(pmod,GDLine->GDOutLogFileIndex, GDLine->NextGDOutLogId, GDLine->NextGDOutLogOffset,sizeof(MSGHEADER)+GDMsgHeader.MsgLen);
		GDLine->NextGDOutLogId++;
		GDLine->NextGDOutLogOffset+=sizeof(MSGHEADER)+GDMsgHeader.MsgLen;
	}
	return TRUE;
}

int WsocksHostImpl::WSHReadCgdInLog(WsocksApp *pmod, int PortNo)
{
	GDHEADER GDHeader;
	if(pmod->CgdPort[PortNo].fGDInLog)
	{
		fclose(pmod->CgdPort[PortNo].fGDInLog);
#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->CgdPort[PortNo].fGDInLog);
#endif
		pmod->CgdPort[PortNo].fGDInLog=NULL;
	}
	pmod->CgdPort[PortNo].NextGDInLogId=1;
	pmod->CgdPort[PortNo].NextGDInLogOffset=0;
	sprintf(pmod->CgdPort[PortNo].fGDInLogName,"DGLogs/CgdLogs/InLog/%d.%s.%s.%s.dat",pmod->CgdPort[PortNo].GDId.LineId,pmod->CgdPort[PortNo].GDId.LineName,pmod->CgdPort[PortNo].GDId.ClientId,pmod->CgdPort[PortNo].GDId.SessionId);
    char *FileName=pmod->CgdPort[PortNo].fGDInLogName;
	WSHMakeLocalDirs(pmod,FileName);
	if(!(pmod->CgdPort[PortNo].fGDInLog=fopen(FileName,"rb+")))
	{
		if(!(pmod->CgdPort[PortNo].fGDInLog=fopen(FileName,"wb+")))
		{
			WSHLogError(pmod,"Error Creating %s",FileName);
			return FALSE;
		}
		else
		{
			#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupOpen(pmod->CgdPort[PortNo].fGDInLog, FileName);
			#endif
			return TRUE;
		}
	}			
#ifdef WS_GUARANTEED_BACKUP
	WSGDBackupOpen(pmod->CgdPort[PortNo].fGDInLog, FileName);
#endif
	while(fread(&GDHeader,1,sizeof(GDHEADER),pmod->CgdPort[PortNo].fGDInLog))
	{
		pmod->CgdPort[PortNo].NextGDInLogId++;
		pmod->CgdPort[PortNo].NextGDInLogOffset+=sizeof(GDHEADER);
	}
	return TRUE;
}

int WsocksHostImpl::WSHWriteUgcOutLog(WsocksApp *pmod, GDLINE *GDLine,char *Block,WORD BlockLen,int PortNo)
{
	if(!GDLine->fGDOutLog)
		return(FALSE);
	fseek(GDLine->fGDOutLog,GDLine->NextGDOutLogOffset,SEEK_SET);
	fwrite(Block,1,BlockLen,GDLine->fGDOutLog);
	WSHAddToFileIndex(pmod,GDLine->GDOutLogFileIndex, GDLine->NextGDOutLogId ,GDLine->NextGDOutLogOffset,BlockLen);
#ifdef WS_GUARANTEED_BACKUP
    WSGDBackupWrite(GDLine->fGDOutLog,GDLine->NextGDOutLogOffset,Block,BlockLen);
#endif
	GDLine->NextGDOutLogId++;
	GDLine->NextGDOutLogOffset+=BlockLen;
	fflush(GDLine->fGDOutLog);
	return TRUE;
}

/*
int WsocksHostImpl::WSHWriteUgcGap(WsocksApp *pmod, GDLINE *GDLine,int NextOutLogId)
{
	if(!GDLine->fGDOutLog)
		return(FALSE);
	fseek(GDLine->fGDOutLog,GDLine->NextGDOutLogOffset,SEEK_SET);
    MSGHEADER bheader;
    memset(&bheader, 0, sizeof(bheader));
    bheader.MsgID=GD_MSG_SEND;
    bheader.MsgLen=sizeof(GDHEADER);
    GDHEADER blank;
    memset(&blank, 0, sizeof(blank));
    blank.Type = GD_TYPE_GAP;
    blank.GDLineId = GDLine->LineId;
    for ( int i=GDLine->NextGDOutLogId; i<NextOutLogId; i++ )
    {
        blank.GDLogId = i;
        fwrite(&bheader,1,sizeof(MSGHEADER),GDLine->fGDOutLog);
	    fwrite(&blank,1,sizeof(blank),GDLine->fGDOutLog);
	    WSHAddToFileIndex(pmod,GDLine->GDOutLogFileIndex, GDLine->NextGDOutLogId ,GDLine->NextGDOutLogOffset,sizeof(GDHEADER));
#ifdef WS_GUARANTEED_BACKUP
        WSGDBackup(GDLine->fGDOutLog,GDLine->fGDOutLogName,GDLine->NextGDOutLogOffset,(char*)&blank,sizeof(GDHEADER));
#endif
	    GDLine->NextGDOutLogId++;
	    GDLine->NextGDOutLogOffset+=sizeof(GDHEADER);
    }
	fflush(GDLine->fGDOutLog);
    return TRUE;
}
*/

int WsocksHostImpl::WSHWriteCgdInLog(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *Msg,int PortNo)
{
	GDHEADER GDHeader;
	GDHeader.Type=GD_TYPE_DATA;
    GDHeader.SourceId=0;
    GDHeader.GDLineId=pmod->CgdPort[PortNo].GDId.LineId;
    GDHeader.Reserved=0;
	GDHeader.GDLogId=pmod->CgdPort[PortNo].NextGDInLogId;
	if(!pmod->CgdPort[PortNo].fGDInLog)
		return(FALSE);
	fseek(pmod->CgdPort[PortNo].fGDInLog,pmod->CgdPort[PortNo].NextGDInLogOffset,SEEK_SET);
	fwrite(&GDHeader,1,sizeof(GDHEADER),pmod->CgdPort[PortNo].fGDInLog);
#ifdef WS_GUARANTEED_BACKUP
    WSGDBackupWrite(pmod->CgdPort[PortNo].fGDInLog,pmod->CgdPort[PortNo].NextGDInLogOffset,(const char *)&GDHeader,sizeof(GDHEADER));
#endif
	pmod->CgdPort[PortNo].NextGDInLogId++;
	pmod->CgdPort[PortNo].NextGDInLogOffset+=sizeof(GDHEADER);
	fflush(pmod->CgdPort[PortNo].fGDInLog);
	return TRUE;
}

int WsocksHostImpl::WSHReadCgdOutLog(WsocksApp *pmod, int PortNo)
{
	char Msg[10240];
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;
	MSGHEADER MsgHeader;
	if(pmod->CgdPort[PortNo].fGDOutLog)
	{
		fclose(pmod->CgdPort[PortNo].fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->CgdPort[PortNo].fGDOutLog);
#endif
		pmod->CgdPort[PortNo].fGDOutLog=NULL;
	}
	pmod->CgdPort[PortNo].NextGDOutLogId=1;
	pmod->CgdPort[PortNo].NextGDOutLogOffset=0;
	sprintf(pmod->CgdPort[PortNo].fGDOutLogName,"DGLogs/CgdLogs/OutLog/%d.%s.%s.%s.dat",pmod->CgdPort[PortNo].GDId.LineId,pmod->CgdPort[PortNo].GDId.LineName,pmod->CgdPort[PortNo].GDId.ClientId,pmod->CgdPort[PortNo].GDId.SessionId);
    char *FileName=pmod->CgdPort[PortNo].fGDOutLogName;
	WSHMakeLocalDirs(pmod,FileName);
	if(!(pmod->CgdPort[PortNo].fGDOutLog=fopen(FileName,"rb+")))
	{
		if(!(pmod->CgdPort[PortNo].fGDOutLog=fopen(FileName,"wb+")))
		{
			WSHLogError(pmod,"Error Creating %s",FileName);
			return FALSE;
		}
	}			
#ifdef WS_GUARANTEED_BACKUP
	WSGDBackupOpen(pmod->CgdPort[PortNo].fGDOutLog, FileName);
#endif
	pmod->CgdPort[PortNo].GDOutLogFileIndex=WSHCreateFileIndex(pmod,NULL,1000,pmod->CgdPort[PortNo].fGDOutLog);

	while(fread(&GDMsgHeader,1,sizeof(MSGHEADER),pmod->CgdPort[PortNo].fGDOutLog))
	{
		if(!fread(&GDHeader,1,sizeof(GDHEADER),pmod->CgdPort[PortNo].fGDOutLog))
		{
			WSHLogError(pmod,"Error Reading %s",FileName);
			fclose(pmod->CgdPort[PortNo].fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
			WSGDBackupClose(pmod->CgdPort[PortNo].fGDOutLog);
#endif
			pmod->CgdPort[PortNo].fGDOutLog=NULL;
			pmod->CgdPort[PortNo].NextGDOutLogId=1;
            pmod->CgdPort[PortNo].LastGDSendLogId=0;
			pmod->CgdPort[PortNo].NextGDOutLogOffset=0;
			return FALSE;
		}
        if(GDHeader.Type!=GD_TYPE_GAP)
        {
		    if(!fread(&MsgHeader,1,sizeof(MSGHEADER),pmod->CgdPort[PortNo].fGDOutLog))
		    {
			    WSHLogError(pmod,"Error Reading %s",FileName);
			    fclose(pmod->CgdPort[PortNo].fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupClose(pmod->CgdPort[PortNo].fGDOutLog);
#endif
			    pmod->CgdPort[PortNo].fGDOutLog=NULL;
			    pmod->CgdPort[PortNo].NextGDOutLogId=1;
                pmod->CgdPort[PortNo].LastGDSendLogId=0;
			    pmod->CgdPort[PortNo].NextGDOutLogOffset=0;
			    return FALSE;
		    }
		    if(!fread(&Msg,1,MsgHeader.MsgLen,pmod->CgdPort[PortNo].fGDOutLog))
		    {
			    WSHLogError(pmod,"Error Reading %s",FileName);
			    fclose(pmod->CgdPort[PortNo].fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupClose(pmod->CgdPort[PortNo].fGDOutLog);
#endif
			    pmod->CgdPort[PortNo].fGDOutLog=NULL;
			    pmod->CgdPort[PortNo].NextGDOutLogId=1;
                pmod->CgdPort[PortNo].LastGDSendLogId=0;
			    pmod->CgdPort[PortNo].NextGDOutLogOffset=0;
			    return FALSE;
		    }
        }
		WSHAddToFileIndex(pmod,pmod->CgdPort[PortNo].GDOutLogFileIndex, pmod->CgdPort[PortNo].NextGDOutLogId, pmod->CgdPort[PortNo].NextGDOutLogOffset,sizeof(MSGHEADER)+GDMsgHeader.MsgLen);
		pmod->CgdPort[PortNo].NextGDOutLogId++;
		pmod->CgdPort[PortNo].NextGDOutLogOffset+=sizeof(MSGHEADER)+GDMsgHeader.MsgLen;
	}
	return TRUE;
}

int WsocksHostImpl::WSHReadUgrInLog(WsocksApp *pmod, int PortNo,const char *ClientId)
{
	GDHEADER GDHeader;
    int UgcPortNo=pmod->UgrPort[PortNo].UgcPort;
	if(pmod->UgrPort[PortNo].fGDInLog)
	{
		fclose(pmod->UgrPort[PortNo].fGDInLog);
#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->UgrPort[PortNo].fGDInLog);
#endif
		pmod->UgrPort[PortNo].fGDInLog=NULL;
	}
	pmod->UgrPort[PortNo].NextGDInLogId=1;
	pmod->UgrPort[PortNo].NextGDInLogOffset=0;
	sprintf(pmod->UgrPort[PortNo].fGDInLogName,"DGLogs/UgcLogs/InLog/%d.%s.%s.%s.dat",pmod->UgrPort[PortNo].GDId.LineId,pmod->UgrPort[PortNo].GDId.LineName,pmod->UgrPort[PortNo].GDId.ClientId,pmod->UgrPort[PortNo].GDId.SessionId);
    char *FileName=pmod->UgrPort[PortNo].fGDInLogName;
	WSHMakeLocalDirs(pmod,FileName);
	if(!(pmod->UgrPort[PortNo].fGDInLog=fopen(FileName,"rb+")))
	{
		if(!(pmod->UgrPort[PortNo].fGDInLog=fopen(FileName,"wb+")))
		{
			WSHLogError(pmod,"Error Creating %s",FileName);
			return FALSE;
		}
		else
		{
			#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupOpen(pmod->UgrPort[PortNo].fGDInLog, FileName);
			#endif
			return TRUE;
		}
	}			
#ifdef WS_GUARANTEED_BACKUP
	WSGDBackupOpen(pmod->UgrPort[PortNo].fGDInLog, FileName);
#endif
	while(fread(&GDHeader,1,sizeof(GDHEADER),pmod->UgrPort[PortNo].fGDInLog))
	{
		pmod->UgrPort[PortNo].NextGDInLogId++;
		pmod->UgrPort[PortNo].NextGDInLogOffset+=sizeof(GDHEADER);
	}
	return TRUE;
}

int WsocksHostImpl::WSHWriteUgrInLog(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *Msg,int PortNo)
{
	GDHEADER GDHeader;
	GDHeader.Type=GD_TYPE_DATA;
    GDHeader.SourceId=0;
	GDHeader.GDLogId=pmod->UgrPort[PortNo].NextGDInLogId;
    GDHeader.Reserved=0;
    GDHeader.GDLineId=pmod->UgrPort[PortNo].GDId.LineId;
	if(!pmod->UgrPort[PortNo].fGDInLog)
		return(FALSE);
	fseek(pmod->UgrPort[PortNo].fGDInLog,pmod->UgrPort[PortNo].NextGDInLogOffset,SEEK_SET);
	fwrite(&GDHeader,1,sizeof(GDHEADER),pmod->UgrPort[PortNo].fGDInLog);
#ifdef WS_GUARANTEED_BACKUP
    WSGDBackupWrite(pmod->UgrPort[PortNo].fGDInLog,pmod->UgrPort[PortNo].NextGDInLogOffset,(const char *)&GDHeader,sizeof(GDHEADER));
#endif
	pmod->UgrPort[PortNo].NextGDInLogId++;
	pmod->UgrPort[PortNo].NextGDInLogOffset+=sizeof(GDHEADER);
	fflush(pmod->UgrPort[PortNo].fGDInLog);
	return TRUE;
}

int WsocksHostImpl::WSHWriteCgdOutLog(WsocksApp *pmod, char *Block,WORD BlockLen,int PortNo)
{
	if(!pmod->CgdPort[PortNo].fGDOutLog)
		return(FALSE);
	fseek(pmod->CgdPort[PortNo].fGDOutLog,pmod->CgdPort[PortNo].NextGDOutLogOffset,SEEK_SET);
	fwrite(Block,1,BlockLen,pmod->CgdPort[PortNo].fGDOutLog);
	WSHAddToFileIndex(pmod,pmod->CgdPort[PortNo].GDOutLogFileIndex, pmod->CgdPort[PortNo].NextGDOutLogId ,pmod->CgdPort[PortNo].NextGDOutLogOffset,BlockLen);
#ifdef WS_GUARANTEED_BACKUP
    WSGDBackupWrite(pmod->CgdPort[PortNo].fGDOutLog,pmod->CgdPort[PortNo].NextGDOutLogOffset,Block,BlockLen);
#endif
	pmod->CgdPort[PortNo].NextGDOutLogId++;
	pmod->CgdPort[PortNo].NextGDOutLogOffset+=BlockLen;
	fflush(pmod->CgdPort[PortNo].fGDOutLog);
	return TRUE;
}

/*
int WsocksHostImpl::WSHWriteCgdGap(WsocksApp *pmod, int PortNo,int NextOutLogId)
{
	if(!pmod->CgdPort[PortNo].fGDOutLog)
		return(FALSE);
	fseek(pmod->CgdPort[PortNo].fGDOutLog,pmod->CgdPort[PortNo].NextGDOutLogOffset,SEEK_SET);
    MSGHEADER bheader;
    memset(&bheader, 0, sizeof(bheader));
    bheader.MsgID=GD_MSG_SEND;
    bheader.MsgLen=sizeof(GDHEADER);
    GDHEADER blank;
    memset(&blank, 0, sizeof(blank));
    blank.Type = GD_TYPE_GAP;
    blank.GDLineId = pmod->CgdPort[PortNo].GDId.LineId;
    for ( int i=pmod->CgdPort[PortNo].NextGDOutLogId; i<NextOutLogId; i++ )
    {
        blank.GDLogId = i;
        fwrite(&bheader,1,sizeof(MSGHEADER),pmod->CgdPort[PortNo].fGDOutLog);
	    fwrite(&blank,1,sizeof(blank),pmod->CgdPort[PortNo].fGDOutLog);
	    WSHAddToFileIndex(pmod,pmod->CgdPort[PortNo].GDOutLogFileIndex, pmod->CgdPort[PortNo].NextGDOutLogId ,pmod->CgdPort[PortNo].NextGDOutLogOffset,sizeof(GDHEADER));
#ifdef WS_GUARANTEED_BACKUP
        WSGDBackup(pmod->CgdPort[PortNo].fGDOutLog,pmod->CgdPort[PortNo].fGDOutLogName,pmod->CgdPort[PortNo].NextGDOutLogOffset,(char*)&blank,sizeof(GDHEADER));
#endif
	    pmod->CgdPort[PortNo].NextGDOutLogId++;
	    pmod->CgdPort[PortNo].NextGDOutLogOffset+=sizeof(GDHEADER);
    }
	fflush(pmod->CgdPort[PortNo].fGDOutLog);
    return TRUE;
}
*/

WORD WsocksHostImpl::WSHGDPack(WsocksApp *pmod, char **NewBlock,int GDType,WORD MsgID,WORD MsgLen,char *MsgOut,unsigned int NextGDOutLogId,WORD LineId)
{
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;
	MSGHEADER MsgHeader;
	WORD BlockLen;
	char *Block;

	GDHeader.Type=GDType;
    GDHeader.SourceId=0;
    GDHeader.GDLineId=LineId;
    GDHeader.Reserved=0;
	GDHeader.GDLogId=NextGDOutLogId;
	MsgHeader.MsgID=MsgID;
	MsgHeader.MsgLen=MsgLen;
	if(MsgLen)
		BlockLen=sizeof(GDHEADER)+sizeof(MSGHEADER)+MsgLen;
	else
		BlockLen=sizeof(GDHEADER);
	GDMsgHeader.MsgID=GD_MSG_SEND;
	GDMsgHeader.MsgLen=BlockLen;
	BlockLen+=sizeof(MSGHEADER);
	Block=(char*)malloc(BlockLen);
	memcpy(Block,&GDMsgHeader,sizeof(MSGHEADER));
	memcpy(&Block[sizeof(MSGHEADER)],&GDHeader,sizeof(GDHEADER));
	if(MsgLen)
	{
		memcpy(&Block[sizeof(MSGHEADER)+sizeof(GDHEADER)],&MsgHeader,sizeof(MSGHEADER));
		memcpy(&Block[sizeof(MSGHEADER)+sizeof(GDHEADER)+sizeof(MSGHEADER)],MsgOut,MsgLen);
	}
	*NewBlock=Block;
	return (BlockLen);
}

int WsocksHostImpl::WSHUgcSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, WORD LineId)
{
	int i;
	char *Block=NULL;
	WORD BlockLen;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UGC_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if(!pmod->UgcPort[PortNo].InUse)
	{
		_ASSERT(false);
		return FALSE;
	}
	GDLINE *GDLine;
    for ( GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
    {
        if (GDLine->LineId==LineId)
            break;
    }
    if ( !GDLine )
        return FALSE;
	if(!(BlockLen=WSHGDPack(pmod,&Block,GD_TYPE_DATA,MsgID,MsgLen,MsgOut,GDLine->NextGDOutLogId,LineId)))
		return FALSE;
	if(!WSHWriteUgcOutLog(pmod,GDLine,Block,BlockLen,PortNo))
		return FALSE;
	int rc=FALSE;
	for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if(pmod->UgrPort[i].SockActive)
		{
			if((pmod->UgrPort[i].UgcPort==PortNo)&&(pmod->UgrPort[i].GDId.LineId==LineId))
			{
				if(pmod->UgrPort[i].NextGDOutLogId==GDLine->NextGDOutLogId-1)
				{
					WSHUgrSendBlock(pmod,Block,BlockLen,i);
					pmod->UgrPort[i].NextGDOutLogId++;
					rc=TRUE;
				}
			}
		}
	}
	free(Block);
	return rc;
}

int WsocksHostImpl::WSHUgcSendGap(WsocksApp *pmod, int PortNo,WORD LineId,int NextOutLogId)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UGC_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if(!pmod->UgcPort[PortNo].InUse)
	{
		_ASSERT(false);
		return FALSE;
	}
	GDLINE *GDLine;
    for ( GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
    {
        if (GDLine->LineId==LineId)
            break;
    }
    if ( !GDLine )
        return FALSE;

    for ( int id=GDLine->NextGDOutLogId; id<NextOutLogId; id++ )
    {
	    int i;
	    char *Block=NULL;
	    WORD BlockLen;

	    if(!(BlockLen=WSHGDPack(pmod,&Block,GD_TYPE_GAP,0,0,0,GDLine->NextGDOutLogId,LineId)))
		    return FALSE;
	    if(!WSHWriteUgcOutLog(pmod,GDLine,Block,BlockLen,PortNo))
		    return FALSE;
	    for(i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	    {
		    if(pmod->UgrPort[i].SockActive)
		    {
			    if((pmod->UgrPort[i].UgcPort==PortNo)&&(pmod->UgrPort[i].GDId.LineId==LineId))
			    {
				    if(pmod->UgrPort[i].NextGDOutLogId==GDLine->NextGDOutLogId-1)
				    {
					    WSHUgrSendBlock(pmod,Block,BlockLen,i);
					    pmod->UgrPort[i].NextGDOutLogId++;
				    }
			    }
		    }
	    }
	    free(Block);
    }
	return TRUE;
}

int WsocksHostImpl::WSHUgrSendNGMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UGR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
    int TotLen=sizeof(MSGHEADER)+sizeof(GDHEADER)+sizeof(MSGHEADER)+MsgLen;
	if((TotLen <= 0)||(TotLen>=pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	if(pmod->UgrPort[PortNo].SockActive)
	{
		if((pmod->UgrPort[PortNo].OutBuffer.Busy)&&(pmod->UgrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: UGR%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->UgrPort[PortNo].OutBuffer.Busy);
			pmod->UgrPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->UgrPort[PortNo].OutBuffer.Busy;
		pmod->UgrPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
        MSGHEADER GDMsgHeader;
        GDMsgHeader.MsgID=GD_MSG_SEND;
        GDMsgHeader.MsgLen=sizeof(GDHEADER)+MsgLen;
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *)&GDMsgHeader,sizeof(MSGHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			return (FALSE);
		}
        GDHEADER GDHeader;
        GDHeader.Type=GD_TYPE_NOGD;
        GDHeader.GDLogId=0;
        GDHeader.GDLineId=pmod->UgrPort[PortNo].GDId.LineId;
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *)&GDHeader,sizeof(GDHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			return (FALSE);
		}
        MSGHEADER MsgHeader;
        MsgHeader.MsgID=MsgID;
        MsgHeader.MsgLen=MsgLen;
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *)&MsgHeader,sizeof(MSGHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			return (FALSE);
		}
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,MsgOut,MsgLen))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			return (FALSE);
		}
		pmod->UgrPort[PortNo].PacketsOut++;
		pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
	#ifdef WS_REALTIMESEND
		WSHUgrSend(pmod,PortNo,false);
	#endif
		return(TRUE);
	}
	else
		return(FALSE);
}

int WsocksHostImpl::WSHUgrSendBlock(WsocksApp *pmod, char *Block,WORD BlockLen,int PortNo)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UGR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((BlockLen <= 0)||(BlockLen>=pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_UGR,PortNo,true);
	if(pmod->UgrPort[PortNo].SockActive)
	{
		if((pmod->UgrPort[PortNo].OutBuffer.Busy)&&(pmod->UgrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: UGR%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->UgrPort[PortNo].OutBuffer.Busy);
			pmod->UgrPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->UgrPort[PortNo].OutBuffer.Busy;
		pmod->UgrPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
		//Send GDMsgHeader
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,Block,BlockLen))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		pmod->UgrPort[PortNo].PacketsOut++;
		pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
	#ifdef WS_REALTIMESEND
		WSHUgrSend(pmod,PortNo,false);
	#endif
		UnlockPort(pmod,WS_UGR,PortNo,true);
		return(TRUE);
	}
	else
	{
		UnlockPort(pmod,WS_UGR,PortNo,true);
		return(FALSE);
	}
}

int WsocksHostImpl::WSHCgdSendLogin(WsocksApp *pmod, int PortNo)
{
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;

	GDMsgHeader.MsgID=GD_MSG_SEND;
	GDMsgHeader.MsgLen=sizeof(GDHEADER)+sizeof(GDID)+sizeof(GDLOGIN);
	GDHeader.Type=GD_TYPE_LOGIN;
    GDHeader.SourceId=0;
    GDHeader.GDLineId=pmod->CgdPort[PortNo].GDId.LineId;
    GDHeader.Reserved=0;
	GDHeader.GDLogId=pmod->CgdPort[PortNo].NextGDInLogId;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CGD_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_CGD,PortNo,true);
	if(pmod->CgdPort[PortNo].SockConnected)
	{
		if((pmod->CgdPort[PortNo].OutBuffer.Busy)&&(pmod->CgdPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: CGD%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->CgdPort[PortNo].OutBuffer.Busy);
			pmod->CgdPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->CgdPort[PortNo].OutBuffer.Busy;
		pmod->CgdPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
		//Send GDMsgHeader
		if(!WSWriteBuff(&pmod->CgdPort[PortNo].OutBuffer,(char *) &GDMsgHeader,sizeof(MSGHEADER)))
		{
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_CGD,PortNo,true);
			return (FALSE);
		}
		//Send GDHeader
		if(!WSWriteBuff(&pmod->CgdPort[PortNo].OutBuffer,(char *) &GDHeader,sizeof(GDHEADER)))
		{
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_CGD,PortNo,true);
			return (FALSE);
		}
		//Send GDID
		if(!WSWriteBuff(&pmod->CgdPort[PortNo].OutBuffer,(char *) &pmod->CgdPort[PortNo].GDId,sizeof(GDID)))
		{
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_CGD,PortNo,true);
			return (FALSE);
		}
        GDLOGIN GDLogin = {pmod->CgdPort[PortNo].NextGDOutLogId, pmod->CgdPort[PortNo].NextGDInLogId -1};
		if(!WSWriteBuff(&pmod->CgdPort[PortNo].OutBuffer,(char *) &GDLogin,sizeof(GDLOGIN)))
		{
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_CGD,PortNo,true);
			return (FALSE);
		}
		pmod->CgdPort[PortNo].PacketsOut++;
		pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
	#ifdef WS_REALTIMESEND
		WSHCgdSend(pmod,PortNo,false);
	#endif
		UnlockPort(pmod,WS_CGD,PortNo,true);
		return(TRUE);
	}
	else
	{
		UnlockPort(pmod,WS_CGD,PortNo,true);
		return(FALSE);
	}
}

int WsocksHostImpl::WSHUgrSendLoginReply(WsocksApp *pmod, int PortNo)
{
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;

	GDMsgHeader.MsgID=GD_MSG_SEND;
	GDMsgHeader.MsgLen=sizeof(GDHEADER)+sizeof(GDID)+sizeof(GDLOGIN);
	GDHeader.Type=GD_TYPE_LOGIN;
    GDHeader.SourceId=0;
    GDHeader.GDLineId=pmod->UgrPort[PortNo].GDId.LineId;
    GDHeader.Reserved=0;
	GDHeader.GDLogId=pmod->UgrPort[PortNo].NextGDInLogId;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UGR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_UGR,PortNo,true);
	if(pmod->UgrPort[PortNo].SockActive)
	{
		if((pmod->UgrPort[PortNo].OutBuffer.Busy)&&(pmod->UgrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: UGR%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->UgrPort[PortNo].OutBuffer.Busy);
			pmod->UgrPort[PortNo].OutBuffer.Busy=0;
			return(FALSE);
		}
		int lastBusy=pmod->UgrPort[PortNo].OutBuffer.Busy;
		pmod->UgrPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
		//Send GDMsgHeader
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &GDMsgHeader,sizeof(MSGHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		//Send GDHeader
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &GDHeader,sizeof(GDHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		//Send GDID
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &pmod->UgrPort[PortNo].GDId,sizeof(GDID)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
        GDLOGIN GDLogin = {pmod->UgrPort[PortNo].NextGDOutLogId, pmod->UgrPort[PortNo].NextGDInLogId -1};
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &GDLogin,sizeof(GDLOGIN)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		pmod->UgrPort[PortNo].PacketsOut++;
		pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
	#ifdef WS_REALTIMESEND
		WSHUgrSend(pmod,PortNo,false);
	#endif
		UnlockPort(pmod,WS_UGR,PortNo,true);
		return(TRUE);
	}
	else
	{
		UnlockPort(pmod,WS_UGR,PortNo,true);
		return(FALSE);
	}
}

int WsocksHostImpl::WSHUgrSendMsg(WsocksApp *pmod, WORD MsgID, WORD MsgLen, char *MsgOut, int PortNo, unsigned int GDLogId)
{
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;
	MSGHEADER MsgOutHeader;

	GDMsgHeader.MsgID=GD_MSG_SEND;
	GDMsgHeader.MsgLen=sizeof(GDHEADER)+sizeof(MSGHEADER)+MsgLen;
	GDHeader.Type=GD_TYPE_DATA;
    GDHeader.SourceId=0;
    GDHeader.GDLineId=pmod->UgrPort[PortNo].GDId.LineId;
    GDHeader.Reserved=0;
	GDHeader.GDLogId=GDLogId;

	if((PortNo < 0)||(PortNo >=pmod->NO_OF_UGR_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	MsgOutHeader.MsgID=MsgID;
	MsgOutHeader.MsgLen=MsgLen;
	LockPort(pmod,WS_UGR,PortNo,true);
	int lastSend=pmod->UgrPort[PortNo].sendThread;
	pmod->UgrPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->UgrPort[PortNo].SockActive)&&(!pmod->UgrPort[PortNo].peerClosed))
	{
		if((pmod->UgrPort[PortNo].OutBuffer.Busy)&&(pmod->UgrPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: UGR%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->UgrPort[PortNo].OutBuffer.Busy);
			pmod->UgrPort[PortNo].OutBuffer.Busy=0;
			return(FALSE);
		}
		int lastBusy=pmod->UgrPort[PortNo].OutBuffer.Busy;
		pmod->UgrPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
		//Send GDMsgHeader
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &GDMsgHeader,sizeof(MSGHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UgrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		//Send GDHeader
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &GDHeader,sizeof(GDHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UgrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		//Send MsgHeader
		if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,(char *) &MsgOutHeader,sizeof(MSGHEADER)))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			pmod->UgrPort[PortNo].sendThread=lastSend;
			UnlockPort(pmod,WS_UGR,PortNo,true);
			return (FALSE);
		}
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if(!WSWriteBuff(&pmod->UgrPort[PortNo].OutBuffer,MsgOut,MsgOutHeader.MsgLen))
			{
				pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->UgrPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_UGR,PortNo,true);
				return(FALSE);
			}
		}
		pmod->UgrPort[PortNo].PacketsOut++;
		pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
		pmod->UgrPort[PortNo].NextGDOutLogId++;
	#ifdef WS_REALTIMESEND
		WSHUgrSend(pmod,PortNo,false);
	#endif
		pmod->UgrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_UGR,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->UgrPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_UGR,PortNo,true);
		return(FALSE);
	}
}

BOOL WsocksHostImpl::WSHUgcPortHasLine(WsocksApp *pmod, int PortNo,WORD LineId)
{
    for ( GDLINE *GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
    {
        if (GDLine->LineId==LineId)
            return TRUE;
    }
    return FALSE;
}

int WsocksHostImpl::WSHUgcGenPortLine(WsocksApp *pmod, UGCPORT *UgcPort,int PortNo,char *pline,int len)
{
	// Build the config string
	char *tptr=pline;
	sprintf(tptr,"G%d:%s:%d",PortNo,UgcPort->LocalIP,UgcPort->Port); tptr+=strlen(tptr);
#ifdef WS_COMPRESS
	if((UgcPort->Compressed)&&(!UgcPort->Encrypted))
	{
		sprintf(tptr,":C"); tptr+=strlen(tptr);
	}
#ifdef WS_ENCRYPTED
	if(UgcPort->Encrypted)
	{
		sprintf(tptr,":E"); tptr+=strlen(tptr);
	}
#endif
#endif
	if(UgcPort->ConnectHold)
	{
		sprintf(tptr,":H"); tptr+=strlen(tptr);
	}
	if(UgcPort->BlockSize!=WS_DEF_BLOCK_SIZE)
	{
		sprintf(tptr,":K%d",UgcPort->BlockSize); tptr+=strlen(tptr);
	}
	if((UgcPort->TimeOut)&&(UgcPort->TimeOut!=1000000))
	{
		sprintf(tptr,":T%d",UgcPort->TimeOut); tptr+=strlen(tptr);
	}
	if((UgcPort->TimeOutSize)&&(UgcPort->TimeOutSize!=60))
	{
		sprintf(tptr,":S%d",UgcPort->TimeOutSize); tptr+=strlen(tptr);
	}
	if(UgcPort->GDCfg[0])
	{
		sprintf(tptr,":G%s",UgcPort->GDCfg); tptr+=strlen(tptr);
	}
	if(UgcPort->GDGap!=100)
	{
		sprintf(tptr,":J%d",UgcPort->GDGap); tptr+=strlen(tptr);
	}
	int m=0;
	for(int i=0;i<8;i++)
	{
		if(UgcPort->MonitorGroups[i][0])
		{
			if(m==0)
				sprintf(tptr,":M%s",UgcPort->MonitorGroups[i]);
			else
				sprintf(tptr,",%s",UgcPort->MonitorGroups[i]);
			tptr+=strlen(tptr); m++;
		}
	}
	if(UgcPort->CfgNote[0])
	{
		sprintf(tptr,":N%s",UgcPort->CfgNote); tptr+=strlen(tptr);
	}
	return 0;
}

int WsocksHostImpl::WSHCfgCgdPort(WsocksApp *pmod, int PortNo)
{
    char cfgcpy[80];
    strcpy(cfgcpy,pmod->CgdPort[PortNo].GDCfg);
    char *inptr=cfgcpy;
    for( int i=0; i<3 && (inptr=strtok(i?0:inptr,","))!=NULL; i++ )
    {
        switch ( i )
        {
        case 0:
            pmod->CgdPort[PortNo].GDId.LineId=atoi(inptr);
            break;
        case 1:
            strcpy(pmod->CgdPort[PortNo].GDId.LineName,inptr);
            break;
        case 2:
            strcpy(pmod->CgdPort[PortNo].GDId.ClientId,inptr);
            break;
        }
    }
    if ((!pmod->CgdPort[PortNo].GDId.LineId)||
        (!pmod->CgdPort[PortNo].GDId.LineName[0])||
        (!pmod->CgdPort[PortNo].GDId.ClientId[0]))
    {
        WSHLogError(pmod,"Incomplete config on CGD %d",PortNo);
        exit(0);
    }
    return(TRUE);
}
void WsocksHostImpl::WSHResetCgdId(WsocksApp *pmod, int PortNo)
{
    WSHCloseCgdPort(pmod,PortNo);
    pmod->WSGetCgdId(PortNo,&pmod->CgdPort[PortNo].GDId);
    strcpy(pmod->CgdPort[PortNo].OnLineStatusText,pmod->CgdPort[PortNo].GDId.SessionId);
	WSHReadCgdOutLog(pmod,PortNo);
    //WSWriteCgdGap(PortNo,pmod->CgdPort[PortNo].NextGDOutLogId+100);
}

void WsocksHostImpl::WSHGetCgdId(WsocksApp *pmod, int PortNo, GDID *GDId)
{
	strcpy(GDId->SessionId,WSHcDate);
}
int WsocksHostImpl::WSHSetupCgdPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;	
	int StartPort=0;
	int EndPort=0;
	int Single=false;

	if(PortNo>=0)
	{
		StartPort=EndPort=PortNo;
		Single=true;
	}
	else
	{
		StartPort=0; EndPort=pmod->NO_OF_CGD_PORTS;
	}
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=StartPort;i<=EndPort;i++)
		{
			memset(&pmod->CgdPort[i],0,sizeof(CGDPORT));
			pmod->CgdPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
		#ifdef WS_DECLINING_RECONNECT
			pmod->CgdPort[i].MinReconnectDelay=MIN_RECONNECT_DELAY;
			pmod->CgdPort[i].MaxReconnectDelay=MAX_RECONNECT_DELAY;
			pmod->CgdPort[i].MinReconnectReset=MIN_RECONNECT_RESET;
			pmod->CgdPort[i].ReconnectDelay=0;
			pmod->CgdPort[i].ReconnectTime=0;
			pmod->CgdPort[i].ConnectTime=0;
		#endif
			pmod->CgdPort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
			pmod->CgdPort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		}
		break;
	case WS_OPEN: // Open
		for (i=StartPort;i<=EndPort;i++)
		{
			if(strlen(pmod->CgdPort[i].LocalIP)>0)
			{
				if(pmod->CgdPort[i].InUse)
					continue;
				pmod->CgdPort[i].rmutex=CreateMutex(0,false,0);
				pmod->CgdPort[i].smutex=CreateMutex(0,false,0);
				pmod->CgdPort[i].recvThread=0;
				pmod->CgdPort[i].sendThread=0;
				WSOpenBuffLimit(&pmod->CgdPort[i].OutBuffer,pmod->CgdPort[i].BlockSize,pmod->CgdPort[i].SendBuffLimit);
				WSOpenBuffLimit(&pmod->CgdPort[i].InBuffer,pmod->CgdPort[i].BlockSize,pmod->CgdPort[i].RecvBuffLimit);
			#ifdef WS_COMPRESS
				WSOpenBuffLimit(&pmod->CgdPort[i].OutCompBuffer,pmod->CgdPort[i].BlockSize,pmod->CgdPort[i].SendBuffLimit);
				WSOpenBuffLimit(&pmod->CgdPort[i].InCompBuffer,pmod->CgdPort[i].BlockSize,pmod->CgdPort[i].RecvBuffLimit);
			#endif
                WSHCfgCgdPort(pmod,i);
                pmod->WSGetCgdId(i,&pmod->CgdPort[i].GDId);
                strcpy(pmod->CgdPort[i].OnLineStatusText,pmod->CgdPort[i].GDId.SessionId);
	            WSHReadCgdOutLog(pmod,i);
				pmod->CgdPort[i].InUse=TRUE;
				//WSHCreatePort(pmod,WS_CGD,i);
				//AddConListItem(GetDispItem(WS_CGD,i));
			}
		}
		//if(!Single)
		//	AddConListItem(GetDispItem(WS_CGD_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=StartPort;i<=EndPort;i++)
		{
			WSHCloseRecording(pmod,&pmod->CgdPort[i].Recording,WS_CGD,i);
			_WSHCloseCgdPort(pmod,i);
			pmod->CgdPort[i].recvThread=0;
			pmod->CgdPort[i].sendThread=0;
			if(pmod->CgdPort[i].smutex)
			{
				//DeleteMutex(pmod->CgdPort[i].smutex); pmod->CgdPort[i].smutex=0;
				DeletePortMutex(pmod,WS_CGD,i,true);
			}
			if(pmod->CgdPort[i].rmutex)
			{
				//DeleteMutex(pmod->CgdPort[i].rmutex); pmod->CgdPort[i].rmutex=0;
				DeletePortMutex(pmod,WS_CGD,i,false);
			}
			if(pmod->CgdPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_CGD,i));
				pmod->CgdPort[i].InUse=FALSE;
			}
            if(pmod->CgdPort[i].fGDOutLog)
            {
                fclose(pmod->CgdPort[i].fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupClose(pmod->CgdPort[i].fGDOutLog);
#endif
                pmod->CgdPort[i].fGDOutLog=0;
            }
			#ifdef WS_OTPP
			_WSHWaitCgdThreadExit(pmod,i);
			#endif
		}
		break;
	}
	return(TRUE);
}
int WsocksHostImpl::WSHOpenCgdPort(WsocksApp *pmod, int PortNo)
{
#ifdef WS_DECLINING_RECONNECT
	if((pmod->CgdPort[PortNo].ReconnectTime)&&(GetTickCount()<pmod->CgdPort[PortNo].ReconnectTime))
		return FALSE;
#endif

	// Don't even create a socket if none of the IPs are active
	bool enabled=false;
	for(int i=0;i<pmod->CgdPort[PortNo].AltIPCount;i++)
	{
		if(pmod->CgdPort[PortNo].AltRemoteIPOn[i])
		{
			enabled=true;
			break;
		}
	}
	if(!enabled)
	{
	#ifdef WS_DECLINING_RECONNECT
		pmod->CgdPort[PortNo].ReconnectTime=0;
	#endif
		return FALSE;
	}

	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->CgdPort[PortNo].BlockSize*8192;

	struct
	{
		int l_onoff;
		int l_linger;
	} linger;
	
	//pmod->CgdPort[PortNo].Sock = socket(AF_INET, SOCK_STREAM, 0);
	pmod->CgdPort[PortNo].Sock = WSHSocket(pmod,WS_CGD,PortNo);
	if (pmod->CgdPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->CgdPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket socket() failed with error=%d",errno);
		perror(NULL);
		return(FALSE);
	}
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->CgdPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->CgdPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket ioctlsocket() failed");
		_WSHCloseCgdPort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->CgdPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->CgdPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket setsockopt() failed");
		_WSHCloseCgdPort(pmod,PortNo);
		return(FALSE);
	}

	linger.l_onoff=1;
	linger.l_linger=0;
	if (WSHSetSockOpt(pmod->CgdPort[PortNo].Sock, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof(linger)) == SOCKET_ERROR) 
	{
		pmod->CgdPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket setsockopt() failed");
		_WSHCloseCgdPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->CgdPort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->CgdPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket setsockopt() failed");
		_WSHCloseCgdPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->CgdPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->CgdPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket setsockopt() failed");
		_WSHCloseCgdPort(pmod,PortNo);
		return(FALSE);
	}

	// There's no evidence this helps
//#ifdef WS_REALTIMESEND
//	// Disable Nagle
//	unsigned long nagleoff=1;
//	if (WSHSetSockOpt(pmod->CgdPort[PortNo].Sock, IPPROTO_TCP, TCP_NODELAY, (char *)(&nagleoff), sizeof(nagleoff)) == SOCKET_ERROR) 
//	{
//		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket setsockopt(TCP_NODELAY) failed");
//		_WSHCloseCgdPort(pmod,PortNo);
//		return(FALSE);
//	}
//#endif

	// Connect sockets will automatically bind an unused port. Otherwise, when the dest is
	// loopback or a local interface, it may accidentally bind the dest port.
	if (strcmp(pmod->CgdPort[PortNo].LocalIP,"AUTO")!=0)
	{
		//   Retrieve the local IP address and TCP Port number
		local_sin.sin_family=AF_INET;
		local_sin.sin_port=INADDR_ANY;
		local_sin.sin_addr.s_addr=inet_addr(pmod->CgdPort[PortNo].LocalIP);

		//  Associate an address with a socket. (bind)
		//if (bind( pmod->CgdPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
		if (WSHBindPort( pmod->CgdPort[PortNo].Sock, &local_sin.sin_addr, INADDR_ANY) == SOCKET_ERROR) 
		{
			pmod->CgdPort[PortNo].ConnectHold=true;
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : CgdSocket bind() failed");
			_WSHCloseCgdPort(pmod,PortNo);
			return(FALSE);
		}
	}
	WSHCgdConnect(pmod,PortNo);
	pmod->CgdPort[PortNo].SockOpen=TRUE;
	return(TRUE);
}
int WsocksHostImpl::WSHCgdConnect(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN remote_sin; // Remote socket addr - internet style 
	int cnt;

	remote_sin.sin_family=AF_INET;
	remote_sin.sin_port=htons(pmod->CgdPort[PortNo].Port);
	strcpy (pmod->CgdPort[PortNo].Note, pmod->CgdPort[PortNo].GDCfg);
	cnt = 0;

	do
	{
		pmod->CgdPort[PortNo].CurrentAltIP++;
		if(pmod->CgdPort[PortNo].CurrentAltIP>=pmod->CgdPort[PortNo].AltIPCount)
			pmod->CgdPort[PortNo].CurrentAltIP=0;
		strcpy(pmod->CgdPort[PortNo].RemoteIP,pmod->CgdPort[PortNo].AltRemoteIP[pmod->CgdPort[PortNo].CurrentAltIP]);
		if (cnt > pmod->CgdPort[PortNo].AltIPCount+1) {
			strcpy(pmod->CgdPort[PortNo].RemoteIP, "");
			break;// none !
		}
		cnt++;

	}while(!pmod->CgdPort[PortNo].AltRemoteIPOn[pmod->CgdPort[PortNo].CurrentAltIP]);

	if(pmod->CgdPort[PortNo].S5Connect)
	{
		remote_sin.sin_addr.s_addr=inet_addr(pmod->CgdPort[PortNo].S5RemoteIP);
		remote_sin.sin_port=htons(pmod->CgdPort[PortNo].S5Port);
	}
	else
	{
		remote_sin.sin_addr.s_addr=inet_addr(pmod->CgdPort[PortNo].RemoteIP);
		remote_sin.sin_port=htons(pmod->CgdPort[PortNo].Port);
	}
	
	pmod->WSCgdConnecting(PortNo);
	//if (WSHConnectPort( pmod->CgdPort[PortNo].Sock, (struct sockaddr *) &remote_sin, sizeof(sockaddr)) == SOCKET_ERROR)  
	if (WSHConnectPort( pmod->CgdPort[PortNo].Sock, &remote_sin.sin_addr, pmod->CgdPort[PortNo].Port) == SOCKET_ERROR)  
	{
		//pmod->CgdPort[PortNo].ReconCount=WS_CONOUT_RECON/(WS_TIMER_INTERVAL?WS_TIMER_INTERVAL:1)+1;
		pmod->CgdPort[PortNo].ReconCount=(GetTickCount() +WS_CONOUT_RECON);
		WSHUpdatePort(pmod,WS_CGD,PortNo);
		return(FALSE);
	}
	WSHCgdConnected(pmod,PortNo);
	return(TRUE);
}
int WsocksHostImpl::WSHCgdSendMsg(WsocksApp *pmod, WORD MsgID,WORD MsgLen,char *MsgOut,int PortNo)
{
	char *Block=NULL;
	WORD BlockLen;
	
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CGD_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if(!pmod->CgdPort[PortNo].InUse)
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_CGD,PortNo,true);
	int lastSend=pmod->CgdPort[PortNo].sendThread;
	pmod->CgdPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->CgdPort[PortNo].OutBuffer.Busy)&&(pmod->CgdPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: CGD%d OutBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->CgdPort[PortNo].OutBuffer.Busy);
		pmod->CgdPort[PortNo].OutBuffer.Busy=0;
	}
	int lastBusy=pmod->CgdPort[PortNo].OutBuffer.Busy;
	pmod->CgdPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
	if(!(BlockLen=WSHGDPack(pmod,&Block,GD_TYPE_DATA,MsgID,MsgLen,MsgOut,pmod->CgdPort[PortNo].NextGDOutLogId,pmod->CgdPort[PortNo].GDId.LineId)))
	{
		pmod->CgdPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CGD,PortNo,true);
		return FALSE;
	}
	if(!WSHWriteCgdOutLog(pmod,Block,BlockLen,PortNo))
	{
		pmod->CgdPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CGD,PortNo,true);
		return FALSE;
	}
	if((pmod->CgdPort[PortNo].SockConnected)&&(pmod->CgdPort[PortNo].GDLoginAck))
	{
        if ( pmod->CgdPort[PortNo].NextGDOutLogId==pmod->CgdPort[PortNo].LastGDSendLogId+2 )
        {
	        WSHCgdSendBlock(pmod,Block,BlockLen,PortNo);
            pmod->CgdPort[PortNo].LastGDSendLogId=pmod->CgdPort[PortNo].NextGDOutLogId-1;
            // Already done in WSWriteCgdOutLog
		    //pmod->CgdPort[PortNo].NextGDOutLogId++;
        }
	}
	free(Block);
	pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
	pmod->CgdPort[PortNo].sendThread=lastSend;
	UnlockPort(pmod,WS_CGD,PortNo,true);
	return TRUE;
}
int WsocksHostImpl::WSHCgdSendBuff(WsocksApp *pmod, int MsgLen,char *MsgOut,int PortNo, int Packets,int ForceSend)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CGD_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((MsgLen <= 0)||(MsgLen>=pmod->CgdPort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	LockPort(pmod,WS_CGD,PortNo,true);
	int lastSend=pmod->CgdPort[PortNo].sendThread;
	pmod->CgdPort[PortNo].sendThread=GetCurrentThreadId();
	if((pmod->CgdPort[PortNo].SockConnected)||(ForceSend))
	{
		//Send Data
		if((MsgLen!=0)&&(MsgOut!=NULL))
		{
			if((pmod->CgdPort[PortNo].OutBuffer.Busy)&&(pmod->CgdPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
			{
				_ASSERT(false);
				WSHLogError(pmod,"!CRASH: CGD%d OutBuffer.Busy detected a possible thread %d crash.",
					PortNo,pmod->CgdPort[PortNo].OutBuffer.Busy);
				pmod->CgdPort[PortNo].OutBuffer.Busy=0;
			}
			int lastBusy=pmod->CgdPort[PortNo].OutBuffer.Busy;
			pmod->CgdPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
			if(!WSWriteBuff(&pmod->CgdPort[PortNo].OutBuffer,MsgOut,MsgLen))
			{
				pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
				pmod->CgdPort[PortNo].sendThread=lastSend;
				UnlockPort(pmod,WS_CGD,PortNo,true);
				return(FALSE);
			}
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
		}
		pmod->CgdPort[PortNo].PacketsOut+=Packets;
	#ifdef WS_REALTIMESEND
		if((Packets)||(ForceSend))
			WSHCgdSend(pmod,PortNo,false);
	#endif
		pmod->CgdPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CGD,PortNo,true);
		return(TRUE);
	}
	else
	{
		pmod->CgdPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CGD,PortNo,true);
		return(FALSE);
	}
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseCgdPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CGD_PORTS))
		return -1;
	if(!pmod->CgdPort[PortNo].InUse)
		return -1;
	if(pmod->CgdPort[PortNo].SockConnected)
		pmod->CgdPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseCgdPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->CgdPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_CGD,PortNo,false);
		lastRecv=pmod->CgdPort[PortNo].recvThread;
		pmod->CgdPort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->CgdPort[PortNo].pendingClose)
		{
			pmod->CgdPort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_CGD,PortNo,false);
			return 0;
		}
		pmod->CgdPort[PortNo].pendingClose=true;
	}
	if(pmod->CgdPort[PortNo].smutex)
	{
		LockPort(pmod,WS_CGD,PortNo,true);
		lastSend=pmod->CgdPort[PortNo].sendThread;
		pmod->CgdPort[PortNo].sendThread=GetCurrentThreadId();
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->CgdPort[PortNo].pendOvlRecvList; pmod->CgdPort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->CgdPort[PortNo].pendOvlSendList; pmod->CgdPort[PortNo].pendOvlSendList=0;
	if((pendOvlRecvList)||(pendOvlSendList))
	{		
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
		if(pendOvlRecvList)
		{
			WSOVERLAPPED *lastRecv=0;
			for(WSOVERLAPPED *povl=pendOvlRecvList;povl;povl=povl->next)
			{
				povl->Cancelled=true;
				lastRecv=povl;
			}
			lastRecv->next=pmod->cxlOvlList;
			if(pmod->cxlOvlList) pmod->cxlOvlList->prev=lastRecv;
			pmod->cxlOvlList=pendOvlRecvList;
		}
		if(pendOvlSendList)
		{
			WSOVERLAPPED *lastSend=0;
			for(WSOVERLAPPED *povl=pendOvlSendList;povl;povl=povl->next)
			{
				povl->Cancelled=true;
				lastSend=povl;
			}
			lastSend->next=pmod->cxlOvlList;
			if(pmod->cxlOvlList) pmod->cxlOvlList->prev=lastSend;
			pmod->cxlOvlList=pendOvlSendList;
		}
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif
	}

	//ResetSendTimeout(WS_CGD,PortNo);
	if(pmod->CgdPort[PortNo].Sock != 0)
		WSHClosePort(pmod->CgdPort[PortNo].Sock);
	WSCloseBuff(&pmod->CgdPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->CgdPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutCompBuffer);
	#endif
    if(pmod->CgdPort[PortNo].fGDInLog)
    {
		WSHLogEvent(pmod,"Logout on Cgd %d for %d.(%s)(%s)(%s).",PortNo,pmod->CgdPort[PortNo].GDId.LineId,pmod->CgdPort[PortNo].GDId.LineName,pmod->CgdPort[PortNo].GDId.ClientId,pmod->CgdPort[PortNo].GDId.SessionId);
        fclose(pmod->CgdPort[PortNo].fGDInLog);
		#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->CgdPort[PortNo].fGDInLog);
		#endif
        pmod->CgdPort[PortNo].fGDInLog=0;
    }
    if(pmod->CgdPort[PortNo].SockConnected)
	{
		pmod->CgdPort[PortNo].SockConnected=0;
	    pmod->WSCgdClosed(PortNo); 
	}
    memset(&pmod->CgdPort[PortNo].SockOpen,0
		,sizeof(CGDPORT) -((char *)&pmod->CgdPort[PortNo].SockOpen -(char *)&pmod->CgdPort[PortNo]));
	pmod->CgdPort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->CgdPort[PortNo].MinReconnectDelay,pmod->CgdPort[PortNo].MaxReconnectDelay,pmod->CgdPort[PortNo].MinReconnectReset,
		pmod->CgdPort[PortNo].ReconnectDelay,pmod->CgdPort[PortNo].ReconnectTime,pmod->CgdPort[PortNo].ConnectTime);
	#endif
	if(pmod->CgdPort[PortNo].smutex)
	{
		pmod->CgdPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CGD,PortNo,true);
	}
	if(pmod->CgdPort[PortNo].rmutex)
	{
		pmod->CgdPort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_CGD,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_CGD,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseCgdPort(WsocksApp *pmod, int PortNo)
{
	if((PortNo<0)||(PortNo>=pmod->NO_OF_CGD_PORTS))
		return -1;
	if(!pmod->CgdPort[PortNo].InUse)
		return -1;
	if(pmod->CgdPort[PortNo].SockConnected)
		pmod->CgdPort[PortNo].appClosed=true;
	return 0;
}
int WsocksHostImpl::_WSHCloseCgdPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->CgdPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_CGD,PortNo,false);
		lastRecv=pmod->CgdPort[PortNo].recvThread;
		pmod->CgdPort[PortNo].recvThread=GetCurrentThreadId();
	}
	if(pmod->CgdPort[PortNo].smutex)
	{
		LockPort(pmod,WS_CGD,PortNo,true);
		lastSend=pmod->CgdPort[PortNo].sendThread;
		pmod->CgdPort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_CGD,PortNo);
	if(pmod->CgdPort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->CgdPort[PortNo].Sock)->sd;
		WSHClosePort(pmod->CgdPort[PortNo].Sock);
		WSHFinishOverlapSend(pmod,sd,&pmod->CgdPort[PortNo].pendOvlSendBeg,&pmod->CgdPort[PortNo].pendOvlSendEnd);
	}

	WSCloseBuff(&pmod->CgdPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->CgdPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutCompBuffer);
	#endif
    if(pmod->CgdPort[PortNo].fGDInLog)
    {
		WSHLogEvent(pmod,"Logout on Cgd %d for %d.(%s)(%s)(%s).",PortNo,pmod->CgdPort[PortNo].GDId.LineId,pmod->CgdPort[PortNo].GDId.LineName,pmod->CgdPort[PortNo].GDId.ClientId,pmod->CgdPort[PortNo].GDId.SessionId);
        fclose(pmod->CgdPort[PortNo].fGDInLog);
		#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->CgdPort[PortNo].fGDInLog);
		#endif
        pmod->CgdPort[PortNo].fGDInLog=0;
    }
    if(pmod->CgdPort[PortNo].SockConnected)
	{
		pmod->CgdPort[PortNo].SockConnected=0;
	    pmod->WSCgdClosed(PortNo); 
	}

	#ifdef WIN32
	HANDLE pthread=pmod->CgdPort[PortNo].pthread;
	#else
	pthread_t pthread=pmod->CgdPort[PortNo].pthread;
	#endif
	memset(&pmod->CgdPort[PortNo].SockOpen,0
		,sizeof(CGDPORT) -((char *)&pmod->CgdPort[PortNo].SockOpen -(char *)&pmod->CgdPort[PortNo]));
	pmod->CgdPort[PortNo].pthread=pthread;
	pmod->CgdPort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->CgdPort[PortNo].MinReconnectDelay,pmod->CgdPort[PortNo].MaxReconnectDelay,pmod->CgdPort[PortNo].MinReconnectReset,
		pmod->CgdPort[PortNo].ReconnectDelay,pmod->CgdPort[PortNo].ReconnectTime,pmod->CgdPort[PortNo].ConnectTime);
	#endif
	if(pmod->CgdPort[PortNo].smutex)
	{
		pmod->CgdPort[PortNo].sendThread=lastSend;
		UnlockPort(pmod,WS_CGD,PortNo,true);
	}
	if(pmod->CgdPort[PortNo].rmutex)
	{
		pmod->CgdPort[PortNo].recvThread=lastRecv;
		UnlockPort(pmod,WS_CGD,PortNo,false);
	}
	WSHUpdatePort(pmod,WS_CGD,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitCgdThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->CgdPort[PortNo].pthread)
	{
		#ifdef WIN32
		#ifdef _DEBUG
		WaitForSingleObject(pmod->CgdPort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->CgdPort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->CgdPort[PortNo].pthread); pmod->CgdPort[PortNo].pthread=0;
		#else
		void *rc=0;
		pthread_join(pmod->CgdPort[PortNo].pthread,&rc);
		#endif
	}
}
struct CgdReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
#ifdef WIN32
DWORD WINAPI _BootCgdReadThread(LPVOID arg)
{
	CgdReadThreadData *ptd=(CgdReadThreadData*)arg;
	int rc=ptd->aimpl->WSHCgdReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	return rc;
}
#else
void *_BootCgdReadThread(void *arg)
{
	CgdReadThreadData *ptd=(CgdReadThreadData*)arg;
	int rc=ptd->aimpl->WSHCgdReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
}
#endif
#else//!WS_OIO
int WsocksHostImpl::WSHCloseCgdPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->CgdPort[PortNo].rmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_CGD,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->CgdPort[PortNo].recvThread)&&(pmod->CgdPort[PortNo].recvThread!=tid))
		{
			UnlockPort(pmod,WS_CGD,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_CGD,PortNo,false);
		}
		pmod->CgdPort[PortNo].recvThread=tid;
	}

	//ResetSendTimeout(WS_CGD,PortNo);
	if(pmod->CgdPort[PortNo].Sock != 0)
		WSHClosePort(pmod->CgdPort[PortNo].Sock);
	WSCloseBuff(&pmod->CgdPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->CgdPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutCompBuffer);
	#endif
    if(pmod->CgdPort[PortNo].fGDInLog)
    {
		WSHLogEvent(pmod,"Logout on Cgd %d for %d.(%s)(%s)(%s).",PortNo,pmod->CgdPort[PortNo].GDId.LineId,pmod->CgdPort[PortNo].GDId.LineName,pmod->CgdPort[PortNo].GDId.ClientId,pmod->CgdPort[PortNo].GDId.SessionId);
        fclose(pmod->CgdPort[PortNo].fGDInLog);
		#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->CgdPort[PortNo].fGDInLog);
		#endif
        pmod->CgdPort[PortNo].fGDInLog=0;
    }
    if(pmod->CgdPort[PortNo].SockConnected)
	    pmod->WSCgdClosed(PortNo); 
    memset(&pmod->CgdPort[PortNo].SockOpen,0
		,sizeof(CGDPORT) -((char *)&pmod->CgdPort[PortNo].SockOpen -(char *)&pmod->CgdPort[PortNo]));
	pmod->CgdPort[PortNo].S5Status=0;

	#ifdef WS_DECLINING_RECONNECT
	SetReconnectTime(pmod->CgdPort[PortNo].MinReconnectDelay,pmod->CgdPort[PortNo].MaxReconnectDelay,pmod->CgdPort[PortNo].MinReconnectReset,
		pmod->CgdPort[PortNo].ReconnectDelay,pmod->CgdPort[PortNo].ReconnectTime,pmod->CgdPort[PortNo].ConnectTime);
	#endif
	if(pmod->CgdPort[PortNo].rmutex)
	{
		pmod->CgdPort[PortNo].recvThread=0;
		UnlockPort(pmod,WS_CGD,PortNo,false);
	}
	//WSHPortChanged(pmod,pmod->CgdPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO

void WsocksHostImpl::WSHGetUgcId(WsocksApp *pmod, int PortNo, char SessionId[20])
{
	if(!SessionId[0])
		strcpy(SessionId,pmod->WScDate);
}
int WsocksHostImpl::WSHSetupUgcPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	return 0;
}
int WsocksHostImpl::WSHOpenUgcPort(WsocksApp *pmod, int PortNo)
{
	SOCKADDR_IN local_sin;  // Local socket - internet style 
	int SndBuf = pmod->UgcPort[PortNo].BlockSize*8192;
	
    // Open the out file even if the socket fails
    if (!WSHCfgUgcPort(pmod,PortNo))
	{
        pmod->UgcPort[PortNo].InUse=FALSE;
		return(FALSE);
	}
    pmod->WSGetUgcId(PortNo,pmod->UgcPort[PortNo].SessionId);
    for ( GDLINE *GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
    {
        WSHReadUgcOutLog(pmod,PortNo,GDLine);
        if(pmod->UgcPort[PortNo].GDGap==0)
            pmod->UgcPort[PortNo].GDGap=100;
		if(!pmod->WSUgcNoGap)
		{
			//WSWriteUgcGap(GDLine,GDLine->NextGDOutLogId+pmod->UgcPort[PortNo].GDGap);
			WSHUgcSendGap(pmod,PortNo,GDLine->LineId,GDLine->NextGDOutLogId+pmod->UgcPort[PortNo].GDGap);
		}
    }

	struct
	{
		int l_onoff;
		int l_linger;
	} linger;
	
	//pmod->UgcPort[PortNo].Sock = socket(AF_INET, SOCK_STREAM, 0);
	pmod->UgcPort[PortNo].Sock = WSHSocket(pmod,WS_UGC,PortNo);
	if (pmod->UgcPort[PortNo].Sock == INVALID_SOCKET_T)
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket socket() failed");
		return(FALSE);
	}
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(pmod->UgcPort[PortNo].Sock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket ioctlsocket() failed");
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
	}
	int wstrue = 1;
	if (WSHSetSockOpt(pmod->UgcPort[PortNo].Sock, SOL_SOCKET, SO_REUSEADDR, (char *)(&wstrue), sizeof(int)) == SOCKET_ERROR) 
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket setsockopt() failed");
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
	}

	linger.l_onoff=1;
	linger.l_linger=0;

	if (WSHSetSockOpt(pmod->UgcPort[PortNo].Sock, SOL_SOCKET, SO_LINGER, (char *)(&linger), sizeof(linger)) == SOCKET_ERROR) 
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket setsockopt() failed");
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->UgcPort[PortNo].Sock, SOL_SOCKET, SO_SNDBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket setsockopt() failed");
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
	}

	if (WSHSetSockOpt(pmod->UgcPort[PortNo].Sock, SOL_SOCKET, SO_RCVBUF, (char *)(&SndBuf), sizeof(SndBuf)) == SOCKET_ERROR) 
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket setsockopt() failed");
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
	}

	local_sin.sin_family=AF_INET;
	local_sin.sin_port=htons(pmod->UgcPort[PortNo].Port);

	if (strcmp(pmod->UgcPort[PortNo].LocalIP,"AUTO")==0)
		local_sin.sin_addr.s_addr=INADDR_ANY;
	else
		local_sin.sin_addr.s_addr=inet_addr(pmod->UgcPort[PortNo].LocalIP);

   //  Associate an address with a socket. (bind)
         
   //if (bind( pmod->UgcPort[PortNo].Sock, (struct sockaddr *) &local_sin, sizeof(sockaddr)) == SOCKET_ERROR) 
   if (WSHBindPort( pmod->UgcPort[PortNo].Sock, &local_sin.sin_addr, pmod->UgcPort[PortNo].Port) == SOCKET_ERROR) 
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgcSocket(%d) bind(%s:%d) failed",PortNo,pmod->UgcPort[PortNo].LocalIP,pmod->UgcPort[PortNo].Port);
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
   }

	// Retrieve bind port
	SOCKADDR_IN laddr;
	int lalen=sizeof(SOCKADDR_IN);
	WSHGetSockName(pmod->UgcPort[PortNo].Sock,(SOCKADDR*)&laddr,&lalen);
	pmod->UgcPort[PortNo].bindPort=ntohs(laddr.sin_port);
	if(local_sin.sin_addr.s_addr==INADDR_ANY)
	{
	#ifdef WIN32
		// Enumerate NICs (Requires Iphlpapi.lib)
		ULONG blen=0;
		GetAdaptersInfo(0,&blen);
		PIP_ADAPTER_INFO pAdapterInfo=(PIP_ADAPTER_INFO)new char[blen];
		memset(pAdapterInfo,0,blen);
		GetAdaptersInfo(pAdapterInfo,&blen);
		for(PIP_ADAPTER_INFO pnic=pAdapterInfo;pnic;pnic=pnic->Next)
		{
			strncpy(pmod->UgcPort[PortNo].bindIP,pnic->IpAddressList.IpAddress.String,20);
			pmod->UgcPort[PortNo].bindIP[19]=0;
			if(pmod->UgcPort[PortNo].bindIP[0])
				break;
		}
		delete pAdapterInfo;
	#else
		strncpy(pmod->UgcPort[PortNo].bindIP,inet_ntoa(local_sin.sin_addr),20);
	#endif
	}
	else
		strncpy(pmod->UgcPort[PortNo].bindIP,inet_ntoa(local_sin.sin_addr),20);

   if (WSHListen( pmod->UgcPort[PortNo].Sock, MAX_PENDING_CONNECTS ) == SOCKET_ERROR) 
	{
		pmod->UgcPort[PortNo].ConnectHold=true;
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UGC%d listen(%d) failed",PortNo,MAX_PENDING_CONNECTS);
		WSHCloseUgcPort(pmod,PortNo);
		return(FALSE);
	}

	pmod->UgcPort[PortNo].SockActive=TRUE;
	WSHUpdatePort(pmod,WS_UGC,PortNo);
	return(TRUE);
}
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseUgcPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UgcPort[PortNo].InUse)
		return -1;
	if(pmod->UgcPort[PortNo].SockActive)
		pmod->UgcPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUgcPort(WsocksApp *pmod, int PortNo)
{
    for ( GDLINE *GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
    {
	    if(GDLine->GDOutLogFileIndex)
		    WSHCloseFileIndex(pmod,GDLine->GDOutLogFileIndex);
	    GDLine->GDOutLogFileIndex=NULL;
	    if(GDLine->fGDOutLog)
        {
		    fclose(GDLine->fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
			WSGDBackupClose(GDLine->fGDOutLog);
#endif
        }
	    GDLine->fGDOutLog=NULL;
    }
    while ( pmod->UgcPort[PortNo].GDAclList )
    {
        GDACL *pacl=pmod->UgcPort[PortNo].GDAclList;
        pmod->UgcPort[PortNo].GDAclList=pmod->UgcPort[PortNo].GDAclList->NextGDAcl;
        free(pacl);
    }
	WSHClosePort(pmod->UgcPort[PortNo].Sock);
	// Don't zero out UGC detail pointer values
	UGCPORT tgport;
	memcpy(&tgport.DetPtr,&pmod->UgcPort[PortNo].DetPtr,11*sizeof(void*));
	memset(&pmod->UgcPort[PortNo].SockActive,0
		,sizeof(UGCPORT)-(int)((char *)&pmod->UgcPort[PortNo].SockActive-(char *)&pmod->UgcPort[PortNo]));
	memcpy(&pmod->UgcPort[PortNo].DetPtr,&tgport.DetPtr,11*sizeof(void*));
	WSHUpdatePort(pmod,WS_UGC,PortNo);
	return (TRUE);
}

GDACL *WsocksHostImpl::WSHParseGDAcl(WsocksApp *pmod, const char *gdcfg)
{
	GDACL *pacl=0;
    char *inptr=0;
	char rbuf[1024]={0};
	strcpy(rbuf,gdcfg);
    for( int i=0; (inptr=strtoke(i?0:rbuf,",\r\n"))!=NULL; i++ )
    {
        switch ( i%5 )
        {
        case 0:
			pacl=(GDACL*)calloc(1,sizeof(GDACL));
			pacl->LineId=atoi(inptr);
            break;
        case 1:
			if(pacl)
				strcpy(pacl->LineName,inptr);
            break;
        case 2:
			if(pacl)
				strcpy(pacl->ClientId,inptr);
            break;
        case 3:
			if(pacl)
			{
				strcpy(pacl->Ip,inptr);
				pacl->lIp=inet_addr(pacl->Ip);
			}
            break;
        case 4:
			if(pacl)
			{
				strcpy(pacl->Mask,inptr);
				pacl->lMask=inet_addr(pacl->Mask);
			}
            break;
        }
    }
	return pacl;
}
int WsocksHostImpl::WSHCfgUgcPort(WsocksApp *pmod, int PortNo)
{
    char cfgcpy[80];
    strcpy(cfgcpy,pmod->UgcPort[PortNo].GDCfg);
    char *inptr=cfgcpy;
	bool badacl=false;
	bool reload=pmod->UgcPort[PortNo].GDLines?true:false;
	GDACL *facl=0,*lacl=0;
	// Load the config from a file
	if(PathFileExists(pmod->UgcPort[PortNo].GDCfg))
	{
		const char *fpath=pmod->UgcPort[PortNo].GDCfg;
		FILE *fp = fopen(fpath, "rt");
		if ( !fp )
		{
			WSHLogError(pmod,"Failed opening GDACL file (%s)",fpath);
			return FALSE;
		}
		char rbuf[1024];
		int lno = 0;
		// Build a new ACL list
		bool badacl=false;
		while ( fgets(rbuf, sizeof(rbuf), fp) )
		{
			lno ++;
			if ( !strncmp(rbuf, "//", 2) )
				continue;
			GDACL *pacl=WSHParseGDAcl(pmod,rbuf);
			if(pacl)
			{
				if ((!pacl->ClientId[0])||(!pacl->lIp)||(!pacl->lMask))
				{
					WSHLogError(pmod,"Incomplete ACL for Ugc Port %d at line %d.",PortNo,lno);
					badacl=true;
				}
				else
				{
					if(lacl)
						lacl->NextGDAcl=pacl;
					else
						facl=pacl;
					lacl=pacl;
				}
			}
		}
		fclose(fp);
	}
	// Directly use the config from the note
	else
	{
		GDACL *pacl=WSHParseGDAcl(pmod,pmod->UgcPort[PortNo].GDCfg);
		if(pacl)
		{
			if ((!pacl->ClientId[0])||(!pacl->lIp)||(!pacl->lMask))
			{
				WSHLogError(pmod,"Incomplete ACL for Ugc Port %d.",PortNo);
				badacl=true;
			}
			else
			{
				if(lacl)
					lacl->NextGDAcl=pacl;
				else
					facl=pacl;
				lacl=pacl;
			}
		}
	}

	if(badacl)
	{
		while(facl)
		{
			GDACL *old=facl;
			facl=facl->NextGDAcl;
			free(old);
		}
		// Keep the old ACL on reload
		if(reload)
			return FALSE;
		// Don't allow to start with no ACL
		else
			exit(0);
	}
	// Destroy the old list
	while ( pmod->UgcPort[PortNo].GDAclList )
	{
		GDACL *old = pmod->UgcPort[PortNo].GDAclList;
		pmod->UgcPort[PortNo].GDAclList = pmod->UgcPort[PortNo].GDAclList->NextGDAcl;
		delete old;
	}
	// Point to the new list
    pmod->UgcPort[PortNo].GDAclList = facl;

	// Close lines that are no longer configured
	GDLINE *lastLine=0;
	for ( GDLINE *GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; )
	{
		GDACL *GDAcl;
		for ( GDAcl=pmod->UgcPort[PortNo].GDAclList; GDAcl; GDAcl=GDAcl->NextGDAcl )
		{
			if((GDAcl->LineId==GDLine->LineId)&&(_stricmp(GDAcl->LineName,GDLine->LineName)==0))
				break;
		}
		if(!GDAcl)
		{
			WSHLogEvent(pmod,"UGC %d: GD Line (%d)(%s) removed.",PortNo,GDLine->LineId,GDLine->LineName);
			if(GDLine->fGDOutLog)
			{
			    fclose(GDLine->fGDOutLog);
#ifdef WS_GUARANTEED_BACKUP
				WSGDBackupClose(GDLine->fGDOutLog);
#endif
			    GDLine->fGDOutLog=NULL;
			    GDLine->NextGDOutLogId=1;
			    GDLine->NextGDOutLogOffset=0;
			}
			if(lastLine)
				lastLine->NextGDLine=GDLine->NextGDLine;
			else
				pmod->UgcPort[PortNo].GDLines=GDLine->NextGDLine;
			GDLINE *old=GDLine;
			GDLine=GDLine->NextGDLine;
			free(old);
		}
		else
		{
			lastLine=GDLine;
			GDLine=GDLine->NextGDLine;
		}
	}
    // Make a GDLINE entry for each line configured on the port
	GDLINE *fline=pmod->UgcPort[PortNo].GDLines;
	GDLINE *lline;
	for(lline=fline;(lline)&&(lline->NextGDLine);lline=lline->NextGDLine)
		;
	for ( GDACL *GDAcl=pmod->UgcPort[PortNo].GDAclList; GDAcl; GDAcl=GDAcl->NextGDAcl )
	{
		// One line may not be configured on more than one G port
		bool found=false;
		for ( int g=0; g<pmod->NO_OF_UGC_PORTS; g++ )
		{
			if(pmod->UgcPort[g].InUse)
			{
				for ( GDLINE *GDLine=pmod->UgcPort[g].GDLines; GDLine; GDLine=GDLine->NextGDLine )
				{
					if (GDLine->LineId==GDAcl->LineId)
					{
						found=true;
						if (g!=PortNo)
						{
							WSHLogError(pmod,"!WSOCKS: FATAL ERROR : LineId %d(%s)(%s) may only be configured for one G port!",GDLine->LineId,GDLine->LineName,GDAcl->LineName);
							return FALSE;
						}
						else if (_stricmp(GDLine->LineName,GDAcl->LineName)!=0)
							WSHLogError(pmod,"Warning: LineId %d with multiple names (%s)(%s)",GDLine->LineId,GDLine->LineName,GDAcl->LineName);
					}
				}
			}
		}
		if(!found)
		{
			GDLINE *GDLine=(GDLINE *)calloc(1,sizeof(GDLINE));
			strcpy(GDLine->LineName,GDAcl->LineName);
			GDLine->LineId=GDAcl->LineId;
			WSHLogEvent(pmod,"UGC %d: GD Line (%d)(%s) configured.",PortNo,GDLine->LineId,GDLine->LineName);

			if(lline)
				lline->NextGDLine=GDLine;
			else
				fline=GDLine;
			lline=GDLine;
		}
	}
	pmod->UgcPort[PortNo].GDLines=fline;

    return(TRUE);
}
int WsocksHostImpl::WSHUgrAuthorize(WsocksApp *pmod, int PortNo,GDID *GDId)
{
    int UgcPortNo=pmod->UgrPort[PortNo].UgcPort;
    DWORD remoteIp=inet_addr(pmod->UgrPort[PortNo].RemoteIP);
    for ( GDACL *GDAcl=pmod->UgcPort[UgcPortNo].GDAclList; GDAcl; GDAcl=GDAcl->NextGDAcl )
    {
        if ( GDAcl->LineId!=GDId->LineId )
            continue;
        if ( _stricmp(GDAcl->LineName,GDId->LineName)!=0 )
            continue;
        if ( _stricmp(GDAcl->ClientId,GDId->ClientId)!=0)
            continue;
        if ( _stricmp(pmod->UgcPort[UgcPortNo].SessionId,GDId->SessionId)!=0)
            continue;
        if ( GDAcl->lIp && (remoteIp &GDAcl->lMask)!=(GDAcl->lIp &GDAcl->lMask) )
            continue;
        // Kick off any other client with same credentials
        for ( int i=0; i<pmod->NO_OF_UGR_PORTS; i++ )
        {
            if ((pmod->UgrPort[i].SockActive)&&
                (pmod->UgrPort[i].GDId.LineId==GDId->LineId)&&
                (strcmp(pmod->UgrPort[i].GDId.ClientId,GDId->ClientId)==0))
            {
				WSHLogError(pmod,"(%s)(%s)(%s) moved from Port %d to Port %d",pmod->UgrPort[i].GDId.LineName,pmod->UgrPort[i].GDId.ClientId,pmod->UgrPort[i].GDId.SessionId,i,PortNo);
                _WSHCloseUgrPort(pmod,i);
                break;
            }
        }
        // Find the line entry
	GDLINE *GDLine;
        for ( GDLine=pmod->UgcPort[UgcPortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
        {
            if ( GDLine->LineId==GDAcl->LineId )
                break;
        }
        if ( !GDLine )
        {
            WSHLogError(pmod,"GD Ugc %d missing LINE %d entry!!!",UgcPortNo,GDAcl->LineId);
            return(FALSE);
        }
        // Make sure the last received from the client is < our next out
        if ( pmod->UgrPort[PortNo].NextGDOutLogId > GDLine->NextGDOutLogId )
        {
            WSHLogError(pmod,"GD Ugc %d Login by %d.(%s)(%s)(%s) has LastRecvId=%d >= NextGDOutLogId=%d", 
                UgcPortNo,GDId->LineId,GDId->LineName,GDId->ClientId,GDId->SessionId,pmod->UgrPort[PortNo].NextGDOutLogId-1,GDLine->NextGDOutLogId);
            return(FALSE);
        }
		// Determine the last in from this client
        memcpy(&pmod->UgrPort[PortNo].GDId,GDId,sizeof(GDID));
        sprintf(pmod->UgrPort[PortNo].Note,"%d,%s,%s",GDId->LineId,GDId->LineName,GDId->ClientId);
		if(!WSHReadUgrInLog(pmod,PortNo,GDId->ClientId))
            return(FALSE);
        // Success
        WSHLogEvent(pmod,"Login on Ugr %d by %d.(%s)(%s)(%s). NextSend=%d, LastRecv=%d",
			PortNo,GDId->LineId,GDId->LineName,GDId->ClientId,GDId->SessionId,
			pmod->UgrPort[PortNo].NextGDOutLogId,pmod->UgrPort[PortNo].NextGDInLogId-1);
        return(TRUE);
    }
	// Only complain once per second!
	char *lastErrStr=pmod->UgrAuth_lastErrStr;
	int& lastErrTime=pmod->UgrAuth_lastErrTime;
	char errStr[1024]={0};
	sprintf(errStr,"UGC Port %d Authorization Failure for %d(%s)(%s)(%s) from %s.",
		UgcPortNo,GDId->LineId,GDId->LineName,GDId->ClientId,GDId->SessionId,pmod->UgrPort[PortNo].RemoteIP);
	if((lastErrTime!=WSHTime)||(strcmp(errStr,lastErrStr)!=0))
	{
		WSHLogError(pmod,errStr);
		strcpy(lastErrStr,errStr);
		lastErrTime=WSHTime;
	}
    return(FALSE);
}
void WsocksHostImpl::WSHResetUgcId(WsocksApp *pmod, int PortNo)
{
    pmod->WSGetUgcId(PortNo,pmod->UgcPort[PortNo].SessionId);
    for ( GDLINE *GDLine=pmod->UgcPort[PortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
    {
        if (!WSHReadUgcOutLog(pmod,PortNo,GDLine))
        {
            WSHCloseUgrPort(pmod,PortNo);
            break;
        }
        if(pmod->UgcPort[PortNo].GDGap==0)
            pmod->UgcPort[PortNo].GDGap=100;
		if(!pmod->WSUgcNoGap)
			//WSWriteUgcGap(GDLine,GDLine->NextGDOutLogId+pmod->UgcPort[PortNo].GDGap);
			WSHUgcSendGap(pmod,PortNo,GDLine->LineId,GDLine->NextGDOutLogId+pmod->UgcPort[PortNo].GDGap);
    }
    // Re-authorize all clients
    for ( int i=0; i<pmod->NO_OF_UGR_PORTS; i++ )
    {
        if ( pmod->UgrPort[i].SockActive )
        {
            if (!WSHUgrAuthorize(pmod,i, &pmod->UgrPort[i].GDId))
			{
                //WSHCloseUgrPort(pmod,i);
				pmod->UgrPort[i].TimeTillClose=5;
				pmod->UgrPort[i].SockActive=0;
			}
        }
    }
}

int WsocksHostImpl::WSHSetupUgrPorts(WsocksApp *pmod, int SMode, int PortNo)
{
	int i;
	
	switch (SMode)
	{
	case WS_INIT: // init
		for (i=0;i<=pmod->NO_OF_UGC_PORTS;i++)
		{
			memset(&pmod->UgcPort[i],0,sizeof(UGCPORT));
			pmod->UgcPort[i].BlockSize=WS_DEF_BLOCK_SIZE;
			pmod->UgcPort[i].RecvBuffLimit=WS_DEF_RECV_BUFF_LIMIT;
			pmod->UgcPort[i].SendBuffLimit=WS_DEF_SEND_BUFF_LIMIT;
		}
		for (i=0;i<=pmod->NO_OF_UGR_PORTS;i++)
		{
			memset(&pmod->UgrPort[i],0,sizeof(UGRPORT));
		}
		break;
	case WS_OPEN: // Open
		for (i=0;i<pmod->NO_OF_UGC_PORTS;i++)
		{
			if(strlen(pmod->UgcPort[i].LocalIP)>0)
			{
				//AddConListItem(GetDispItem(WS_UGC,i));
				pmod->UgcPort[i].tmutex=CreateMutex(0,false,0);
				pmod->UgcPort[i].activeThread=0;
				pmod->UgcPort[i].InUse=TRUE;
				//WSHCreatePort(pmod,WS_UGC,i);
				WSHOpenUgcPort(pmod,i);
			}
		}
		//AddConListItem(GetDispItem(WS_UGR_TOT,0));
		break;
	case WS_CLOSE: // close
		for (i=0;i<=pmod->NO_OF_UGC_PORTS;i++)
		{
			if(pmod->UgcPort[i].SockActive)
				WSHCloseUgcPort(pmod,i);
			if(pmod->UgcPort[i].InUse)
			{
				//DeleteConListItem(GetDispItem(WS_UGC,i));
				pmod->UgcPort[i].InUse=FALSE;
			}
			pmod->UgcPort[i].activeThread=0;
			if(pmod->UgcPort[i].tmutex)
			{
				//DeleteMutex(pmod->UgcPort[i].tmutex); pmod->UgcPort[i].tmutex=0;
				DeletePortMutex(pmod,WS_UGC,i,false);
			}			
		}
		for (i=0;i<=pmod->NO_OF_UGR_PORTS;i++)
		{
			if(pmod->UgrPort[i].Sock)// Account for TimeTillClose
			{
				WSHCloseRecording(pmod,&pmod->UgrPort[i].Recording,WS_UGR,i);
				_WSHCloseUgrPort(pmod,i);
				#ifdef WS_OTPP
				_WSHWaitUgrThreadExit(pmod,i);
				#endif
			}
		}
		break;
	}
	return(TRUE);
}
#ifdef WS_OIO
// This version is only called by host UI or app itself
int WsocksHostImpl::WSHCloseUgrPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UgrPort[PortNo].SockActive)
		return -1;
	if(pmod->UgrPort[PortNo].SockActive)
		pmod->UgrPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUgrPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->UgrPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_UGR,PortNo,false);
		lastRecv=pmod->UgrPort[PortNo].recvThread;
		pmod->UgrPort[PortNo].recvThread=GetCurrentThreadId();
		if(pmod->UgrPort[PortNo].pendingClose)
		{
			pmod->UgrPort[PortNo].recvThread=lastRecv;
			UnlockPort(pmod,WS_UGR,PortNo,false);
			return 0;
		}
		pmod->UgrPort[PortNo].pendingClose=true;
	}
	if(pmod->UgrPort[PortNo].smutex)
	{
		LockPort(pmod,WS_UGR,PortNo,true);
		lastSend=pmod->UgrPort[PortNo].sendThread;
		pmod->UgrPort[PortNo].sendThread=GetCurrentThreadId();
	}

	// Cancel all pending overlapped notifications
	WSOVERLAPPED *pendOvlRecvList=pmod->UgrPort[PortNo].pendOvlRecvList; pmod->UgrPort[PortNo].pendOvlRecvList=0;
	WSOVERLAPPED *pendOvlSendList=pmod->UgrPort[PortNo].pendOvlSendList; pmod->UgrPort[PortNo].pendOvlSendList=0;
	if((pendOvlRecvList)||(pendOvlSendList))
	{		
		#ifdef OVLMUX_CRIT_SECTION
		EnterCriticalSection(&ovlMutex);
		#else
		WaitForSingleObject(ovlMutex,INFINITE);
		#endif
		if(pendOvlRecvList)
		{
			WSOVERLAPPED *lastRecv=0;
			for(WSOVERLAPPED *povl=pendOvlRecvList;povl;povl=povl->next)
			{
				povl->Cancelled=true;
				lastRecv=povl;
			}
			lastRecv->next=pmod->cxlOvlList;
			if(pmod->cxlOvlList) pmod->cxlOvlList->prev=lastRecv;
			pmod->cxlOvlList=pendOvlRecvList;
		}
		if(pendOvlSendList)
		{
			WSOVERLAPPED *lastSend=0;
			for(WSOVERLAPPED *povl=pendOvlSendList;povl;povl=povl->next)
			{
				povl->Cancelled=true;
				lastSend=povl;
			}
			lastSend->next=pmod->cxlOvlList;
			if(pmod->cxlOvlList) pmod->cxlOvlList->prev=lastSend;
			pmod->cxlOvlList=pendOvlSendList;
		}
		#ifdef OVLMUX_CRIT_SECTION
		LeaveCriticalSection(&ovlMutex);
		#else
		ReleaseMutex(ovlMutex);
		#endif
	}

	//ResetSendTimeout(WS_UGR,PortNo);
	if(pmod->UgrPort[PortNo].GDId.LineId)
		WSHLogEvent(pmod,"Logout on Ugr %d by %d.(%s)(%s)(%s).",PortNo,pmod->UgrPort[PortNo].GDId.LineId,pmod->UgrPort[PortNo].GDId.LineName,pmod->UgrPort[PortNo].GDId.ClientId,pmod->UgrPort[PortNo].GDId.SessionId);
	if(pmod->UgrPort[PortNo].fGDInLog)
	{
		fclose(pmod->UgrPort[PortNo].fGDInLog);
		#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->UgrPort[PortNo].fGDInLog);
		#endif
		pmod->UgrPort[PortNo].fGDInLog=NULL;
	}
	pmod->UgrPort[PortNo].SockActive=0;
	pmod->WSUgrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_UGR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_UGR,PortNo));
	WSHClosePort(pmod->UgrPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->UgrPort[PortNo].Recording,WS_UGR,PortNo);
	WSCloseBuff(&pmod->UgrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UgrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UgrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UgrPort[PortNo].OutCompBuffer);
	#endif
	if(pmod->UgrPort[PortNo].smutex)
	{
		pmod->UgrPort[PortNo].sendThread=lastSend;
		//DeleteMutex(pmod->UgrPort[PortNo].rmutex); pmod->UgrPort[PortNo].smutex=0;
		DeletePortMutex(pmod,WS_UGR,PortNo,false);
	}
	if(pmod->UgrPort[PortNo].rmutex)
	{
		pmod->UgrPort[PortNo].recvThread=lastRecv;
		//DeleteMutex(pmod->UgrPort[PortNo].rmutex); pmod->UgrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_UGR,PortNo,false);
	}
	memset(&pmod->UgrPort[PortNo],0,sizeof(UGRPORT));
	WSHUpdatePort(pmod,WS_UGR,PortNo);
	return (TRUE);
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCloseUgrPort(WsocksApp *pmod, int PortNo)
{
	if(!pmod->UgrPort[PortNo].SockActive)
		return -1;
	if((pmod->UgrPort[PortNo].SockActive)||(pmod->UgrPort[PortNo].TimeTillClose))
		pmod->UgrPort[PortNo].appClosed=true;
	return 0;
}
// This is only called by WSHSyncLoop directly
int WsocksHostImpl::_WSHCloseUgrPort(WsocksApp *pmod, int PortNo)
{
	int lastRecv=0,lastSend=0;
	if(pmod->UgrPort[PortNo].rmutex)
	{
		LockPort(pmod,WS_UGR,PortNo,false);
		lastRecv=pmod->UgrPort[PortNo].recvThread;
		pmod->UgrPort[PortNo].recvThread=GetCurrentThreadId();
	}
	if(pmod->UgrPort[PortNo].smutex)
	{
		LockPort(pmod,WS_UGR,PortNo,true);
		lastSend=pmod->UgrPort[PortNo].sendThread;
		pmod->UgrPort[PortNo].sendThread=GetCurrentThreadId();
	}

	//ResetSendTimeout(WS_UGR,PortNo);
	if(pmod->UgrPort[PortNo].GDId.LineId)
		WSHLogEvent(pmod,"Logout on Ugr %d by %d.(%s)(%s)(%s).",PortNo,pmod->UgrPort[PortNo].GDId.LineId,pmod->UgrPort[PortNo].GDId.LineName,pmod->UgrPort[PortNo].GDId.ClientId,pmod->UgrPort[PortNo].GDId.SessionId);
	if(pmod->UgrPort[PortNo].fGDInLog)
	{
		fclose(pmod->UgrPort[PortNo].fGDInLog);
		#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->UgrPort[PortNo].fGDInLog);
		#endif
		pmod->UgrPort[PortNo].fGDInLog=NULL;
	}
	pmod->UgrPort[PortNo].SockActive=0;
	pmod->WSUgrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_UGR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_UGR,PortNo));
	if(pmod->UgrPort[PortNo].Sock != 0)
	{
		SOCKET sd=((WSPort*)pmod->UgrPort[PortNo].Sock)->sd;
		WSHClosePort(pmod->UgrPort[PortNo].Sock); pmod->UgrPort[PortNo].Sock=0;
		WSHFinishOverlapSend(pmod,sd,&pmod->UgrPort[PortNo].pendOvlSendBeg,&pmod->UgrPort[PortNo].pendOvlSendEnd);
	}

	#ifdef WIN32
	HANDLE pthread=pmod->UgrPort[PortNo].pthread;
	#else
	pthread_t pthread=pmod->UgrPort[PortNo].pthread;
	#endif
	WSHCloseRecording(pmod,&pmod->UgrPort[PortNo].Recording,WS_UGR,PortNo);
	WSCloseBuff(&pmod->UgrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UgrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UgrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UgrPort[PortNo].OutCompBuffer);
	#endif
	HANDLE srm=0,ssm=0;
	DWORD srcnt=0,sscnt=0;
	if(pmod->UgrPort[PortNo].smutex)
	{
		ssm=pmod->UgrPort[PortNo].smutex;
		pmod->UgrPort[PortNo].sendThread=lastSend;
		//DeleteMutex(pmod->UgrPort[PortNo].rmutex); pmod->UgrPort[PortNo].smutex=0;
		DeletePortMutex(pmod,WS_UGR,PortNo,true);
		sscnt=pmod->UgrPort[PortNo].smutcnt;
	}
	if(pmod->UgrPort[PortNo].rmutex)
	{
		srm=pmod->UgrPort[PortNo].rmutex;
		pmod->UgrPort[PortNo].recvThread=lastRecv;
		//DeleteMutex(pmod->UgrPort[PortNo].rmutex); pmod->UgrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_UGR,PortNo,false);
		srcnt=pmod->UgrPort[PortNo].rmutcnt;
	}
	memset(&pmod->UgrPort[PortNo],0,sizeof(UGRPORT));
	pmod->UgrPort[PortNo].pthread=pthread;
	if(sscnt)
	{
		pmod->UgrPort[PortNo].smutex=ssm;
		pmod->UgrPort[PortNo].smutcnt=sscnt;
	}
	if(srcnt)
	{
		pmod->UgrPort[PortNo].rmutex=srm;
		pmod->UgrPort[PortNo].rmutcnt=srcnt;
	}
	WSHUpdatePort(pmod,WS_UGR,PortNo);
	return (TRUE);
}
void WsocksHostImpl::_WSHWaitUgrThreadExit(WsocksApp *pmod, int PortNo)
{
	if(pmod->UgrPort[PortNo].pthread)
	{
	#ifdef WIN32
		#ifdef _DEBUG
		WaitForSingleObject(pmod->UgrPort[PortNo].pthread,INFINITE);
		#else
		WaitForSingleObject(pmod->UgrPort[PortNo].pthread,3000);
		#endif
		CloseHandle(pmod->UgrPort[PortNo].pthread); pmod->UgrPort[PortNo].pthread=0;
	#else
		void *rc=0;
		pthread_join(pmod->UgrPort[PortNo].pthread,&rc);
	#endif
	}
}
struct UgrReadThreadData
{
	WsocksHostImpl *aimpl;
	WSPort *pport;
};
#ifdef WIN32
DWORD WINAPI _BootUgrReadThread(LPVOID arg)
{
	UgrReadThreadData *ptd=(UgrReadThreadData*)arg;
	WsocksHostImpl *aimpl=ptd->aimpl;
	WsocksApp *pmod=ptd->pport->pmod;
	int PortNo=ptd->pport->PortNo;
	delete ptd;
	int rc=aimpl->WSHUgrReadThread(pmod,PortNo);
	return rc;
}
#else
void *_BootUgrReadThread(void *arg)
{
	UgrReadThreadData *ptd=(UgrReadThreadData*)arg;
	int rc=ptd->aimpl->WSHUgrReadThread(ptd->pport->pmod,ptd->pport->PortNo);
	delete ptd;
	#ifndef _CONSOLE
	pthread_exit((void*)(PTRCAST)rc);
	#endif
	return (void*)(PTRCAST)rc;
}
#endif
int WsocksHostImpl::WSHUgrReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->UgrPort[PortNo].Sock;
	int tsize=pmod->UgrPort[PortNo].InBuffer.BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->UgrPort[PortNo].Sock)// Account for TimeTillClose
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before lockiing the InBuffer and presenting it to the app.
		while((pmod->UgrPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
		{
			int tbytes=recv(pport->sd,tbuf,tsize,0);
			if(tbytes<0)
			{
				err=WSAGetLastError();
				break;
			}			
			else if(!tbytes)
			{
				err=WSAECONNRESET;
				break;
			}
			if(!WSWriteBuff(&TBuffer,tbuf,tbytes))
			{
				err=WSAENOBUFS; _ASSERT(false);
				break;
			}
		}
		pmod->UgrPort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port
		if((TBuffer.Size>0)&&(pmod->UgrPort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_UGR,PortNo,false);
			if(pmod->UgrPort[PortNo].Sock)
			{
				int lastRecv=pmod->UgrPort[PortNo].recvThread;
				pmod->UgrPort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->UgrPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->UgrPort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->UgrPort[PortNo].InBuffer.BlockSize*1024;
					WSHUgrRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->CgdPort[PortNo].TBufferSize=TBuffer.Size;
				pmod->UgrPort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_UGR,PortNo,false);
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->UgrPort[PortNo].Sock))
		pmod->UgrPort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#else//!WS_OIO
int WsocksHostImpl::WSHCloseUgrPort(WsocksApp *pmod, int PortNo)
{
	if(pmod->UgrPort[PortNo].rmutex)
	{
		// Handle close from non-active thread
		LockPort(pmod,WS_UGR,PortNo,false);
		DWORD tid=GetCurrentThreadId();
		while((pmod->UgrPort[PortNo].recvThread)&&(pmod->UgrPort[PortNo].recvThread!=tid))
		{
			UnlockPort(pmod,WS_UGR,PortNo,false);
			SleepEx(100,true);
			LockPort(pmod,WS_UGR,PortNo,false);
		}
		pmod->UgrPort[PortNo].recvThread=tid;
	}
	//ResetSendTimeout(WS_UGR,PortNo);
	if(pmod->UgrPort[PortNo].GDId.LineId)
		WSHLogEvent(pmod,"Logout on Ugr %d by %d.(%s)(%s)(%s).",PortNo,pmod->UgrPort[PortNo].GDId.LineId,pmod->UgrPort[PortNo].GDId.LineName,pmod->UgrPort[PortNo].GDId.ClientId,pmod->UgrPort[PortNo].GDId.SessionId);
	if(pmod->UgrPort[PortNo].fGDInLog)
	{
		fclose(pmod->UgrPort[PortNo].fGDInLog);
		#ifdef WS_GUARANTEED_BACKUP
		WSGDBackupClose(pmod->UgrPort[PortNo].fGDInLog);
		#endif
		pmod->UgrPort[PortNo].fGDInLog=NULL;
	}
	pmod->UgrPort[PortNo].SockActive=0;
	pmod->WSUgrClosed(PortNo);
	//#ifdef WS_MONITOR
	//WSHSendMonClosed(pmod,-1,WS_UGR,PortNo);
	//#endif
	//DeleteConListItem(GetDispItem(WS_UGR,PortNo));
	WSHClosePort(pmod->UgrPort[PortNo].Sock);
	WSHCloseRecording(pmod,&pmod->UgrPort[PortNo].Recording,WS_UGR,PortNo);
	WSCloseBuff(&pmod->UgrPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->UgrPort[PortNo].OutBuffer);
	#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->UgrPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->UgrPort[PortNo].OutCompBuffer);
	#endif
	pmod->UgrPort[PortNo].recvThread=0;
	if(pmod->UgrPort[PortNo].rmutex)
	{
		//DeleteMutex(pmod->UgrPort[PortNo].rmutex); pmod->UgrPort[PortNo].rmutex=0;
		DeletePortMutex(pmod,WS_UGR,PortNo,false);
	}
	memset(&pmod->UgrPort[PortNo],0,sizeof(UGRPORT));
	//WSHPortChanged(pmod,pmod->UgrPort[PortNo],PortNo);
	return (TRUE);
}
#endif//!WS_OIO

int WsocksHostImpl::WSHCgdConnected(WsocksApp *pmod, int PortNo)
{
	WSCloseBuff(&pmod->CgdPort[PortNo].InBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutBuffer);
	WSOpenBuffLimit(&pmod->CgdPort[PortNo].OutBuffer,pmod->CgdPort[PortNo].BlockSize,pmod->CgdPort[PortNo].SendBuffLimit);
	WSOpenBuffLimit(&pmod->CgdPort[PortNo].InBuffer,pmod->CgdPort[PortNo].BlockSize,pmod->CgdPort[PortNo].RecvBuffLimit);
#ifdef WS_COMPRESS
	WSCloseBuff(&pmod->CgdPort[PortNo].InCompBuffer);
	WSCloseBuff(&pmod->CgdPort[PortNo].OutCompBuffer);
	WSOpenBuffLimit(&pmod->CgdPort[PortNo].OutCompBuffer,pmod->CgdPort[PortNo].BlockSize,pmod->CgdPort[PortNo].SendBuffLimit);
	WSOpenBuffLimit(&pmod->CgdPort[PortNo].InCompBuffer,pmod->CgdPort[PortNo].BlockSize,pmod->CgdPort[PortNo].RecvBuffLimit);
#endif
	pmod->CgdPort[PortNo].SinceLastBeatCount=0;
	if (pmod->CgdPort[PortNo].S5Connect)
	{
		if (pmod->CgdPort[PortNo].S5Status<100)
		{
			WSHS5Login(pmod,PortNo);
			return(FALSE);
		}
	}
#ifdef WS_DECLINING_RECONNECT
	pmod->CgdPort[PortNo].ConnectTime=GetTickCount();
#endif
	pmod->CgdPort[PortNo].SockConnected=TRUE;
	pmod->CgdPort[PortNo].ReconCount=0;

	WSHReadCgdInLog(pmod,PortNo);
	pmod->CgdPort[PortNo].LastGDSendLogId=0;
	pmod->CgdPort[PortNo].GDLoginAck=FALSE;
	WSHCgdSendLogin(pmod,PortNo);
    strcpy(pmod->CgdPort[PortNo].OnLineStatusText,pmod->CgdPort[PortNo].GDId.SessionId);

    if(pmod->CgdPort[PortNo].GDGap==0)
        pmod->CgdPort[PortNo].GDGap=100;
    //WSCgdSendGap(PortNo,pmod->CgdPort[PortNo].NextGDOutLogId+pmod->CgdPort[PortNo].GDGap);
#ifdef WIN32
	UuidCreate((UUID*)pmod->CgdPort[PortNo].Uuid);
#endif
#ifdef WS_LOOPBACK
	// Detect loopback connections and remember the corresponding port
	if(!strcmp(pmod->CgdPort[PortNo].RemoteIP,"127.0.0.1"))
	{
		for(int u=0;u<pmod->NO_OF_UGR_PORTS;u++)
		{
			if((pmod->UgrPort[u].SockActive)&&
			   (pmod->UgcPort[pmod->UgrPort[u].UgcPort].Port==pmod->CgdPort[PortNo].Port)&&
			   (!strcmp(pmod->UgcPort[pmod->UgrPort[u].UgcPort].LocalIP,"AUTO")||
			    !strcmp(pmod->UgcPort[pmod->UgrPort[u].UgcPort].LocalIP,"127.0.0.1")))
			{
				SOCKADDR_IN paddr,laddr;
				int palen=sizeof(SOCKADDR_IN),lalen=sizeof(SOCKADDR_IN);
				WSHGetPeerName(pmod->CgdPort[PortNo].Sock,(SOCKADDR*)&paddr,&palen);
				WSHGetSockName(pmod->UgrPort[u].Sock,(SOCKADDR*)&laddr,&lalen);
				if((paddr.sin_addr.s_addr==laddr.sin_addr.s_addr)&&
				   (paddr.sin_port==laddr.sin_port))
				{
					if((pmod->CgdPort[PortNo].Compressed)||(pmod->UgcPort[pmod->UgrPort[u].UgcPort].Compressed))
						WSHLogError(pmod,"Loopback connection from CGD%d to UGR%d detected, but cannot be optimized due to compression.",PortNo,u);
					else
					{
						pmod->CgdPort[PortNo].DetPtr=(void*)(PTRCAST)MAKELONG(u,127);
						pmod->UgrPort[u].DetPtr=(void*)(PTRCAST)MAKELONG(PortNo,127);
						WSHLogEvent(pmod,"Loopback connection from CGD%d to UGR%d detected.",PortNo,u);
					}
				}
			}
		}
	}
#endif
	pmod->CgdPort[PortNo].LastDataTime=GetTickCount();
	WSHUpdatePort(pmod,WS_CGD,PortNo);
	if(pmod->CgdPort[PortNo].DoRecOnOpen)
	{
		pmod->WSOpenRecording(&pmod->CgdPort[PortNo].Recording,pmod->WShWnd,WS_CGD,PortNo);
		pmod->CgdPort[PortNo].DoRecOnOpen=pmod->CgdPort[PortNo].Recording.DoRec;
	}
	pmod->WSCgdOpened(PortNo);

#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->CgdPort[PortNo].Sock;
	::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0);
	for(int o=0;o<WS_OVERLAP_MAX;o++)
	{
		if(WSHCgdIocpBegin(pmod,PortNo)<0)
			return FALSE;
	}
#elif defined(WS_OTPP)
	WSPort *pport=(WSPort*)pmod->CgdPort[PortNo].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
		DWORD tid=0;
		CgdReadThreadData *ptd=new CgdReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
	#ifdef WIN32
		pmod->CgdPort[PortNo].pthread=CreateThread(0,0,_BootCgdReadThread,ptd,0,&tid);
		if(!pmod->CgdPort[PortNo].pthread)
		{
			WSHLogError(pmod,"CGD%d: Failed creating read thread: %d!",PortNo,GetLastError());
			_WSHCloseCgdPort(pmod,PortNo);
			pmod->CgdPort[PortNo].ConnectHold=true;
		}
	#else
		pthread_create(&pmod->CgdPort[PortNo].pthread,0,_BootCgdReadThread,ptd);
	#endif
	#ifdef WS_LOOPBACK
	}
	#endif
#endif
	return (TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHCgdIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	// Issue overlapped recvs
	WSPort *pport=(WSPort*)pmod->CgdPort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->CgdPort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->CgdPort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->CgdPort[PortNo].InBuffer.BlockSize*1024;
	povl->bytes=0;
	povl->flags=0;

	int rc=WSARecv(pport->sd,&povl->wsabuf,1,&povl->bytes,&povl->flags,povl,0);
	if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
	{
		povl->Pending=true;
	}
	else
	{
		delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
		FreeOverlap(&pmod->CgdPort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#elif defined(WS_OTPP)
int WsocksHostImpl::WSHCgdReadThread(WsocksApp *pmod, int PortNo)
{
	DWORD tid=GetCurrentThreadId();
	WSPort *pport=(WSPort*)pmod->CgdPort[PortNo].Sock;
	int tsize=pmod->CgdPort[PortNo].InBuffer.BlockSize*1024;
	char *tbuf=new char[tsize];
	DWORD RecvBuffLimit=(DWORD)pmod->CgdPort[PortNo].RecvBuffLimit*1024;
	if(!RecvBuffLimit)
		RecvBuffLimit=ULONG_MAX;
	BUFFER TBuffer;
	WSOpenBuffLimit(&TBuffer,pmod->CgdPort[PortNo].BlockSize,RecvBuffLimit/1024);
	int err=0;
	while(pmod->CgdPort[PortNo].SockConnected)
	{
		// This implementation increases throughput by reading from the socket as long as data is 
		// available (up to some limit) before lockiing the InBuffer and presenting it to the app.
		while((pmod->CgdPort[PortNo].InBuffer.Size +TBuffer.Size)<RecvBuffLimit)
		{
			int tbytes=recv(pport->sd,tbuf,tsize,0);
			if(tbytes<0)
			{
				err=WSAGetLastError();
				break;
			}			
			else if(!tbytes)
			{
				err=WSAECONNRESET;
				break;
			}
			if(!WSWriteBuff(&TBuffer,tbuf,tbytes))
			{
				err=WSAENOBUFS; _ASSERT(false);
				break;
			}
		}
		pmod->CgdPort[PortNo].TBufferSize=TBuffer.Size;
		// Save what we've pulled off the port
		if((TBuffer.Size>0)&&(pmod->CgdPort[PortNo].InBuffer.Size<RecvBuffLimit))
		{
			LockPort(pmod,WS_CGD,PortNo,false);
			int lastRecv=pmod->CgdPort[PortNo].recvThread;
			if(pmod->CgdPort[PortNo].SockConnected)
			{
				pmod->CgdPort[PortNo].recvThread=tid;
				while((TBuffer.Size>0)&&(pmod->CgdPort[PortNo].InBuffer.Size<RecvBuffLimit))
				{
					DWORD rsize=TBuffer.LocalSize;
					if(rsize>pmod->CgdPort[PortNo].InBuffer.BlockSize*1024)
						rsize=pmod->CgdPort[PortNo].InBuffer.BlockSize*1024;
					WSHCgdRead(pmod,PortNo,TBuffer.Block,rsize);
					if(!WSStripBuff(&TBuffer,rsize))
					{
						err=WSAENOBUFS; _ASSERT(false);
						break;
					}
				}
				pmod->CgdPort[PortNo].TBufferSize=TBuffer.Size;
				pmod->CgdPort[PortNo].recvThread=lastRecv;
			}
			else
				err=WSAECONNABORTED;
			UnlockPort(pmod,WS_CGD,PortNo,false);
		}
		if((err)&&(err!=WSAEWOULDBLOCK))
			break;
		SleepEx(1,true); // Ohterwise, CPU will be utilized 100%
	}
	if((err!=WSAECONNABORTED)&&(pmod->CgdPort[PortNo].Sock))
		pmod->CgdPort[PortNo].peerClosed=true;
	WSCloseBuff(&TBuffer);
	delete tbuf;
	return 0;
}
#endif

int WsocksHostImpl::_WSHBeforeCgdSend(WsocksApp *pmod, int PortNo)
{
	if((pmod->CgdPort[PortNo].GDLoginAck)&&(pmod->CgdPort[PortNo].SendTimeOut==0))
	{
		char *Block;
		int MaxBlockSize=pmod->CgdPort[PortNo].BlockSize -1;
		if(MaxBlockSize>63)
			MaxBlockSize=63;
		MaxBlockSize*=1024;
		unsigned int BlockSize;
		unsigned int BlockCount;
		unsigned int Count=0;
		if (pmod->CgdPort[PortNo].NextGDOutLogId-1>pmod->CgdPort[PortNo].LastGDSendLogId)
			Count=pmod->CgdPort[PortNo].NextGDOutLogId-1-pmod->CgdPort[PortNo].LastGDSendLogId;
		Block=(char*)malloc(MaxBlockSize);
		if(Count > 0)
		{
			BlockSize=MaxBlockSize;
			BlockCount=Count;
			if(WSHReadFILEBlocks(pmod,pmod->CgdPort[PortNo].GDOutLogFileIndex, Block, &BlockSize,pmod->CgdPort[PortNo].LastGDSendLogId+1,&BlockCount))
			{
				if(BlockCount)
				{
					pmod->CgdPort[PortNo].LastGDSendLogId+=BlockCount;
					if(BlockSize)
						WSHCgdSendBlock(pmod,Block,(WORD)BlockSize,PortNo);
				}
			}
		}
		free(Block);
	}
	return pmod->WSBeforeCgdSend(PortNo);
}
int WsocksHostImpl::_WSHBeforeUgrSend(WsocksApp *pmod, int PortNo)
{
	if(pmod->UgrPort[PortNo].SendTimeOut==0)
	{
		int UgcPortNo=pmod->UgrPort[PortNo].UgcPort;
		int MaxBlockSize=pmod->UgcPort[UgcPortNo].BlockSize -1;
		if(MaxBlockSize>63)
			MaxBlockSize=63;
		MaxBlockSize*=1024;
		GDLINE *GDLine;
		for ( GDLine=pmod->UgcPort[UgcPortNo].GDLines; GDLine; GDLine=GDLine->NextGDLine )
		{
			if (GDLine->LineId==pmod->UgrPort[PortNo].GDId.LineId)
				break;
		}
		if ( !GDLine )
		{
			pmod->WSBeforeUgrSend(PortNo);
			return FALSE;
		}
		char *Block;
		unsigned int BlockSize;
		unsigned int BlockCount;
		unsigned int Count=0;
		if (GDLine->NextGDOutLogId>pmod->UgrPort[PortNo].NextGDOutLogId)
			Count=GDLine->NextGDOutLogId-pmod->UgrPort[PortNo].NextGDOutLogId;
		if(Count > 0)
		{
			Block=(char*)malloc(MaxBlockSize);
			BlockSize=MaxBlockSize;
			BlockCount=Count;
			if(WSHReadFILEBlocks(pmod,GDLine->GDOutLogFileIndex, Block, &BlockSize,pmod->UgrPort[PortNo].NextGDOutLogId,&BlockCount))
			{
				if(BlockCount)
				{
					pmod->UgrPort[PortNo].NextGDOutLogId=pmod->UgrPort[PortNo].NextGDOutLogId+BlockCount;
					if(BlockSize)
						WSHUgrSendBlock(pmod,Block,(WORD)BlockSize,PortNo);

				}
			}
			free(Block);
		}
	}
	return pmod->WSBeforeUgrSend(PortNo);
}
#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHCgdRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	pmod->CgdPort[i].LastDataTime=GetTickCount();
	pmod->CgdPort[i].BytesIn+=bytes;
	pmod->CgdPort[i].BlocksIn++;

	// SOCKS5 protocol
	if((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status>=10)&&(pmod->CgdPort[i].S5Status<100))
	{
		_ASSERT(false);//untested
		bool s5err=false;
		switch (pmod->CgdPort[i].S5Status)
		{
		case 10:
			if(pmod->CgdPort[i].InBuffer.LocalSize>=2)
			{
				int Len=2;
				int Version=pmod->CgdPort[i].InBuffer.Block[0];
				int Methode=pmod->CgdPort[i].InBuffer.Block[1];
				if ((Version!=pmod->CgdPort[i].S5Version)||(Methode!=pmod->CgdPort[i].S5Methode))
				{
					s5err=true;
					break;
				}
				WSStripBuff(&pmod->CgdPort[i].InBuffer, Len);
				WSHS5Connect(pmod,i);
				break;
			}
			break;
		case 20:
			if(pmod->CgdPort[i].InBuffer.LocalSize>=4)
			{
				unsigned int Len=4;
				int Version=pmod->CgdPort[i].InBuffer.Block[0];
				int Reply=pmod->CgdPort[i].InBuffer.Block[1];
				int AdressType=pmod->CgdPort[i].InBuffer.Block[3];
				switch(AdressType)
				{
				case 1:
					Len+=4; break;
				default:
					s5err=true;
					break;
				};
				// AddPort Len
				Len+=2;
				if(pmod->CgdPort[i].InBuffer.LocalSize<Len)
					break;
				WSStripBuff(&pmod->CgdPort[i].InBuffer, Len);
				pmod->CgdPort[i].S5Status=100;
				WSHCgdConnected(pmod,i);
				break;
			}
			break;
		};
		if(s5err)
		{
			WSHCloseCgdPort(pmod,i);
			WSHLogError(pmod,"Socks5 Failed on CGD%d",i);
		}
	}
#ifdef WS_COMPRESS
	// Decompression
	if(pmod->CgdPort[i].Compressed&&(!((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status<100))))
	{
		WSHWaitMutex(0x01,INFINITE);
		char *szTemp=pmod->CgdRead_szTemp;
		char *szDecompBuff=pmod->CgdRead_szDecompBuff;
		#ifdef WS_ENCRYPTED
		char *szDecryptBuff=pmod->CgdRead_szDecryptBuff;
		unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
		#endif

		if(!WSWriteBuff(&pmod->CgdPort[i].InCompBuffer,(char*)buf,bytes))
		{
			WSHReleaseMutex(0x01);
			return(FALSE);
		}
GetNextBlock:
		if(pmod->CgdPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
		{
			unsigned int *CompSize=(unsigned int*)pmod->CgdPort[i].InCompBuffer.Block;
			unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);
			#ifdef BIT64
			unsigned long lCompSize=*CompSize;
			unsigned long lDecompSize=DecompSize;
			#endif

			if(pmod->CgdPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
			{
			#ifdef WS_ENCRYPTED
				// Decryption
				if(pmod->CgdPort[i].EncryptionOn)
				{
					WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
						,&pmod->CgdPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_CGD);
				}
				else
				{
					memcpy(szDecryptBuff,&pmod->CgdPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
					DecryptSize=*CompSize;
				}
				if(uncompress(szDecompBuff,&DecompSize
					,szDecryptBuff,DecryptSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseCgdPort(pmod,i);
					return(FALSE);
				}
			#else
				if(uncompress(szDecompBuff,&DecompSize
					,&pmod->CgdPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseCgdPort(pmod,i);
					return(FALSE);
				}
			#endif
				WSHRecord(pmod,&pmod->CgdPort[i].Recording,szDecompBuff,DecompSize,false);
				WSStripBuff(&pmod->CgdPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
				if(!WSWriteBuff(&pmod->CgdPort[i].InBuffer,szDecompBuff,DecompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseCgdPort(pmod,i);
					return(FALSE);
				}
				goto GetNextBlock;
			}
		}
		WSHReleaseMutex(0x01);
	}
	else
#endif//WS_COMPRESS
	{
		WSHRecord(pmod,&pmod->CgdPort[i].Recording,buf,bytes,false);
		if(!WSWriteBuff(&pmod->CgdPort[i].InBuffer,(char*)buf,bytes))
		{
			WSHCloseCgdPort(pmod,i);
			return(FALSE);
		}
	}
	return TRUE;
}
#else//!WS_OIO
int WsocksHostImpl::WSHCgdRead(WsocksApp *pmod, int PortNo)
{
	char *szTemp=pmod->CgdRead_szTemp;
#ifdef WS_COMPRESS
	char *szDecompBuff=pmod->CgdRead_szDecompBuff;
#ifdef WS_ENCRYPTED
	char *szDecryptBuff=pmod->CgdRead_szDecryptBuff;
	unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
#endif
#endif
	int status;             // Status Code 
	int i=PortNo;

	if((pmod->CgdPort[i].InBuffer.Busy)&&(pmod->CgdPort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: CGD%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->CgdPort[i].InBuffer.Busy);
		pmod->CgdPort[i].InBuffer.Busy=0;
	}
	pmod->CgdPort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common CgdRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
#ifdef WS_COMPRESS
	if(pmod->CgdPort[i].Compressed&&(!((pmod->CgdPort[i].S5Connect)&&(pmod->CgdPort[i].S5Status<100))))
	{
	#ifdef WS_OIO
		_ASSERT(false);
	#else
		status = WSHRecv(pmod->CgdPort[i].Sock, szTemp, ((pmod->CgdPort[i].BlockSize-1)*1024), NO_FLAGS_SET );
	#endif

		if (status>0) 
		{
			pmod->CgdPort[i].LastDataTime=GetTickCount();
			pmod->CgdPort[i].BytesIn+=status;
			pmod->CgdPort[i].BlocksIn++;

			if(!WSWriteBuff(&pmod->CgdPort[i].InCompBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				pmod->CgdPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
GetNextBlock:
			if(pmod->CgdPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
			{
				unsigned int *CompSize=(unsigned int *)pmod->CgdPort[i].InCompBuffer.Block;
				unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

				if(pmod->CgdPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
				{
#ifdef WS_ENCRYPTED
					if(pmod->CgdPort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->CgdPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_CGD);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->CgdPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize
						,szDecryptBuff,DecryptSize))
#else
					if(uncompress(szDecompBuff,&DecompSize
						,&pmod->CgdPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
#endif
					{
						WSHReleaseMutex(0x01);
						WSHCloseCgdPort(pmod,i);
						return(FALSE);
					}
					WSHRecord(pmod,&pmod->CgdPort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->CgdPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
					if(!WSWriteBuff(&pmod->CgdPort[i].InBuffer,szDecompBuff,DecompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseCgdPort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
			}
			pmod->CgdPort[i].InBuffer.Busy=lastBusy;
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->CgdPort[i].InBuffer.Size>0)
			{
				while(pmod->WSCgdMsgReady(i))
					pmod->WSCgdStripMsg(i);
			}
			WSHCloseCgdPort(pmod,i);
			return(FALSE);
		}
	}
	else
#endif
	{
		status = WSHRecv(pmod->CgdPort[i].Sock, szTemp, ((pmod->CgdPort[i].BlockSize-1)*1024), NO_FLAGS_SET );

		if (status>0) 
		{
    		pmod->CgdPort[i].LastDataTime=GetTickCount();
			pmod->CgdPort[i].BytesIn+=status;
			if(status==132||status==8)
			{
				if(*((WORD *)szTemp)!=16)
				{
					pmod->CgdPort[i].BlocksIn++;
				}
			}
			else
			{
				pmod->CgdPort[i].BlocksIn++;
			}
			WSHRecord(pmod,&pmod->CgdPort[i].Recording,szTemp,status,false);
			if(!WSWriteBuff(&pmod->CgdPort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseCgdPort(pmod,i);
				return(FALSE);
			}
			pmod->CgdPort[i].InBuffer.Busy=lastBusy;
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->CgdPort[i].InBuffer.Size>0)
			{
				while(pmod->WSCgdMsgReady(i))
					pmod->WSCgdStripMsg(i);
			}
			WSHCloseCgdPort(pmod,i);
			return(FALSE);
		}
	}
	WSHReleaseMutex(0x01);
	pmod->CgdPort[i].InBuffer.Busy=lastBusy;
	return(TRUE);
}
#endif//!WS_OIO
#if defined(WS_OIO)||defined(WS_OTPP)
int WsocksHostImpl::WSHUgrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	int i=PortNo;
	//pmod->UgrPort[i].LastDataTime=GetTickCount();
	pmod->UgrPort[i].BytesIn+=bytes;
	pmod->UgrPort[i].BlocksIn++;

#ifdef WS_COMPRESS
	if((pmod->UgcPort[pmod->UgrPort[i].UgcPort].Compressed))//||(pmod->UgcPort[pmod->UgrPort[i].UgcPort].CompressType>0))
	{
		WSHWaitMutex(0x01,INFINITE);
		char *szTemp=pmod->UgrRead_szTemp;
		char *szDecompBuff=pmod->UgrRead_szDecompBuff;
		#ifdef WS_ENCRYPTED
		char *szDecryptBuff=pmod->UgrRead_szDecryptBuff;
		unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
		#endif
		if(!WSWriteBuff(&pmod->UgrPort[i].InCompBuffer,(char*)buf,bytes))
		{
			WSHReleaseMutex(0x01);
			return(FALSE);
		}
GetNextBlock:
		if(pmod->UgrPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
		{
			unsigned int *CompSize=(unsigned int*)pmod->UgrPort[i].InCompBuffer.Block;
			unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

			if(pmod->UgrPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
			{
			#ifdef WS_ENCRYPTED
				if(pmod->UgrPort[i].EncryptionOn)
				{
					WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
						,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_UGR);
				}
				else
				{
					memcpy(szDecryptBuff,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
					DecryptSize=*CompSize;
				}
				if(uncompress(szDecompBuff,&DecompSize
					,szDecryptBuff,DecryptSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUgrPort(pmod,i);
					return(FALSE);
				}
			#else
				//if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].CompressType==2)
				//{
				//	if(csuncompress(szDecompBuff,&DecompSize
				//		,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],CompSize))
				//	{
				//		WSHReleaseMutex(0x01);
				//		WSHCloseUgrPort(pmod,i);
				//		return(FALSE);
				//	}
				//}
				//else 
				if(uncompress(szDecompBuff,&DecompSize
					,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUgrPort(pmod,i);
					return(FALSE);
				}
			#endif

				WSHRecord(pmod,&pmod->UgrPort[i].Recording,szDecompBuff,DecompSize,false);
				WSStripBuff(&pmod->UgrPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
				if(!WSWriteBuff(&pmod->UgrPort[i].InBuffer,szDecompBuff,DecompSize))
				{
					WSHReleaseMutex(0x01);
					WSHCloseUgrPort(pmod,i);
					return(FALSE);
				}
				goto GetNextBlock;
			}
		}
		WSHReleaseMutex(0x01);
	}
	else
#endif//WS_COMPRESS
	{
		WSHRecord(pmod,&pmod->UgrPort[i].Recording,buf,bytes,false);
		if(!WSWriteBuff(&pmod->UgrPort[i].InBuffer,(char*)buf,bytes))
		{
			WSHCloseUgrPort(pmod,i);
			return(FALSE);
		}
	}
	return TRUE;
}
#else//!WS_OIO
int WsocksHostImpl::WSHUgrRead(WsocksApp *pmod, int PortNo, const char *buf, int bytes)
{
	char *szTemp=pmod->UgrRead_szTemp;
#ifdef WS_COMPRESS
	char *szDecompBuff=pmod->UgrRead_szDecompBuff;
#ifdef WS_ENCRYPTED
	char *szDecryptBuff=pmod->UgrRead_szDecryptBuff;
	unsigned int DecryptSize=(WS_MAX_BLOCK_SIZE*1024);
#endif
#endif
	int status;             // Status Code 
	int i=PortNo;

	if((pmod->UgrPort[i].InBuffer.Busy)&&(pmod->UgrPort[i].InBuffer.Busy!=GetCurrentThreadId()))
	{
		_ASSERT(false);
		WSHLogError(pmod,"!CRASH: UGR%d InBuffer.Busy detected a possible thread %d crash.",
			PortNo,pmod->UgrPort[i].InBuffer.Busy);
		pmod->UgrPort[i].InBuffer.Busy=0;
	}
	int lastBusy=pmod->UgrPort[i].InBuffer.Busy;
	pmod->UgrPort[i].InBuffer.Busy=GetCurrentThreadId();

	// We can't just protect the zlib call, but the common UgrRead_xxx buffers we're using too
	WSHWaitMutex(0x01,INFINITE);
#ifdef WS_COMPRESS
	if((pmod->UgcPort[pmod->UgrPort[i].UgcPort].Compressed))//||(pmod->UgcPort[pmod->UgrPort[i].UgcPort].CompressType>0))
	{
		status = WSHRecv(pmod->UgrPort[i].Sock, szTemp, ((pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize-1)*1024), NO_FLAGS_SET );
		if (status>0) 
		{
			pmod->UgrPort[i].BytesIn+=status;
			pmod->UgrPort[i].BlocksIn++;

			if(!WSWriteBuff(&pmod->UgrPort[i].InCompBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				pmod->UgrPort[i].InBuffer.Busy=lastBusy;
				return(FALSE);
			}
GetNextBlock:
			if(pmod->UgrPort[i].InCompBuffer.LocalSize>=sizeof(unsigned int))
			{
				unsigned int *CompSize=(unsigned int*)pmod->UgrPort[i].InCompBuffer.Block;
				unsigned int DecompSize=(WS_MAX_BLOCK_SIZE*1024);

				if(pmod->UgrPort[i].InCompBuffer.LocalSize>=((*CompSize)+sizeof(unsigned int)))
				{
#ifdef WS_ENCRYPTED
					if(pmod->UgrPort[i].EncryptionOn)
					{
						WSHDecrypt(pmod,szDecryptBuff,&DecryptSize
							,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize,i,WS_UGR);
					}
					else
					{
						memcpy(szDecryptBuff,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize);
						DecryptSize=*CompSize;
					}
					if(uncompress(szDecompBuff,&DecompSize
						,szDecryptBuff,DecryptSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUgrPort(pmod,i);
						return(FALSE);
					}
#else
					if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].CompressType==2)
					{
						if(csuncompress(szDecompBuff,&DecompSize
							,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],CompSize))
						{
							WSHReleaseMutex(0x01);
							WSHCloseUgrPort(pmod,i);
							return(FALSE);
						}
					}
					else if(uncompress(szDecompBuff,&DecompSize
						,&pmod->UgrPort[i].InCompBuffer.Block[sizeof(unsigned int)],*CompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUgrPort(pmod,i);
						return(FALSE);
					}
#endif

					WSHRecord(pmod,&pmod->UgrPort[i].Recording,szDecompBuff,DecompSize,false);
					WSStripBuff(&pmod->UgrPort[i].InCompBuffer,((*CompSize)+sizeof(unsigned int)));
					if(!WSWriteBuff(&pmod->UgrPort[i].InBuffer,szDecompBuff,DecompSize))
					{
						WSHReleaseMutex(0x01);
						WSHCloseUgrPort(pmod,i);
						return(FALSE);
					}
					goto GetNextBlock;
				}
			}
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->UgrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUgrMsgReady(i))
					pmod->WSUgrStripMsg(i);
			}
			WSHCloseUgrPort(pmod,i);
			return(FALSE);
		}
	}
	else
#endif
	{
		status = WSHRecv(pmod->UgrPort[i].Sock, szTemp, ((pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize-1)*1024), NO_FLAGS_SET );

		if (status>0) 
		{
			pmod->UgrPort[i].BytesIn+=status;
			if(status==132||status==8)
			{
				if(*((WORD *)szTemp)!=16)
				{
					pmod->UgrPort[i].BlocksIn++;
				}
			}
			else
			{
				pmod->UgrPort[i].BlocksIn++;
			}
			WSHRecord(pmod,&pmod->UgrPort[i].Recording,szTemp,status,false);
			if(!WSWriteBuff(&pmod->UgrPort[i].InBuffer,szTemp,status))
			{
				WSHReleaseMutex(0x01);
				WSHCloseUgrPort(pmod,i);
				return(FALSE);
			}
		}
		else
		{
			WSHReleaseMutex(0x01);
			// One last chance to read the data before port closed by peer
			if(pmod->UgrPort[i].InBuffer.Size>0)
			{
				while(pmod->WSUgrMsgReady(i))
					pmod->WSUgrStripMsg(i);
			}
			WSHCloseUgrPort(pmod,i);
			return(FALSE);
		}
	}
	WSHReleaseMutex(0x01);
	return(TRUE);
}
#endif//!WS_OIO

int WsocksHostImpl::WSHCgdSendBlock(WsocksApp *pmod, char *Block,WORD BlockLen,int PortNo)
{
	if((PortNo < 0)||(PortNo >=pmod->NO_OF_CGD_PORTS))
	{
		_ASSERT(false);
		return FALSE;
	}
	if((BlockLen <= 0)||(BlockLen>=pmod->CgdPort[PortNo].BlockSize*1024))
	{
		_ASSERT(false);
		return FALSE;
	}
	if(pmod->CgdPort[PortNo].SockConnected)
	{
		if((pmod->CgdPort[PortNo].OutBuffer.Busy)&&(pmod->CgdPort[PortNo].OutBuffer.Busy!=GetCurrentThreadId()))
		{
			_ASSERT(false);
			WSHLogError(pmod,"!CRASH: CGD%d OutBuffer.Busy detected a possible thread %d crash.",
				PortNo,pmod->CgdPort[PortNo].OutBuffer.Busy);
			pmod->CgdPort[PortNo].OutBuffer.Busy=0;
		}
		int lastBusy=pmod->CgdPort[PortNo].OutBuffer.Busy;
		pmod->CgdPort[PortNo].OutBuffer.Busy=GetCurrentThreadId();
		//Send GDMsgHeader
		if(!WSWriteBuff(&pmod->CgdPort[PortNo].OutBuffer,Block,BlockLen))
		{
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
			return (FALSE);
		}
		pmod->CgdPort[PortNo].PacketsOut++;
		pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
	#ifdef WS_REALTIMESEND
		WSHCgdSend(pmod,PortNo,false);
	#endif
		return(TRUE);
	}
	else
		return(FALSE);
}
int WsocksHostImpl::WSHUgrAccept(WsocksApp *pmod, int PortNo)
{
	SOCKET_T TSock;			// temp socket to hold connection while we
							// determine what port it was for 
	SOCKADDR_IN AccSin;    // Accept socket address - internet style 
	int AccSinLen;        // Accept socket address length 
	int i;
	int j=PortNo;

	ws_fd_set rds;
	WS_FD_ZERO(&rds);
	WS_FD_SET(pmod->UgcPort[j].Sock,&rds);
	struct timeval TimeVal={0,0};
	int nrds=WSHSelect((PTRCAST)pmod->UgcPort[j].Sock+1,&rds,NULL,NULL,(timeval*)&TimeVal);
	if(nrds<1)
		return FALSE;
	// determine which user channel is available
	for (i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	{
		if((!pmod->UgrPort[i].SockActive)&&(pmod->UgrPort[i].TimeTillClose<=0)
			#ifdef WS_OIO
			&&(!pmod->UgrPort[i].pendingClose)
			#endif
			&&(!pmod->UgrPort[i].rmutcnt)&&(!pmod->UgrPort[i].smutcnt)
			)
			break;
	}
	if (i>=pmod->NO_OF_UGR_PORTS) // no more ports avaliable
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : No more user ports avaliable");
		// Accept the incoming connection with port -1 to drop it
		AccSinLen = sizeof( AccSin );
		TSock = WSHAccept(pmod,pmod->UscPort[j].Sock,WS_UGR,-1,(struct sockaddr *) &AccSin,
			(int *) &AccSinLen );
		return(FALSE);
	}
	pmod->UgrPort[i].UgcPort=j;

    AccSinLen = sizeof( AccSin );
    //TSock = accept(pmod->UgcPort[j].Sock,(struct sockaddr *) &AccSin,
    TSock = WSHAccept(pmod,pmod->UgcPort[j].Sock,WS_UGR,i,(struct sockaddr *) &AccSin,
        (int *) &AccSinLen );

	if (TSock == INVALID_SOCKET_T)
	{
		if(i!=-1)
			WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UGC%d accept() failed",j);
		return(FALSE);
	}

#if !defined(WIN32)&&!defined(_CONSOLE)
	// On Windows, the accepted socket inherits async behavior from listen socket,
	// but on Linux, it doesn't inherit fcntl attributes
	unsigned long wstrue_ul = 1;
	if (WSHIoctlSocket(TSock,FIONBIO,&wstrue_ul)== SOCKET_ERROR)
	{
		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : USC%d WSUsrAccept  USR%d ioctlsocket(!FIONBIO) failed",j,i);
		WSHClosePort(TSock);
		return(FALSE);
	}
#endif

	// There's no evidence this helps
#ifdef WS_REALTIMESEND
//	// Disable Nagle
//	unsigned int nagleoff=1;
//	if (WSHSetSockOpt(TSock, IPPROTO_TCP, TCP_NODELAY, (char *)(&nagleoff), sizeof(nagleoff)) == SOCKET_ERROR) 
//	{
//		WSHLogError(pmod,"!WSOCKS: FATAL ERROR : UgrSocket setsockopt(TCP_NODELAY) failed");
//        WSHClosePort( TSock );
//		return(FALSE);
//	}
	pmod->UgrPort[i].ImmediateSendLimit=pmod->UgcPort[pmod->UgrPort[i].UgcPort].ImmediateSendLimit;
#endif

	//// determine which user channel is available
	//for (i=0;i<pmod->NO_OF_UGR_PORTS;i++)
	//{
	//	if((!pmod->UgrPort[i].SockActive)&&(pmod->UgrPort[i].TimeTillClose<=0))
	//		break;
	//}
	//if (i>=pmod->NO_OF_UGR_PORTS) // no more ports avaliable
	//{
	//	WSHLogError(pmod,"!WSOCKS: FATAL ERROR : No more user ports avaliable");
 //       WSHClosePort( TSock );
	//	return(FALSE);
	//}

    // DONT MOVE THIS LINE OR THE BUFFER SIZE IN THE NEXT LINE WILL FAIL
	//pmod->UgrPort[i].UgcPort=j;

	WSOpenBuffLimit(&pmod->UgrPort[i].InBuffer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize,pmod->UgcPort[pmod->UgrPort[i].UgcPort].RecvBuffLimit);
	WSOpenBuffLimit(&pmod->UgrPort[i].OutBuffer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize,pmod->UgcPort[pmod->UgrPort[i].UgcPort].SendBuffLimit);
#ifdef WS_COMPRESS
	WSOpenBuffLimit(&pmod->UgrPort[i].InCompBuffer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize,pmod->UgcPort[pmod->UgrPort[i].UgcPort].RecvBuffLimit);
	WSOpenBuffLimit(&pmod->UgrPort[i].OutCompBuffer,pmod->UgcPort[pmod->UgrPort[i].UgcPort].BlockSize,pmod->UgcPort[pmod->UgrPort[i].UgcPort].SendBuffLimit);
#endif
	pmod->UgrPort[i].rmutcnt=0;
	pmod->UgrPort[i].rmutex=CreateMutex(0,false,0);
	pmod->UgrPort[i].smutcnt=0;
	pmod->UgrPort[i].smutex=CreateMutex(0,false,0);
	pmod->UgrPort[i].recvThread=0;
	pmod->UgrPort[i].sendThread=0;
	pmod->UgrPort[i].Sock=TSock;
	strcpy(pmod->UgrPort[i].Note,pmod->UgcPort[j].GDCfg);
	pmod->UgrPort[i].SockActive=TRUE;
	LockPort(pmod,WS_UGR,i,false);
	LockPort(pmod,WS_UGR,i,true);
	#ifdef WIN32
	sprintf(pmod->UgrPort[i].RemoteIP,"%d.%d.%d.%d"
		,AccSin.sin_addr.S_un.S_un_b.s_b1
		,AccSin.sin_addr.S_un.S_un_b.s_b2
		,AccSin.sin_addr.S_un.S_un_b.s_b3
		,AccSin.sin_addr.S_un.S_un_b.s_b4);
	#else
	strcpy(pmod->UgrPort[i].RemoteIP,inet_ntoa(AccSin.sin_addr));
	#endif
	//AddConListItem(GetDispItem(WS_UGR,i));
#ifdef WIN32
	UuidCreate((UUID*)pmod->UgrPort[i].Uuid);
#endif
	if(pmod->UgcPort[pmod->UgrPort[i].UgcPort].Recording.DoRec)
		WSHOpenRecording(pmod,&pmod->UgrPort[i].Recording,0,WS_UGR,i);
	//pmod->WSUgrOpened(i);
	UnlockPort(pmod,WS_UGR,i,true);
	UnlockPort(pmod,WS_UGR,i,false);
	WSHUpdatePort(pmod,WS_UGR,PortNo);

#ifdef WS_OIO
	WSPort *pport=(WSPort*)pmod->UgrPort[i].Sock;
	::CreateIoCompletionPort((HANDLE)pport->sd,pmod->hIOPort,(ULONG_PTR)pport,0);
	for(int o=0;o<WS_OVERLAP_MAX;o++)
	{
		if(WSHUgrIocpBegin(pmod,i)<0)
			return FALSE;
	}
#elif defined(WS_OTPP)
	WSPort *pport=(WSPort*)pmod->UgrPort[i].Sock;
	#ifdef WS_LOOPBACK
	if((pport)&&(!pport->lbaPeer))
	{
	#endif
		DWORD tid=0;
		UgrReadThreadData *ptd=new UgrReadThreadData;
		ptd->aimpl=this;
		ptd->pport=pport;
	#ifdef WIN32
		pmod->UgrPort[i].pthread=CreateThread(0,0,_BootUgrReadThread,ptd,0,&tid);
		if(!pmod->UgrPort[i].pthread)
		{
			WSHLogError(pmod,"UGR%d: Failed creating read thread: %d!",i,GetLastError());
			_WSHCloseUgrPort(pmod,i);
		}
	#else
		pthread_create(&pmod->UgrPort[i].pthread,0,_BootUgrReadThread,ptd);
	#endif
	#ifdef WS_LOOPBACK
	}
	#endif
#endif
	return(TRUE);
}
#ifdef WS_OIO
int WsocksHostImpl::WSHUgrIocpBegin(WsocksApp *pmod, int PortNo, WSOVERLAPPED *povl)
{
	// Issue overlapped recvs
	WSPort *pport=(WSPort*)pmod->UgrPort[PortNo].Sock;
	if(!pport)
		return -1;
	if(!povl)
		povl=AllocOverlap(&pmod->UgrPort[PortNo].pendOvlRecvList);
	if(!povl)
		return -1;
	povl->PortType=WS_CON;
	povl->PortNo=PortNo;
	povl->Pending=false;
	povl->Cancelled=false;
	povl->RecvOp=1;
	if(!povl->buf)
		povl->buf=new char[pmod->UgrPort[PortNo].InBuffer.BlockSize*1024];
	povl->wsabuf.buf=povl->buf;
	povl->wsabuf.len=pmod->UgrPort[PortNo].InBuffer.BlockSize*1024;
	povl->bytes=0;
	povl->flags=0;

	int rc=WSARecv(pport->sd,&povl->wsabuf,1,&povl->bytes,&povl->flags,povl,0);
	if((!rc)||((rc==SOCKET_ERROR)&&(WSAGetLastError()==WSA_IO_PENDING)))
	{
		povl->Pending=true;
	}
	else
	{
		delete povl->buf; povl->wsabuf.buf=povl->buf=0; povl->wsabuf.len=0;
		FreeOverlap(&pmod->UgrPort[PortNo].pendOvlRecvList,povl);
		return -1;
	}
	return 0;
}
#endif

int WsocksHostImpl::WSHCgdSend(WsocksApp *pmod, int PortNo, bool flush)
{
	int nsent=0;
	int i=PortNo;
	// Socks5 protocol exchange not compressed
	bool socks5=false,compressed=false;
	if((pmod->CgdPort[PortNo].S5Connect)&&(pmod->CgdPort[PortNo].S5Status>=10)&&(pmod->CgdPort[PortNo].S5Status<100))
		socks5=true;
	else if(!pmod->CgdPort[PortNo].SockConnected)
		return -1;

	int lastBusy=pmod->CgdPort[PortNo].OutBuffer.Busy;
	pmod->CgdPort[PortNo].OutBuffer.Busy=pmod->CgdPort[PortNo].sendThread;
	pmod->CgdPort[PortNo].SendTimeOut=0;
	BUFFER *sendBuffer=&pmod->CgdPort[PortNo].OutBuffer;
#ifdef WS_COMPRESS
	if((!socks5)&&(pmod->CgdPort[PortNo].Compressed))
	{
		compressed=true;
		sendBuffer=&pmod->CgdPort[PortNo].OutCompBuffer;
		// Compress as much as possible to fill one send block
		while((pmod->CgdPort[PortNo].OutBuffer.Size>0)&&
			  ((flush)||((int)pmod->CgdPort[PortNo].OutBuffer.Size>=pmod->CgdPort[PortNo].ImmediateSendLimit)))
		{
			unsigned int CompSize=pmod->CgdPort[PortNo].OutBuffer.LocalSize;
			if(CompSize>((WS_COMP_BLOCK_LEN*1024)*99/100))
				CompSize=((WS_COMP_BLOCK_LEN*1024)*99/100);
			if(CompSize>0)
			{
				WSHWaitMutex(0x01,INFINITE);
				char *CompBuff=pmod->SyncLoop_CompBuff;
				unsigned int CompBuffSize=(WS_MAX_BLOCK_SIZE*1024)*2;
				mtcompress(CompBuff,&CompBuffSize,pmod->CgdPort[PortNo].OutBuffer.Block,CompSize);
				// Compression +encryption
				#ifdef WS_ENCRYPTED
				char *EncryptBuff=CompBuff;
				unsigned int EncryptSize=CompBuffSize;
				if(pmod->CgdPort[PortNo].EncryptionOn)
				{
					EncryptBuff=pmod->SyncLoop_EncryptBuff;
					EncryptSize=sizeof(pmod->SyncLoop_EncryptBuff);
					WSHEncrypt(pmod,EncryptBuff,&EncryptSize,CompBuff,CompBuffSize,PortNo,WS_CGD);
				}
				WSWriteBuff(sendBuffer,(char*)&EncryptSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,EncryptBuff,EncryptSize);
				//Optimized for compression only
				#else//!WS_ENCRYPTED
				WSWriteBuff(sendBuffer,(char*)&CompBuffSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,CompBuff,CompBuffSize);
				#endif//WS_ENCRYPTED
				WSHRecord(pmod,&pmod->CgdPort[PortNo].Recording,pmod->CgdPort[PortNo].OutBuffer.Block,CompSize,true);
				WSStripBuff(&pmod->CgdPort[PortNo].OutBuffer,CompSize);
				WSHReleaseMutex(0x01);
			}
		}
	}
#endif//WS_COMPRESS
	// Send as much as possible or up to the choke limit
	while((sendBuffer->Size>0)&&((flush)||((int)sendBuffer->Size>=pmod->CgdPort[PortNo].ImmediateSendLimit)))
	{
		int size=sendBuffer->LocalSize;
		if((pmod->CgdPort[PortNo].ChokeSize>0)&&(pmod->CgdPort[PortNo].ChokeSize -(int)pmod->CgdPort[PortNo].LastChokeSize<size))
		{
			size=(pmod->CgdPort[PortNo].ChokeSize-pmod->CgdPort[PortNo].LastChokeSize);
			if(size<=0)
				break;
		}
		if(size>pmod->CgdPort[PortNo].BlockSize*1024)
			size=pmod->CgdPort[PortNo].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->CgdPort[PortNo].Sock;
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=WSHSendNonBlock(pmod,WS_CGD,PortNo,(SOCKET_T)pport,sendBuffer->Block,size);
		#else
		int rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_CGDE_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->CgdPort[PortNo].SendTimeOut)
				{
					pmod->CgdPort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->CgdPort[PortNo].SendTimeOut<WS_CON_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: CGDE%d [%s] Reset by Peer",i,pmod->CgdPort[PortNo].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: CGDE%d [%s] Send Failed: %d",i,pmod->CgdPort[PortNo].CfgNote,lerr);
				WSHCloseCgdPort(pmod,i);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		if(!compressed)
			WSHRecord(pmod,&pmod->CgdPort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->CgdPort[PortNo].BytesOut+=rc;
		pmod->CgdPort[PortNo].BlocksOut++;
		pmod->CgdPort[PortNo].SendTimeOut=0;
		pmod->CgdPort[PortNo].LastChokeSize+=rc;
		nsent+=rc;
	}
	pmod->CgdPort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}
int WsocksHostImpl::WSHUgrSend(WsocksApp *pmod, int PortNo, bool flush)
{
	int nsent=0;
	if(!pmod->UgrPort[PortNo].SockActive)
		return -1;

	bool compressed=false;
	int lastBusy=pmod->UgrPort[PortNo].OutBuffer.Busy;
	pmod->UgrPort[PortNo].OutBuffer.Busy=pmod->UgrPort[PortNo].sendThread;
	pmod->UgrPort[PortNo].SendTimeOut=0;
	BUFFER *sendBuffer=&pmod->UgrPort[PortNo].OutBuffer;
#ifdef WS_COMPRESS
	if(pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].Compressed)
	{
		compressed=true;
		sendBuffer=&pmod->UgrPort[PortNo].OutCompBuffer;
		// Compress as much as possible to fill one send block
		while((pmod->UgrPort[PortNo].OutBuffer.Size>0)&&
			  ((flush)||((int)pmod->UgrPort[PortNo].OutBuffer.Size>=pmod->UgrPort[PortNo].ImmediateSendLimit)))
		{
			unsigned int CompSize=pmod->UgrPort[PortNo].OutBuffer.LocalSize;
			if(CompSize>((WS_COMP_BLOCK_LEN*1024)*99/100))
				CompSize=((WS_COMP_BLOCK_LEN*1024)*99/100);
			if(CompSize>0)
			{
				WSHWaitMutex(0x01,INFINITE);
				char *CompBuff=pmod->SyncLoop_CompBuff;
				unsigned int CompBuffSize=sizeof(pmod->SyncLoop_CompBuff);
				mtcompress(CompBuff,&CompBuffSize,pmod->UgrPort[PortNo].OutBuffer.Block,CompSize);
				// Compression +encryption
				#ifdef WS_ENCRYPTED
				char *EncryptBuff=CompBuff;
				unsigned int EncryptSize=CompBuffSize;
				if(pmod->UgrPort[PortNo].EncryptionOn)
				{
					EncryptBuff=pmod->SyncLoop_EncryptBuff;
					EncryptSize=sizeof(pmod->SyncLoop_EncryptBuff);
					WSHEncrypt(pmod,EncryptBuff,&EncryptSize,CompBuff,CompBuffSize,PortNo,WS_UGR);
				}
				WSWriteBuff(sendBuffer,(char*)&EncryptSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,EncryptBuff,EncryptSize);
				// Compression only
				#else//!WS_ENCRYPTED
				WSWriteBuff(sendBuffer,(char*)&CompBuffSize,sizeof(unsigned int));
				WSWriteBuff(sendBuffer,CompBuff,CompBuffSize);
				#endif//WS_ENCRYPTED
				WSHRecord(pmod,&pmod->UgrPort[PortNo].Recording,pmod->UgrPort[PortNo].OutBuffer.Block,CompSize,true);
				WSStripBuff(&pmod->UgrPort[PortNo].OutBuffer,CompSize);
				WSHReleaseMutex(0x01);
			}
		}
	}
#endif//WS_COMPRESS
	// Send as much as possible
	while((sendBuffer->Size>0)&&((flush)||(compressed)||((int)sendBuffer->Size>=pmod->UgrPort[PortNo].ImmediateSendLimit)))
	{
		int size=sendBuffer->LocalSize;
		if(size>pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].BlockSize*1024)
			size=pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].BlockSize*1024;
		WSPort *pport=(WSPort*)pmod->UgrPort[PortNo].Sock;
		#if defined(WS_OIO)||defined(WS_OTPP)
		int rc=WSHSendNonBlock(pmod,WS_UGR,PortNo,(SOCKET_T)pport,sendBuffer->Block,size);
		#else
		int rc=WSHSendPort(pport,sendBuffer->Block,size,0);
		#endif
		if(rc<0)
		{
			int lerr=WSAGetLastError();
			// Give up after WS_UGR_TIMEOUT fragmented sends
			#ifndef WS_OIO
			if(lerr==WSAEWOULDBLOCK)
			{
				DWORD tnow=GetTickCount();
				if(!pmod->UgrPort[PortNo].SendTimeOut)
				{
					pmod->UgrPort[PortNo].SendTimeOut=tnow;
					lerr=WSAEWOULDBLOCK;
				}
				else if(tnow -pmod->UgrPort[PortNo].SendTimeOut<WS_USR_TIMEOUT)
					lerr=WSAEWOULDBLOCK;
				else
					lerr=WSAECONNRESET;
			}
			#endif
			// Send failure
			if((lerr!=WSAEWOULDBLOCK)&&(lerr!=WSAENOBUFS))
			{
				if(lerr==WSAECONNRESET)
					WSHLogEvent(pmod,"!WSOCKS: UGR%d [%s] Reset by Peer",PortNo,pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].CfgNote);
				else
					WSHLogEvent(pmod,"!WSOCKS: UGR%d [%s] Send Failed: %d",PortNo,pmod->UgcPort[pmod->UgrPort[PortNo].UgcPort].CfgNote,lerr);
				WSHCloseUgrPort(pmod,PortNo);
				return -1;
			}
			//if(flush)
			//{
			//	SleepEx(10,true);
			//	continue;
			//}
			//else
				pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
				return 0;
		}
		if(!compressed)
			WSHRecord(pmod,&pmod->UgrPort[PortNo].Recording,sendBuffer->Block,rc,true);
		if(!WSStripBuff(sendBuffer,rc))
		{
			pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
			return -1;
		}
		pmod->UgrPort[PortNo].BytesOut+=rc;
		pmod->UgrPort[PortNo].BlocksOut++;
		pmod->UgrPort[PortNo].SendTimeOut=0;
		nsent+=rc;
	}
	pmod->UgrPort[PortNo].OutBuffer.Busy=lastBusy;
	return nsent;
}

int WsocksHostImpl::WSHCgdMsgReady(WsocksApp *pmod, int PortNo)
{
	MSGHEADER MsgHeader;

	if(pmod->CgdPort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
	{
		return(0);
	}
	memcpy((char *)&MsgHeader,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(pmod->CgdPort[PortNo].InBuffer.Size<(sizeof(MSGHEADER)+MsgHeader.MsgLen))
	{
		return(0);
	}
	return(1);
}
int WsocksHostImpl::WSHCgdStripMsg(WsocksApp *pmod, int PortNo)
{
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;
	MSGHEADER *MsgHeader;
	char *Msg;
    GDID GDId;
    GDLOGIN GDLogin;

	memcpy((char *)&GDMsgHeader,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,sizeof(MSGHEADER));
	memcpy((char *)&GDHeader,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(GDHEADER));
	WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,sizeof(GDHEADER));
    pmod->CgdPort[PortNo].PacketsIn++;

	switch(GDHeader.Type)
	{
		case GD_TYPE_DATA:
			MsgHeader=new MSGHEADER;
			memcpy((char *)MsgHeader,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
			WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,sizeof(MSGHEADER));
			Msg=new char[MsgHeader->MsgLen];
			memcpy(Msg,pmod->CgdPort[PortNo].InBuffer.Block,MsgHeader->MsgLen);
			WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,MsgHeader->MsgLen);
			if(pmod->CgdPort[PortNo].NextGDInLogId==GDHeader.GDLogId)
			{
				pmod->WSTranslateCgdMsg(MsgHeader,Msg,PortNo);
				if(!WSHWriteCgdInLog(pmod,MsgHeader->MsgID,MsgHeader->MsgLen,Msg,PortNo))
                {
                    delete Msg;
                    delete MsgHeader;
					return(0);
                }
			}
            delete Msg;
            delete MsgHeader;
			break;
        case GD_TYPE_NOGD:
			MsgHeader=new MSGHEADER;
			memcpy((char *)MsgHeader,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
            WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,sizeof(MSGHEADER));
			Msg=new char[MsgHeader->MsgLen];
			memcpy(Msg,pmod->CgdPort[PortNo].InBuffer.Block,MsgHeader->MsgLen);
			WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,MsgHeader->MsgLen);
			pmod->WSTranslateCgdMsg(MsgHeader,Msg,PortNo);
            delete Msg;
            delete MsgHeader;
            break;
		case GD_TYPE_LOGIN:
            memcpy(&GDId,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(GDID));
	        WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,sizeof(GDID));
            memcpy(&GDLogin,pmod->CgdPort[PortNo].InBuffer.Block,sizeof(GDLOGIN));
	        WSStripBuff(&pmod->CgdPort[PortNo].InBuffer,sizeof(GDLOGIN));
			if(GDLogin.LastRecvId<pmod->CgdPort[PortNo].NextGDOutLogId)
			{
				pmod->CgdPort[PortNo].LastGDSendLogId=GDLogin.LastRecvId;
				pmod->CgdPort[PortNo].GDLoginAck=TRUE;
				WSHLogEvent(pmod,"Login on Cgd %d with %d.(%s)(%s)(%s). NextSend=%d, LastRecv=%d",
					PortNo,GDId.LineId,GDId.LineName,GDId.ClientId,GDId.SessionId,
					pmod->CgdPort[PortNo].NextGDOutLogId,pmod->CgdPort[PortNo].NextGDInLogId-1);
			}
			else
			{
				WSHLogError(pmod,"CGD%d got higher login seq %d, but expected < %d!",PortNo,GDLogin.LastRecvId,pmod->CgdPort[PortNo].NextGDOutLogId);
				pmod->CgdPort[PortNo].LastGDSendLogId=pmod->CgdPort[PortNo].NextGDOutLogId-1;
				pmod->CgdPort[PortNo].GDLoginAck=TRUE;
				WSHLogEvent(pmod,"Login on Cgd %d with %d.(%s)(%s)(%s). NextSend=%d, LastRecv=%d",
					PortNo,GDId.LineId,GDId.LineName,GDId.ClientId,GDId.SessionId,
					pmod->CgdPort[PortNo].NextGDOutLogId,pmod->CgdPort[PortNo].NextGDInLogId-1);
			}
			break;
        case GD_TYPE_GAP:
			if(pmod->CgdPort[PortNo].NextGDInLogId==GDHeader.GDLogId)
			{
				if(!WSHWriteCgdInLog(pmod,0,0,0,PortNo))
					return(0);
			}
            break;
	}
	return(0);
}

int WsocksHostImpl::WSHUgrMsgReady(WsocksApp *pmod, int PortNo)
{
	MSGHEADER MsgHeader;

	if(pmod->UgrPort[PortNo].InBuffer.Size<sizeof(MSGHEADER))
	{
		return(0);
	}
	memcpy((char *)&MsgHeader,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	if(pmod->UgrPort[PortNo].InBuffer.Size<(sizeof(MSGHEADER)+MsgHeader.MsgLen))
	{
		return(0);
	}

#ifdef WS_UGRMSGREADY_LIMIT
	static DWORD lastReadyStart=0;
	DWORD tlast=lastReadyStart;
	lastReadyStart=0;

	DWORD tnow=GetTickCount();
	if((tlast)&&(tnow -tlast>=WS_UGRMSGREADY_LIMIT))
		return(0);
	lastReadyStart=tlast?tlast:tnow;
#endif
	return(1);
}
int WsocksHostImpl::WSHUgrStripMsg(WsocksApp *pmod, int PortNo)
{
	MSGHEADER GDMsgHeader;
	GDHEADER GDHeader;
	MSGHEADER *MsgHeader;
	char *Msg;
    GDID GDId;
    GDLOGIN GDLogin;

	memcpy((char *)&GDMsgHeader,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
	WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,sizeof(MSGHEADER));
	memcpy((char *)&GDHeader,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(GDHEADER));
	WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,sizeof(GDHEADER));
    pmod->UgrPort[PortNo].PacketsIn++;

	switch(GDHeader.Type)
	{
		case GD_TYPE_DATA:
			MsgHeader=new MSGHEADER;
			memcpy((char *)MsgHeader,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
            WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,sizeof(MSGHEADER));
			Msg=new char[MsgHeader->MsgLen];
			memcpy(Msg,pmod->UgrPort[PortNo].InBuffer.Block,MsgHeader->MsgLen);
			WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,MsgHeader->MsgLen);
			if(pmod->UgrPort[PortNo].NextGDInLogId==GDHeader.GDLogId)
			{
				pmod->WSTranslateUgrMsg(MsgHeader,Msg,PortNo,GDHeader.GDLineId);
				if(!WSHWriteUgrInLog(pmod,MsgHeader->MsgID,MsgHeader->MsgLen,Msg,PortNo))
                {
					delete Msg;
					delete MsgHeader;
					return(0);
                }
			}
			else if(pmod->UgrPort[PortNo].NextGDInLogId<GDHeader.GDLogId)
			{
				WSHLogError(pmod,"UGR%d got higher seq %d, but expected %d!",PortNo,GDHeader.GDLogId,pmod->UgrPort[PortNo].NextGDInLogId);
				WSHCloseUgrPort(pmod,PortNo);
			}
            delete Msg;
            delete MsgHeader;
			break;
		case GD_TYPE_LOGIN:
            memcpy(&GDId,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(GDID));
	        WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,sizeof(GDID));
            memcpy(&GDLogin,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(GDLOGIN));
	        WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,sizeof(GDLOGIN));
			pmod->UgrPort[PortNo].NextGDOutLogId=GDLogin.LastRecvId +1;
		    if (!WSHUgrAuthorize(pmod,PortNo,&GDId))
			{
				sprintf(pmod->UgrPort[PortNo].Note,"%d,%s,%s %s",GDId.LineId,GDId.LineName,GDId.ClientId,GDId.SessionId);
                //WSCloseUgrPort(PortNo);
				pmod->UgrPort[PortNo].TimeTillClose=5;
				pmod->UgrPort[PortNo].SockActive=0;
				return(-1);
			}
			else
			{
				WSHUgrSendLoginReply(pmod,PortNo);
				pmod->WSUgrOpened(PortNo);
			}
            break;
        case GD_TYPE_NOGD:
			MsgHeader=new MSGHEADER;
			memcpy((char *)MsgHeader,pmod->UgrPort[PortNo].InBuffer.Block,sizeof(MSGHEADER));
            WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,sizeof(MSGHEADER));
			Msg=new char[MsgHeader->MsgLen];
			memcpy(Msg,pmod->UgrPort[PortNo].InBuffer.Block,MsgHeader->MsgLen);
			WSStripBuff(&pmod->UgrPort[PortNo].InBuffer,MsgHeader->MsgLen);
			pmod->WSTranslateUgrMsg(MsgHeader,Msg,PortNo,GDHeader.GDLineId);
            delete Msg;
            delete MsgHeader;
			break;
        case GD_TYPE_GAP:
			if(pmod->UgrPort[PortNo].NextGDInLogId==GDHeader.GDLogId)
			{
				if(!WSHWriteUgrInLog(pmod,0,0,0,PortNo))
					return(0);
			}
            break;
	}
	return(0);
}
#endif //#ifdef WS_GUARANTEED
