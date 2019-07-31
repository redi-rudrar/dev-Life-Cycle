#include "stdafx.h"

#include "UnitTest_VSCodecV3.h"

// Codec testing
TEST(UT_VSCodecV3, Init_Successfull)
{
	VSCodec codec;
	MockVSCNotifyV3 mnotify;

	int actual=codec.Init(&mnotify);

	ASSERT_EQ(0, actual);
}
TEST(UT_VSCodecV3, EncodeDatRequest_Decoded)
{
	// Arrange
	VSCodec codec;
	MockVSCNotifyV3 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeDatRequest(mptr,sizeof(mbuf),2,"*","ORDERS","AppInstID=CLSERVER*",1000,true,false,1);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyDatRequest(Eq((void*)-1),2,StrEq("*"),StrEq("ORDERS"),StrEq("AppInstID=CLSERVER*"),1000,true,false,1))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV3 destruction
}
TEST(UT_VSCodecV3, EncodeDatReply_Decoded)
{
	// Arrange
	VSCodec codec;
	MockVSCNotifyV3 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	const char *datmsgs[2]={"generic string 1", "generic string 2"};
	int ndat=2;
	short slen[2]={(short)strlen(datmsgs[0]),(short)strlen(datmsgs[1])};
	codec.EncodeDatReply(mptr,sizeof(mbuf),1,8,2,6,true,0x01,slen,(const void**)datmsgs,ndat,true,1,"reason");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,mVSCNotifyDatReply(
		AllOf(Field(&mockVSCNotifyV3::mREPLYHEADER3::udata,Eq((void*)-1)),
			  Field(&mockVSCNotifyV3::mREPLYHEADER3::rc,1),
			  Field(&mockVSCNotifyV3::mREPLYHEADER3::rid,8),
			  Field(&mockVSCNotifyV3::mREPLYHEADER3::endcnt,2),
			  Field(&mockVSCNotifyV3::mREPLYHEADER3::totcnt,6)),
		true,0x01,_,_,2,true,1,StrEq("reason")))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV3 destruction	
}
