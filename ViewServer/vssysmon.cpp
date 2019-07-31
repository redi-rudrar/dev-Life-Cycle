
#include "stdafx.h"
#include "vsdefs.h"
#include "ViewServer.h"
#include "vsquery.h"
#include "wstring.h"
#ifdef WIN32
#include <shlwapi.h>
#endif
#ifdef TEST_HASH_DIST
#include <math.h>
#endif
#include <time.h>

// SysmonConsole commands
int ViewServer::WSSysmonHelp(int UmrPortNo, int reqid, string cmd, TVMAP& tvmap, string& oboi, string& dtid, TVMAP& rvmap)
{
	char parms[2048]={0};

	// Begin help
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"Begin","Y");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// getsetupini
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","getsetupini");
	WSSetValue(rvmap,"Desc","download setup.ini file");
	WSSetValue(rvmap,"NumParms","1");
		// FileKey
		WSSetValue(rvmap,"Parm1","FileKey");
		WSSetValue(rvmap,"ParmDef1","setup.ini");
		WSSetValue(rvmap,"ParmOpt1","N");
		WSSetValue(rvmap,"ParmDesc1","local file name");
	WSSetValue(rvmap,"NumSamples","1");
	WSSetValue(rvmap,"Sample1","getsetupini FileKey=setup.ini");
	WSSetValue(rvmap,"SampleDesc1","normal (default)");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// putsetupini
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","putsetupini");
	WSSetValue(rvmap,"Desc","upload setup.ini file");
	WSSetValue(rvmap,"NumParms","1");
		// SrcPath
		WSSetValue(rvmap,"Parm1","SrcPath");
		#ifdef WIN32
		WSSetValue(rvmap,"ParmDef1","SysmonTempFiles\\setup.ini");
		#else
		WSSetValue(rvmap,"ParmDef1","SysmonTempFiles/setup.ini");
		#endif
		WSSetValue(rvmap,"ParmOpt1","N");
		WSSetValue(rvmap,"ParmDesc1","local file path");
	WSSetValue(rvmap,"NumSamples","1");
	#ifdef WIN32
	WSSetValue(rvmap,"Sample1","putsetupini SrcPath=SysmonTempFiles\\\\setup.ini");
	#else
	WSSetValue(rvmap,"Sample1","putsetupini SrcPath=SysmonTempFiles/setup.ini");
	#endif
	WSSetValue(rvmap,"SampleDesc1","normal (default)");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// reloadsetupini
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","reloadsetupini");
	WSSetValue(rvmap,"Desc","reloads setup.ini");
	WSSetValue(rvmap,"NumParms","0");
	WSSetValue(rvmap,"NumSamples","1");
	WSSetValue(rvmap,"Sample1","reloadsetupini");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// reloadconfig
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","reloadconfig");
	WSSetValue(rvmap,"Desc","reloads all configuration files");
	WSSetValue(rvmap,"NumParms","0");
	WSSetValue(rvmap,"NumSamples","1");
	WSSetValue(rvmap,"Sample1","reloadconfig");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// restart
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","restart");
	WSSetValue(rvmap,"Desc","restart iqmatrix");
	WSSetValue(rvmap,"NumParms","2");
		// User
		WSSetValue(rvmap,"Parm1","User");
		WSSetValue(rvmap,"ParmDef1","");
		WSSetValue(rvmap,"ParmOpt1","N");
		WSSetValue(rvmap,"ParmDesc1","your username");
		// Pass
		WSSetValue(rvmap,"Parm2","Reason");
		WSSetValue(rvmap,"ParmDef2","");
		WSSetValue(rvmap,"ParmOpt2","N");
		WSSetValue(rvmap,"ParmDesc2","Description");
	WSSetValue(rvmap,"NumSamples","1");
	WSSetValue(rvmap,"Sample1","restart User=<user>/Reason=<desc>");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// dropusers
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","dropusers");
	WSSetValue(rvmap,"Desc","disconnects all users");
	WSSetValue(rvmap,"NumParms","0");
	WSSetValue(rvmap,"NumSamples","1");
	WSSetValue(rvmap,"Sample1","dropusers");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	// filereport
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"HelpCmd","filereport");
	WSSetValue(rvmap,"Desc","loaded file info");
	WSSetValue(rvmap,"NumParms","1");
		// FileKey
		WSSetValue(rvmap,"Parm1","FileKey");
		WSSetValue(rvmap,"ParmDef1","filereport.csv");
		WSSetValue(rvmap,"ParmOpt1","N");
		WSSetValue(rvmap,"ParmDesc1","local file name");
	WSSetValue(rvmap,"NumSamples","1");
	WSSetValue(rvmap,"Sample1","fileinfo FileKey=filereport.csv");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	if(VSDIST_HUB)
	{
		// ordermap
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","ordermap");
		WSSetValue(rvmap,"Desc","export ordermap details (long op)");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","ordermap.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// NoPrompt
			WSSetValue(rvmap,"Parm2","NoPrompt");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","required when remote command");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","ordermap FileKey=ordermap.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// parentmap
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","parentmap");
		WSSetValue(rvmap,"Desc","export parent details (long op)");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","parentmap.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// NoPrompt
			WSSetValue(rvmap,"Parm2","NoPrompt");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","required when remote command");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","parentmap FileKey=parentmap.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// vieworder
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","vieworder");
		WSSetValue(rvmap,"Desc","show order details");
		WSSetValue(rvmap,"NumParms","4");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","order.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// AppSystem
			WSSetValue(rvmap,"Parm2","AppSystem");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","");
			// ClOrdID
			WSSetValue(rvmap,"Parm3","ClOrdID");
			WSSetValue(rvmap,"ParmDef3","");
			WSSetValue(rvmap,"ParmOpt3","N");
			WSSetValue(rvmap,"ParmDesc3","");
			// OrderDate
			WSSetValue(rvmap,"Parm4","OrderDate");
			WSSetValue(rvmap,"ParmDef4","");
			WSSetValue(rvmap,"ParmOpt4","Y");
			WSSetValue(rvmap,"ParmDesc4","yyyymmdd");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","vieworder FileKey=order.csv/AppSystem=/ClOrdID=");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// usertask
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","usertask");
		WSSetValue(rvmap,"Desc","export task details");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","usertask.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","usertask FileKey=usertask.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// userlist
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","userlist");
		WSSetValue(rvmap,"Desc","user list");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","userlist.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","userlist FileKey=userlist.txt");
		WSSetValue(rvmap,"SampleDesc1","all users");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	#ifdef _DEBUG
		// lex
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","lex");
		WSSetValue(rvmap,"Desc","export task details (debug only)");
		WSSetValue(rvmap,"NumParms","2");
			// Expr
			WSSetValue(rvmap,"Parm1","Expr");
			WSSetValue(rvmap,"ParmDef1","tag(32)>0");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","expression");
			// FixMsg
			WSSetValue(rvmap,"Parm2","FixMsg");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","defaults to internal test message");
		WSSetValue(rvmap,"NumSamples","11");
		WSSetValue(rvmap,"Sample1","lex Expr=tag(32)>0");
		WSSetValue(rvmap,"SampleDesc1","fills");
		WSSetValue(rvmap,"Sample2","lex Expr=tag(150)==4 OR tag(39)==4");
		WSSetValue(rvmap,"SampleDesc2","cancelled orders");
		WSSetValue(rvmap,"Sample3","lex Expr=tag(150)==0");
		WSSetValue(rvmap,"SampleDesc3","confirmed orders");
		WSSetValue(rvmap,"Sample4","lex Expr=tag(35)=='D' AND (tag(40)==1 OR tag(44)==NULL OR tag(44)==0)");
		WSSetValue(rvmap,"SampleDesc4","market orders");
		WSSetValue(rvmap,"Sample5","lex Expr=tag(35)=='D' AND tag(38)>1000");
		WSSetValue(rvmap,"SampleDesc5","orders with qty>1000");
		WSSetValue(rvmap,"Sample6","lex Expr=substr(tag(60),0,8)");
		WSSetValue(rvmap,"SampleDesc6","sub-string");
		WSSetValue(rvmap,"Sample7","lex Expr=strlen(tag(11))");
		WSSetValue(rvmap,"SampleDesc7","string length");
		WSSetValue(rvmap,"Sample8","lex Expr=strstr(tag(11),'467')");
		WSSetValue(rvmap,"SampleDesc8","case insensitive string search");
		WSSetValue(rvmap,"Sample9","lex Expr=time(tag(60))");
		WSSetValue(rvmap,"SampleDesc9","timestamp type");
		WSSetValue(rvmap,"Sample10","lex Expr=time(0) -time(tag(60))");
		WSSetValue(rvmap,"SampleDesc10","time difference");
		WSSetValue(rvmap,"Sample11","lex Expr=time(0) +60");
		WSSetValue(rvmap,"SampleDesc11","time addition");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// sqlquery
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","sqlquery");
		WSSetValue(rvmap,"Desc","SQL-like query with any combination of tags (debug only)");
		WSSetValue(rvmap,"NumParms","5");
			// Select
			WSSetValue(rvmap,"Parm1","Select");
			WSSetValue(rvmap,"ParmDef1","11505,11,41,5055,70129,55,31,32,14,151");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","* or tag # list");
			// From
			WSSetValue(rvmap,"Parm2","From");
			WSSetValue(rvmap,"ParmDef2","details");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","orders/details virtual table");		 
			// Where
			WSSetValue(rvmap,"Parm3","Where");
			WSSetValue(rvmap,"ParmDef3","tag(32)>0"); // fills only
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","tag(#) <operator> value");
			// Recurse
			WSSetValue(rvmap,"Parm4","Recurse");
			WSSetValue(rvmap,"ParmDef4","Y");
			WSSetValue(rvmap,"ParmOpt4","Y");
			WSSetValue(rvmap,"ParmDesc4","Y/N");
			// FileKey
			WSSetValue(rvmap,"Parm5","FileKey");
			WSSetValue(rvmap,"ParmDef5","sqlquery.txt");
			WSSetValue(rvmap,"ParmOpt5","N");
			WSSetValue(rvmap,"ParmDesc5","local file name");
		WSSetValue(rvmap,"NumSamples","5");
		WSSetValue(rvmap,"Sample1","sqlquery FileKey=sqlquery.txt/Select=11,55,65,32,31/From=details/Where=tag(32)>0");
		WSSetValue(rvmap,"SampleDesc1","fills");
		//WSSetValue(rvmap,"Sample2","sqlquery FileKey=sqlquery.txt/Select=11,55,65,44,38,150,39/From=details/Where=tag(150)==4//tag(39)==4");
		WSSetValue(rvmap,"Sample2","sqlquery FileKey=sqlquery.txt/Select=11,55,65,44,38,150,39/From=details/Where=tag(150)==4 OR tag(39)==4");
		WSSetValue(rvmap,"SampleDesc2","cancelled orders");
		WSSetValue(rvmap,"Sample3","sqlquery FileKey=sqlquery.txt/Select=11,55,65,44,38/From=details/Where=tag(150)==0");
		WSSetValue(rvmap,"SampleDesc3","confirmed orders");
		//WSSetValue(rvmap,"Sample4","sqlquery FileKey=sqlquery.txt/Select=11,40,55,65,44,38/From=details/Where=tag(35)==D&&(tag(40)==1//tag(44)==NULL//tag(44)==0)");
		WSSetValue(rvmap,"Sample4","sqlquery FileKey=sqlquery.txt/Select=11,40,55,65,44,38/From=details/Where=tag(35)=='D' AND (tag(40)==1 OR tag(44)==NULL OR tag(44)==0)");
		WSSetValue(rvmap,"SampleDesc4","market orders");
		//WSSetValue(rvmap,"Sample5","sqlquery FileKey=sqlquery.txt/Select=11,55,65,44,38/From=details/Where=tag(35)==D&&tag(38)>1000");
		WSSetValue(rvmap,"Sample5","sqlquery FileKey=sqlquery.txt/Select=11,55,65,44,38/From=details/Where=tag(35)=='D' AND tag(38)>1000");
		WSSetValue(rvmap,"SampleDesc5","orders with qty>1000");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	#endif//_DEBUG

	#ifdef TEST_ALL_ORDERIDS
		// dbgloadoids
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","dbgloadoids");
		WSSetValue(rvmap,"Desc","test full ordermap (debug only)");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","dbgloadoids.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","dbgloadoids FileKey=dbgloadoids.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	#endif

		// histogram
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","histogram");
		WSSetValue(rvmap,"Desc","orderid histogram (long op)");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","histogram.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// NoPrompt
			WSSetValue(rvmap,"Parm2","NoPrompt");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","required when remote command");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","histogram FileKey=histogram.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// indices
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","indices");
		WSSetValue(rvmap,"Desc","show indices (long op)");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","indices.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// NoPrompt
			WSSetValue(rvmap,"Parm2","NoPrompt");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","required when remote command");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","indices FileKey=indices.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// openorders
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","openorders");
		WSSetValue(rvmap,"Desc","show openorders (long op)");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","openorders.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// NoPrompt
			WSSetValue(rvmap,"Parm2","NoPrompt");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","required when remote command");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","openorders FileKey=openorders.csv");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// fills
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","fills");
		WSSetValue(rvmap,"Desc","show fills (long op)");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","fills.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// NoPrompt
			WSSetValue(rvmap,"Parm2","NoPrompt");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","required when remote command");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","fills FileKey=fills.csv");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	#ifdef MULTI_HASH
		// multihash
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","multihash");
		WSSetValue(rvmap,"Desc","show multihash stats");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","multihash.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","multihash FileKey=multihash.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	#endif

		// instances
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","instances");
		WSSetValue(rvmap,"Desc","show systems and instances");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","instances.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","instances FileKey=instances.csv");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// accounts
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","accounts");
		WSSetValue(rvmap,"Desc","show accounts");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","accounts.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","accounts FileKey=accounts.csv");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

	#ifdef IQDNS
		// samreport
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","samreport");
		WSSetValue(rvmap,"Desc","SAM user account report");
		WSSetValue(rvmap,"NumParms","4");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","sam.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// Upload
			WSSetValue(rvmap,"Parm2","Upload");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","upload to file server");
			// Domain
			WSSetValue(rvmap,"Parm3","Domain");
			WSSetValue(rvmap,"ParmDef3","");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","domain filter");
			// User
			WSSetValue(rvmap,"Parm4","User");
			WSSetValue(rvmap,"ParmDef4","");
			WSSetValue(rvmap,"ParmOpt4","Y");
			WSSetValue(rvmap,"ParmDesc4","user filter");
		WSSetValue(rvmap,"NumSamples","3");
		WSSetValue(rvmap,"Sample1","samreport FileKey=sam.csv");
		WSSetValue(rvmap,"SampleDesc1","report no upload");
		WSSetValue(rvmap,"Sample2","samreport FileKey=sam.csv/Upload=Y");
		WSSetValue(rvmap,"SampleDesc2","report and upload");
		WSSetValue(rvmap,"Sample3","samreport FileKey=sam.csv/Domain=/User=");
		WSSetValue(rvmap,"SampleDesc3","report for single user");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// iqsamreport
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","iqsamreport");
		WSSetValue(rvmap,"Desc","IQ user account report");
		WSSetValue(rvmap,"NumParms","4");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","iqsam.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// Upload
			WSSetValue(rvmap,"Parm2","Upload");
			WSSetValue(rvmap,"ParmDef2","Y");
			WSSetValue(rvmap,"ParmOpt2","Y");
			WSSetValue(rvmap,"ParmDesc2","upload to file server");
			// Domain
			WSSetValue(rvmap,"Parm3","Domain");
			WSSetValue(rvmap,"ParmDef3","");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","domain filter");
			// User
			WSSetValue(rvmap,"Parm4","User");
			WSSetValue(rvmap,"ParmDef4","");
			WSSetValue(rvmap,"ParmOpt4","Y");
			WSSetValue(rvmap,"ParmDesc4","user filter");
		WSSetValue(rvmap,"NumSamples","3");
		WSSetValue(rvmap,"Sample1","iqsamreport FileKey=iqsam.csv");
		WSSetValue(rvmap,"SampleDesc1","report no upload");
		WSSetValue(rvmap,"Sample2","iqsamreport FileKey=iqsam.csv/Upload=Y");
		WSSetValue(rvmap,"SampleDesc2","report and upload");
		WSSetValue(rvmap,"Sample3","iqsamreport FileKey=iqsam.csv/Domain=/User=");
		WSSetValue(rvmap,"SampleDesc3","report for single user");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	#endif

	#ifdef MULTI_DAY_HIST
		if(odb.HISTORIC_DAYS>0)
		{
			// importhistoric
			rvmap.clear();
			WSSetValue(rvmap,"OnBehalfOf",dtid);
			WSSetValue(rvmap,"DeliverTo",oboi);
			WSSetValue(rvmap,"HelpCmd","importhistoric");
			WSSetValue(rvmap,"Desc","export distjournal details");
			WSSetValue(rvmap,"NumParms","1");
				// FileDate
				WSSetValue(rvmap,"Parm1","FileDate");
				WSSetValue(rvmap,"ParmDef1","yyyymmdd");
				WSSetValue(rvmap,"ParmOpt1","N");
				WSSetValue(rvmap,"ParmDesc1","file date or *");
			WSSetValue(rvmap,"NumSamples","1");
			WSSetValue(rvmap,"Sample1","importhistoric FileDate=20130125");
			WSSetValue(rvmap,"SampleDesc1","");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
	#endif

		// modifyorder
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","modifyorder");
		WSSetValue(rvmap,"Desc","modifies order attributes or change order associations");
		WSSetValue(rvmap,"NumParms","11");
			// AppInstID
			WSSetValue(rvmap,"Parm1","AppInstID");
			WSSetValue(rvmap,"ParmDef1","REDIRPT");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","match app instance");
			// ClOrdID
			WSSetValue(rvmap,"Parm2","ClOrdID");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","match order id or * for all");
			// OrderDate
			WSSetValue(rvmap,"Parm3","OrderDate");
			WSSetValue(rvmap,"ParmDef3","yyyymmdd");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","match date");
			// Account
			WSSetValue(rvmap,"Parm4","Account");
			WSSetValue(rvmap,"ParmDef4","00947200");
			WSSetValue(rvmap,"ParmOpt4","Y");
			WSSetValue(rvmap,"ParmDesc4","match account");
			// ClientID
			WSSetValue(rvmap,"Parm5","ClientID");
			WSSetValue(rvmap,"ParmDef5","u705758");
			WSSetValue(rvmap,"ParmOpt5","Y");
			WSSetValue(rvmap,"ParmDesc5","match user");
			// Symbol
			WSSetValue(rvmap,"Parm6","Symbol");
			WSSetValue(rvmap,"ParmDef6","");
			WSSetValue(rvmap,"ParmOpt6","Y");
			WSSetValue(rvmap,"ParmDesc6","match symbol");
			// NewAppInstID
			WSSetValue(rvmap,"Parm7","NewAppInstID");
			WSSetValue(rvmap,"ParmDef7","DEMOKMA6");
			WSSetValue(rvmap,"ParmOpt7","Y");
			WSSetValue(rvmap,"ParmDesc7","new app instance");
			// NewAccount
			WSSetValue(rvmap,"Parm8","NewAccount");
			WSSetValue(rvmap,"ParmDef8","00947000");
			WSSetValue(rvmap,"ParmOpt8","Y");
			WSSetValue(rvmap,"ParmDesc8","new account");
			// NewRootOrderID
			WSSetValue(rvmap,"Parm9","NewRootOrderID");
			WSSetValue(rvmap,"ParmDef9","");
			WSSetValue(rvmap,"ParmOpt9","Y");
			WSSetValue(rvmap,"ParmDesc9","new parent tree");
			// NewFirstClOrdID
			WSSetValue(rvmap,"Parm10","NewFirstClOrdID");
			WSSetValue(rvmap,"ParmDef10","");
			WSSetValue(rvmap,"ParmOpt10","Y");
			WSSetValue(rvmap,"ParmDesc10","new c/r chain");
			// NewSymbol
			WSSetValue(rvmap,"Parm11","NewSymbol");
			WSSetValue(rvmap,"ParmDef11","");
			WSSetValue(rvmap,"ParmOpt11","Y");
			WSSetValue(rvmap,"ParmDesc11","new symbol");
		WSSetValue(rvmap,"NumSamples","2");
		WSSetValue(rvmap,"Sample1","modifyorder AppInstID=REDIRPT/ClOrdID=YJAQZLDA0024DUF/OrderDate=20130430/NewAppInstID=DEMOKMA6");
		WSSetValue(rvmap,"SampleDesc1","change domain");
		WSSetValue(rvmap,"Sample2","modifyorder AppInstID=REDIRPT/ClOrdID=YJAQZLDA0024DUF/OrderDate=20130430/NewAccount=00947000");
		WSSetValue(rvmap,"SampleDesc2","change account");
		//WSSetValue(rvmap,"Sample3","modifyorder AppSystem=TWIST/AppInstID=REDIRPT/ClOrdID=YJ2CALDA0022E2F/NewAppInstID=DEMOKMA6");
		//WSSetValue(rvmap,"SampleDesc3","change domain");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		#ifdef REDIOLEDBCON
		// reloadlastmarketdest
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloadlastmarketdest");
		WSSetValue(rvmap,"Desc","reloads LastMarketDestination.txt");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloadlastmarketdest");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloadredidestcsv");
		WSSetValue(rvmap,"Desc","reloads RediDestination.csv");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloadredidestcsv");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloadalgostratcsv");
		WSSetValue(rvmap,"Desc","reloads AlgoStrategy.csv");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloadalgostratcsv");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		#endif
	}
	else//!VSDIST_HUB
	{
		// distjournal
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","distjournal");
		WSSetValue(rvmap,"Desc","export distjournal details");
		WSSetValue(rvmap,"NumParms","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","distjournal.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","distjournal FileKey=distjournal.txt");
		WSSetValue(rvmap,"SampleDesc1","");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		#ifdef SPECTRUM
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","zmapReset");
		WSSetValue(rvmap,"Desc","Reset zmap for HUB replay");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","0");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		#endif

		#ifdef REDIOLEDBCON
		// reloadredibusini
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloadredibusini");
		WSSetValue(rvmap,"Desc","reloads redibus.ini");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloadredibusini");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// reloaddoneawayusersini
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloaddoneawayusersini");
		WSSetValue(rvmap,"Desc","reloads DoneAwayUsers.ini");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloaddoneawayusersini");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// reloaddoneawayaccountscsv
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","reloaddoneawayaccountscsv");
		WSSetValue(rvmap,"Desc","reloads DoneAwayAccounts.csv");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","reloaddoneawayaccountscsv");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","genredidestcsv");
		WSSetValue(rvmap,"Desc","generates RediDestination.csv");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","genredidestcsv");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","gentsxlist");
		WSSetValue(rvmap,"Desc","generates TSXList-yyyymmdd.txt");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","gentsxlist");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","geneulist");
		WSSetValue(rvmap,"Desc","generates EUList-yyyymmdd.txt");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","geneulist");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// redioleselect
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","redioleselect");
		WSSetValue(rvmap,"Desc","Redi OLEDB SQL query");
		WSSetValue(rvmap,"NumParms","3");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","redioleselect.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// Select
			WSSetValue(rvmap,"Parm2","Select");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","select query");
			// Wait
			WSSetValue(rvmap,"Parm3","Wait");
			WSSetValue(rvmap,"ParmDef3","Y");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","Y/N");
		#ifdef IQDEVTEST
		WSSetValue(rvmap,"NumSamples","5");
		WSSetValue(rvmap,"Sample1","redioleselect FileKey=redioleselect.csv/Select=Select * from RediBus.dbo.SLOrders");
		WSSetValue(rvmap,"Sample2","redioleselect FileKey=redioleselect.csv/Select=Select * from Symbols.dbo.Fixes");
		WSSetValue(rvmap,"Sample3","redioleselect FileKey=redioleselect.csv/Select=Select * from Symbols.dbo.EquitiesIndices");
		WSSetValue(rvmap,"Sample4","redioleselect FileKey=redioleselect.csv/Select=Select * from Symbols.dbo.Futures");
		WSSetValue(rvmap,"Sample5","redioleselect FileKey=redioleselect.csv/Select=Select * from Symbols.dbo.OptionsDefinitions");
		#else
		WSSetValue(rvmap,"NumSamples","3");
		WSSetValue(rvmap,"Sample1","redioleselect FileKey=redioleselect.csv/Select=Select * from SYM2 where Symbol=='MSFT'");
		WSSetValue(rvmap,"Sample2","redioleselect FileKey=redioleselect.csv/Select=Select * from L2 where Symbol=='MSFT'");
		WSSetValue(rvmap,"Sample3","redioleselect FileKey=redioleselect.csv/Select=Select * from FX");
		#endif
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// rediposupdate
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","rediposupdate");
		WSSetValue(rvmap,"Desc","Append data\\RTPOSITIONS.csv");
		WSSetValue(rvmap,"NumParms","0");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","rediposupdate");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		#ifdef _DEBUG
		// redisqlscript
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","redisqlscript");
		WSSetValue(rvmap,"Desc","Create SQL Server table creation script from CAT.csv");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","redisqlscript.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// Tables
			WSSetValue(rvmap,"Parm2","Tables");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","list of table names");
		WSSetValue(rvmap,"NumSamples","4");
		WSSetValue(rvmap,"Sample1","redisqlscript FileKey=redisqlscript.txt/Tables=RTDOMAIN,RTDOMAIN_TRANS,RTUSER,RTACCOUNT,RTACCOUNT_TRANS");
		WSSetValue(rvmap,"Sample2","redisqlscript FileKey=redisqlscript.txt/Tables=RTAGGUNIT,RTAGGUNIT_TRANS,RTAGGUNITTOUSER,RTAGGUNITTOUSER_TRANS");
		WSSetValue(rvmap,"Sample3","redisqlscript FileKey=redisqlscript.txt/Tables=RTPOSITION,RTPOSITION_TRANS");
		WSSetValue(rvmap,"Sample3","redisqlscript FileKey=redisqlscript.txt/Tables=SLORDER,SLPOS,SLTRAN_DETAIL,SLTRAN_TRANS,SLBROKERS,SLLOCATEBROKERS,SLLOCATEBROKERS_TRANS");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);

		// redisqlload
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","redisqlload");
		WSSetValue(rvmap,"Desc","Load SQL Server table from <table>.csv file");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","redisqlload.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// Tables
			WSSetValue(rvmap,"Parm2","Tables");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","list of table names");
		WSSetValue(rvmap,"NumSamples","4");
		WSSetValue(rvmap,"Sample1","redisqlload FileKey=redisqlload.txt/Tables=RTDOMAIN,RTDOMAIN_TRANS,RTUSER,RTACCOUNT,RTACCOUNT_TRANS");
		WSSetValue(rvmap,"Sample2","redisqlload FileKey=redisqlload.txt/Tables=RTAGGUNIT,RTAGGUNIT_TRANS,RTAGGUNITTOUSER,RTAGGUNITTOUSER_TRANS");
		WSSetValue(rvmap,"Sample3","redisqlload FileKey=redisqlload.txt/Tables=RTPOSITION,RTPOSITION_TRANS");
		WSSetValue(rvmap,"Sample3","redisqlload FileKey=redisqlload.txt/Tables=SLORDER,SLPOS,SLTRAN_DETAIL,SLTRAN_TRANS,SLBROKERS,SLLOCATEBROKERS,SLLOCATEBROKERS_TRANS");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		#endif

		#ifdef REDISQLOATS
		// redisqlimportoats
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","redisqlimportoats");
		WSSetValue(rvmap,"Desc","Load OATS reports into sql server");
		WSSetValue(rvmap,"NumParms","2");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","redisqlimportoats.txt");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// FilePath
			WSSetValue(rvmap,"Parm2","FilePath");
			WSSetValue(rvmap,"ParmDef2","");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","OATS report file path");
		WSSetValue(rvmap,"NumSamples","1");
		WSSetValue(rvmap,"Sample1","redisqlimportoats FileKey=redisqlimportoats.txt/FilePath=SysmonTempFiles\\\\OATS_MLONTDRP_20120917.txt");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		#endif

		// redibustask
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","redibustask");
		//WSSetValue(rvmap,"Desc","Run or modify redibus.ini tasks");
		WSSetValue(rvmap,"Desc","Run redibus.ini tasks");
		WSSetValue(rvmap,"NumParms","3");
			// FileKey
			WSSetValue(rvmap,"Parm1","FileKey");
			WSSetValue(rvmap,"ParmDef1","redibustask.csv");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","local file name");
			// Cmd
			WSSetValue(rvmap,"Parm2","Cmd");
			WSSetValue(rvmap,"ParmDef2","list");
			WSSetValue(rvmap,"ParmOpt2","N");
			WSSetValue(rvmap,"ParmDesc2","list/run/modify");
			// Taskid
			WSSetValue(rvmap,"Parm3","Taskid");
			WSSetValue(rvmap,"ParmDef3","");
			WSSetValue(rvmap,"ParmOpt3","Y");
			WSSetValue(rvmap,"ParmDesc3","# list/failed/all");
			// DateTime
			//WSSetValue(rvmap,"Parm4","DateTime");
			//WSSetValue(rvmap,"ParmDef4","");
			//WSSetValue(rvmap,"ParmOpt4","Y");
			//WSSetValue(rvmap,"ParmDesc4","yyyymmdd-hhmmss");
		WSSetValue(rvmap,"NumSamples","3");
		WSSetValue(rvmap,"Sample1","redibustask FileKey=redibustask.csv/Cmd=list");
		WSSetValue(rvmap,"SampleDesc1","lists tasks");
		WSSetValue(rvmap,"Sample2","redibustask FileKey=redibustask.csv/Cmd=run/Taskid=failed/");
		WSSetValue(rvmap,"SampleDesc2","re-run failed tasks");
		WSSetValue(rvmap,"Sample3","redibustask FileKey=redibustask.csv/Cmd=run/Taskid=all/");
		WSSetValue(rvmap,"SampleDesc3","run failed and scheduled tasks");
		//WSSetValue(rvmap,"Sample4","redibustask FileKey=redibustask.csv/Cmd=modify/Taskid=#/DateTime=/");
		//WSSetValue(rvmap,"SampleDesc4","reschedule task");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		#endif

		// setlastemitseqno
		rvmap.clear();
		WSSetValue(rvmap,"OnBehalfOf",dtid);
		WSSetValue(rvmap,"DeliverTo",oboi);
		WSSetValue(rvmap,"HelpCmd","setlastemitseqno");
		WSSetValue(rvmap,"Desc","Sets the next Emit order sequence number");
		WSSetValue(rvmap,"NumParms","1");
		WSSetValue(rvmap,"NumSamples","1");
			// FileKey
			WSSetValue(rvmap,"Parm1","SeqNo");
			WSSetValue(rvmap,"ParmDef1","0");
			WSSetValue(rvmap,"ParmOpt1","N");
			WSSetValue(rvmap,"ParmDesc1","last seqno");
		WSSetValue(rvmap,"Sample1","setlastemitseqno SeqNo=100");
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	}

	// end help
	rvmap.clear();
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	WSSetValue(rvmap,"End","Y");
	WSSetTagValues(rvmap,parms,sizeof(parms));
	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	return 0;
}
int ViewServer::WSSysmonMsg(int UmrPortNo, int reqid, string cmd, const char *parm, int plen, void *udata)
{
	TVMAP tvmap;
	if(WSGetTagValues(tvmap,parm,plen)<0)
		return -1;
	string oboi=WSGetValue(tvmap,"OnBehalfOf");
	string dtid=WSGetValue(tvmap,"DeliverTo");
	TVMAP rvmap;
	WSSetValue(rvmap,"OnBehalfOf",dtid);
	WSSetValue(rvmap,"DeliverTo",oboi);
	char parms[2048]={0};
	// List app commands
	if(cmd=="help")
	{
		return WSSysmonHelp(UmrPortNo,reqid,cmd,tvmap,oboi,dtid,rvmap);
	}
	else if(cmd=="getsetupini")
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#ifdef WIN32
		string filepath="setup\\setup.ini";
		char fpath[MAX_PATH]={0};
		strcpy(fpath,filepath.c_str());
		if(fpath[0]!='\\')
			sprintf(fpath,"%s\\%s",pcfg->RunPath.c_str(),filepath.c_str());
	#else
		string filepath="setup/setup.ini";
		char fpath[MAX_PATH]={0};
		strcpy(fpath,filepath.c_str());
		if(fpath[0]!='/')
			sprintf(fpath,"%s/%s",pcfg->RunPath.c_str(),filepath.c_str());
	#endif
		if(PathFileExists(fpath)<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if(cmd=="putsetupini")
	{
		string srcpath=WSGetValue(tvmap,"SrcPath");
		if(srcpath.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires SrcPath parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#ifdef WIN32
		string filepath="setup\\setup.ini";
		char fpath[MAX_PATH]={0};
		strcpy(fpath,filepath.c_str());
		if(fpath[0]!='\\')
			sprintf(fpath,"%s\\%s",pcfg->RunPath.c_str(),filepath.c_str());
	#else
		string filepath="setup/setup.ini";
		char fpath[MAX_PATH]={0};
		strcpy(fpath,filepath.c_str());
		if(fpath[0]!='/')
			sprintf(fpath,"%s/%s",pcfg->RunPath.c_str(),filepath.c_str());
	#endif
		string overwrite="Y";
		if((PathFileExists(fpath)<0)&&(overwrite!="Y")&&(overwrite!="y"))
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"FilePath (%s) exists, requires Overwrite=Y",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			if(WSBeginUpload(srcpath.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo)<0)
			{
				WSSetValue(rvmap,"rc","-1");
				char tstr[1024]={0};
				sprintf(tstr,"Failed uploading (%s) from (%s)",fpath,srcpath.c_str());
				WSSetValue(rvmap,"Reason",tstr);
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			}
			else
			{
				WSSetValue(rvmap,"rc","0");
				char tstr[1024]={0};
				sprintf(tstr,"Uploading (%s)...",fpath);
				WSSetValue(rvmap,"Text",tstr);
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			}
		}
		return 0;
	}
	else if(cmd=="reloadsetupini")
	{
		if(LoadSetupIni()<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload setup.ini failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded setup.ini");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if(cmd=="reloadconfig")
	{
		int rc=LoadSystemConfig();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload config failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded config");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if(cmd=="restart")
	{
		string user=WSGetValue(tvmap,"User");
		if(user.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires User parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string reason=WSGetValue(tvmap,"Reason");
		if(user.empty()||reason.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires Reason parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		WSLogError("Restart by %s: %s",user.c_str(),reason.c_str());
		if(Restart()<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Restart failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Restart succeeded");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if(cmd=="dropusers")
	{
		WSLogEvent("UMR%d: Dropping all users",UmrPortNo);
		int ndrop=0;
		for(int i=0;i<NO_OF_USR_PORTS;i++)
		{
			if(!UsrPort[i].SockActive)
				continue;
			//if(UsrPort[i].DetPtr==(void*)PROTO_INSTAQUOTE)
			{
				WSCloseUsrPort(i); ndrop++;
			}
		}
		WSSetValue(rvmap,"rc","0");
		char tmsg[256]={0};
		sprintf(tmsg,"%d users dropped",ndrop);
		WSSetValue(rvmap,"Text",tmsg);
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		return 0;
	}
	else if(cmd=="filereport")
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\filereport.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/filereport.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowFileReport(fp);
		fclose(fp);
		WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
		WSSetValue(rvmap,"rc","0");
		char tstr[1024]={0};
		sprintf(tstr,"Downloading (%s)",fpath);
		WSSetValue(rvmap,"Text",tstr);
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		return 0;
	}
	#ifdef SPECTRUM
	else if(cmd=="zmapReset")
	{
		WSLogEvent("Resetting zmap for HUB reload");
		zmapReset=true;
		for(int i=0;i<NO_OF_CON_PORTS;i++)
		{
			if(!ConPort[i].SockConnected)
				continue;
			if(ConPort[i].DetPtr==(void*)PROTO_VSDIST)
			{
				WSLogError("Rejecting zmap reset request, HUB connection not halted on C%d",i);
				zmapReset=false;
				break;
			}
		}
		return 0;
	}
	#endif
	else if(cmd=="ordermap")
	{
	#ifdef WIN32
		if(stristr(oboi.c_str(),"SMConsole/LBM"))
		{
			if(MessageBox(WShWnd,
				"Depending on the number of orders in the system,\n"
				"this operation may hold up the server for a very long time,\n"
				"and there is no way to cancel the operation once it starts.\n"
				"Continue?",
				"Warning",MB_ICONWARNING|MB_YESNO)!=IDYES)
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason","Aborted on console");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
		else
		{
			string noprompt=WSGetValue(tvmap,"NoPrompt");
			if((noprompt!="Y")&&(noprompt!="y"))
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason",
					"Depending on the number of orders in the system, "
					"this operation may hold up the server for a very long time, "
					"and there is no way to cancel the operation once it starts. "
					"Send 'NoPrompt=Y' to override.");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
	#else
		printf(
			"Depending on the number of orders in the system,\n"
			"this operation may hold up the server for a very long time,\n"
			"and there is no way to cancel the operation once it starts.\n"
			"Continue? Y/N");
		char ch=getchar();
		if(toupper(ch)!='Y')
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Aborted on console");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#endif
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\ordermap.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/ordermap.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowOrderMap(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if(cmd=="parentmap")
	{
	#ifdef WIN32
		if(stristr(oboi.c_str(),"SMConsole/LBM"))
		{
			if(MessageBox(WShWnd,
				"Depending on the number of orders in the system,\n"
				"this operation may hold up the server for a very long time,\n"
				"and there is no way to cancel the operation once it starts.\n"
				"Continue?",
				"Warning",MB_ICONWARNING|MB_YESNO)!=IDYES)
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason","Aborted on console");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
		else
		{
			string noprompt=WSGetValue(tvmap,"NoPrompt");
			if((noprompt!="Y")&&(noprompt!="y"))
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason",
					"Depending on the number of orders in the system, "
					"this operation may hold up the server for a very long time, "
					"and there is no way to cancel the operation once it starts. "
					"Send 'NoPrompt=Y' to override.");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
	#else
		printf(
			"Depending on the number of orders in the system,\n"
			"this operation may hold up the server for a very long time,\n"
			"and there is no way to cancel the operation once it starts.\n"
			"Continue? Y/N");
		char ch=getchar();
		if(toupper(ch)!='Y')
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Aborted on console");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#endif
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\parentmap.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/parentmap.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowParentMap(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if(cmd=="vieworder")
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string appsystem=WSGetValue(tvmap,"AppSystem");
		if(appsystem.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires AppSystem parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string clordid=WSGetValue(tvmap,"ClOrdID");
		if(clordid.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires ClOrdID parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string orderdate=WSGetValue(tvmap,"OrderDate");
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\ordermap.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/ordermap.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowOrder(fp,appsystem.c_str(),clordid.c_str(),atoi(orderdate.c_str()));
		fclose(fp);
		// ShowOrder writes the error reason into the file
		//if(rc<0)
		//{
		//	WSSetValue(rvmap,"rc","-1");
		//	char tstr[1024]={0};
		//	sprintf(tstr,"Order not found");
		//	WSSetValue(rvmap,"Reason",tstr);
		//	WSSetTagValues(rvmap,parms,sizeof(parms));
		//	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		//}
		//else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="usertask"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\usertask.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/usertask.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowUserTask(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="userlist"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\userlist.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/userlist.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowUserList(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#ifdef _DEBUG
	// For debugging ExprTok class
	else if((VSDIST_HUB)&&(cmd=="lex"))
	{
		string expr=WSGetValue(tvmap,"Expr");
		const char *eptr=expr.c_str();
		if(expr.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires Expr parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string fixmsg=WSGetValue(tvmap,"FixMsg");
		if(fixmsg.empty())
		//{
		//	WSSetValue(rvmap,"rc","-1");
		//	WSSetValue(rvmap,"Reason","Requires FixMsg parameter.");
		//	WSSetTagValues(rvmap,parms,sizeof(parms));
		//	WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		//	return 0;
		//}
			// Default internal test message
			fixmsg="35=8\00149=PH_SORTF2\00156=SOR_FIDA4\001128=SOR_FIDA4-ARCA\00157=MLCC\00182109=High Net Worth Desk\00129=1\00111555= Y-SORTKDB\0015001=AX10:80005220.02\00139=2\00111524=sta-ax2-g.prd.etsd.ml.com:13503\00111568=P\00182284=High Net Worth Desk\001284=High Net Worth Desk\001TransNum=5401195830\00182017=1688862745380989\0015035=AX10:80005220.02\001150=2\0019373=A\00130=ARCX\00118=6\00120=0\0015055=AI15318467OM1\00182900=SORT\00132=400\0015054=CHILD\00176=ARCX\0015037=FLIP\00170129=00023387353ORNY1\00144=21.59\0015010=SOR_FIDA4\00111522=GPS.N\001200010=AX10:80005220.01_1\001_PH_CACHE_NUM=4\00158=Fill\001151=0\00160=20100603-16:32:54\00114=400\00117=ARCX1MA_FID21688862745380989\0011=03160450\0016=21.59\00111505=AI\00159=0\00174008=SORT1_6_9b\00185060=20100603-16:32:54.802\00137=1688862760810832\00155=GPS\00174000=FLIP\00173104=ALL\001_INFOBUS_TOPIC=CACHE_4\00111=AI15318467OM1.0\00115=USD\0016528=A\00140=2\001418=L\00182000=N\0019730=A\00174124=NonInst AG TT\001100010=AX10:80005220.01\00138=400\001109=BUYBACK\00147=A\00131=21.59\00154=1\00148=GPS.N\00182012=MLCO\00122=5\00172284=NY Listed\001_PH_SEQUENCE=1195830\001";
		FIXINFO ifix;
		ifix.FIXDELIM=0x01;
		ifix.noSession=true;
		ifix.supressChkErr=true;
		ifix.FixMsgReady((char*)fixmsg.c_str(),(int)fixmsg.length());
		ExprTok eval;
		eval.pnotify=this;
		int rc=eval.EvalExpr(eptr,eptr +strlen(eptr),&ifix);
		WSSetValue(rvmap,"rc","0");
		char tstr[1024]={0};
		if(rc<0)
			sprintf(tstr,"Parse error");
		else
		{
			switch(eval.vtype)
			{
			#ifdef WIN32
			case 'I': sprintf(tstr,"Value=%I64d",eval.ival); break;
			#else
			case 'I': sprintf(tstr,"Value=%lld",eval.ival); break;
			#endif
			case 'F': sprintf(tstr,"Value=%f",eval.fval); break;
			case 'S': sprintf(tstr,"Value=%s",eval.sval); break;
			case 'B': sprintf(tstr,"Value=%s",eval.bval?"TRUE":"FALSE"); break;
			#ifdef WIN32
			case 'T': sprintf(tstr,"Value=%I64d",eval.ival); break;
			#else
			case 'T': sprintf(tstr,"Value=%lld",eval.ival); break;
			#endif
			default: sprintf(tstr,"Value=Unknown type");
			};
		}
		WSSetValue(rvmap,"Text",tstr);
		WSSetTagValues(rvmap,parms,sizeof(parms));
		WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
	}
#endif
#ifdef _DEBUG
	else if((VSDIST_HUB)&&(cmd=="sqlquery"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string select=WSGetValue(tvmap,"Select");
		if(select.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires Select parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string from=WSGetValue(tvmap,"From");
		if(from.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires From parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string where=WSGetValue(tvmap,"Where");
		string recurse=WSGetValue(tvmap,"Where");
		int maxrows=atoi(WSGetValue(tvmap,"MaxRows").c_str());
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\sqlquery.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/sqlquery.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=SqlQuery(fp,select.c_str(),from.c_str(),where.c_str(),UmrPortNo);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
	}
#endif
	else if((!VSDIST_HUB)&&(cmd=="distjournal"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\distjournal.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/distjournal.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowDistJournal(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#ifdef TEST_ALL_ORDERIDS
	else if((VSDIST_HUB)&&cmd=="dbgloadoids"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\dbgloadoids.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/dbgloadoids.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=DbgLoadOids(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
	else if((VSDIST_HUB)&&(cmd=="histogram"))
	{
	#ifdef WIN32
		if(stristr(oboi.c_str(),"SMConsole/LBM"))
		{
			if(MessageBox(WShWnd,
				"Depending on the number of orders in the system,\n"
				"this operation may hold up the server for a very long time,\n"
				"and there is no way to cancel the operation once it starts.\n"
				"Continue?",
				"Warning",MB_ICONWARNING|MB_YESNO)!=IDYES)
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason","Aborted on console");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
		else
		{
			string noprompt=WSGetValue(tvmap,"NoPrompt");
			if((noprompt!="Y")&&(noprompt!="y"))
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason",
					"Depending on the number of orders in the system, "
					"this operation may hold up the server for a very long time, "
					"and there is no way to cancel the operation once it starts. "
					"Send 'NoPrompt=Y' to override.");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
	#else
		printf(
			"Depending on the number of orders in the system,\n"
			"this operation may hold up the server for a very long time,\n"
			"and there is no way to cancel the operation once it starts.\n"
			"Continue? Y/N");
		char ch=getchar();
		if(toupper(ch)!='Y')
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Aborted on console");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#endif
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
		int rc=WriteHistogram(fpath);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="indices"))
	{
	#ifdef WIN32
		if(stristr(oboi.c_str(),"SMConsole/LBM"))
		{
			if(MessageBox(WShWnd,
				"Depending on the number of orders in the system,\n"
				"this operation may hold up the server for a very long time,\n"
				"and there is no way to cancel the operation once it starts.\n"
				"Continue?",
				"Warning",MB_ICONWARNING|MB_YESNO)!=IDYES)
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason","Aborted on console");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
		else
		{
			string noprompt=WSGetValue(tvmap,"NoPrompt");
			if((noprompt!="Y")&&(noprompt!="y"))
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason",
					"Depending on the number of orders in the system, "
					"this operation may hold up the server for a very long time, "
					"and there is no way to cancel the operation once it starts. "
					"Send 'NoPrompt=Y' to override.");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
	#else
		printf(
			"Depending on the number of orders in the system,\n"
			"this operation may hold up the server for a very long time,\n"
			"and there is no way to cancel the operation once it starts.\n"
			"Continue? Y/N");
		char ch=getchar();
		if(toupper(ch)!='Y')
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Aborted on console");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#endif
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\indices.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/indices.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowIndices(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="openorders"))
	{
	#ifdef WIN32
		if(stristr(oboi.c_str(),"SMConsole/LBM"))
		{
			if(MessageBox(WShWnd,
				"Depending on the number of orders in the system,\n"
				"this operation may hold up the server for a very long time,\n"
				"and there is no way to cancel the operation once it starts.\n"
				"Continue?",
				"Warning",MB_ICONWARNING|MB_YESNO)!=IDYES)
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason","Aborted on console");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
		else
		{
			string noprompt=WSGetValue(tvmap,"NoPrompt");
			if((noprompt!="Y")&&(noprompt!="y"))
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason",
					"Depending on the number of orders in the system, "
					"this operation may hold up the server for a very long time, "
					"and there is no way to cancel the operation once it starts. "
					"Send 'NoPrompt=Y' to override.");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
	#else
		printf(
			"Depending on the number of orders in the system,\n"
			"this operation may hold up the server for a very long time,\n"
			"and there is no way to cancel the operation once it starts.\n"
			"Continue? Y/N");
		char ch=getchar();
		if(toupper(ch)!='Y')
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Aborted on console");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#endif
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\openorders.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/openorders.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowOpenOrders(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="fills"))
	{
	#ifdef WIN32
		if(stristr(oboi.c_str(),"SMConsole/LBM"))
		{
			if(MessageBox(WShWnd,
				"Depending on the number of orders in the system,\n"
				"this operation may hold up the server for a very long time,\n"
				"and there is no way to cancel the operation once it starts.\n"
				"Continue?",
				"Warning",MB_ICONWARNING|MB_YESNO)!=IDYES)
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason","Aborted on console");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
		else
		{
			string noprompt=WSGetValue(tvmap,"NoPrompt");
			if((noprompt!="Y")&&(noprompt!="y"))
			{
				WSSetValue(rvmap,"rc","-1");
				WSSetValue(rvmap,"Reason",
					"Depending on the number of orders in the system, "
					"this operation may hold up the server for a very long time, "
					"and there is no way to cancel the operation once it starts. "
					"Send 'NoPrompt=Y' to override.");
				WSSetTagValues(rvmap,parms,sizeof(parms));
				WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
				return 0;
			}
		}
	#else
		printf(
			"Depending on the number of orders in the system,\n"
			"this operation may hold up the server for a very long time,\n"
			"and there is no way to cancel the operation once it starts.\n"
			"Continue? Y/N");
		char ch=getchar();
		if(toupper(ch)!='Y')
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Aborted on console");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
	#endif
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\fills.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/fills.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowFills(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#ifdef MULTI_HASH
	else if((VSDIST_HUB)&&(cmd=="multihash"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\multihash.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/multihash.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=odb.omap.ShowMultiHash(fp,"omap");
		odb.pmap.ShowMultiHash(fp,"pmap");
		odb.fmap.ShowMultiHash(fp,"fmap");
		odb.symmap.ShowMultiHash(fp,"symmap");
		odb.acctmap.ShowMultiHash(fp,"acctmap");
		odb.omap2.ShowMultiHash(fp,"omap2");
		odb.fomap.ShowMultiHash(fp,"fomap");
		odb.oomap.ShowMultiHash(fp,"oomap");
		odb.aimap.ShowMultiHash(fp,"aimap");
		odb.cimap.ShowMultiHash(fp,"cimap");
		odb.tsmap.ShowMultiHash(fp,"tsmap");
		odb.cnmap.ShowMultiHash(fp,"cnmap");
		odb.cpmap.ShowMultiHash(fp,"cpmap");
		odb.cpmap.ShowMultiHash(fp,"akmap");
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
	else if((VSDIST_HUB)&&(cmd=="instances"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\instances.tmp",pcfg->RunPath.c_str());
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowInstances(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="accounts"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\accounts.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/accounts.tmp",pcfg->RunPath.c_str());
	#endif
		FILE *fp=fopen(fpath,"wt");
		int rc=ShowAccounts(fp);
		fclose(fp);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#ifdef IQDNS
	else if((VSDIST_HUB)&&(cmd=="samreport"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string upload=WSGetValue(tvmap,"Upload");
		string domain=WSGetValue(tvmap,"Domain");
		string user=WSGetValue(tvmap,"User");
		char fpath[MAX_PATH]={0};
		int rc=SAMReport(fpath,upload=="Y"||upload=="y",domain.c_str(),user.c_str());
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="iqsamreport"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string upload=WSGetValue(tvmap,"Upload");
		string domain=WSGetValue(tvmap,"Domain");
		string user=WSGetValue(tvmap,"User");
		char fpath[MAX_PATH]={0};
		int rc=IQSAMReport(fpath,upload=="Y"||upload=="y",domain.c_str(),user.c_str());
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Download (%s) failed",fpath);
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
#ifdef MULTI_DAY_HIST
	else if((VSDIST_HUB)&&(cmd=="importhistoric")&&(odb.HISTORIC_DAYS>0))
	{
		string filedate=WSGetValue(tvmap,"FileDate");
		if(filedate.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileDate parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		int rc=-1;
		if(filedate=="*")
		{
			int ndays=odb.HISTORIC_DAYS +odb.HISTORIC_DAYS*2/7 +1;
			for(int d=1;(ndays>0)&&(d<=odb.HISTORIC_DAYS);d++)
			{
				int sdate=CalcTPlusDate(odb.wsdate,-d,false);
				rc=ImportHistoricFiles(sdate);
				if(rc<0)
					break;
			}
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Imported historic files for %s",filedate.c_str());
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		else
			rc=ImportHistoricFiles(atoi(filedate.c_str()));
		if(rc<3)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Import historic files for %s failed",filedate.c_str());
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Imported historic files for %s",filedate.c_str());
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
#ifdef REDIOLEDBCON
	else if((!VSDIST_HUB)&&(cmd=="redioleselect"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
	#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\redioleselect.tmp",pcfg->RunPath.c_str());
	#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/redioleselect.tmp",pcfg->RunPath.c_str());
	#endif
		string select=WSGetValue(tvmap,"Select");
		string wait=WSGetValue(tvmap,"Wait");

		int ConPortNo=-1;
		for(int i=0;i<NO_OF_CON_PORTS;i++)
		{
			if(!ConPort[i].SockConnected)
				continue;
			if(ConPort[i].DetPtr==(void*)PROTO_REDIOLEDB)
			{
				ConPortNo=i;
				break;
			}
		}
		if(ConPortNo<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","No connected $REDIOLEDB$ ports found.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		FILE *wfp=fopen(fpath,"wt");
		fprintf(wfp,"%s\n",select.c_str());
		int qid=VSRediQuery(ConPortNo,select.c_str(),-1,-1,wfp,(void*)WSTime);
		if((qid>0)&&((wait!="N")&&(wait!="n")))
		{
			int qtmax=30;
			if((strstr(select.c_str(),"RTSYM"))||(strstr(select.c_str(),"RTPOSITION")))
				qtmax=600;
			if(VSWaitRediQuery(ConPortNo,qtmax,qid,true)<0)
				qid=-1;
		}
		fclose(wfp);
		if(qid<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Query (%s) failed",select.c_str());
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="rediposupdate"))
	{
		if(SendRediPositions("data\\RTPOSITION.csv"))
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Success");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#if defined(_DEBUG)&&defined(IQDEVTEST)
	else if((!VSDIST_HUB)&&(cmd=="redisqlscript"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string tables=WSGetValue(tvmap,"Tables");
		if(tables.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires Tables parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
		#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\redisqlscript.tmp",pcfg->RunPath.c_str());
		#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/redisqlscript.tmp",pcfg->RunPath.c_str());
		#endif

		FILE *fp=fopen(fpath,"wt");
		int rc=RediSqlScript(fp,tables.c_str());
		fclose(fp); fp=0;
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Script generation failed");
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="redisqlload"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string tables=WSGetValue(tvmap,"Tables");
		if(tables.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires Tables parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
		#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\redisqlload.tmp",pcfg->RunPath.c_str());
		#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/redisqlload.tmp",pcfg->RunPath.c_str());
		#endif

		FILE *fp=fopen(fpath,"wt");
		int rc=RediSqlLoad(fp,tables.c_str());
		fclose(fp); fp=0;
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Script generation failed");
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
#ifdef REDISQLOATS
	else if((!VSDIST_HUB)&&(cmd=="redisqlimportoats"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string filepath=WSGetValue(tvmap,"FilePath");
		if(filepath.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FilePath parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		char fpath[MAX_PATH]={0};
		#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\redisqlimportoats.tmp",pcfg->RunPath.c_str());
		#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/redisqlimportoats.tmp",pcfg->RunPath.c_str());
		#endif

		FILE *fp=fopen(fpath,"wt");
		int rc=RediSqlImportOats(fp,filepath.c_str());
		fclose(fp); fp=0;
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"Import failed");
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
	else if((VSDIST_HUB)&&(cmd=="reloadlastmarketdest"))
	{
		int rc=LoadLastMarketDestTxt();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload LastMarketDestination.txt failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded LastMarketDestination.txt");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="reloadredidestcsv"))
	{
		int rc=LoadRediDestCsv();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload RediDestination.csv failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded RediDestination.csv");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((VSDIST_HUB)&&(cmd=="reloadalgostratcsv"))
	{
		int rc=LoadAlgoStrategyCsv();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload AlgoStrategy.csv failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded AlgoStrategy.csv");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="reloadredibusini"))
	{
		int rc=LoadRedibusIni();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload redibus.ini failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded redibus.ini");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="reloaddoneawayusersini"))
	{
		int rc=LoadDoneAwayUsersIni();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload DoneAwayUsers.ini failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded DoneAwayUsers.ini");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="reloaddoneawayaccountscsv"))
	{
		int rc=LoadDoneAwayAccountsCsv();
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Reload DoneAwayAccounts.csv failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Reloaded DoneAwayAccounts.csv");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="genredidestcsv"))
	{
		char dopath[MAX_PATH]={0},dipath[MAX_PATH]={0};
		sprintf(dopath,"%s\\data\\RediDestination.csv",pcfg->RunPath.c_str());
		sprintf(dipath,"%s\\data\\DESTINATIONINFO_%08d.csv",pcfg->RunPath.c_str(),WSDate);
		int rc=GenRediDestinationCsv(dopath,dipath);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Gen RediDestination.csv failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Generated RediDestination.csv");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="gentsxlist"))
	{
		char dopath[MAX_PATH]={0},dipath[MAX_PATH]={0};
		sprintf(dopath,"%s\\data\\TSXList-%08d.txt",pcfg->RunPath.c_str(),WSDate);
		sprintf(dipath,"P:\\mguide\\RTSYM_EQ.csv",pcfg->RunPath.c_str());
		int rc=GenTsxList(dopath,dipath);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Gen TSXList failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Generated TSXList");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="geneulist"))
	{
		char dopath[MAX_PATH]={0},dipath[MAX_PATH]={0};
		sprintf(dopath,"%s\\data\\EUist-%08d.txt",pcfg->RunPath.c_str(),WSDate);
		sprintf(dipath,"P:\\mguide\\RTSYM_EQ_ALL.csv",pcfg->RunPath.c_str());
		int rc=GenEUList(dopath,dipath);
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Gen EUList failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			WSSetValue(rvmap,"Text","Generated EUList");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="redibustask"))
	{
		string filekey=WSGetValue(tvmap,"FileKey");
		if(filekey.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires FileKey parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string cmd=WSGetValue(tvmap,"Cmd");
		if(cmd.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires Cmd parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string taskid=WSGetValue(tvmap,"Taskid");
		string datetime=WSGetValue(tvmap,"DateTime");
		char fpath[MAX_PATH]={0};
		#ifdef WIN32
		sprintf(fpath,"%s\\SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s\\SysmonTempFiles\\redibustask.tmp",pcfg->RunPath.c_str());
		#else
		sprintf(fpath,"%s/SysmonTempFiles",pcfg->RunPath.c_str());
		CreateDirectory(fpath,0);
		sprintf(fpath,"%s/SysmonTempFiles/redibustask.tmp",pcfg->RunPath.c_str());
		#endif

		FILE *fp=fopen(fpath,"wt");
		int rc=RedibusTask(fp,cmd.c_str(),taskid.c_str(),datetime.c_str());
		fclose(fp); fp=0;
		if(rc<0)
		{
			WSSetValue(rvmap,"rc","-1");
			char tstr[1024]={0};
			sprintf(tstr,"redibustask failed");
			WSSetValue(rvmap,"Reason",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSendFile(filekey.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,WS_UMR,UmrPortNo);
			WSSetValue(rvmap,"rc","0");
			char tstr[1024]={0};
			sprintf(tstr,"Downloading (%s)",fpath);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
#endif
	else if((VSDIST_HUB)&&(cmd=="modifyorder"))
	{
		string appinstid=WSGetValue(tvmap,"AppInstID");
		if(appinstid.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires AppInstID parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string clordid=WSGetValue(tvmap,"ClOrdID");
		if(clordid.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires ClOrdID parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		string orderdate=WSGetValue(tvmap,"OrderDate");
		string account=WSGetValue(tvmap,"Account");
		string clientid=WSGetValue(tvmap,"ClientID");
		string symbol=WSGetValue(tvmap,"Symbol");
		string newappinstid=WSGetValue(tvmap,"NewAppInstID");
		string newaccount=WSGetValue(tvmap,"NewAccount");
		string newrootorderid=WSGetValue(tvmap,"NewRootOrderID");
		string newfirstclordid=WSGetValue(tvmap,"NewFirstClOrdID");
		string newsymbol=WSGetValue(tvmap,"NewSymbol");
		char okey[256]={0};
		strcpy(okey,clordid.c_str());
		if((strcmp(okey,"*"))&&(okey[8]!='.'))
		{
			if(odb.INDEX_ORDERDATE)
				sprintf(okey,"%08d.%s",atoi(orderdate.c_str()),clordid.c_str());
			else
				strcpy(okey,clordid.c_str());
		}
		int rc=ModifyOrder(appinstid.c_str(),okey,atoi(orderdate.c_str()),
			account.c_str(),clientid.c_str(),symbol.c_str(),
			newappinstid.c_str(),newaccount.c_str(),newrootorderid.c_str(),newfirstclordid.c_str(),
			newsymbol.c_str());
		if(rc<1)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Modify failed");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			WSSetValue(rvmap,"rc","0");
			char tstr[256]={0};
			sprintf(tstr,"Modified %d orders",rc);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	else if((!VSDIST_HUB)&&(cmd=="setlastemitseqno"))
	{
		string seqno=WSGetValue(tvmap,"SeqNo");
		if(seqno.empty())
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Requires SeqNo parameter.");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
			return 0;
		}
		int newSeqNo=atoi(seqno.c_str());
		if(newSeqNo<0)
		{
			WSSetValue(rvmap,"rc","-1");
			WSSetValue(rvmap,"Reason","Invalid SeqNo parameter");
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		else
		{
			emitSeqno=newSeqNo;
			WSSetValue(rvmap,"rc","0");
			char tstr[256]={0};
			sprintf(tstr,"Last Emit seqno set to %d",emitSeqno);
			WSSetValue(rvmap,"Text",tstr);
			WSSetTagValues(rvmap,parms,sizeof(parms));
			WSSysmonResp(UmrPortNo,reqid,cmd,parms,0);
		}
		return 0;
	}
	return -1;
}
// Restart ourself
int ViewServer::Restart()
{
#ifdef WIN32
    char epath[MAX_PATH];
    GetModuleFileName(0,epath,sizeof(epath));
	char args[64]={0};
	//strcpy(args,"/startdelay 30");
	#ifdef _DEBUG
	sprintf(args,"/startwait %d 86400",GetCurrentProcessId());
	#else
	// Max 3 minutes to snapshot 1 million symbols (500MB)
	sprintf(args,"/startwait %d 360",GetCurrentProcessId());
	#endif
    WSLogEvent("Restarting [%s %s]",epath,args);
    ShellExecute(WShWnd,"open",epath,args,0,SW_SHOW);
	WSExit(0);
#else
	_ASSERT(false);//unfinished ViewServer::Restart
#endif
	return 0;
}
// Dump a file of user tasks
int ViewServer::ShowUserTask(FILE *fp)
{
	// Parent orders
	fprintf(fp,"UsrPortNo,Rid,TaskItem,TaskName,Select,From,Where,Unique,Hist,Live,MaxOrders,Spos,Tpos,Rcnt,Time,\n");
	int ucnt=0,tcnt=0;
	taskmgr.Lock();
	DWORD tnow=GetTickCount();
	time_t ttnow=time(0);
	for(TASKITEMMAP::iterator tit=taskmgr.ibegin();tit!=taskmgr.iend();tit++)
	{
		TASKITEM *titem=tit->second;
		for(TASKENTRY *ptask=titem->ptasks;ptask;ptask=ptask->inext)
		{
			if((ptask->udel)||(ptask->idel))
				continue;
			char tname[64]={0};
			switch(ptask->taskid)
			{
			case TASK_CLORDID: strcpy(tname,"ClOrdID"); break;
			case TASK_PARENTCLORDID: strcpy(tname,"ParentClOrdID"); break;
			case TASK_APPINST: strcpy(tname,"AppInstID"); break;
			case TASK_APPSYS: strcpy(tname,"AppSys"); break;
			case TASK_SYMBOL: strcpy(tname,"Symbol"); break;
			case TASK_ROOTORDERID: strcpy(tname,"RootOrderID"); break;
			case TASK_FIRSTCLORDID: strcpy(tname,"FirstClOrdID"); break;
			case TASK_ACCOUNT: strcpy(tname,"Account"); break;
			case TASK_ECNORDERID: strcpy(tname,"ECNOrderID"); break;
			case TASK_CLIENTID: strcpy(tname,"ClientID"); break;
			case TASK_SQL_DETAILS: strcpy(tname,"SendSqlDetailsLive"); break;
			case TASK_SQL_ORDERS: strcpy(tname,"SendSqlOrdersLive"); break;
			case TASK_SQL_ACCOUNTS: strcpy(tname,"SendSqlAccountsLive"); break;
			default: strcpy(tname,"???");
			};
			fprintf(fp,"%d,%d,%s,%s\n",ptask->fuser->PortNo,ptask->rid,tname,titem->skey.c_str());

			/*
			VSQuery *pquery=(VSQuery *)ptask->udata;
			DWORD tdiff=tnow -pquery->tstart;
			tm ltm=*localtime(&ttnow);
			ltm.tm_sec-=(tdiff/1000);
			mktime(&ltm);
			int qtime=(ltm.tm_hour *10000) +(ltm.tm_min*100) +(ltm.tm_sec);
			fprintf(fp,"%d,%d,%s,%s,\"%s\",\"%s\",\"%s\",%d,%d,%d,%d,%d,%d,%d,%06d\n",
				ptask->fuser->PortNo,pquery->rid,titem->skey.c_str(),tname,
				pquery->select.c_str(),pquery->from.c_str(),pquery->where.c_str(),pquery->unique,
				pquery->hist,pquery->live,pquery->maxorders,pquery->spos,pquery->tpos,pquery->rcnt,qtime);
			*/
			tcnt++;
		}
	}
	fprintf(fp,"Tot %d users, %d tasks\n",taskmgr.CountUsers(),tcnt);
	taskmgr.Unlock();
	return 0;
}
// Dump a file of users
int ViewServer::ShowUserList(FILE *fp)
{
	// Parent orders
	fprintf(fp,"UsrPortNo,IP,User,LoginDate,LoginTime,Authed,RecvBytes,SendBytes,\n");
	Lock();
	int ucnt=0;
	for(int i=0;i<=NO_OF_USR_PORTS;i++)
	{
		FeedUser *fuser=0;
		if(UsrPort[i].DetPtr==(void*)PROTO_VSCLIENT)
		{
			fuser=(FeedUser *)UsrPort[i].DetPtr9;
			if(!fuser)
				continue;
			LONGLONG rbytes=0,sbytes=0;
			if((fuser->PortNo>=0)&&(fuser->PortNo<NO_OF_USR_PORTS))
			{
				rbytes=UsrPort[fuser->PortNo].BytesIn;
				sbytes=UsrPort[fuser->PortNo].BytesOut;
			}
			fprintf(fp,"%d,%s,%s,%08d,%06d,%s,%s,%s,\n",
				i,UsrPort[i].RemoteIP,fuser->user.c_str(),fuser->loginDate,fuser->loginTime,fuser->authed?"Y":"N",
				SizeStr(rbytes,true),SizeStr(sbytes,true));
			ucnt++;
		}
	}
	Unlock();
	fprintf(fp,"Total %d users\n",ucnt);
	return 0;
}
int ViewServer::SqlQuery(FILE *fp, const char *select, const char *from, const char *where, int UmrPortNo)
{
	// Parent orders
	fprintf(fp,"select %s from %s where %s\n",select,from,where);

	VSCodec gbc;
	gbc.Init(this);
	char rbuf[2048]={0},*rptr=rbuf;
	gbc.EncodeSqlRequest(rptr,sizeof(rbuf),1,select,from,where,false,0,true,false);

	UmrPort[UmrPortNo].DetPtr6=&gbc;
	//FeedUser *fuser=taskmgr.AddUser(WS_UMR,UmrPortNo);
	//fuser->authed=true;
	FeedUser fuser;
	fuser.PortType=WS_UMR;
	fuser.PortNo=UmrPortNo;
	fuser.authed=true;
	UmrPort[UmrPortNo].DetPtr9=&fuser;
	UmrPort[UmrPortNo].DetPtr3=fp;
	gbc.DecodeRequest(rbuf,(int)(rptr -rbuf),(void*)(PTRCAST)MAKELONG(UmrPortNo,WS_UMR)); // will call NotifyXXXRequest
	//taskmgr.RemUser(WS_UMR,UmrPortNo);
	gbc.Shutdown();
	return 0;
}
// Dump a file of dist journal contents
int ViewServer::ShowDistJournal(FILE *fp)
{
	for(int i=0;i<NO_OF_USC_PORTS;i++)
	{
		if(!UscPort[i].InUse)
			continue;
		VSDistJournal *dj=(VSDistJournal *)UscPort[i].DetPtr5;
		if(dj)
		{
			dj->Lock();
			int lstart=dj->firstPage?dj->firstPage->lstart:0;
			int lend=dj->lastPage?dj->lastPage->lend:0;
			fprintf(fp,"USC%d[%s] [%d,%d] %d lines\n",
				i,UscPort[i].CfgNote,lstart,lend,lstart>0 ?lend -lstart +1 :0);
			int pno=0,lno=0;
			for(JPAGE *jpage=dj->firstPage;jpage;jpage=jpage->next)
			{
				#ifdef WIN32
				fprintf(fp,"PAGE %d: [%d,%d,%d] %d lines, rend=%I64d\n",
				#else
				fprintf(fp,"PAGE %d: [%d,%d,%d] %d lines, rend=%lld\n",
				#endif
					++pno,jpage->lstart,jpage->lend,jpage->lsent,jpage->lend -jpage->lstart +1,jpage->rend);
				const char *pptr=jpage->page +jpage->pbeg;
				for(int lidx=jpage->lstart;lidx<=jpage->lend;lidx++)
				{
					fprintf(fp,"%d,%s\n",lidx,pptr); lno++;
					pptr+=strlen(pptr) +1;
				}
			}
			fprintf(fp,"Total %d pages, %d lines\n",pno,lno);
			dj->Unlock();
		}
	}
	return 0;
}
#ifdef TEST_ALL_ORDERIDS
int ViewServer::DbgLoadOids(FILE *fp)
{
	char rpath[MAX_PATH]={0};
#ifdef WIN32
	sprintf(rpath,"%s\\orderids_20100603.txt",pcfg->RunPath.c_str());
#else
	sprintf(rpath,"%s/orderids_20100603.txt",pcfg->RunPath.c_str());
#endif
	FILE *rfp=fopen(rpath,"rt");
	if(!rfp)
		return -1;
	char rbuf[1024]={0};
	int lno=0;
	ITEMLOC nloc(0,0);
	while(fgets(rbuf,sizeof(rbuf),rfp))
	{
		lno++;
		const char *RootOrderID=strtoke(rbuf,",\r\n");
		if((!RootOrderID)||(!*RootOrderID))
			continue;
		const char *ClOrdID=strtoke(0,",\r\n");
		if((!ClOrdID)||(!*ClOrdID))
			continue;
		nloc.off++;
		//odb.omap.insert(ClOrdID,nloc);
		//if(odb.INDEX_ROOTORDERID)
		//	odb.pmap.insert(RootOrderID,nloc);
		if((ClOrdID[0]=='H')&&(ClOrdID[1]=='F'))
		{
			static string lcoid;
			if(lcoid!=ClOrdID)
			{
				fprintf(fp,"%s\n",ClOrdID);
				lcoid=ClOrdID;
			}

		}
		//if(nloc.off>=10000000)
		//	break;
	}
	fclose(rfp);
	return 0;
}
#endif
int ViewServer::WriteHistogram(char *hpath)
{
#ifdef WIN32
	sprintf(hpath,"%s\\histogram_%08d.txt",pcfg->RunPath.c_str(),odb.wsdate);
#else
	sprintf(hpath,"%s/histogram_%08d.txt",pcfg->RunPath.c_str(),odb.wsdate);
#endif
	FILE *fp=fopen(hpath,"wt");
	if(!fp)
	{
		WSLogError("Failed writing %s.",hpath);
		return -1;
	}
	DWORD shist[27][27];
	memset(shist,0,sizeof(shist));
	DWORD ahist[27][27];
	memset(ahist,0,sizeof(ahist));
	DWORD eohist[27][27];
	memset(eohist,0,sizeof(ahist));
	DWORD fohist[27][27];
	memset(fohist,0,sizeof(fohist));
	DWORD oohist[27][27];
	memset(oohist,0,sizeof(oohist));
	DWORD aihist[27][27];
	memset(aihist,0,sizeof(aihist));
	DWORD cnhist[27][27];
	memset(cnhist,0,sizeof(cnhist));
	DWORD cphist[27][27];
	memset(cphist,0,sizeof(cphist));
	DWORD akhist[27][27];
	memset(akhist,0,sizeof(akhist));
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		AddHistogram(shist,ahist,eohist,fohist,oohist,aihist,cnhist,cphist,akhist,asys);
	}
	ReleaseMutex(asmux);

	fprintf(fp,"ClOrdID Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",ohist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nSymbol Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",shist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nAccount Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",ahist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nEcnOrderID Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",eohist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nFilled Orders Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",fohist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nOpen Orders Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",oohist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nAppInst Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",aihist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nConnections Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",cnhist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}

	fprintf(fp,"\nClParentOrderID Histogram:\n");
	for(int ch1=0;ch1<27;ch1++)
	{
		fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
		for(int ch2=0;ch2<27;ch2++)
		{
			fprintf(fp,"%d,",cphist[ch1][ch2]);
		}
		fprintf(fp,"\n");
	}
	//DT9398
	for(unsigned int i=0;i<odb.INDEX_AUXKEYS;i++)
	{
		fprintf(fp,"\n%s Histogram:\n",odb.AUXKEY_NAMES[i].c_str());
		for(int ch1=0;ch1<27;ch1++)
		{
			fprintf(fp,"%c,",ch1<26?'A'+ch1:'$');
			for(int ch2=0;ch2<27;ch2++)
			{
				fprintf(fp,"%d,",akhist[ch1][ch2]);
			}
			fprintf(fp,"\n");
		}
	}
	fclose(fp);
	WSLogEvent("Wrote %s",hpath);
	return 0;
}
int ViewServer::ShowIndices(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		ShowIndices(fp,asys);
	}
	ReleaseMutex(asmux);
	return 0;
}
int ViewServer::ShowOpenOrders(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		ShowOpenOrders(fp,asys);
	}
	ReleaseMutex(asmux);
	return 0;
}
int ViewServer::ShowFills(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		ShowFills(fp,asys);
	}
	ReleaseMutex(asmux);
	return 0;
}
// Show files that are loaded or not, but only the ones that exist
int ViewServer::ShowFileReport(FILE *fp)
{
	_ASSERT(fp);
	DETAILFILEMAP& dfmap=odb.GetDetailFiles();
	int fcnt=0;
	fprintf(fp,"type,fpath,prefix,wsdate,readonly,portno,proto,"
			   "whnd,rhnd,fsize,fend,aend,rthnd,rend,\n");
	// VSDB .ext and .zmap files
	int ndays=odb.HISTORIC_DAYS;
	for(int d=1;(ndays>0)&&(d<=odb.HISTORIC_DAYS);d++)
	{
		int sdate=CalcTPlusDate(odb.wsdate,-d);
		if(sdate<HISTORIC_BEGINDATE)
			break;
		char epath[MAX_PATH]={0};
		// .ext file
		sprintf(epath,"%s\\data\\VSDB_%08d.ext",pcfg->RunPath.c_str(),sdate);
		WIN32_FIND_DATA fdata;
		HANDLE fhnd=FindFirstFile(epath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			FindClose(fhnd);
			fprintf(fp,"VSExtFile,%s,VSDB,%08d,%d,,,"
				   ",,%d,,,,,\n",
				epath,sdate,odb.readonly,fdata.nFileSizeLow);
			fcnt++;
		}
		// .zmap file
		sprintf(epath,"%s\\data\\VSDB_%08d.zmap",pcfg->RunPath.c_str(),sdate);
		fhnd=FindFirstFile(epath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			FindClose(fhnd);
			fprintf(fp,"VSZmapFile,%s,VSDB,%08d,%d,,,"
				   ",,%d,,,,,\n",
				epath,sdate,odb.readonly,fdata.nFileSizeLow);
			fcnt++;
		}
	}
	// Detail files
	for(DETAILFILEMAP::iterator dit=dfmap.begin();dit!=dfmap.end();dit++)
	{
		VSDetailFile *dfile=dit->second;
		fprintf(fp,"VSDetailFile,%s,%s,%08d,%d,%d,%d,"
				   "%X,%X,%I64d,%I64d,%I64d,%X,%I64d,\n",
			dfile->fpath.c_str(),dfile->prefix.c_str(),dfile->wsdate,dfile->readonly,dfile->portno,dfile->proto,
			dfile->whnd,dfile->rhnd,dfile->fsize,dfile->fend,dfile->aend,dfile->rthnd,dfile->rend);
		fcnt++;
	}
	// .dat files
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		list<OrderDB *> odblist;
		if(odb.HISTORIC_DAYS>0)
		{
			for(list<OrderDB *>::iterator oit=asys->odblist.begin();oit!=asys->odblist.end();oit++)
				odblist.push_back(*oit);
		}
		else
			odblist.push_back(&asys->odb);
		for(list<OrderDB *>::iterator oit=odblist.begin();oit!=odblist.end();oit++)
		{
			OrderDB *pdb=*oit;
			// Not loaded yet
			if(pdb->fpath.empty())
			{
				char dpath[MAX_PATH]={0};
				sprintf(dpath,"%s\\data\\%s_%08d.dat",pcfg->RunPath.c_str(),asys->sname.c_str(),pdb->wsdate);
				WIN32_FIND_DATA fdata;
				HANDLE fhnd=FindFirstFile(dpath,&fdata);
				if(fhnd!=INVALID_HANDLE_VALUE)
				{
					FindClose(fhnd);
					fprintf(fp,"VSOrderDBFile,%s,%s,%08d,%d,,,"
							"%X,%X,%d,,,,,\n",
						dpath,asys->sname.c_str(),pdb->wsdate,odb.readonly,
						INVALID_HANDLE_VALUE,INVALID_HANDLE_VALUE,fdata.nFileSizeLow);
					fcnt++;
				}
			}
			// Already loaded
			else
			{
				HANDLE fhnd;
				LARGE_INTEGER fsize,fend;
				pdb->GetFileReportStats(fhnd,fsize,fend);
				fprintf(fp,"VSOrderDBFile,%s,%s,%08d,%d,,,"
						"%X,%X,%I64d,%I64d,,,,\n",
					pdb->fpath.c_str(),asys->sname.c_str(),pdb->wsdate,pdb->readonly,
					pdb->readonly?INVALID_HANDLE_VALUE:fhnd,fhnd,fsize,fend);
				fcnt++;
			}
		}
	}
	ReleaseMutex(asmux);
	fprintf(fp,"Total %d files\n",fcnt);
	return 0;
}
int ViewServer::ShowOrderMap(FILE *fp, AppSystem *asys)
{
	_ASSERT((fp)&&(asys));
#ifdef MULTI_DAY_HIST
	list<OrderDB *> odblist;
	if(odb.HISTORIC_DAYS>0)
	{
		for(list<OrderDB *>::iterator oit=asys->odblist.begin();oit!=asys->odblist.end();oit++)
			odblist.push_back(*oit);
	}
	else
		odblist.push_back(&asys->odb);
	for(list<OrderDB *>::iterator oit=odblist.begin();oit!=odblist.end();oit++)
	{
		OrderDB *pdb=*oit;
#else
		OrderDB *pdb=&asys->odb;
#endif
		fprintf(fp,"[%s] %08d\n",asys->sname.c_str(),pdb->wsdate);
		if(pdb->omap.size()<1)
			continue;
		// All FIX orders
		//fprintf(fp,"OrderKey,RootOrderID,FirstClOrdID,AppInstID,Account,Offset,InMem,Term,NumDetails,Symbol,EcnOrderNo,HighMsgType,HighExecType,OrderQty,CumQty,FillQty,Side,TransactTime,Connection,OrderDate,LocaleOrderDate,");
		fprintf(fp,"OrderKey,RootOrderID,FirstClOrdID,AppInstID,Account,Offset,InMem,Term,NumDetails,Symbol,EcnOrderNo,HighMsgType,HighExecType,OrderQty,CumQty,FillQty,Side,TransactTime,Connection,OrderDate,");
		fprintf(fp,"\n");
		int ocnt=0,tdets=0,mdets=0;
		for(VSINDEX::iterator oit=pdb->omap.begin();oit!=pdb->omap.end();oit++)
		{
			ITEMLOC& oloc=oit.second;
			//bool unload=false;
			VSOrder *porder=pdb->ReadOrder(oloc); //unload=true;
			if(porder)
			{
				char inMem=porder->IsTerminated()?'N':'Y';
				ocnt++;
				int ndets=porder->GetNumDetails();
				tdets+=ndets;
				if(ndets>mdets)
					mdets=ndets;
				char hmt[2]={porder->GetHighMsgType(),0};
				char het[2]={porder->GetHighExecType(),0};
				char sidestr[2]={porder->GetSide(),0};
				#ifdef WIN32
				//fprintf(fp,"%s,%s,%s,%s,%s,%I64d,%c,%c,%d,%s,%s,%s,%s,%d,%d,%d,%s,%I64d,%s,%08d,%08d,\n",
				fprintf(fp,"%s,%s,%s,%s,%s,%I64d,%c,%c,%d,%s,%s,%s,%s,%d,%d,%d,%s,%I64d,%s,%08d,\n",
				#else
				fprintf(fp,"%s,%s,%s,%s,%s,%lld,%c,%c,%d,%s,%s,%s,%s,%d,%d,%d,%s,%lld,%s,%08d,\n",
				#endif
					oit.first.c_str(),porder->GetRootOrderID(),porder->GetFirstClOrdID(),
					porder->GetAppInstID(),porder->GetAccount(),(LONGLONG)oloc,inMem,
					porder->IsTerminated()?'Y':'N',ndets,porder->GetSymbol(),
					porder->GetEcnOrderID(),hmt,het,
					porder->GetOrderQty(),porder->GetCumQty(),porder->GetFillQty(),
					//sidestr,porder->GetTransactTime(),porder->GetConnection(),porder->GetOrderDate(),porder->GetLocaleOrderDate(QueryTimeZoneBias));
					sidestr,porder->GetTransactTime(),porder->GetConnection(),porder->GetOrderDate());
				//if(unload)
				//{
				//	pdb->FreeOrder(porder); porder=0;
				//}
			}
		}
		int adets=tdets/(ocnt?ocnt:1);
	#ifdef NOMAP_TERM_ORDERS
		fprintf(fp,"Tot %d orders (%d term orders not displayed), %d details, max %d, avg %d\n",ocnt +oterm,oterm,tdets,mdets,adets);
	#else
		fprintf(fp,"Tot %d orders, %d details, max %d, avg %d\n",ocnt,tdets,mdets,adets);
	#endif
#ifdef MULTI_DAY_HIST
		fprintf(fp,"\n");
	}
#endif
	return 0;
}
int ViewServer::ShowOrderMap(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		ShowOrderMap(fp,asys);
		fprintf(fp,"\n");
	}
	ReleaseMutex(asmux);
	return 0;
}
int ViewServer::ShowParentMap(FILE *fp, AppSystem *asys)
{
	_ASSERT((fp)&&(asys));
	OrderDB *pdb=&asys->odb;
	fprintf(fp,"[%s]\n",asys->sname.c_str());
	// Parent orders
	fprintf(fp,"ClientParentOrderID,NumChildren,ChildOrders,\n");
	int maxNumKids=0;
	for(VSINDEX::m_iterator pit=pdb->pmap.m_begin();pit!=pdb->pmap.m_end();)
	{
		string poid=pit.first;
		int nkids=0;
		VSINDEX::m_iterator npit=pit;
		for(;(npit!=pdb->pmap.m_end())&&(npit.first==pit.first);npit++)
			nkids++;
		fprintf(fp,"%s,%d,",pit.first.c_str(),nkids);
		if(nkids>maxNumKids)
			maxNumKids=nkids;
		for(VSINDEX::m_iterator mpit=pit;mpit!=npit;mpit++)
		{
			_ASSERT(mpit.first==pit.first);
			ITEMLOC& oloc=mpit.second;
			//bool unload=false;
			VSOrder *porder=pdb->ReadOrder(oloc);
			//unload=true;
			if(porder)
			{
				_ASSERT(mpit.first==porder->GetRootOrderID());
				fprintf(fp,"%s,",porder->GetOrderKey());
				//if(unload)
				//{
				//	pdb->FreeOrder(porder); porder=0;
				//}
			}
		}
		fprintf(fp,"\n");
		for(pit++;(pit!=pdb->pmap.m_end())&&(pit.first==poid);pit++)
			;
	}
	float avgNumKids=(float)(pdb->omap.size())/(pdb->pmap.m_size()?pdb->pmap.m_size():1);
	fprintf(fp,"Tot %d parents, max %d kids, avg %.1f kids\n",pdb->pmap.size(),maxNumKids,avgNumKids);
	return 0;
}
int ViewServer::ShowParentMap(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		ShowParentMap(fp,asys);
		fprintf(fp,"\n");
	}
	ReleaseMutex(asmux);
	return 0;
}
#ifdef MULTI_DAY_HIST
int ViewServer::ShowOrder(FILE *fp, const char *asname, const char *clordid, int odate)
#else
int ViewServer::ShowOrder(FILE *fp, const char *asname, const char *clordid)
#endif
{
	char skey[32]={0};
	sprintf(skey,"%s*",asname);
	_strupr(skey);
	AppSystem *asys=GetAppSystem(skey);
	if(!asys)
	{
		fprintf(fp,"AppSystem(%s) not found!\n",asname);
		return -1;
	}
	OrderDB *pdb=&asys->odb;
#ifdef MULTI_DAY_HIST
	if(pdb->HISTORIC_DAYS>0)
	{
		OrderDB *sdb=asys->GetDB(odate);
		if(sdb) pdb=sdb;
	}
	char okey[256]={0};
	if(odb.INDEX_ORDERDATE)
		sprintf(okey,"%08d.%s",odate,clordid);
	else
		strcpy(okey,clordid);
	VSINDEX::iterator oit=pdb->omap.find(okey);
#else
	VSINDEX::iterator oit=pdb->omap.find(clordid);
#endif
	if(oit==pdb->omap.end())
	{
		fprintf(fp,"Order(%s/%s) not found!\n",asname,clordid);
		return -1;
	}
	ITEMLOC& oloc=oit.second;
	VSOrder *porder=pdb->ReadOrder(oloc);
	if(!porder)
	{
		fprintf(fp,"Failed reading order(%s/%s)!\n",asname,clordid);
		return -1;
	}
	fprintf(fp,"Order: %s/%s\n",asname,clordid);
	//fprintf(fp,"marker: %08d\n",porder->marker);
	//fprintf(fp,"rtype: %d\n",porder->rtype);
	//fprintf(fp,"rlen: %d\n",porder->rlen);
	fprintf(fp,"AppInstID: %s\n",porder->GetAppInstID());
	fprintf(fp,"ClOrdID: %s\n",porder->GetClOrdID());
	fprintf(fp,"RootOrderID: %s\n",porder->GetRootOrderID());
	fprintf(fp,"FirstClOrdID: %s\n",porder->GetFirstClOrdID());
	fprintf(fp,"ClParentOrderID: %s\n",porder->GetClParentOrderID());
	fprintf(fp,"Symbol: %s\n",porder->GetSymbol());
	fprintf(fp,"Price: %f\n",porder->GetPrice());
	fprintf(fp,"Account: %s\n",porder->GetAccount());
	fprintf(fp,"EcnOrderID: %s\n",porder->GetEcnOrderID());
	fprintf(fp,"ClientID: %s\n",porder->GetClientID());
	fprintf(fp,"OrderQty: %d\n",porder->GetOrderQty());
	fprintf(fp,"CumQty: %d\n",porder->GetCumQty());
	fprintf(fp,"FillQty: %d\n",porder->GetFillQty());
	fprintf(fp,"term: %s\n",porder->IsTerminated()?"Y":"N");
	char mstr[2]={porder->GetHighMsgType(),0};
	fprintf(fp,"HighMsgType: %s\n",mstr);
	char estr[2]={porder->GetHighExecType(),0};
	fprintf(fp,"HighExecType: %s\n",estr);
	char sstr[2]={porder->GetSide(),0};
	fprintf(fp,"Side: %s\n",sstr);
	fprintf(fp,"TransactTime: %I64d\n",porder->GetTransactTime());
	fprintf(fp,"Connection: %s\n",porder->GetConnection());
#ifdef MULTI_DAY
	fprintf(fp,"OrderDate: %08d\n",porder->GetOrderDate());
	//fprintf(fp,"LocaleOrderDate: %08d\n",porder->GetLocaleOrderDate(QueryTimeZoneBias));
#endif
	fprintf(fp,"AuxKey: %s\n",porder->GetAuxKey());
	fprintf(fp,"dcnt: %d\n",porder->GetNumDetails());
	fprintf(fp,"noff: %I64d\n",(LONGLONG)porder->GetOffset());

	fprintf(fp,"\nDetails:\nIndex,UscPortNo,Offset,Length,[String],\n");
	for(int d=0;d<porder->GetNumDetails();d++)
	{
		unsigned short UscPortNo=(unsigned short)-1;
		unsigned short dlen=0;
		LONGLONG doff=porder->GetDetail(d,UscPortNo,dlen);
		if(((short)dlen<0)||(dlen>4096)) // sanity limit
			continue;
		if(((short)UscPortNo<0)||(UscPortNo>=NO_OF_USC_PORTS)||(!UscPort[UscPortNo].InUse))
			continue;
	#ifdef MULTI_DAY_HIST
		if(pdb->HISTORIC_DAYS>0)
			UscPort[UscPortNo].DetPtr1=odb.GetDetailFile(UscPortNo,odate);
	#endif
		VSDetailFile *dfile=(VSDetailFile*)UscPort[UscPortNo].DetPtr1;
		if(!dfile)
		{
			fprintf(fp,"USC%d: Missing detail file!\n",UscPortNo);
			continue;
		}
		int proto=dfile->proto;
		char *dptr=new char[dlen+1];
		memset(dptr,0,dlen+1);
		if(pdb->ReadDetail(dfile,doff,dptr,dlen)>0)
		{
			fprintf(fp,"%d,%d,%I64d,%d,[%s],\n",d+1,UscPortNo,doff,dlen,dptr);
		}
		delete dptr;
	}
	pdb->FreeOrder(porder);
	return 0;
}
int ViewServer::AddHistogram(DWORD shist[27][27], 
							 DWORD ahist[27][27], 
							 DWORD eohist[27][27], 
							 DWORD fohist[27][27], 
							 DWORD oohist[27][27], 
							 DWORD aihist[27][27], 
							 DWORD cnhist[27][27], 
							 DWORD cphist[27][27], 
							 DWORD akhist[27][27], 
							 AppSystem *asys)
{
	_ASSERT(asys);
	OrderDB *pdb=&asys->odb;
	for(VSINDEX::m_iterator sit=pdb->symmap.m_begin();sit!=pdb->symmap.m_end();sit++)
	{
		const char *sym=sit.first.c_str();
		char ch1=toupper(sym[0]),ch2=toupper(sym[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		shist[i1][i2]++;
	}
	for(VSINDEX::m_iterator sit=pdb->acctmap.m_begin();sit!=pdb->acctmap.m_end();sit++)
	{
		const char *acct=sit.first.c_str();
		char ch1=toupper(acct[0]),ch2=toupper(acct[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		ahist[i1][i2]++;
	}
	for(VSINDEX::m_iterator sit=pdb->omap2.m_begin();sit!=pdb->omap2.m_end();sit++)
	{
		const char *eoid=sit.first.c_str();
		char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		eohist[i1][i2]++;
	}
	for(VSINDEX::iterator sit=pdb->fomap.begin();sit!=pdb->fomap.end();sit++)
	{
		const char *eoid=sit.first.c_str();
		char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		fohist[i1][i2]++;
	}
	for(VSINDEX::iterator ooit=pdb->oomap.begin();ooit!=pdb->oomap.end();ooit++)
	{
		const char *eoid=ooit.first.c_str();
		char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		oohist[i1][i2]++;
	}
	for(VSINDEX::m_iterator ciit=pdb->cimap.m_begin();ciit!=pdb->cimap.m_end();ciit++)
	{
		const char *eoid=ciit.first.c_str();
		char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		aihist[i1][i2]++;
	}
	//for(TSINDEX::iterator tsit=pdb->tsmap.begin();tsit!=pdb->tsmap.end();tsit++)
	//{
	//	const char *eoid=tsit.first.c_str();
	//	char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
	//	int i1=26,i2=26;
	//	if((ch1>='A')&&(ch1<='Z'))
	//		i1=ch1 -'A';
	//	if((ch2>='A')&&(ch2<='Z'))
	//		i2=ch2 -'A';
	//	oohist[i1][i2]++;
	//}
	for(VSINDEX::m_iterator cnit=pdb->cnmap.m_begin();cnit!=pdb->cnmap.m_end();cnit++)
	{
		const char *eoid=cnit.first.c_str();
		char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		cnhist[i1][i2]++;
	}
	for(VSINDEX::m_iterator cpit=pdb->cpmap.m_begin();cpit!=pdb->cpmap.m_end();cpit++)
	{
		const char *eoid=cpit.first.c_str();
		char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
		int i1=26,i2=26;
		if((ch1>='A')&&(ch1<='Z'))
			i1=ch1 -'A';
		if((ch2>='A')&&(ch2<='Z'))
			i2=ch2 -'A';
		cphist[i1][i2]++;
	}
	//DT9398
	for(unsigned int idx=0;idx<AUX_KEYS_MAX_NUM;idx++)
	{
		for(VSINDEX::m_iterator akit=pdb->akmap[idx].m_begin();akit!=pdb->akmap[idx].m_end();akit++)
		{
			const char *eoid=akit.first.c_str();
			char ch1=toupper(eoid[0]),ch2=toupper(eoid[1]);
			int i1=26,i2=26;
			if((ch1>='A')&&(ch1<='Z'))
				i1=ch1 -'A';
			if((ch2>='A')&&(ch2<='Z'))
				i2=ch2 -'A';
			akhist[i1][i2]++;
		}
	}
	return 0;
}
int ViewServer::ShowIndices(FILE *fp, AppSystem *asys)
{
	_ASSERT((fp)&&(asys));
#ifdef MULTI_DAY_HIST
	list<OrderDB *> odblist;
	if(odb.HISTORIC_DAYS>0)
	{
		for(list<OrderDB *>::iterator oit=asys->odblist.begin();oit!=asys->odblist.end();oit++)
			odblist.push_back(*oit);
	}
	else
		odblist.push_back(&asys->odb);
	for(list<OrderDB *>::iterator oit=odblist.begin();oit!=odblist.end();oit++)
	{
		OrderDB *pdb=*oit;
#else
		OrderDB *pdb=&asys->odb;
#endif
		fprintf(fp,"\n[%s] %08d\n",asys->sname.c_str(),pdb->wsdate);
		if(pdb->omap.size()<1)
			continue;
		fprintf(fp,"\n%d ClOrdIDs to %d child locations\n",pdb->omap.size(),pdb->omap.size());
		fprintf(fp,"run \"ordermap\" command for details\n");

		int cnt=0;
		string last;
		if(pdb->INDEX_ROOTORDERID)
		{
			fprintf(fp,"\n%d RootOrderIDs to %d child locations\n",pdb->pmap.size(),pdb->pmap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"RootOrderID,Count,\n");
			for(VSINDEX::m_iterator pit=pdb->pmap.m_begin();pit!=pdb->pmap.m_end();pit++)
			{
				if((last.empty())||(pit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=pit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}

		if(pdb->INDEX_FIRSTCLORDID)
		{
			fprintf(fp,"\n%d FirstClOrdIDs to %d child locations\n",pdb->fmap.size(),pdb->fmap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"FirstClOrdID,Count,\n");
			for(VSINDEX::m_iterator fit=pdb->fmap.m_begin();fit!=pdb->fmap.m_end();fit++)
			{
				if((last.empty())||(fit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=fit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}

		if(pdb->INDEX_CLPARENTORDERID)
		{
			fprintf(fp,"\n%d ClParentOrderIDs to %d child locations\n",pdb->cpmap.size(),pdb->cpmap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"ClParentOrderID,Count,\n");
			for(VSINDEX::m_iterator cpit=pdb->cpmap.m_begin();cpit!=pdb->cpmap.m_end();cpit++)
			{
				if((last.empty())||(cpit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=cpit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}

		if(pdb->INDEX_SYMBOL)
		{
			fprintf(fp,"\n%d symbols to %d child locations\n",pdb->symmap.size(),pdb->symmap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"Symbol,Count,\n");
			for(VSINDEX::m_iterator sit=pdb->symmap.m_begin();sit!=pdb->symmap.m_end();sit++)
			{
				if((last.empty())||(sit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=sit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}

		if(pdb->INDEX_ACCOUNT)
		{
			fprintf(fp,"\n%d accounts to %d child locations\n",pdb->acctmap.size(),pdb->acctmap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"Account,Count,\n");
			for(VSINDEX::m_iterator ait=pdb->acctmap.m_begin();ait!=pdb->acctmap.m_end();ait++)
			{
				if((last.empty())||(ait.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=ait.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}

		if(pdb->INDEX_ECNORDERID)
		{
			fprintf(fp,"\n%d EcnOrderIDs to %d child locations\n",pdb->omap2.size(),pdb->omap2.m_size());
			cnt=0; last.clear();
			for(VSINDEX::m_iterator eit=pdb->omap2.m_begin();eit!=pdb->omap2.m_end();eit++)
			{
				if((last.empty())||(eit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=eit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}


		if(pdb->INDEX_CLIENTID)
		{
			fprintf(fp,"\n%d ClientIDs to %d child locations\n",pdb->cimap.size(),pdb->cimap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"ClientID,Count,\n");
			for(VSINDEX::m_iterator ciit=pdb->cimap.m_begin();ciit!=pdb->cimap.m_end();ciit++)
			{
				if((last.empty())||(ciit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=ciit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}

		if(pdb->INDEX_TRANSACTTIME)
		{
			fprintf(fp,"\n%d TimeStamps to %d child locations\n",pdb->tsmap.size(),pdb->tsmap.m_size());
			cnt=0; 
			LONGLONG tlast=0;
			fprintf(fp,"Time,Count,\n");
			for(TSINDEX::m_iterator tsit=pdb->tsmap.m_begin();tsit!=pdb->tsmap.m_end();tsit++)
			{
				if((!tlast)||(tsit.first==tlast))
					cnt++;
				else
				{
					#ifdef WIN32
					fprintf(fp,"%I64d,%d,\n",tlast,cnt); cnt=1;
					#else
					fprintf(fp,"%lld,%d,\n",tlast,cnt); cnt=1;
					#endif
				}
				tlast=tsit.first;
			}
			if(cnt>0)
				#ifdef WIN32
				fprintf(fp,"%I64d,%d,\n",tlast,cnt);
				#else
				fprintf(fp,"%lld,%d,\n",tlast,cnt);
				#endif
		}

		if(pdb->INDEX_FILLED_ORDERS)
		{
			fprintf(fp,"\n%d filled orders to %d child locations\n",pdb->fomap.size(),pdb->fomap.size());
			fprintf(fp,"run \"fills\" command for details\n");
		}

		if(pdb->INDEX_OPEN_ORDERS)
		{
			fprintf(fp,"\n%d open orders to %d child locations\n",pdb->oomap.size(),pdb->oomap.size());
			fprintf(fp,"run \"openorders\" command for details\n");
		}

		if(pdb->INDEX_CONNECTIONS)
		{
			fprintf(fp,"\n%d Connections to %d child locations\n",pdb->cnmap.size(),pdb->cnmap.m_size());
			cnt=0; last.clear();
			fprintf(fp,"Connection,Count,\n");
			for(VSINDEX::m_iterator coit=pdb->cnmap.m_begin();coit!=pdb->cnmap.m_end();coit++)
			{
				if((last.empty())||(coit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=coit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}
		//DT9398
		for(unsigned int i=0;i<odb.INDEX_AUXKEYS;i++)
		{
			fprintf(fp,"\n%d %s to %d child locations\n",pdb->akmap[i].size(),pdb->AUXKEY_NAMES[i].c_str(),pdb->akmap[i].m_size());
			cnt=0; last.clear();
			fprintf(fp,"AuxKey,Count,\n");
			for(VSINDEX::m_iterator akit=pdb->akmap[i].m_begin();akit!=pdb->akmap[i].m_end();akit++)
			{
				if((last.empty())||(akit.first==last))
					cnt++;
				else
				{
					fprintf(fp,"%s,%d,\n",last.c_str(),cnt); cnt=1;
				}
				last=akit.first;
			}
			if(cnt>0)
				fprintf(fp,"%s,%d,\n",last.c_str(),cnt);
		}
	}
	return 0;
}
int ViewServer::ShowOpenOrders(FILE *fp, AppSystem *asys)
{
	_ASSERT((fp)&&(asys));
	OrderDB *pdb=&asys->odb;
	fprintf(fp,"\n[%s]\n",asys->sname.c_str());
	fprintf(fp,"%d open orders with unique ids to %d child locations\n",pdb->oomap.size(),pdb->oomap.size());
	fprintf(fp,"AppInstID,ClOrdID,RootOrderID,FirstClOrdID,Account,EcnOrderID,Symbol,HighMsgType,HighExecType,FillQty,OrderQty,CumQty,OpenQty,Exception,\n");
	int ocnt=0,nexcept=0;
	for(VSINDEX::iterator ooit=pdb->oomap.begin();ooit!=pdb->oomap.end();ooit++)
	{
		//bool unload=false;
		VSOrder *porder=pdb->ReadOrder(ooit.second);
		//unload=true;
		if(porder)
		{
			int oqty=porder->GetOrderQty();
			int cqty=porder->GetCumQty();
			int fqty=porder->GetFillQty();
			int pqty=oqty -fqty;
			char except[256]={0};
			if(cqty<fqty)
			{
				sprintf(except,"CumQty less than FillQty"); nexcept++;
			}
			else if(oqty<=0)
			{
				sprintf(except,"Unknown OrderQty"); nexcept++;
			}
			else if(oqty<fqty)
			{
				sprintf(except,"Overfilled or bad OrderQty"); nexcept++;
			}
			char hmt[2]={porder->GetHighMsgType(),0};
			char het[2]={porder->GetHighExecType(),0};
			fprintf(fp,"%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%d,%s,\n",
				porder->GetAppInstID(),
				porder->GetClOrdID(),
				porder->GetRootOrderID(),
				porder->GetFirstClOrdID(),
				porder->GetAccount(),
				porder->GetEcnOrderID(),
				porder->GetSymbol(),
				hmt,het,fqty,oqty,cqty,pqty,except);
			ocnt++;
			//if(unload)
			//{
			//	pdb->FreeOrder(porder); porder=0;
			//}
		}
	}
	fprintf(fp,"\nTotal %d open orders, %d exceptions\n",ocnt,nexcept);
	return 0;
}
int ViewServer::ShowFills(FILE *fp, AppSystem *asys)
{
	_ASSERT((fp)&&(asys));
	OrderDB *pdb=&asys->odb;
	fprintf(fp,"\n[%s]\n",asys->sname.c_str());
	fprintf(fp,"%d filled orders with unique ids to %d child locations\n",pdb->fomap.size(),pdb->fomap.size());
	fprintf(fp,"AppInstID,ClOrdID,RootOrderID,FirstClOrdID,Account,EcnOrderID,Symbol,HighMsgType,HighExecType,FillQty,OrderQty,CumQty,Exception,\n");
	LONGLONG toqty=0,tcqty=0,tfqty=0;
	int ocnt=0,nexcept=0;
	for(VSINDEX::iterator fit=pdb->fomap.begin();fit!=pdb->fomap.end();fit++)
	{
		//bool unload=false;
		VSOrder *porder=pdb->ReadOrder(fit.second);
		//unload=true;
		if(porder)
		{
			int oqty=porder->GetOrderQty();
			int cqty=porder->GetCumQty();
			int fqty=porder->GetFillQty();
			char except[256]={0};
			if(cqty<fqty)
			{
				sprintf(except,"CumQty less than FillQty"); nexcept++;
			}
			else if(oqty<=0)
			{
				sprintf(except,"Unknown OrderQty"); nexcept++;
			}
			else if(oqty<fqty)
			{
				sprintf(except,"Overfilled or bad OrderQty"); nexcept++;
			}
			char hmt[2]={porder->GetHighMsgType(),0};
			char het[2]={porder->GetHighExecType(),0};
			fprintf(fp,"%s,%s,%s,%s,%s,%s,%s,%s,%s,%d,%d,%d,%s,\n",
				porder->GetAppInstID(),
				porder->GetClOrdID(),
				porder->GetRootOrderID(),
				porder->GetFirstClOrdID(),
				porder->GetAccount(),
				porder->GetEcnOrderID(),
				porder->GetSymbol(),
				hmt,het,fqty,oqty,cqty,except);
			ocnt++; toqty+=oqty; tcqty+=cqty; tfqty+=fqty;
			//if(unload)
			//{
			//	pdb->FreeOrder(porder); porder=0;
			//}
		}
	}
	int fpperc=(int)(tfqty*100/(toqty?toqty:1));
	int frperc=(int)(tfqty*100/(tcqty?tcqty:1));
	fprintf(fp,"\nTotal %d filled orders, %d exceptions\n",ocnt,nexcept);
	#ifdef WIN32
	fprintf(fp,"Total %I64d shares placed on filled orders\n",toqty);
	fprintf(fp,"Total %I64d shares reported\n",tcqty);
	fprintf(fp,"Total %I64d shares in fill reports (%d%% of placed, %d%% of reported)\n",tfqty,fpperc,frperc);
	#else
	fprintf(fp,"Total %lld shares placed on filled orders\n",toqty);
	fprintf(fp,"Total %lld shares reported\n",tcqty);
	fprintf(fp,"Total %lld shares in fill reports (%d%% of placed, %d%% of reported)\n",tfqty,fpperc,frperc);
	#endif
	return 0;
}
int ViewServer::ShowInstances(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	_ASSERT(fp);
	fprintf(fp,"AppSystem,AppInstance,AcctOrderQty,AcctCumQty,AcctFillQty,AcctUpdateTime,AcctMsgCnt,\n");
	int scnt=0,icnt=0;
	for(APPSYSMAP::iterator ait=asmap.begin();ait!=asmap.end();ait++)
	{
		AppSystem *asys=ait->second;
		#ifdef WIN32
			fprintf(fp,"%s,,%I64d,%I64d,%I64d,%06d,%d\n",
		#else
			fprintf(fp,"%s,,%lld,%lld,%lld,%06d,%d\n",
		#endif
				asys->sname.c_str(),
				asys->AcctOrderQty,asys->AcctCumQty,asys->AcctFillQty,
				asys->AcctUpdateTime,asys->AcctMsgCnt);
		scnt++;
		for(APPINSTMAP::iterator iit=asys->imap.begin();iit!=asys->imap.end();iit++)
		{
			AppInstance *ainst=iit->second;
		#ifdef WIN32
			fprintf(fp,"%s,%s,%I64d,%I64d,%I64d,%06d,%d\n",
		#else
			fprintf(fp,"%s,%s,%lld,%lld,%lld,%06d,%d\n",
		#endif
				asys->sname.c_str(),ainst->iname.c_str(),
				ainst->AcctOrderQty,ainst->AcctCumQty,ainst->AcctFillQty,
				ainst->AcctUpdateTime,ainst->AcctMsgCnt);
			icnt++;
		}
	}
	fprintf(fp,"\nTotal %d systems, %d instances\n",scnt,icnt);
	ReleaseMutex(asmux);
	return 0;
}
int ViewServer::ShowAccounts(FILE *fp)
{
	WaitForSingleObject(asmux,INFINITE);
	_ASSERT(fp);
	fprintf(fp,"AcctName,AcctType,AcctOrderQty,AcctCumQty,AcctFillQty,AcctUpdateTime,AcctMsgCnt,\n");
	for(ACCTMAP::iterator ait=acmap.begin();ait!=acmap.end();ait++)
	{
		Account *acc=ait->second;
		#ifdef WIN32
		fprintf(fp,"%s,%s,%I64d,%I64d,%I64d,%06d,%d,\n",
		#else
		fprintf(fp,"%s,%s,%lld,%lld,%lld,%06d,%d,\n",
		#endif
			acc->AcctName.c_str(),
			acc->AcctType.c_str(),
			acc->AcctOrderQty,
			acc->AcctCumQty,
			acc->AcctFillQty,
			acc->AcctUpdateTime,
			acc->AcctMsgCnt);
	}
	fprintf(fp,"Total %d ViewServer accounts\n\n",acmap.size());
	ReleaseMutex(asmux);

	fprintf(fp,"tacmap_key,tacmap_domain,\n");
	for(TACCTMAP::iterator ait=tacmap.begin();ait!=tacmap.end();ait++)
	{
		string domain=ait->second.first;
		Account *pacc=ait->second.second;
		fprintf(fp,"%s,%s,\n",ait->first.c_str(),domain.c_str());
	}
	fprintf(fp,"Total %d tacmap accounts\n",tacmap.size());
	return 0;
}
