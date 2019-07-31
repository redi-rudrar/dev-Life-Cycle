
#include "stdafx.h"
#include "smcmds.h"
#include "wsocksimpl.h"
#include "ps.h"
#if defined(WIN32)||defined(_CONSOLE)
#include <shlwapi.h>
#include "wstring.h"
#else
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

SMCmds::SMCmds()
	:cnotify(0)
	,ccodec(0)
{
}
SMCmds::~SMCmds()
{
}

int SMCmds::Init(SMCmdsNotify *cnotify, SysCmdCodec *ccodec)
{
	this->cnotify=cnotify;
	this->ccodec=ccodec;
	return 0;
}
int SMCmds::Shutdown()
{
	cnotify=0;
	ccodec=0;
	return 0;
}

int SMCmds::SMUSysmonResp(int UmrPortNo, int reqid, string cmd, const char *parm, int plen)
{
	if((!ccodec)||(!cnotify))
		return -1;
	char mbuf[2048]={0},*mptr=mbuf;
	int mlen=sizeof(mbuf);
	ccodec->Encode(mptr,mlen,false,reqid,cmd,parm,plen);
	cnotify->SMCSendMsg(1109,(int)(mptr -mbuf),mbuf,WS_UMR,UmrPortNo);
	return 0;
}

int SMCmds::SMCNotifyRequest(int reqid, string cmd, const char *parm, int plen, WSPortType PortType, int PortNo)
{
	if((!ccodec)||(!cnotify))
		return -1;
	TVMAP tvmap;
	if(ccodec->GetTagValues(tvmap,parm,plen)<0)
		return -1;

	char parms[1024]={0},resp[1024]={0},*rptr=resp;
	int rlen=sizeof(resp);
	string oboi=ccodec->GetValue(tvmap,"OnBehalfOf");
	string dtid=ccodec->GetValue(tvmap,"DeliverTo");

	// Filter out common app functionality
	if(cmd=="helpcommon")
	{
		TVMAP rvmap;
		// Assume the caller may have other commands to add to the help
		//// Begin help
		//rvmap.clear();
		//ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		//ccodec->SetValue(rvmap,"DeliverTo",oboi);
		//ccodec->SetValue(rvmap,"Begin","Y");
		//ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		//SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// gettime
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","gettime");
		ccodec->SetValue(rvmap,"Desc","returns the current time");
		ccodec->SetValue(rvmap,"NumParms","0");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","gettime");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// subscribe
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","subscribe");
		ccodec->SetValue(rvmap,"Desc","sysmon and EYE feeds");
		ccodec->SetValue(rvmap,"NumParms","3");
			// Feed
			ccodec->SetValue(rvmap,"Parm1","Feed");
			ccodec->SetValue(rvmap,"ParmDef1","level1");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","level1/level2/errorlog/eventlog");
			// Hist
			ccodec->SetValue(rvmap,"Parm2","Hist");
			ccodec->SetValue(rvmap,"ParmDef2","Y");
			ccodec->SetValue(rvmap,"ParmOpt2","Y");
			ccodec->SetValue(rvmap,"ParmDesc2","Y/N");
			// Live
			ccodec->SetValue(rvmap,"Parm3","Live");
			ccodec->SetValue(rvmap,"ParmDef3","Y");
			ccodec->SetValue(rvmap,"ParmOpt3","Y");
			ccodec->SetValue(rvmap,"ParmDesc3","Y/N");
		ccodec->SetValue(rvmap,"NumSamples","2");
		ccodec->SetValue(rvmap,"Sample1","subscribe Feed=level1/Live=Y");
		ccodec->SetValue(rvmap,"SampleDesc1","");
		ccodec->SetValue(rvmap,"Sample2","subscribe Feed=errorlog/Hist=Y/Live=Y");
		ccodec->SetValue(rvmap,"SampleDesc2","");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// exit
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","exit");
		ccodec->SetValue(rvmap,"Desc","exit the app");
		ccodec->SetValue(rvmap,"NumParms","2");
			// User
			ccodec->SetValue(rvmap,"Parm1","User");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","your username");
			// Reason
			ccodec->SetValue(rvmap,"Parm2","Reason");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","reason");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","exit user=/reason=");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

        // getfilesize
        rvmap.clear();
        ccodec->SetValue(rvmap, "OnBehalfOf", dtid);
        ccodec->SetValue(rvmap, "DeliverTo", oboi);
        ccodec->SetValue(rvmap, "HelpCmd", "getfilesize");
        ccodec->SetValue(rvmap, "Desc", "get file size");
        ccodec->SetValue(rvmap, "NumParms", "1");
            // FilePath
            ccodec->SetValue(rvmap, "Parm1", "FilePath");
            ccodec->SetValue(rvmap, "ParmDef1", "");
            ccodec->SetValue(rvmap, "ParmOpt1", "N");
            ccodec->SetValue(rvmap, "ParmDesc1", "relative to app run dir");
        ccodec->SetValue(rvmap, "NumSamples", "1");
        ccodec->SetValue(rvmap, "Sample1", "getfilesize FilePath=<path>");
        ccodec->SetTagValues(rvmap, parms, sizeof(parms));
        SMUSysmonResp(PortNo, reqid, cmd, parms, 0);

		// deletefile
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","deletefile");
		ccodec->SetValue(rvmap,"Desc","deletes a file");
		ccodec->SetValue(rvmap,"NumParms","1");
			// FilePath
			ccodec->SetValue(rvmap,"Parm1","FilePath");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","relative to app run dir");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","deletefile FilePath=<path>");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// mkdir
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","mkdir");
		ccodec->SetValue(rvmap,"Desc","creates a directory");
		ccodec->SetValue(rvmap,"NumParms","1");
			// DirPath
			ccodec->SetValue(rvmap,"Parm1","DirPath");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","relative to app run dir");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","mkdir DirPath=<path>");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// rmdir
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","rmdir");
		ccodec->SetValue(rvmap,"Desc","removes a directory");
		ccodec->SetValue(rvmap,"NumParms","1");
			// DirPath
			ccodec->SetValue(rvmap,"Parm1","DirPath");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","relative to app run dir");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","rmdir DirPath=<path>");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// getosinfo
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","getosinfo");
		ccodec->SetValue(rvmap,"Desc","returns OS version and number of CPUs");
		ccodec->SetValue(rvmap,"NumParms","0");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","getosinfo");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// gethostinfo
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","gethostinfo");
		ccodec->SetValue(rvmap,"Desc","returns hostname, ip, etc.");
		ccodec->SetValue(rvmap,"NumParms","0");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","gethostinfo");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// browsefiles
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","browsefiles");
		ccodec->SetValue(rvmap,"Desc","browse a directory");
		ccodec->SetValue(rvmap,"NumParms","6");
			// FileKey
			ccodec->SetValue(rvmap,"Parm1","FileKey");
			ccodec->SetValue(rvmap,"ParmDef1","browse.txt");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","local file name");
			// Dir
			ccodec->SetValue(rvmap,"Parm2","Dir");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","remote directory");
			// Match
			ccodec->SetValue(rvmap,"Parm3","Match");
			ccodec->SetValue(rvmap,"ParmDef3","*.*");
			ccodec->SetValue(rvmap,"ParmOpt3","N");
			ccodec->SetValue(rvmap,"ParmDesc3","wildcard pattern");
			// Recurse
			ccodec->SetValue(rvmap,"Parm4","Recurse");
			ccodec->SetValue(rvmap,"ParmDef4","Y");
			ccodec->SetValue(rvmap,"ParmOpt4","Y");
			ccodec->SetValue(rvmap,"ParmDesc4","Y/N");
			// Format
			ccodec->SetValue(rvmap,"Parm5","Format");
			ccodec->SetValue(rvmap,"ParmDef5","1");
			ccodec->SetValue(rvmap,"ParmOpt5","Y");
			ccodec->SetValue(rvmap,"ParmDesc5","1-dir, 2-dir \\/w, 3-FindFile");
			// NewerThan
			ccodec->SetValue(rvmap,"Parm6","NewerThan");
			ccodec->SetValue(rvmap,"ParmDef6","20120000");
			ccodec->SetValue(rvmap,"ParmOpt6","Y");
			ccodec->SetValue(rvmap,"ParmDesc6","yyyymmdd or +/-# days");
		ccodec->SetValue(rvmap,"NumSamples","4");
		ccodec->SetValue(rvmap,"Sample1","browsefiles FileKey=browse.txt/Dir=setup/Match=*.*/Format=1");
		ccodec->SetValue(rvmap,"Sample2","browsefiles FileKey=browse.txt/Dir=logs/Match=*.*/Format=1");
		ccodec->SetValue(rvmap,"Sample3","browsefiles FileKey=browse.txt/Dir=./Match=*.*/Format=1");
		ccodec->SetValue(rvmap,"Sample4","browsefiles FileKey=browse.txt/Dir=./Match=*.*/Format=1/NewerThan=-1");
		ccodec->SetValue(rvmap,"SampleDesc4","Files last written since yesterday");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// downloadfile
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","downloadfile");
		ccodec->SetValue(rvmap,"Desc","download a file");
		ccodec->SetValue(rvmap,"NumParms","7");
			// FileKey
			ccodec->SetValue(rvmap,"Parm1","FileKey");
			ccodec->SetValue(rvmap,"ParmDef1","download.tmp");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","local file name");
			// FilePath
			ccodec->SetValue(rvmap,"Parm2","FilePath");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","remote file name or file list");
			// Begin
			ccodec->SetValue(rvmap,"Parm3","Begin");
			ccodec->SetValue(rvmap,"ParmDef3","0");
			ccodec->SetValue(rvmap,"ParmOpt3","Y");
			ccodec->SetValue(rvmap,"ParmDesc3","begin offset");
			// End
			ccodec->SetValue(rvmap,"Parm4","End");
			ccodec->SetValue(rvmap,"ParmDef4","-1");
			ccodec->SetValue(rvmap,"ParmOpt4","Y");
			ccodec->SetValue(rvmap,"ParmDesc4","end offset");
			// Size
			ccodec->SetValue(rvmap,"Parm5","Size");
			ccodec->SetValue(rvmap,"ParmDef5","-1");
			ccodec->SetValue(rvmap,"ParmOpt5","Y");
			ccodec->SetValue(rvmap,"ParmDesc5","bytes to transfer");
            // MaxRate
            ccodec->SetValue(rvmap,"Parm6","MaxRate");
            ccodec->SetValue(rvmap,"ParmDef6","2MB");
            ccodec->SetValue(rvmap,"ParmOpt6","Y");
            ccodec->SetValue(rvmap,"ParmDesc6","transfer bytes/sec. use B/KB/MB/MAX. 2MB by default.");
            // FileList
            ccodec->SetValue(rvmap,"Parm7","FileList");
            ccodec->SetValue(rvmap,"ParmDef7","Y");
            ccodec->SetValue(rvmap,"ParmOpt7","Y");
            ccodec->SetValue(rvmap,"ParmDesc7","Y/N");
		ccodec->SetValue(rvmap,"NumSamples","4");
		ccodec->SetValue(rvmap,"Sample1","downloadfile FileKey=ports.txt/FilePath=setup\\\\ports.txt");
		ccodec->SetValue(rvmap,"SampleDesc1","ports.txt");
		ccodec->SetValue(rvmap,"Sample2","downloadfile FileKey=setup.ini/FilePath=setup\\\\setup.ini");
		ccodec->SetValue(rvmap,"SampleDesc2","setup.ini");
		ccodec->SetValue(rvmap,"Sample3","downloadfile FileKey=app.ini/FilePath=setup\\\\app.ini");
		ccodec->SetValue(rvmap,"SampleDesc3","app.ini");
		ccodec->SetValue(rvmap,"Sample4","downloadfile FileKey=eve.txt/FilePath=logs\\\\20090000eve.txt");
		ccodec->SetValue(rvmap,"SampleDesc4","eventlog");
		// This exceeds the help size limit
		//ccodec->SetValue(rvmap,"Sample5","downloadfile FileKey=*/FilePath=SysmonTempFiles\\\\filelist.txt/FileList=Y");
		//ccodec->SetValue(rvmap,"SampleDesc5","download by list file");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// uploadfile
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","uploadfile");
		ccodec->SetValue(rvmap,"Desc","upload a file");
		ccodec->SetValue(rvmap,"NumParms","3");
			// SrcPath
			ccodec->SetValue(rvmap,"Parm1","SrcPath");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","local file name");
			// FilePath
			ccodec->SetValue(rvmap,"Parm2","FilePath");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","remote file name");
			// Overwrite
			ccodec->SetValue(rvmap,"Parm3","Overwrite");
			ccodec->SetValue(rvmap,"ParmDef3","Y");
			ccodec->SetValue(rvmap,"ParmOpt3","Y");
			ccodec->SetValue(rvmap,"ParmDesc3","Y/N");
		ccodec->SetValue(rvmap,"NumSamples","3");
		ccodec->SetValue(rvmap,"Sample1","uploadfile SrcPath=SysmonTempFiles\\\\ports.txt/FilePath=setup\\\\ports.txt/Overwrite=Y");
		ccodec->SetValue(rvmap,"SampleDesc1","ports.txt");
		ccodec->SetValue(rvmap,"Sample2","uploadfile SrcPath=SysmonTempFiles\\\\setup.ini/FilePath=setup\\\\setup.ini/Overwrite=Y");
		ccodec->SetValue(rvmap,"SampleDesc2","setup.ini");
		ccodec->SetValue(rvmap,"Sample3","uploadfile SrcPath=SysmonTempFiles\\\\app.ini/FilePath=setup\\\\app.ini/Overwrite=Y");
		ccodec->SetValue(rvmap,"SampleDesc3","app.ini");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// copyfile
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","copyfile");
		ccodec->SetValue(rvmap,"Desc","remote file copy");
		ccodec->SetValue(rvmap,"NumParms","3");
			// SrcFile
			ccodec->SetValue(rvmap,"Parm1","SrcFile");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","file to copy from");
			// DstFile
			ccodec->SetValue(rvmap,"Parm2","DstFile");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","file to copy to");
			// Overwrite
			ccodec->SetValue(rvmap,"Parm3","Overwrite");
			ccodec->SetValue(rvmap,"ParmDef3","Y");
			ccodec->SetValue(rvmap,"ParmOpt3","Y");
			ccodec->SetValue(rvmap,"ParmDesc3","Y/N");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","copyfile SrcFile=<src>/DstFile=<dst>/Overwrite=N");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

        // movefile
        rvmap.clear();
        ccodec->SetValue(rvmap, "OnBehalfOf", dtid);
        ccodec->SetValue(rvmap, "DeliverTo", oboi);
        ccodec->SetValue(rvmap, "HelpCmd", "movefile");
        ccodec->SetValue(rvmap, "Desc", "remote file move");
        ccodec->SetValue(rvmap, "NumParms", "2");
			// SrcFile
			ccodec->SetValue(rvmap,"Parm1","SrcFile");
			ccodec->SetValue(rvmap,"ParmDef1","");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","file to move from");
			// DstFile
			ccodec->SetValue(rvmap,"Parm2","DstFile");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","file to move to");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","movefile SrcFile=<src>/DstFile=<dst>");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// enumdrives
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","enumdrives");
		ccodec->SetValue(rvmap,"Desc","local and mapped drives");
		ccodec->SetValue(rvmap,"NumParms","1");
			// FileKey
			ccodec->SetValue(rvmap,"Parm1","FileKey");
			ccodec->SetValue(rvmap,"ParmDef1","enumdrives.csv");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","local file name");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","enumdrives FileKey=enumdrives.csv");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// ps
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","ps");
		ccodec->SetValue(rvmap,"Desc","process list");
		ccodec->SetValue(rvmap,"NumParms","1");
			// FileKey
			ccodec->SetValue(rvmap,"Parm1","FileKey");
			ccodec->SetValue(rvmap,"ParmDef1","ps.csv");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","local file name");
		ccodec->SetValue(rvmap,"NumSamples","1");
		ccodec->SetValue(rvmap,"Sample1","ps FileKey=ps.csv");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		// shell
		rvmap.clear();
		ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		ccodec->SetValue(rvmap,"DeliverTo",oboi);
		ccodec->SetValue(rvmap,"HelpCmd","shell");
		ccodec->SetValue(rvmap,"Desc","remote shell");
		ccodec->SetValue(rvmap,"NumParms","2");
			// FileKey
			ccodec->SetValue(rvmap,"Parm1","FileKey");
			ccodec->SetValue(rvmap,"ParmDef1","shell.txt");
			ccodec->SetValue(rvmap,"ParmOpt1","N");
			ccodec->SetValue(rvmap,"ParmDesc1","local file name");
			// ShellCmd
			ccodec->SetValue(rvmap,"Parm2","ShellCmd");
			ccodec->SetValue(rvmap,"ParmDef2","");
			ccodec->SetValue(rvmap,"ParmOpt2","N");
			ccodec->SetValue(rvmap,"ParmDesc2","shell command");
		ccodec->SetValue(rvmap,"NumSamples","2");
		ccodec->SetValue(rvmap,"Sample1","shell FileKey=shell.txt/ShellCmd=net use");
		ccodec->SetValue(rvmap,"Sample2","shell FileKey=shell.txt/ShellCmd=ipconfig \\/all");
		ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		SMUSysmonResp(PortNo,reqid,cmd,parms,0);

		//// end help
		//rvmap.clear();
		//ccodec->SetValue(rvmap,"OnBehalfOf",dtid);
		//ccodec->SetValue(rvmap,"DeliverTo",oboi);
		//ccodec->SetValue(rvmap,"End","Y");
		//ccodec->SetTagValues(rvmap,parms,sizeof(parms));
		//SMUSysmonResp(PortNo,reqid,cmd,parms,0);
		return 1;
	}
	// Get date and time
	else if(cmd=="gettime")
	{
	#ifdef WIN32
		SYSTEMTIME tsys;
		GetLocalTime(&tsys);		
		char tzstr[32]={0};
		time_t tnow=time(0);
		tm *ltm=localtime(&tnow);
		if(ltm)
			strftime(tzstr,32,"%z",ltm);
		int wsdate=(tsys.wYear*10000) +(tsys.wMonth*100) +(tsys.wDay);
		int wstime=(tsys.wHour*10000) +(tsys.wMinute*100) +(tsys.wSecond);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Date=%04d%02d%02d|Time=%02d%02d%02d.%03d|Tick=%u|TimeZone=%s|WSDate=%08d|WSTime=%06d|",
			dtid.c_str(),oboi.c_str(),
			tsys.wYear,tsys.wMonth,tsys.wDay,
			tsys.wHour,tsys.wMinute,tsys.wSecond,tsys.wMilliseconds,
			GetTickCount(),tzstr,wsdate,wstime);
	#else
		char tzstr[32]={0};
		time_t tnow=time(0);
		tm *ltm=localtime(&tnow);
		if(ltm)
			strftime(tzstr,32,"%z",ltm);
		int wsdate=::WSDate();
		int wstime=::WSTime();
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Date=%08d|Time=%06d|Tick=%u|TimeZone=%s|WSDate=%08d|WSTime=%06d|",
			dtid.c_str(),oboi.c_str(),
			wsdate,wstime,GetTickCount(),tzstr,wsdate,wstime);
	#endif
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
    else if(cmd=="getfilesize")
    {
        string filePath = ccodec->GetValue(tvmap, "FilePath");
        if(filePath.empty())
        {
            sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'FilePath' parameter!|",
                dtid.c_str(), oboi.c_str());
            ccodec->Encode(rptr, sizeof(resp), false, reqid, cmd, parms, 0);
            cnotify->SMCSendMsg(1109, (int)(rptr - resp), resp, PortType,PortNo);
            return(1);
        }
    #ifdef WIN32
        HANDLE hFile = CreateFile(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
        if(hFile == INVALID_HANDLE_VALUE)
        {
            char szReason[256];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
            size_t nLengthReason = strlen(szReason);
            while(szReason[0] && strchr("\r\n", szReason[nLengthReason - 1]))
                szReason[--nLengthReason] = 0;
            sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|FilePath=%s|Reason=Failed to open the file: %s|",
                dtid.c_str(), oboi.c_str(), filePath.c_str(), szReason);
            ccodec->Encode(rptr, sizeof(resp), false, reqid, cmd, parms, 0);
            cnotify->SMCSendMsg(1109, (int)(rptr - resp), resp, PortType,PortNo);
            return(1);
        }
        LARGE_INTEGER iFileSize;
        if(!GetFileSizeEx(hFile, &iFileSize))
        {
            char szReason[256];
            FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, 0, GetLastError(), 0, szReason, sizeof(szReason), 0);
            size_t nLengthReason = strlen(szReason);
            while(szReason[0] && strchr("\r\n", szReason[nLengthReason - 1]))
                szReason[--nLengthReason] = 0;
            sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|FilePath=%s|Reason=Failed to get file size: %s|",
                dtid.c_str(), oboi.c_str(), filePath.c_str(), szReason);
            ccodec->Encode(rptr, sizeof(resp), false, reqid, cmd, parms, 0);
            cnotify->SMCSendMsg(1109, (int)(rptr - resp), resp, PortType,PortNo);
            CloseHandle(hFile);
            return(1);
        }
        CloseHandle(hFile);
        unsigned __int64 sizeFile = iFileSize.QuadPart;
        sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=0|FilePath=%s|FileSize=%I64u|",
    #else
        FILE *pfile = fopen(filePath.c_str(), "rb");
        if(!pfile)
        {
            sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|FilePath=%s|Reason=Get file size failed|",
                dtid.c_str(), oboi.c_str(), filePath.c_str());
            ccodec->Encode(rptr, sizeof(resp), false, reqid, cmd, parms, 0);
            cnotify->SMCSendMsg(1109, (int)(rptr - resp), resp, PortType,PortNo);
            return(1);
        }
        fseek(pfile, 0, SEEK_END);
        size_t sizeFile = ftell(pfile);
        fclose(pfile);
        sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=0|FilePath=%s|FileSize=%u|",
    #endif
            dtid.c_str(), oboi.c_str(), filePath.c_str(), sizeFile);
        ccodec->Encode(rptr, sizeof(resp), false, reqid, cmd, parms, 0);
        cnotify->SMCSendMsg(1109, (int)(rptr - resp), resp, PortType,PortNo);
        return(1);
    }
	// Delete file
	else if(cmd=="deletefile")
	{
		string filePath=ccodec->GetValue(tvmap,"FilePath");
		if(filePath.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'FilePath' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
			return 1;
		}
		char fpath[MAX_PATH]={0},*fname=0;
		GetFullPathNameEx(filePath.c_str(),MAX_PATH,fpath,&fname);
	#ifdef WIN32
		if(DeleteFile(fpath))
	#else
		unlink(fpath);
		if(!PathFileExists(fpath))
	#endif
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Delete file (%s) successful|",
				dtid.c_str(),oboi.c_str(),fpath);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Reason=Delete file (%s) failed|",
				dtid.c_str(),oboi.c_str(),GetLastError(),fpath);
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
	// Create directory
	else if(cmd=="mkdir")
	{
		string dirPath=ccodec->GetValue(tvmap,"DirPath");
		if(dirPath.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'DirPath' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
			return 1;
		}
		char dpath[MAX_PATH]={0},*dname=0;
		GetFullPathNameEx(dirPath.c_str(),MAX_PATH,dpath,&dname);
		if(CreateDirectory(dpath,0))
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Create directory (%s) successful|",
				dtid.c_str(),oboi.c_str(),dpath);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Reason=Create directory (%s) failed|",
				dtid.c_str(),oboi.c_str(),GetLastError(),dpath);
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
	// Remove directory
	else if(cmd=="rmdir")
	{
		string dirPath=ccodec->GetValue(tvmap,"DirPath");
		if(dirPath.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'DirPath' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
			return 1;
		}
		char dpath[MAX_PATH]={0},*dname=0;
		GetFullPathNameEx(dirPath.c_str(),MAX_PATH,dpath,&dname);
	#if defined(WIN32)||defined(_CONSOLE)
		if(RemoveDirectory(dpath))
	#else
		rmdir(dpath);
		if(!PathFileExists(dpath))
	#endif
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Remove directory (%s) successful|",
				dtid.c_str(),oboi.c_str(),dpath);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Reason=Remove directory (%s) failed|",
				dtid.c_str(),oboi.c_str(),GetLastError(),dpath);
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
	// Upload file
	else if(cmd=="uploadfile")
	{
		string srcpath=ccodec->GetValue(tvmap,"SrcPath");
		if(srcpath.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'SrcPath' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
			return 1;
		}
		string filepath=ccodec->GetValue(tvmap,"FilePath");
		if(filepath.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Requires 'FilePath' parameter!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
			return 1;
		}
		string overwrite=ccodec->GetValue(tvmap,"Overwrite");
		char fpath[MAX_PATH]={0},*fname=0;
		GetFullPathNameEx(filepath.c_str(),MAX_PATH,fpath,&fname);
		if((PathFileExists(fpath))&&(overwrite!="Y")&&(overwrite!="y"))
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=FilePath (%s) exists, requires Overwrite=Y|",
				dtid.c_str(),oboi.c_str(),fpath);
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
			return 1;
		}	
		if(cnotify->SMCBeginUpload(srcpath.c_str(),oboi.c_str(),dtid.c_str(),reqid,fpath,PortType,PortNo,0)<0)
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Upload (%s) failed|",
				dtid.c_str(),oboi.c_str(),fpath);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Uploading (%s)...|",
				dtid.c_str(),oboi.c_str(),fpath);
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
	// OS Information
	else if(cmd=="getosinfo")
	{
	#if defined(WIN32)||defined(_CONSOLE)
		OSVERSIONINFO osi;
		memset(&osi,0,sizeof(OSVERSIONINFO));
		osi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
		GetVersionEx(&osi);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|MajVer=%d|MinVer=%d|Build=%d|Platform=%d|Spver=%s|",
			dtid.c_str(),oboi.c_str(),osi.dwMajorVersion,osi.dwMinorVersion,osi.dwBuildNumber,osi.dwPlatformId,osi.szCSDVersion);
	#else
		char scmd[1024]={0};
		char tpath[MAX_PATH]={0},*fname=0;
		GetFullPathNameEx("SysmonTempFiles/uname.txt",MAX_PATH,tpath,&fname);
		sprintf(scmd,"uname -a > %s",tpath);
		system(scmd);
		char ubuf[1024]={0};
		FILE *fp=fopen(tpath,"rt");
		if(fp)
		{
			fgets(ubuf,sizeof(ubuf),fp);
			fclose(fp);
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Uname=%s|",dtid.c_str(),oboi.c_str(),ubuf);
	#endif
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
	// Host Information
	else if(cmd=="gethostinfo")
	{
		char dnsname[256]={0},nbname[256]={0},ipstr[32]={0};
		DWORD nblen=256;
		gethostname(dnsname,256);
		HOSTENT *phe=gethostbyname(dnsname);
		if(phe)
			strcpy(ipstr,inet_ntoa(*(IN_ADDR*)phe->h_addr_list[0]));
		//GetComputerNameEx(ComputerNameDnsFullyQualified,dnsname,256);

	#if defined(WIN32)||defined(_CONSOLE)
		GetComputerName(nbname,&nblen);
		//GetComputerNameEx(ComputerNameNetBIOS,nbname,256);

		SYSTEM_INFO isys;
		memset(&isys,0,sizeof(SYSTEM_INFO));
		GetSystemInfo(&isys);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|DnsName=%s|NetbiosName=%s|Ip=%s|Cpus=%d|",
			dtid.c_str(),oboi.c_str(),dnsname,nbname,ipstr,isys.dwNumberOfProcessors);
	#else
		int ncpus=0;
		char scmd[1024]={0};
		char tpath[MAX_PATH]={0},*fname=0;
		GetFullPathNameEx("SysmonTempFiles/cpuinfo",MAX_PATH,tpath,&fname);
		sprintf(scmd,"cp /proc/cpuinfo %s",tpath);
		system(scmd);
		char rbuf[1024]={0};
		FILE *fp=fopen(tpath,"rt");
		if(fp)
		{
			while(fgets(rbuf,sizeof(rbuf),fp))
			{
				if(!strncmp(rbuf,"processor",9))
					ncpus++;
			}
			fclose(fp);
		}
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|DnsName=%s|Cpus=%d|",
			dtid.c_str(),oboi.c_str(),dnsname,ncpus);
	#endif
		ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
		cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		return 1;
	}
	// These commands run in a separate thread so they won't hold up the app
	else if((cmd=="browsefiles")||
			(cmd=="downloadfile")||
			(cmd=="copyfile")||
            (cmd=="movefile")||
			(cmd=="enumdrives")||
			//(cmd=="enumnics")||
			(cmd=="ps")||
			(cmd=="shell"))
	{
		// DeliverTo stays the same since this is loopback (i.e. <dest> to <dest>)
		//ccodec->SetValue(tvmap,"OnBehalfOf",noboi); // OnBehalfOf gets bigger (i.e. <src> to <src>/LBM.#)
		//if(BeginThreadedCmd(reqid,cmd,reqid,noboi,dtid,parm,plen,PortType,PortNo,60000)<0)
		// Since these commands are handled by LBMonitor, don't add /LBM.# to oboi
		if(BeginThreadedCmd(reqid,cmd,reqid,oboi,dtid,parm,plen,PortType,PortNo,60000)<0)
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Failed starting cmd thread!|",
				dtid.c_str(),oboi.c_str());
			ccodec->Encode(rptr,sizeof(resp),false,reqid,cmd,parms,0);
			cnotify->SMCSendMsg(1109,(int)(rptr -resp),resp,PortType,PortNo);
		}
        return 1;
	}
	// Not handled
	return 0;
}

#ifdef WIN32
int SMCmds::MultiFileSend(WSThreadedCmdInfo *pdata, const char *fdir, const char *fmatch, bool recurse, DWORD& msprev, DWORD& sentLastFile)
{
	int nsent=0;
	LARGE_INTEGER fsize;
	fsize.QuadPart=0;
	char mpath[MAX_PATH]={0},tpath[MAX_PATH]={0};
	sprintf(mpath,"%s\\%s",fdir,fmatch);
	WIN32_FIND_DATA fdata;
	HANDLE fhnd=FindFirstFile(mpath,&fdata);
	list<WIN32_FIND_DATA> dlist;
	if(fhnd!=INVALID_HANDLE_VALUE)
	{
		// Throttle between files at max rate
		if(!msprev)
			msprev=GetTickCount();
		do
		{
			if((!strcmp(fdata.cFileName,"."))||(!strcmp(fdata.cFileName,"..")))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
				dlist.push_back(fdata);
			else
			{
				// Return relative path for multi-transfer
				sprintf(tpath,"%s\\%s",fdir,fdata.cFileName);
				if(!strincmp(tpath,pdata->mdir,pdata->mdlen))
					memmove(tpath,tpath +pdata->mdlen,strlen(tpath +pdata->mdlen)+1);
				LARGE_INTEGER fsize;
				fsize.HighPart=fdata.nFileSizeHigh;
				fsize.LowPart=fdata.nFileSizeLow;
				// Always send entire file for wildcards
				pdata->fileOffset=pdata->fileBegin=0;
				pdata->fileBytes=fsize.QuadPart;
				while(true)
				{
					// If the file is larger than max rate, then send nicely
					bool throttle=false;
					__int64 sendMax=pdata->fileBegin +pdata->fileBytes -pdata->fileOffset;
					if(sendMax>pdata->bytesMaxRate)
					{
						sendMax=pdata->bytesMaxRate; throttle=true;
					}
					DWORD sendBytes=(sendMax<ULONG_MAX) ?(DWORD)sendMax :ULONG_MAX;

					int rc=pdata->pSMCmds->cnotify->SMCSendFile(pdata->fileKey.c_str(),pdata->dtid.c_str(),pdata->oboi.c_str(),
						pdata->reqid,tpath,pdata->PortType,pdata->PortNo,
						pdata->fileOffset,pdata->fileBytes,pdata->fileBytes,sendBytes,true);
					if(rc<0)
						return rc;
					// Throttle this file at max rate
					DWORD msnow=GetTickCount();
					if(throttle)
					{
						DWORD mssleep=(msprev +1000>msnow) ?msprev +1000 -msnow :0;
						#ifdef WIN32
						SleepEx(mssleep,true);
						#else
						// sw debug TODO: Linux Version: Sleep millisecSleep milli seconds
						#endif
						msnow=GetTickCount();
					}
					msprev=msnow;

					pdata->fileOffset+=sendBytes;
					if(pdata->fileOffset>=pdata->fileBegin +pdata->fileBytes)
					{
						sentLastFile+=sendBytes; nsent++;
						break;
					}
					sentLastFile=0;
				}
			}
		}while(FindNextFile(fhnd,&fdata));
		FindClose(fhnd);
	}
	// Breadth-first recursion
	for(list<WIN32_FIND_DATA>::iterator dit=dlist.begin();dit!=dlist.end();dit++)
	{
		fdata=*dit;
		sprintf(tpath,"%s\\%s",fdir,fdata.cFileName);
		if(recurse)
		{
			int rc=MultiFileSend(pdata,tpath,fmatch,recurse,msprev,sentLastFile);
			if(rc<0)
				return rc;
			else
				nsent+=rc;
		}
	}
	dlist.clear();
	return nsent;
}
int SMCmds::FileListSend(WSThreadedCmdInfo *pdata, const char *fileList)
{
	int nsent=0;
	FILE *fp=fopen(fileList,"rt");
	if(!fp)
		return -1;
	// Throttle between files at max rate
	DWORD msprev=GetTickCount(),sentLastFile=0;
	char rbuf[1024]={0};
	while(fgets(rbuf,sizeof(rbuf),fp))
	{
		char *rptr=rbuf;
		for(;(*rptr)&&(isspace(*rptr));rptr++)
			;
		if((!*rptr)||(*rptr=='\r')||(*rptr=='\n')||(!strncmp(rptr,"//",2)))
			continue;
		char *eptr=(char*)strendl(rptr,rptr +strlen(rptr));
		if(eptr) *eptr=0;
		char *tpath=rptr;
		// Return relative path for multi-transfer
		if(!strincmp(tpath,pdata->mdir,pdata->mdlen))
			memmove(tpath,tpath +pdata->mdlen,strlen(tpath +pdata->mdlen)+1);
		WIN32_FIND_DATA fdata;
		HANDLE fhnd=FindFirstFile(tpath,&fdata);
		if(fhnd!=INVALID_HANDLE_VALUE)
		{
			LARGE_INTEGER fsize;
			fsize.HighPart=fdata.nFileSizeHigh;
			fsize.LowPart=fdata.nFileSizeLow;
			// Always send entire file for file list
			pdata->fileOffset=pdata->fileBegin=0;
			pdata->fileBytes=fsize.QuadPart;
			FindClose(fhnd);
		}
		while(true)
		{
			// If the file is larger than max rate, then send nicely
			bool throttle=false;
			__int64 sendMax=pdata->fileBegin +pdata->fileBytes -pdata->fileOffset;
			if(sendMax>pdata->bytesMaxRate)
			{
				sendMax=pdata->bytesMaxRate; throttle=true;
			}
			DWORD sendBytes=(sendMax<ULONG_MAX) ?(DWORD)sendMax :ULONG_MAX;
			
			int rc=pdata->pSMCmds->cnotify->SMCSendFile(pdata->fileKey.c_str(),pdata->dtid.c_str(),pdata->oboi.c_str(),
				pdata->reqid,tpath,pdata->PortType,pdata->PortNo,
				pdata->fileOffset,pdata->fileBytes,pdata->fileBytes,sendBytes,true);
			if(rc<0)
			{
				fclose(fp); return rc;
			}
			// Throttle this file at max rate
			DWORD msnow=GetTickCount();
			if(throttle)
			{
				DWORD mssleep=(msprev +1000>msnow) ?msprev +1000 -msnow :0;
				#ifdef WIN32
				SleepEx(mssleep,true);
				#else
				// sw debug TODO: Linux Version: Sleep millisecSleep milli seconds
				#endif
				msnow=GetTickCount();
			}
			msprev=msnow;

			pdata->fileOffset+=sendBytes;
			if(pdata->fileOffset>=pdata->fileBegin +pdata->fileBytes)
			{
				sentLastFile+=sendBytes; nsent++;
				break;
			}
			sentLastFile=0;
		}
	}
	fclose(fp);
	return nsent;
}
DWORD WINAPI SMCmds::BootThreadedCmd(LPVOID arg)
#else
void *SMCmds::BootThreadedCmd(void *arg)
#endif
{
	SMCmds::WSThreadedCmdInfo *pdata=(SMCmds::WSThreadedCmdInfo *)arg;
	pdata->pmod->WSHThreadedCmdHandler(pdata->pmod,pdata);
	pdata->doneCommandHandling = true;
	// Return reply
	if((!pdata->fileKey.empty())&&((pdata->PortType==WS_UMR)||(pdata->PortType==WS_USR)))
	{
		// downloadfile or other commands returning a file
		if(pdata->fpath[0])
		{
		#if defined(WIN32)||defined(_CONSOLE)
			// File list download
			if(pdata->fileList[0])
				FileListSend(pdata,pdata->fileList);
			// Wildcard download
			else if(pdata->multiFile)
			{
				char fdir[MAX_PATH]={0},*fptr=0;
				GetFullPathName(pdata->fpath,sizeof(fdir),fdir,&fptr);
				DWORD msprev=0,sentLastFile=0;
				if(fptr) 
				{
					fptr[-1]=0;
					MultiFileSend(pdata,fdir,fptr,pdata->recurse,msprev,sentLastFile);
				}
				else
				{
					MultiFileSend(pdata,".\\",fdir,pdata->recurse,msprev,sentLastFile);
				}
			}
			// Single file download
			else
			{
				LARGE_INTEGER fsize;
				fsize.QuadPart=0;
				WIN32_FIND_DATA fdata;
				HANDLE fhnd=FindFirstFile(pdata->fpath,&fdata);
				if(fhnd!=INVALID_HANDLE_VALUE)
				{
					fsize.HighPart=fdata.nFileSizeHigh;
					fsize.LowPart=fdata.nFileSizeLow;
					if(pdata->fileBytes<0)
					{
						pdata->fileBegin=fsize.QuadPart +pdata->fileBytes;
						if(pdata->fileBegin<0) 
						{
							pdata->fileBegin=0; pdata->fileBytes=fsize.QuadPart;
						}
						else
						{
							pdata->fileOffset=pdata->fileBegin;
							pdata->fileBytes=-pdata->fileBytes;
						}
					}
					if(!pdata->fileBytes)
                        pdata->fileBytes=fsize.QuadPart -pdata->fileBegin;
					FindClose(fhnd);
				}
				DWORD msprev=GetTickCount();
				while(true)
				{
					// If the file is larger than max rate, then send nicely
					bool throttle=false;
					__int64 sendMax=pdata->fileBegin +pdata->fileBytes -pdata->fileOffset;
					if(sendMax>pdata->bytesMaxRate)
					{
						sendMax=pdata->bytesMaxRate; throttle=true;
					}
					DWORD sendBytes=(sendMax<ULONG_MAX) ?(DWORD)sendMax :ULONG_MAX;
					if(pdata->pSMCmds->cnotify->SMCSendFile(pdata->fileKey.c_str(),pdata->dtid.c_str(),pdata->oboi.c_str(),
						pdata->reqid,pdata->fpath,pdata->PortType,pdata->PortNo,
						pdata->fileOffset,pdata->fileBytes,fsize.QuadPart,sendBytes,false)<0)
						break;
					// Throttle this file at max rate
					DWORD msnow=GetTickCount();
					if(throttle)
					{
						DWORD mssleep=(msprev +1000>msnow) ?msprev +1000 -msnow :0;
						#ifdef WIN32
						SleepEx(mssleep,true);
						#else
						// sw debug TODO: Linux Version: Sleep millisecSleep milli seconds
						#endif
						msnow=GetTickCount();
					}
					msprev=msnow;

					pdata->fileOffset+=sendBytes;
					if(pdata->fileOffset>=pdata->fileBegin +pdata->fileBytes)
						break;
				}
			}
		#else
			struct stat fst;
			stat(pdata->fpath,&fst);
			LONGLONG fsize=fst.st_size;
			if(pdata->fileBytes<0)
			{
				pdata->fileBegin=fsize +pdata->fileBytes;
				if(pdata->fileBegin<0) 
				{
					pdata->fileBegin=0; pdata->fileBytes=fsize;
				}
				else
				{
					pdata->fileOffset=pdata->fileBegin;
					pdata->fileBytes=-pdata->fileBytes;
				}
                if(!pdata->fileBytes)
                    pdata->fileBytes = fsize;
			}
			// If the file is larger than max rate, then send nicely
			DWORD sendBytes=(DWORD)(pdata->fileBegin +pdata->fileBytes -pdata->fileOffset);
            if(sendBytes>pdata->bytesMaxRate)
                sendBytes=pdata->bytesMaxRate;
			pdata->pSMCmds->cnotify->SMCSendFile(pdata->fileKey.c_str(),pdata->dtid.c_str(),pdata->oboi.c_str(),
				pdata->reqid,pdata->fpath,pdata->PortType,pdata->PortNo,
				pdata->fileOffset,pdata->fileBytes,fsize,sendBytes);
			pdata->fileOffset+=sendBytes;
			if(pdata->fileOffset<pdata->fileBegin +pdata->fileBytes)
                continue;
		#endif
		}
	}
	pdata->pSMCmds->cnotify->SMCSendMsg(1109,(int)(pdata->rptr -pdata->resp),pdata->resp,pdata->PortType,pdata->PortNo);
	// Wait till everything gets sent before we exit this thread.
	// Otherwise overlapped send operations may abort, triggering UMR port closure!
	while(!pdata->pSMCmds->cnotify->SMCSendFinished(pdata->PortType,pdata->PortNo,0))
		SleepEx(15,true);
    pdata->doneTransferring = true;
	#if !defined(_CONSOLE) && !defined(WIN32)
	pthread_exit((void*)0);
	#endif
	return 0;
}
int SMCmds::BeginThreadedCmd(int cmdid, string cmd, int reqid, string dtid, string oboi, const char *parm, int plen, WSPortType PortType, int PortNo, DWORD timeout)
{
	if((!ccodec)||(!cnotify))
		return -1;
	WSThreadedCmdInfo *pdata=new WSThreadedCmdInfo;
    pdata->pSMCmds=this;
	pdata->cmdid=cmdid;
	pdata->cmd=cmd;
	pdata->reqid=reqid;
	pdata->dtid=dtid;
	pdata->oboi=oboi;
	if(plen>0)
	{
		pdata->parm=new char[plen];
		memcpy(pdata->parm,parm,plen);
		pdata->plen=plen;
	}
	else
	{
		pdata->parm=0;
		pdata->plen=0;
	}
	pdata->PortType=PortType;
	pdata->PortNo=PortNo;
	pdata->tstart=GetTickCount();
	pdata->timeout=timeout;
	pdata->doneCommandHandling=false;
    pdata->doneTransferring=false;
	pdata->fileKey="";
	pdata->fpath[0]=0;
	pdata->fileBegin=0;
	pdata->fileOffset=0;
	pdata->fileBytes=0;
    pdata->bytesMaxRate=2 * 1024 * 1024; // default 2MB/sec
	memset(pdata->resp,0,1024);
	pdata->rptr=pdata->resp;
	pdata->pmod=this;
#ifdef WIN32
	pdata->multiFile=false;
	pdata->mdlen=0;
	pdata->recurse=false;
	memset(pdata->fileList,0,sizeof(pdata->fileList));
	pdata->thnd=CreateThread(0,0,BootThreadedCmd,pdata,0,&pdata->tid);
#else
	pthread_create(&pdata->thnd,0,BootThreadedCmd,pdata);
#endif
	WSThreadedCmdList.push_back(pdata);
	return pdata->thnd ?0:-1;
}
// Recursive file search
static int BrowseFiles(FILE *fp, const char *fdir, const char *match, bool recurse, int format, int newerDate)
{
	int nfiles=0;
#if defined(WIN32)||defined(_CONSOLE)
	WIN32_FIND_DATAA fdata;
	char fmatch[MAX_PATH];
	sprintf(fmatch,"%s\\%s",fdir,match);
	HANDLE fhnd=FindFirstFile(fmatch,&fdata);
	if(fhnd==INVALID_HANDLE_VALUE)
		return -1;
	map<string,WIN32_FIND_DATA> fmap; // for sorting
	do
	{
		char skey[MAX_PATH]={0};
		strcpy(skey,fdata.cFileName);
		_strupr(skey); // case-insensitive sorting
		// Skip files not in this date range
		if((newerDate)&&!(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		{
			FILETIME lft;
			FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&lft);
			SYSTEMTIME fsys;
			FileTimeToSystemTime(&lft,&fsys);
			int wdate=(fsys.wYear*10000) +(fsys.wMonth*100) +fsys.wDay;
			if(wdate<newerDate)
				continue;
		}
		fmap[skey]=fdata;
	}while(FindNextFile(fhnd,&fdata));
	FindClose(fhnd);

	// Arrange into display order
	list<WIN32_FIND_DATA> flist;
	int ndirs=0;
	map<string,WIN32_FIND_DATA>::iterator fit=fmap.find(".");
	if(fit!=fmap.end())
	{
		flist.push_back(fit->second); ndirs++;
	}
	fit=fmap.find("..");
	if(fit!=fmap.end())
	{
		flist.push_back(fit->second); ndirs++;
	}
	int maxnlen=0;
	for(map<string,WIN32_FIND_DATA>::iterator fit=fmap.begin();fit!=fmap.end();fit++)
	{
		fdata=fit->second;
		if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
			continue;
		flist.push_back(fit->second);
		if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			ndirs++;
		int nlen=(int)strlen(fdata.cFileName);
		if(nlen>maxnlen)
			maxnlen=nlen;
	}
	fmap.clear();

	if((format==1)||(format==2))
		fprintf(fp," Directory of %s\n\n",fdir);
	char wline[256]={0},*wptr=wline;
	__int64 dbytes=0;
	int ncols=80/(maxnlen +3);
	if(ncols<1) ncols=1;
	for(list<WIN32_FIND_DATA>::iterator lit=flist.begin();lit!=flist.end();lit++)
	{
		fdata=*lit;
		// dir
		if(format==1)
		{
			FILETIME lwtime;
			SYSTEMTIME stime;
			FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&lwtime);
			FileTimeToSystemTime(&lwtime,&stime);
			char sstr[32]={0};
			if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				memset(sstr,' ',11); sstr[11]=0;
			}
			else
				sprintf(sstr,"%11d",fdata.nFileSizeLow); // needs to be comma-formatted
			fprintf(fp,"%02d/%02d/%04d  %02d:%02d %s    %5s  %s %s\n",
				stime.wMonth,stime.wDay,stime.wYear,stime.wHour>12?stime.wHour -12:stime.wHour,
				stime.wMinute,stime.wHour>12?"PM":"AM",
				(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)?"<DIR>":"     ",
				sstr,fdata.cFileName);
		}
		// dir /w
		else if(format==2)
		{
			memset(wptr,' ',maxnlen +3);
			if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				sprintf(wptr,"[%s]",fdata.cFileName); 
				wptr[strlen(fdata.cFileName) +2]=' ';
			}
			else
			{
				sprintf(wptr,"%s",fdata.cFileName);
				wptr[strlen(fdata.cFileName)]=' ';
			}
			wptr+=maxnlen +3; *wptr=0;
			if((nfiles+1)%ncols==0)
			{
				fprintf(fp,"%s\n",wline); wptr=wline;
			}
		}
		// FindFirstFile
		else if(format==3)
		{
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				continue;
			FILETIME lctime,latime,lwtime;
			FileTimeToLocalFileTime(&fdata.ftCreationTime,&lctime);
			FileTimeToLocalFileTime(&fdata.ftLastAccessTime,&latime);
			FileTimeToLocalFileTime(&fdata.ftLastWriteTime,&lwtime);
			SYSTEMTIME sctime,satime,swtime;
			FileTimeToSystemTime(&lctime,&sctime);
			FileTimeToSystemTime(&latime,&satime);
			FileTimeToSystemTime(&lwtime,&swtime);
			#ifdef WIN32
			fprintf(fp,"%s\\%s,%ld,%ld,%08X,%04d%02d%02d-%02d:%02d:%02d,%04d%02d%02d-%02d:%02d:%02d,%04d%02d%02d-%02d:%02d:%02d,%s,%s\n",
			#else
			fprintf(fp,"%s/%s,%ld,%ld,%08X,%04d%02d%02d-%02d:%02d:%02d,%04d%02d%02d-%02d:%02d:%02d,%04d%02d%02d-%02d:%02d:%02d,%s,%s\n",
			#endif
				fdir,fdata.cFileName,fdata.nFileSizeHigh,fdata.nFileSizeLow,fdata.dwFileAttributes,
				sctime.wYear,sctime.wMonth,sctime.wDay,sctime.wHour,sctime.wMinute,sctime.wSecond,
				satime.wYear,satime.wMonth,satime.wDay,satime.wHour,satime.wMinute,satime.wSecond,
				swtime.wYear,swtime.wMonth,swtime.wDay,swtime.wHour,swtime.wMinute,swtime.wSecond,
				fdata.cFileName,fdata.cAlternateFileName);
		}
		nfiles++;
		dbytes+=fdata.nFileSizeLow;
	}
	char dstr[32]={0};
	#ifdef WIN32
	sprintf(dstr,"%I64d",dbytes);
	#else
	sprintf(dstr,"%lld",dbytes);
	#endif
	if(format==1)
	{
		fprintf(fp,"%16d File(s)       %13s bytes\n",nfiles -ndirs,dstr);
		fprintf(fp,"%16d Dirs(s)                   ? bytes free\n\n",ndirs);
	}
	else if(format==2)
	{
        if(wptr>wline)
			fprintf(fp,"%s\n",wline);
		fprintf(fp,"%16d File(s)       %13s bytes\n",nfiles -ndirs,dstr);
		fprintf(fp,"%16d Dirs(s)                   ? bytes free\n\n",ndirs);
	}

	// Breadth-first recursion
	if(recurse)
	{
		for(list<WIN32_FIND_DATA>::iterator lit=flist.begin();lit!=flist.end();lit++)
		{
			fdata=*lit;
			if(!strcmp(fdata.cFileName,".")||!strcmp(fdata.cFileName,".."))
				;
			else if(fdata.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)
			{
				char sdir[MAX_PATH];
				#ifdef WIN32
				sprintf(sdir,"%s\\%s",fdir,fdata.cFileName);
				#else
				sprintf(sdir,"%s/%s",fdir,fdata.cFileName);
				#endif
				int rc=BrowseFiles(fp,sdir,match,recurse,format,newerDate);
				if(rc>0)
					nfiles+=rc;
			}
		}
	}
	flist.clear();
#else//!WIN32
	DIR *pdir=opendir(fdir);
	if(!pdir)
		return -1;
	dirent *fdata=0;
	map<string,dirent> fmap; // for sorting
	do{
		fdata=readdir(pdir);
		if(fdata)
		{
			//if((!strcmp(fdata->d_name,"."))||(!strcmp(fdata->d_name,"..")))
			//	continue;
			char skey[MAX_PATH]={0};
			strcpy(skey,fdata->d_name);
			_strupr(skey); // case-insensitive sorting
			fmap[skey]=*fdata;
		}
	}while(fdata);
	closedir(pdir);

	// Arrange into display order
	list<dirent> flist;
	int ndirs=0;
	map<string,dirent>::iterator fit=fmap.find(".");
	if(fit!=fmap.end())
	{
		flist.push_back(fit->second); ndirs++;
	}
	fit=fmap.find("..");
	if(fit!=fmap.end())
	{
		flist.push_back(fit->second); ndirs++;
	}
	int maxnlen=0;
	for(map<string,dirent>::iterator fit=fmap.begin();fit!=fmap.end();fit++)
	{
		fdata=&fit->second;
		if(!strcmp(fdata->d_name,".")||!strcmp(fdata->d_name,".."))
			continue;
		flist.push_back(fit->second);
		struct stat fst;
		stat(fdata->d_name,&fst);
		if(fst.st_mode&S_IFDIR)
			ndirs++;
		int nlen=(int)strlen(fdata->d_name);
		if(nlen>maxnlen)
			maxnlen=nlen;
	}
	fmap.clear();

	if((format==1)||(format==2))
		fprintf(fp," Directory of %s\n\n",fdir);
	char wline[256]={0},*wptr=wline;
	__int64 dbytes=0;
	int ncols=80/(maxnlen +3);
	if(ncols<1) ncols=1;
	for(list<dirent>::iterator lit=flist.begin();lit!=flist.end();lit++)
	{
		fdata=&(*lit);
		struct stat fst;
		stat(fdata->d_name,&fst);
		int mdate=0,mtime=0;
		struct tm *ltm=localtime(&fst.st_mtime);
		if(ltm)
		{
			mdate=(1900 +ltm->tm_year)*10000 +(ltm->tm_mon+1)*100 +ltm->tm_mday;
			mtime=(ltm->tm_hour*10000) +(ltm->tm_min*100) +ltm->tm_sec;
		}
		// dir
		if(format==1)
		{
			char sstr[32]={0};
			if(fst.st_mode&S_IFDIR)
			{
				memset(sstr,' ',11); sstr[11]=0;
			}
			else
				sprintf(sstr,"%11d",fst.st_size); // needs to be comma-formatted
			fprintf(fp,"%02d/%02d/%04d  %02d:%02d %s    %5s  %s %s\n",
				(mdate%1000)/100,mdate%100,(mdate/10000),(mtime/10000)>12?(mtime/10000) -12:(mtime/10000),
				(mtime%1000)/100,(mtime/10000)>12?"PM":"AM",
				(fst.st_mode&S_IFDIR)?"<DIR>":"     ",
				sstr,fdata->d_name);
		}
		// dir /w
		else if(format==2)
		{
			memset(wptr,' ',maxnlen +3);
			if(fst.st_mode&S_IFDIR)
			{
				sprintf(wptr,"[%s]",fdata->d_name); 
				wptr[strlen(fdata->d_name) +2]=' ';
			}
			else
			{
				sprintf(wptr,"%s",fdata->d_name);
				wptr[strlen(fdata->d_name)]=' ';
			}
			wptr+=maxnlen +3; *wptr=0;
			if((nfiles+1)%ncols==0)
			{
				fprintf(fp,"%s\n",wline); wptr=wline;
			}
		}
		// FindFirstFile
		else if(format==3)
		{
			if(!strcmp(fdata->d_name,".")||!strcmp(fdata->d_name,".."))
				continue;
			int cdate=0,adate=0;
			int ctime=0,atime=0;
			ltm=localtime(&fst.st_ctime);
			if(ltm)
			{
				cdate=(1900 +ltm->tm_year)*10000 +(ltm->tm_mon+1)*100 +ltm->tm_mday;
				ctime=(ltm->tm_hour*10000) +(ltm->tm_min*100) +ltm->tm_sec;
			}
			ltm=localtime(&fst.st_atime);
			if(ltm)
			{
				adate=(1900 +ltm->tm_year)*10000 +(ltm->tm_mon+1)*100 +ltm->tm_mday;
				atime=(ltm->tm_hour*10000) +(ltm->tm_min*100) +ltm->tm_sec;
			}
			LONGLONG fsize=fst.st_size;
			fprintf(fp,"%s/%s,%ld,%lld,%08X,%08d-%06d,%08d-%06d,%08d-%06d,%s,%s\n",
				fdir,fdata->d_name,0,fsize,fst.st_mode,
				cdate,ctime,adate,atime,mdate,mtime,fdata->d_name,"");
		}
		nfiles++;
		dbytes+=fst.st_size;
	}
	char dstr[32]={0};
	sprintf(dstr,"%lld",dbytes);
	if(format==1)
	{
		fprintf(fp,"%16d File(s)       %13s bytes\n",nfiles -ndirs,dstr);
		fprintf(fp,"%16d Dirs(s)                   ? bytes free\n\n",ndirs);
	}
	else if(format==2)
	{
        if(wptr>wline)
			fprintf(fp,"%s\n",wline);
		fprintf(fp,"%16d File(s)       %13s bytes\n",nfiles -ndirs,dstr);
		fprintf(fp,"%16d Dirs(s)                   ? bytes free\n\n",ndirs);
	}

	// Breadth-first recursion
	if(recurse)
	{
		for(list<dirent>::iterator lit=flist.begin();lit!=flist.end();lit++)
		{
			fdata=&(*lit);
			struct stat fst;
			stat(fdata->d_name,&fst);
			if(!strcmp(fdata->d_name,".")||!strcmp(fdata->d_name,".."))
				;
			else if(fst.st_mode&S_IFDIR)
			{
				char sdir[MAX_PATH];
				sprintf(sdir,"%s/%s",fdir,fdata->d_name);
				int rc=BrowseFiles(fp,sdir,match,recurse,format);
				if(rc>0)
					nfiles+=rc;
			}
		}
	}
	flist.clear();
#endif//!WIN32
	return nfiles;
}
static int CalcTPlusDate(int wsdate, int n, bool skipwe)
{
	// Convert wsdate to time_t
	tm ltm;
	memset(&ltm,0,sizeof(tm));
	ltm.tm_year=wsdate/10000 -1900;
	ltm.tm_mon=(wsdate%10000)/100 -1;
	ltm.tm_mday=wsdate%100;
	ltm.tm_isdst=-1;
	time_t wtm=mktime(&ltm);
	if(!wtm)
		return 0;
	// Add or subtract n business days
	while(n!=0)
	{
		if(n>0) 
		{
			wtm+=86400; ltm.tm_wday++;
			if(ltm.tm_wday>6) ltm.tm_wday=0;
			if(!skipwe)
				n--;
			// 0-Sunday,...,6-Saturday
			else if((ltm.tm_wday>0)&&(ltm.tm_wday<6))
				n--;
		}
		else 
		{
			wtm-=86400; ltm.tm_wday--;
			if(ltm.tm_wday<0) ltm.tm_wday=6;
			if(!skipwe)
				n++;
			// 0-Sunday,...,6-Saturday
			else if((ltm.tm_wday>0)&&(ltm.tm_wday<6))
				n++;
		}
	}
	tm *ptm=localtime(&wtm);
	if(ptm)
		wsdate=((ptm->tm_year +1900)*10000)+((ptm->tm_mon+1)*100)+(ptm->tm_mday);
	return wsdate;
}
int SMCmds::WSHThreadedCmdHandler(SMCmds *pmod, WSThreadedCmdInfo *pdata)
{
	if((!ccodec)||(!cnotify))
		return -1;
	string& cmd=pdata->cmd;
	TVMAP tvmap;
	ccodec->GetTagValues(tvmap,pdata->parm,pdata->plen);
	char parms[2048]={0};
	// Remote file browsing
	if(cmd=="browsefiles")
	{
		pdata->fileKey=ccodec->GetValue(tvmap,"FileKey");
		if(pdata->fileKey.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'filekey'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		string dir=ccodec->GetValue(tvmap,"Dir");
		if(dir.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'dir'.",
					pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		string match=ccodec->GetValue(tvmap,"Match");
		if(match.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'match'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		bool recurse=(ccodec->GetValue(tvmap,"Recurse")=="Y")?true:false;
		string format=ccodec->GetValue(tvmap,"Format");
		int fmt=1;
		if(!format.empty())
			fmt=atol(format.c_str());
		string newerthan=ccodec->GetValue(tvmap,"NewerThan");
		SYSTEMTIME tsys;
		GetLocalTime(&tsys);
		int newerDate=(tsys.wYear*10000) +(tsys.wMonth*100) +tsys.wDay;
		if((*newerthan.c_str()=='-')||(*newerthan.c_str()=='+'))
			newerDate=CalcTPlusDate(newerDate,atoi(newerthan.c_str()),false);
		else
			newerDate=atoi(newerthan.c_str());

		#ifdef WIN32
		sprintf(pdata->fpath,".\\%s\\browse.txt.tmp",WSTEMPFILESDIR);
		#else
		sprintf(pdata->fpath,"./%s/browse.txt.tmp",WSTEMPFILESDIR);
		#endif
		CreateDirectory(WSTEMPFILESDIR,0);
		FILE *fp=fopen(pdata->fpath,"wt");
		if(!fp)
		{
			pdata->fpath[0]=0;
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Failed creating temp file.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		char dpath[MAX_PATH]={0},*dname=0;
		GetFullPathNameEx(dir.c_str(),MAX_PATH,dpath,&dname);
		if(fmt==3)
			fprintf(fp,"FilePath,nFileSizeHigh,nFileSizeLow,dwFileAttributes,ftCreationTime,ftLastAccessTime,ftLastWriteTime,cFileName,cAlternateFileName\n");
		int nfiles=BrowseFiles(fp,dpath,match.c_str(),recurse,fmt,newerthan.empty()?0:newerDate);
		if(fmt==3)
			fprintf(fp,"%d files found.\n",nfiles);
		fclose(fp);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|filekey=%s|Text=%d files found.|",
			pdata->oboi.c_str(),pdata->dtid.c_str(),pdata->fileKey.c_str(),nfiles);
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
		return 0;
	}
	// Download file
	else if(cmd=="downloadfile")
	{
		pdata->fileKey=ccodec->GetValue(tvmap,"FileKey");
		if(pdata->fileKey.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'filekey'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		string filepath=ccodec->GetValue(tvmap,"FilePath");
		if(filepath.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'FilePath'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		LONGLONG begin=_atoi64(ccodec->GetValue(tvmap,"Begin").c_str());
		LONGLONG end=_atoi64(ccodec->GetValue(tvmap,"End").c_str());
		LONGLONG size=atoi(ccodec->GetValue(tvmap,"Size").c_str());
		if((begin>0)&&(end>0)&&(end<=begin))
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason='End' may not be <= 'Begin'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
        string strMaxRate = ccodec->GetValue(tvmap, "MaxRate");
        if(!strMaxRate.empty())
        {
            DWORD dwMaxRate = atoi(strMaxRate.c_str());
            const char *pszUnitBegin = strMaxRate.c_str();
            while(pszUnitBegin[0] >= '0' && pszUnitBegin[0] <= '9')
                pszUnitBegin++;
            if(!_stricmp(pszUnitBegin, "B"))
                pdata->bytesMaxRate = dwMaxRate;
            else if(!_stricmp(pszUnitBegin, "KB"))
                pdata->bytesMaxRate = dwMaxRate * 1024;
            else if(!_stricmp(pszUnitBegin, "MB"))
                pdata->bytesMaxRate = dwMaxRate * 1024 * 1024;
            else if(!_stricmp(pszUnitBegin, "MAX"))
                pdata->bytesMaxRate = MAXDWORD;
            else // unknown unit
            {
                sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Unknown 'MaxRate' unit (%s). Use B/KB/MB/MAX.",
                    pdata->oboi.c_str(), pdata->dtid.c_str(),
                    strMaxRate.c_str());
                ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
                return(0);
            }
            if(!pdata->bytesMaxRate)
            {
                sprintf_s(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Invalid 'MaxRate' (%s).",
                    pdata->oboi.c_str(), pdata->dtid.c_str(),
                    strMaxRate.c_str());
                ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
                return(0);
            }
        }
	#ifdef WIN32
		string filelist=ccodec->GetValue(tvmap,"FileList");
		if(!filelist.empty())
		{
			pdata->multiFile=true;
			strcpy(pdata->fileList,filelist.c_str());
			char *fname=0;
			GetFullPathNameEx(filepath.c_str(),MAX_PATH,pdata->fpath,&fname);
			GetCurrentDirectory(MAX_PATH,pdata->mdir);
			strcat(pdata->mdir,"\\");
			pdata->mdlen=(int)strlen(pdata->mdir);
		}
		else if((strchr(filepath.c_str(),'*'))||(strchr(filepath.c_str(),'?')))
		{
			pdata->multiFile=true;
			char *fname=0;
			GetFullPathNameEx(filepath.c_str(),MAX_PATH,pdata->fpath,&fname);
			GetCurrentDirectory(MAX_PATH,pdata->mdir);
			strcat(pdata->mdir,"\\");
			pdata->mdlen=(int)strlen(pdata->mdir);
			string recurse = ccodec->GetValue(tvmap, "Recurse");
			if((recurse=="y")||(recurse=="Y"))
				pdata->recurse=true;
		}
		else
	#endif
		{
			char *fname=0;
			GetFullPathNameEx(filepath.c_str(),MAX_PATH,pdata->fpath,&fname);
			if(!PathFileExists(pdata->fpath))
			{
				pdata->fpath[0]=0;
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=File not found.",
					pdata->oboi.c_str(),pdata->dtid.c_str());
				ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
				return 0;
			}
		}
		if(begin<0) begin=0;
		// No size specified
		if(!size)
		{
			// [begin,end)
			if((begin>=0)&&(end>0)&&(end>begin))
				size=end -begin;
			// else [begin,eof)
		}
		// Negative size
		else if(size<0)
		{
			// [end -size,end)
			if(end>0)
			{
				begin=end +size; size=-size;
			}
			// [eof -size,eof)
			else
			{
				end=0;
			}
		}
		// Positive size [begin,begin+size)
		else//if(size>0)
		{
			// if end and size specified, end is overriden
			end=begin +size;
		}
		pdata->fileBegin=begin;
		pdata->fileOffset=begin;
		pdata->fileBytes=size;
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|filekey=%s|Text=Downloaded (%s)...|",
			pdata->oboi.c_str(),pdata->dtid.c_str(),pdata->fileKey.c_str(),pdata->fpath);
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
		return 0;
	}
	// Copy file
	else if(cmd=="copyfile")
	{
		string srcfile=ccodec->GetValue(tvmap,"SrcFile");
		if(srcfile.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'SrcFile'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		char spath[MAX_PATH]={0},*sname=0;
		GetFullPathNameEx(srcfile.c_str(),MAX_PATH,spath,&sname);
		string dstfile=ccodec->GetValue(tvmap,"DstFile");
		if(dstfile.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'DstFile'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		char dpath[MAX_PATH]={0},*dname=0;
		GetFullPathNameEx(dstfile.c_str(),MAX_PATH,dpath,&dname);

		bool overwrite=(ccodec->GetValue(tvmap,"Overwrite")=="Y")?true:false;
	#if defined(WIN32)||defined(_CONSOLE)
		if(CopyFile(spath,dpath,!overwrite))
	#else
		char scmd[1024]={0};
		sprintf(scmd,"cp --reply=yes -f %s %s",spath,dpath);
		system(scmd);
		if(PathFileExists(dpath))
	#endif
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|Text=Copy (%s) to (%s) successful|", pdata->oboi.c_str(), pdata->dtid.c_str(),spath,dpath);
		else
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=%d|Reason=Copy (%s) to (%s) failed|",
				pdata->oboi.c_str(),pdata->dtid.c_str(),GetLastError(),spath,dpath);
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
		return 0;
	}
    // Move file
    else if(cmd == "movefile")
    {
        string strSrcFile = ccodec->GetValue(tvmap, "SrcFile");
        string strDstFile = ccodec->GetValue(tvmap, "DstFile");
        if(strSrcFile.empty() || strDstFile.empty())
        {
            sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm '%s'.|",
                pdata->oboi.c_str(), pdata->dtid.c_str(), strSrcFile.empty() ? "SrcFile" : "DstFile");
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
            return(0);
        }
        char szSrcPath[MAX_PATH] = {0};
        char *pszSrcName = 0;
        GetFullPathNameEx(strSrcFile.c_str(), sizeof(szSrcPath), szSrcPath, &pszSrcName);
        char szDstPath[MAX_PATH] = {0};
        char *pszDstName = 0;
        GetFullPathNameEx(strDstFile.c_str(), sizeof(szDstPath), szDstPath, &pszDstName);
        if(PathFileExists(szDstPath))
        {
            sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-2|Reason=The same name already exists|SrcFile=%s|DstFile=%s|", pdata->oboi.c_str(), pdata->dtid.c_str(), szSrcPath, szDstPath);
            ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
            return(0);
        }
#if defined WIN32
        if(MoveFile(szSrcPath, szDstPath))
#else
        char scmd[1024] = {0};
        sprintf(scmd, "mv -f %s %s", szSrcPath, szDestPath);
        system(scmd);
        if(PathFileExists(szDestPath))
#endif
            sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=0|SrcFile=%s|DstFile=%s|", pdata->oboi.c_str(), pdata->dtid.c_str(), szSrcPath, szDstPath);
        else
            sprintf(parms, "OnBehalfOf=%s|DeliverTo=%s|rc=-3|Reason=Failed|SrcFile=%s|DstFile=%s|", pdata->oboi.c_str(), pdata->dtid.c_str(), szSrcPath, szDstPath);
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
        return(0);
    }
#ifdef WIN32
	// Enum drives
	else if(cmd=="enumdrives")
	{
		pdata->fileKey=ccodec->GetValue(tvmap,"FileKey");
		if(pdata->fileKey.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'filekey'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		// Temporary file
		char rpath[MAX_PATH]={0},fpath[MAX_PATH]={0},*fname=0;
		#ifdef WIN32
		sprintf(rpath,"%s\\enumdrives.csv.tmp",WSTEMPFILESDIR);
		#else
		sprintf(rpath,"%s/enumdrives.csv.tmp",WSTEMPFILESDIR);
		#endif
		GetFullPathNameEx(rpath,MAX_PATH,fpath,&fname);
		CreateDirectory(WSTEMPFILESDIR,0);
		FILE *tfp=fopen(fpath,"wt");
		if(!tfp)
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Failed writing temp file (%s)!",
				pdata->oboi.c_str(),pdata->dtid.c_str(),fpath);
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		fprintf(tfp,"Drive,Type,Vname,Sn,SysFlags,Fsname\n");
		// Get drive information
		char dnames[1024]={0};
		DWORD dlen=GetLogicalDriveStrings(1024,dnames);
		int dno=0;
		for(const char *dptr=dnames;(*dptr)&&(dptr<dnames +dlen);dptr+=4)
		{
			DWORD dtype=GetDriveType(dptr);
			char vname[256]={0},fsname[256]={0};
			DWORD vsn=0,vmcl=0,vflags=0;
			GetVolumeInformation(dptr,vname,256,&vsn,&vmcl,&vflags,fsname,256);
			dno++;
			fprintf(tfp,"%c,%d,%s,%08X,%08X,%s\n",dptr[0],dtype,vname,vsn,vflags,fsname); 
		}
		fclose(tfp);
		// Caller sends back the file
		strcpy(pdata->fpath,fpath);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|numdrives=%d|FileKey=%s|",
			pdata->oboi.c_str(),pdata->dtid.c_str(),dlen/4,pdata->fileKey.c_str());
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);

		//// Requires _WIN32_WINNT>=f500
		//char vname[256]={0};
		//HANDLE fhnd=FindFirstVolume(vname,256);
		//if(fhnd!=INVALID_HANDLE_VALUE)
		//{
		//	do
		//	{
		//	}while(FindNextVolume(fhnd,vname,256));
		//	FindVolumeClose(fhnd);
		//}
		return 0;
	}
#endif
	// Process list
	else if(cmd=="ps")
	{
		pdata->fileKey=ccodec->GetValue(tvmap,"FileKey");
		if(pdata->fileKey.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'filekey'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		// Temporary file
		CreateDirectory(WSTEMPFILESDIR,0);
		char rpath[MAX_PATH]={0},fpath[MAX_PATH]={0},*fname=0;
		#ifdef WIN32
		sprintf(rpath,"%s\\ps.csv.tmp",WSTEMPFILESDIR);
		#else
		sprintf(rpath,"%s/ps.csv.tmp",WSTEMPFILESDIR);
		#endif
		GetFullPathNameEx(rpath,MAX_PATH,fpath,&fname);
	#if defined(WIN32)||defined(_CONSOLE)
		FILE *tfp=fopen(fpath,"wt");
		if(!tfp)
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Failed writing temp file (%s)!",
				pdata->oboi.c_str(),pdata->dtid.c_str(),fpath);
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		fprintf(tfp,"Pid,Path,Domain,Account,Time\n");
		// Get app information
		FindProcData *fpd=new FindProcData[256];
		memset(fpd,0,256*sizeof(FindProcData));
		int nprocs=ps(fpd,256);
		for(int i=0;i<nprocs;i++)
		{
			int wslast=0;
			if(fpd[i].last)
			{
				struct tm *ptm=localtime(&fpd[i].last);
				if(ptm)
					wslast=(ptm->tm_hour*10000)+(ptm->tm_min*100)+(ptm->tm_sec);
			}
			fprintf(tfp,"%d,%s,%s,%s,%06d\n",
				fpd[i].pid,fpd[i].path,fpd[i].domain,fpd[i].account,wslast); 
		}
		delete fpd;
		fclose(tfp);
		// Caller sends back the file
		strcpy(pdata->fpath,fpath);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|numprocs=%d|FileKey=%s|",
			pdata->oboi.c_str(),pdata->dtid.c_str(),nprocs,pdata->fileKey.c_str());
	#else
		char scmd[1024]={0};
		sprintf(scmd,"ps -ef > %s",fpath);
		system(scmd);
		// Caller sends back the file
		strcpy(pdata->fpath,fpath);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|FileKey=%s|",
			pdata->oboi.c_str(),pdata->dtid.c_str(),pdata->fileKey.c_str());
	#endif
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
		return 0;
	}
	// Command shell (e.g. ipconfig, ping, tracert, nslookup)
	else if(cmd=="shell")
	{
		pdata->fileKey=ccodec->GetValue(tvmap,"FileKey");
		if(pdata->fileKey.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'FileKey'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		string shellcmd=ccodec->GetValue(tvmap,"ShellCmd");
		if(shellcmd.empty())
		{
			sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Missing required parm 'ShellCmd'.",
				pdata->oboi.c_str(),pdata->dtid.c_str());
			ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
			return 0;
		}
		// TODO: support pipes in the shell command
		#ifdef WIN32
		sprintf(pdata->fpath,".\\%s\\shell.txt.tmp",WSTEMPFILESDIR);
		#else
		sprintf(pdata->fpath,"./%s/shell.txt.tmp",WSTEMPFILESDIR);
		#endif
		CreateDirectory(WSTEMPFILESDIR,0);
		char scmd[1024]={0};
		sprintf(scmd,"%s>%s 2>&1",shellcmd.c_str(),pdata->fpath);
		system(scmd);
		sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=0|FileKey=%s|",
			pdata->oboi.c_str(),pdata->dtid.c_str(),pdata->fileKey.c_str());
		ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
		return 0;
	}
	sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Unknown threaded cmd.",
		pdata->oboi.c_str(),pdata->dtid.c_str());
	ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
	return 0;
}
// Check for completion of threaded command
void SMCmds::CheckThreadedCmd()
{
	if((!ccodec)||(!cnotify))
		return;
	DWORD tnow=GetTickCount();
	for(WSTCMDLIST::iterator dit=WSThreadedCmdList.begin();dit!=WSThreadedCmdList.end();)
	{
		WSThreadedCmdInfo *pdata=*dit;
        if(pdata->doneTransferring)
        {
	        if(pdata->parm)
		        delete pdata->parm;
	        delete pdata;
	        #ifdef WIN32
	        dit=WSThreadedCmdList.erase(dit);
	        #else
	        WSThreadedCmdList.erase(dit++);
	        #endif
        }
        else if(!pdata->doneCommandHandling && tnow -pdata->tstart>=pdata->timeout)
		{
			#ifdef WIN32
			TerminateThread(pdata->thnd,86);
			#elif !defined(_CONSOLE)
			_ASSERT(false);//untested
			pthread_kill(pdata->thnd,9);
			#endif
			if((pdata->PortType==WS_UMR)||(pdata->PortType==WS_USR))
			{
				char parms[2048]={0};
				sprintf(parms,"OnBehalfOf=%s|DeliverTo=%s|rc=-1|Reason=Cmd timed out after %d ms!|",
					pdata->oboi.c_str(),pdata->dtid.c_str(),pdata->timeout);
				ccodec->Encode(pdata->rptr,1024,false,pdata->cmdid,pdata->cmd,parms,0);
				cnotify->SMCSendMsg(1109,(int)(pdata->rptr -pdata->resp),pdata->resp,pdata->PortType,pdata->PortNo);
			}
			#ifdef WIN32
			CloseHandle(pdata->thnd);
			#else
			void *rc=0;
			pthread_join(pdata->thnd,&rc);
			#endif
			if(pdata->parm)
				delete pdata->parm;
			delete pdata;
			dit=WSThreadedCmdList.erase(dit);
		}
		// Still going
		else
			dit++;
	}
}