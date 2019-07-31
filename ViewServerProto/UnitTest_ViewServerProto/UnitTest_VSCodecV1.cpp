#include "stdafx.h"

#include "UnitTest_VSCodecV1.h"

// Codec testing
TEST(UT_VSCodecV1, Init_Successfull)
{
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;

	int actual=codec.Init(&mnotify);

	ASSERT_EQ(0, actual);
}
// We know the encoder works if the decoder can get the same values
TEST(UT_VSCodecV1, EncodeLoginRequest_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeLoginRequest(mptr,sizeof(mbuf),1,"user","pass");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyLoginRequest(Eq((void*)-1),1,StrEq("user"),_,16))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction
}
TEST(UT_VSCodecV1, EncodeSqlRequest_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeSqlRequest(mptr,sizeof(mbuf),2,"*","ORDERS","AppInstID=TWIST*",1000,true,false,1);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifySqlRequest(Eq((void*)-1),2,StrEq("*"),StrEq("ORDERS"),StrEq("AppInstID=TWIST*"),1000,true,false,1))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction
}
TEST(UT_VSCodecV1, EncodeSqlIndexRequest_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeSqlIndexRequest(mptr,sizeof(mbuf),3,"*","Symbols","AppInstID=TWIST*",1000,false,500,+1,1);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifySqlIndexRequest(Eq((void*)-1),3,StrEq("*"),StrEq("Symbols"),StrEq("AppInstID=TWIST*"),1000,false,500,+1,1))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction
}
TEST(UT_VSCodecV1, EncodeCancelRequest_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeCancelRequest(mptr,sizeof(mbuf),4,1);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyCancelRequest(Eq((void*)-1),4,1))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction
}
TEST(UT_VSCodecV1, EncodeDescribeRequest_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeDescribeRequest(mptr,sizeof(mbuf),5,"DETAILS",1000,1);
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyDescribeRequest(Eq((void*)-1),5,StrEq("DETAILS"),1000,1))
		.Times(1);
	codec.DecodeRequest(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction
}
TEST(UT_VSCodecV1, EncodeLoginReply_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	codec.EncodeLoginReply(mptr,sizeof(mbuf),1,6,"Authentication failure");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyLoginReply(Eq((void*)-1),1,6,StrEq("Authentication failure")))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction
}
TEST(UT_VSCodecV1, EncodeSqlFixReply_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	FIXINFO fixmsgs[2];
	memset(&fixmsgs,0,sizeof(fixmsgs));
	fixmsgs[0].FIXDELIM='|';
	fixmsgs[1].FIXDELIM='|';
	char *msg1="8=FIX.4.2|9=277|35=8|49=REDIRPT|56=IQDROP2|50=r145889|143=MLCS DMA|34=76377|52=20140807-19:57:16|37=178129682|11=20140807YB29682|109=AAE00343|17=17812968221914|20=0|150=0|39=0|1=RT00501720|55=AAPL|54=2|38=1000|40=1|59=0|47=A|32=0|31=0|151=1000|14=0|6=0.0000|60=20140807-19:57:16|77=C|100=MLCS|10=179|";
	char *msg2="8=FIX.4.2|9=325|35=8|49=REDIRPT|56=IQDROP2|128=REDI|50=r145889|34=76378|52=20140807-19:57:16|37=178129682|11=20140807YB29682|109=RT00501720|76=MLCO|17=91779577921914|20=0|150=1|39=1|1=RT00501720|55=AAPL|54=2|38=1000|40=1|15=USD|59=0|47=A|32=300|31=94.400000|30=I|29=1|151=700|14=300|6=94.4000|60=20140807-19:57:16|113=N|77=C|439=REDI|8023=R|10=083|";
	fixmsgs[0].FixMsgReady(msg1,(int)strlen(msg1));
	fixmsgs[1].FixMsgReady(msg2,(int)strlen(msg2));
	int nfix=2;
	codec.EncodeSqlFixReply(mptr,sizeof(mbuf),1,7,2,118,true,0x01,fixmsgs,nfix,true,1,"reason");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,mVSCNotifySqlFixReply(
		AllOf(Field(&mockVSCNotifyV1::mREPLYHEADER::udata,Eq((void*)-1)),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::rc,1),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::rid,7),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::endcnt,2),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::totcnt,118)),
		true,0x01,_,2,true,1,StrEq("reason")))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction	
}
TEST(UT_VSCodecV1, EncodeSqlDsvReply_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	const char *csvmsgs[3]={
		"AppInstID,ClOrdID,RootOrderID,FirstClOrdID,Symbol,Account,EcnOrderID,ClientID,Price,Side,OrderQty,CumQty,FillQty,Term,HighMsgType,HighExecType,TransactTime,OrderLoc,Connection,ClParentOrderID,OrderDate,RoutingBroker,SecurityType,",
		"TRIADBNP,20140807AAPL2NAW02760,,20140807AAPL2NAW02760,AAPL,RT00500895,,R145426,94.524,SL,500,500,500,TRUE,,2,20140807-155703,15313444,IQDROP2.REDIRPT,,20140807,AWAY,,",
		"TRIADBNP,20140807AAPL1NAW02759,,20140807AAPL1NAW02759,AAPL,RT00500895,,R145426,94.72,B,500,500,500,TRUE,,2,20140807-155703,15312060,IQDROP2.REDIRPT,,20140807,AWAY,,"};
	int ncsv=3;
	codec.EncodeSqlDsvReply(mptr,sizeof(mbuf),1,8,3,10,true,0x01,csvmsgs,ncsv,true,1,"reason");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,mVSCNotifySqlDsvReply(
		AllOf(Field(&mockVSCNotifyV1::mREPLYHEADER::udata,Eq((void*)-1)),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::rc,1),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::rid,8),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::endcnt,3),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::totcnt,10)),
		true,0x01,_,3,true,1,StrEq("reason")))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction	
}
TEST(UT_VSCodecV1, EncodeSqlIndexReply_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	const char *csvmsgs[5]={
		"AppSystem,Key,Pos,OrderLoc,",
		"TWIST,AAME,1,367AE80,",
		"TWIST,AAPL,13,367AE80,",
		"TWIST,AAPL  140808C00095000,38,367AE80,",
		"TWIST,AAPL  140808C00096000,44,367AE80,"};
	int ncsv=5;
	codec.EncodeSqlIndexReply(mptr,sizeof(mbuf),1,9,5,628,0x01,csvmsgs,ncsv,true,1,"reason");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,mVSCNotifySqlIndexReply(
		AllOf(Field(&mockVSCNotifyV1::mREPLYHEADER::udata,Eq((void*)-1)),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::rc,1),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::rid,9),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::endcnt,5),
			  Field(&mockVSCNotifyV1::mREPLYHEADER::totcnt,628)),
		0x01,_,5,true,1,StrEq("reason")))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction	
}
TEST(UT_VSCodecV1, EncodeDescribeReply_Decoded)
{
	// Arrange
	VSCodecV1 codec;
	MockVSCNotifyV1 mnotify;
	codec.Init(&mnotify);
	char mbuf[1024]={0},*mptr=mbuf;
	const char *pstr[]={"desc col 1","desc col 2",0};
	int nstr=2;
	codec.EncodeDescribeReply(mptr,sizeof(mbuf),0,10,2,5,pstr,nstr,true,1,"reason");
	const char *rptr=mbuf;
	int rlen=(int)(mptr -mbuf);
	void *udata=(void*)-1;

	// Act
	EXPECT_CALL(mnotify,VSCNotifyDescribeReply(Eq((void*)-1),0,10,2,5,_,2,true,1,StrEq("reason")))
		.Times(1);
	codec.DecodeReply(rptr,rlen,udata);

	// Assert done on MockVSCNotifyV1 destruction	
}
