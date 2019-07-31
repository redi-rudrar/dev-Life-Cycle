
#include "stdafx.h"
#ifndef MAKE_STATIC
#include "wsocksapi.h"
#endif
#include "wsbuffer.h"

static void WSLogEvent(const char *Event, ... )
{
	_ASSERT(false);
}
static void WSLogError (const char *format, ... )
{
	_ASSERT(false);
}

int WSOpenBuff(BUFFER *Buffer,int BlockSize)
{
    if(BlockSize>WS_MAX_BLOCK_SIZE)
    {
		WSLogError("!WSOCKS: FATAL ERROR : Creating Buffer Size=%d, but WS_MAX_BLOCK_SIZE=%d",BlockSize,WS_MAX_BLOCK_SIZE);
        exit(0);
    }
	memset((char*)Buffer,0,sizeof(BUFFER));
	Buffer->BlockSize=(unsigned long)BlockSize;
	if (NULL==(Buffer->Block=(char *)calloc(Buffer->BlockSize*1024,2)))
	{
		WSLogError("!WSOCKS: FATAL ERROR : Creating Buffer");
		exit(0);
	}
    Buffer->BlockStart=Buffer->Block;
    Buffer->BlockOffset=0;
	return(TRUE);
}

int WSOpenBuffLimit(BUFFER *Buffer,int BlockSize,int MaxSize)
{
    if(BlockSize>WS_MAX_BLOCK_SIZE)
    {
		WSLogError("!WSOCKS: FATAL ERROR : Creating Buffer Size=%d, but WS_MAX_BLOCK_SIZE=%d",BlockSize,WS_MAX_BLOCK_SIZE);
        exit(0);
    }
	memset((char*)Buffer,0,sizeof(BUFFER));
	Buffer->BlockSize=(unsigned long)BlockSize;
	Buffer->MaxSize=MaxSize;
	if (NULL==(Buffer->Block=(char *)calloc(Buffer->BlockSize*1024,2)))
	{
		WSLogError("!WSOCKS: FATAL ERROR : Creating Buffer");
		exit(0);
	}
    Buffer->BlockStart=Buffer->Block;
    Buffer->BlockOffset=0;
	return(TRUE);
}

int WSResetBuffStart(BUFFER *Buffer)
{
    if(Buffer->BlockOffset)
    {
	    if (Buffer->LocalSize>0)
		    memmove(Buffer->BlockStart,Buffer->Block,Buffer->LocalSize);
        Buffer->Block=Buffer->BlockStart;
        Buffer->BlockOffset=0;
        return(1);
    }
    return(0);
}

int WSWriteBuff(BUFFER *Buffer,char *Source,int Length)
{
	unsigned long Offset=0;
	int RemainingLength=Length;
	BUFFBLOCK *TempBuffBlock;

#ifdef WS_WRITEBUFF_FULL
	if((Length <= 0)||(((unsigned long)Length)>Buffer->BlockSize*1024))
		return(FALSE);
#else
	if((Length <= 0)||(((unsigned long)Length)>=Buffer->BlockSize*1024))
		return(FALSE);
#endif
	// WsocksHost code will enforce the buffer size, but allow small amounts over the max
	//if((Buffer->MaxSize>0)&&(Buffer->Size +Length>(Buffer->MaxSize*1024)))
	//	return FALSE;
	if(Buffer->Block==NULL)
		return(FALSE);
	if(Buffer->TopBlock==NULL)
	{
        int Try=TRUE;
        while(Try)
        {
            Try=FALSE;
		    if(Buffer->LocalSize+RemainingLength>((Buffer->BlockSize*1024)*2-Buffer->BlockOffset))
		    {
                if(Buffer->BlockOffset)
                {
                    WSResetBuffStart(Buffer);
                    Try=TRUE;
                    continue;
                }
			    memcpy(&Buffer->Block[Buffer->LocalSize], &Source[Offset], ((Buffer->BlockSize*1024)*2)-Buffer->LocalSize);
			    RemainingLength-=((Buffer->BlockSize*1024)*2)-Buffer->LocalSize;
			    Offset+=((Buffer->BlockSize*1024)*2)-Buffer->LocalSize;
			    Buffer->LocalSize=((Buffer->BlockSize*1024)*2);
		    }
		    else
		    {
			    memcpy(&Buffer->Block[Buffer->LocalSize], &Source[Offset], RemainingLength);
			    Buffer->LocalSize+=RemainingLength;
			    RemainingLength=0;
		    }
        }
	}
	while(RemainingLength>0)
	{
		if(Buffer->BotBlock==NULL)
		{
			if (NULL==(Buffer->TopBlock=(BUFFBLOCK *)calloc(sizeof(BUFFBLOCK),1)))
			{
			WSLogError("!WSOCKS: FATAL ERROR : Extending Buffer");
			return(FALSE);
			}
			if (NULL==(Buffer->TopBlock->Block=(char *)calloc(Buffer->BlockSize*1024,1)))
			{
			WSLogError("!WSOCKS: FATAL ERROR : Extending Buffer");
			return(FALSE);
			}
			Buffer->BotBlock=Buffer->TopBlock;
		}
		if(Buffer->BotBlock->Size==(Buffer->BlockSize*1024))
		{
			if (NULL==(TempBuffBlock=(BUFFBLOCK *)calloc(sizeof(BUFFBLOCK),1)))
			{
			WSLogError("!WSOCKS: FATAL ERROR : Extending Buffer");
			return(FALSE);
			}
			if (NULL==(TempBuffBlock->Block=(char *)calloc(Buffer->BlockSize*1024,1)))
			{
			WSLogError("!WSOCKS: FATAL ERROR : Extending Buffer");
			return(FALSE);
			}
			Buffer->BotBlock->NextBuffBlock=TempBuffBlock;
			Buffer->BotBlock=TempBuffBlock;
		}
		if(Buffer->BotBlock->Size+RemainingLength>(Buffer->BlockSize*1024))
		{
			memcpy(&Buffer->BotBlock->Block[Buffer->BotBlock->Size], &Source[Offset], (Buffer->BlockSize*1024)-Buffer->BotBlock->Size);
			RemainingLength-=(Buffer->BlockSize*1024)-Buffer->BotBlock->Size;
			Offset+=(Buffer->BlockSize*1024)-Buffer->BotBlock->Size;
			Buffer->BotBlock->Size=(Buffer->BlockSize*1024);
		}
		else
		{
			memcpy(&Buffer->BotBlock->Block[Buffer->BotBlock->Size], &Source[Offset], RemainingLength);
			Buffer->BotBlock->Size+=RemainingLength;
			RemainingLength=0;
		}
	}
	Buffer->Size += Length;
	return(TRUE);
}

int WSStripBuff(BUFFER *Buffer,int Length)
{
	BUFFBLOCK *TempBuffBlock;

	//if((Length <= 0)||(((unsigned long)Length)>=Buffer->BlockSize*1024)||(((unsigned long)Length) > Buffer->LocalSize))
	//	return(FALSE);
	if((Length<=0)||(Length>(int)Buffer->LocalSize))
		return FALSE;
	else if(Length>(int)Buffer->BlockSize*1024)
	{
		while(Length>0)
		{
			if(Length>=(int)Buffer->BlockSize*1024)
			{
				if(!WSStripBuff(Buffer,(int)Buffer->BlockSize*1024))
					return FALSE;
				Length-=(int)Buffer->BlockSize*1024;
			}
			else
			{
				if(!WSStripBuff(Buffer,Length))
					return FALSE;
				Length=0;
			}
		}
		return TRUE;
	}

	Buffer->Size -= Length;
	Buffer->LocalSize -= Length;
	if (Buffer->LocalSize>0)
    {
        Buffer->Block=&Buffer->Block[Length];
        Buffer->BlockOffset+=Length;
        if(Buffer->BlockOffset>=(Buffer->BlockSize*1024))
           WSResetBuffStart(Buffer);
    }
    else 
        WSResetBuffStart(Buffer);

	if((Buffer->TopBlock!=NULL)&&(Buffer->LocalSize<(Buffer->BlockSize*1024)))
	{
        WSResetBuffStart(Buffer);
		memcpy(&Buffer->Block[Buffer->LocalSize], Buffer->TopBlock->Block, Buffer->TopBlock->Size);
		Buffer->LocalSize+=Buffer->TopBlock->Size;
		TempBuffBlock=Buffer->TopBlock;
		Buffer->TopBlock=(BUFFBLOCK *)TempBuffBlock->NextBuffBlock;
		if(Buffer->TopBlock==NULL)
			Buffer->BotBlock=NULL;
		free(TempBuffBlock->Block);
		free(TempBuffBlock);
	}
	return(TRUE);
}

int WSCloseBuff(BUFFER *Buffer)
{
	BUFFBLOCK *TempBuffBlock;

	if(Buffer==NULL)
		return(FALSE);
	if(Buffer->Block==NULL)
		return(FALSE);
    free(Buffer->BlockStart);
    Buffer->BlockStart=NULL;
	Buffer->Block=NULL;
	Buffer->LocalSize=0;
	Buffer->Size=0;
    Buffer->BlockOffset=0;
	while(Buffer->TopBlock!=NULL)
	{
		TempBuffBlock=Buffer->TopBlock;
		Buffer->TopBlock=(BUFFBLOCK *)TempBuffBlock->NextBuffBlock;
		free(TempBuffBlock->Block);
		free(TempBuffBlock);
	}
	Buffer->BotBlock=NULL;
	return(TRUE);
}
