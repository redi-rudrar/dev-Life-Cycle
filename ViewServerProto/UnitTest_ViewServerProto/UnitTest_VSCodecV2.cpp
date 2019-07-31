#include "stdafx.h"

#include "UnitTest_VSCodecV2.h"

// Codec testing
TEST(UT_VSCodecV2, Init_Successfull)
{
	VSCodecV2 codec;
	MockVSCNotifyV2 mnotify;

	int actual=codec.Init(&mnotify);

	ASSERT_EQ(0, actual);
}
// We know the encoder works if the decoder can get the same values
TEST(UT_VSCodecV2, EncodeLoginRequest2_Decoded)
{
	// Arrange
	VSCodecV2 codec;
	MockVSCNotifyV2 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeLoginRequest2(mptr,sizeof(mbuf),1,"user","pass");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyLoginRequest2(Eq((void*)-1),1,StrEq("user"),_,16))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV2 destruction
}
TEST(UT_VSCodecV2, EncodeSqlRequest2_Decoded)
{
	// Arrange
	VSCodecV2 codec;
	MockVSCNotifyV2 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeSqlRequest2(mptr,sizeof(mbuf),2,"*","ORDERS","AppInstID=TWIST*",1000,true,false,1,3);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifySqlRequest2(Eq((void*)-1),2,StrEq("*"),StrEq("ORDERS"),StrEq("AppInstID=TWIST*"),1000,true,false,1,3))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV2 destruction
}
TEST(UT_VSCodecV2, EncodeSqlIndexRequest2_Decoded)
{
	// Arrange
	VSCodecV2 codec;
	MockVSCNotifyV2 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeSqlIndexRequest2(mptr,sizeof(mbuf),3,"*","Symbols","AppInstID=TWIST*",1000,false,500,+1,1,3);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,mVSCNotifySqlIndexRequest2(
		AllOf(Field(&mockVSCNotifyV2::mREPLYHEADER2::udata,Eq((void*)-1)),
			  Field(&mockVSCNotifyV2::mREPLYHEADER2::rid,3)),
		StrEq("*"),StrEq("Symbols"),StrEq("AppInstID=TWIST*"),1000,false,500,+1,1,3))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV2 destruction
}
TEST(UT_VSCodecV2, EncodeLoginReply2_Decoded)
{
	// Arrange
	VSCodecV2 codec;
	MockVSCNotifyV2 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeLoginReply2(mptr,sizeof(mbuf),1,6,"Authentication failure","VSDB_REDIOATS_BAO",20140808);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyLoginReply2(Eq((void*)-1),1,6,StrEq("Authentication failure"),StrEq("VSDB_REDIOATS_BAO"),20140808))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV2 destruction
}
