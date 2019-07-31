#include "stdafx.h"

using namespace std;

#include <map>
#include <string>
#include <vector>
//#include <bstring>

void StringSplit(string str, string delim, vector<string> & results)
{
	size_t cutAt;
	while( (cutAt = str.find_first_of(delim)) != str.npos )
	{
		if(cutAt > 0)
		{
			results.push_back(str.substr(0,cutAt));
		}
		str = str.substr(cutAt+1);
	}
	if(str.length() > 0)
	{
		results.push_back(str);
	}

}
/*

		fgets(line,sizeof(line),fp);
		
		splitITF( line, rbuf); // rbuf will contain the fix peice 

	
*/
 

void splitITF(string itfMessage, char *out)
{
	*out=0;

	vector<string> groups;

	 StringSplit(itfMessage, "\035", groups );

	 if (groups.size()>4)
	 {
		int msgType = atoi(groups[1].c_str());

		switch (msgType)
		{
		case 1:
			{
				string rawFIX = groups[4];
				strcpy(out, rawFIX.c_str());
				break;
			}

		default:
			break;
		}//switch
	 }
	 else
		strcpy(out, itfMessage.c_str());

}


