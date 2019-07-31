
#include "stdafx.h"
#include "wstring.h"
#include "stdio.h"
#ifdef USE_MYRT
#include "myrt.h"
#else
#define mytolower tolower
#endif

#ifdef DYNAMIC_TLS
#undef TLSDEF
#define TLSDEF
#elif !defined(TLSDEF)
#ifdef WIN32
	#ifdef WSOCKSAPI
	#define TLSDEF __declspec(thread)
	#else
	#define TLSDEF
	#endif
#else
	#define TLSDEF __thread
#endif
#endif

#ifdef _WIN32
// Convenience UNICODE to ANSI
static TLSDEF char w2str_str[MAX_PATH];
const char *w2str(const WCHAR *wstr)
{
    memset(w2str_str, 0, sizeof(w2str_str));
    if ( wstr )
    {
        for ( int i=0; i<sizeof(w2str_str) -1 && wstr[i]; i++ )
            w2str_str[i] = (char)wstr[i];
    }
    else
        strcpy(w2str_str, "null");
    return w2str_str;
}
// Convenience ANSI to UNICODE
static TLSDEF WCHAR str2w_str[MAX_PATH];
const WCHAR *str2w(const char *str)
{
    memset(str2w_str, 0, sizeof(str2w_str));
    if ( str )
    {
        for ( DWORD i=0; i<sizeof(str2w_str) -1 && str[i]; i++ )
            str2w_str[i] = str[i];
    }
    else
        wcscpy(str2w_str, L"null");
    return str2w_str;
}
#endif//_WIN32

// Convenience function to find substrings without case sensitivity.
// Pass in 'bend' for non-null-terminated buffer.
const char *strestr(const char *src, const char *sub, const char *bend)
{
    if ( !src || !sub )
        return 0;
    int slen = bend ?(int)(bend -src) :(int)strlen(src);
    int tlen = (int)strlen(sub);
    const char *send = bend ?bend :src +slen;
    const char *tptr = sub;
    for ( const char *sptr=src; sptr<send; sptr++ )
    {
        char lch = mytolower(*sptr);
        char ltch = mytolower(*tptr);
        if ( lch == ltch )
        {
            tptr ++;
            if ( !*tptr )
                return sptr -tlen +1;
        }
        else if ( tptr != sub )
        {
            tptr = sub;
            ltch = mytolower(*tptr);
            if ( lch == ltch )
            {
                tptr ++;
                if ( !*tptr )
                    return sptr -tlen +1;
            }
        }
    }
    return 0;
}
const char *stristr(const char *src, const char *sub)
{
    return strestr(src, sub, 0);
}
// Convenience to do strchr up to end of buffer 'bend'
const char *strechr(const char *buf, char ch, const char *bend)
{
    if ( !buf || !bend )
        return 0;
    for ( const char *bptr=buf; bptr<bend; bptr++ )
    {
        if ( *bptr == ch )
            return bptr;
    }
    return 0;
}
// Efficient version of strechr for 0x0D0A that takes into
// account, malformed or truncated lines
const char *strendl(const char *buf, const char *bend)
{
    if ( !buf || !bend )
        return 0;
    for ( const char *bptr=buf; bptr<bend; bptr++ )
    {
        if ( *bptr == 0x0D || *bptr == 0x0A )
            return bptr;
    }
    return 0;
}
// Efficient version of strechr for 0x0D0A that takes into
// account, malformed or truncated lines
const char *strendln(const char *buf, const char *bend)
{
    if ( !buf || !bend )
        return 0;
    for ( const char *bptr=buf; bptr<bend; bptr++ )
    {
        if ( *bptr == 0 || *bptr == 0x0D || *bptr == 0x0A )
            return bptr;
    }
    return 0;
}
// Convenience function to do right string case-insensitive comparisons
int strrcmp(const char *src, const char *sub)
{
    if ( !src || !sub )
        return 0;
    int slen = (int)strlen(src);
    int sublen = (int)strlen(sub);
    if ( slen < sublen )
        return 0;
    if ( stristr(src +slen -sublen, sub) == src +slen -sublen )
        return 1;
    return 0;
}

// Case-insensitive strncmp
int strincmp(const char *s1, const char *s2, int cnt)
{
    int i;
    for ( i=0; i<cnt && s1[i] && s2[i]; i++ )
    {
        int cmp = mytolower(s1[i]) -mytolower(s2[i]);
        if ( cmp )
            return cmp;
    }
    return i<cnt ?s1[i] -s2[i]:0;
}

#ifdef WSTRING_STRTOKE
// This one allows empty values between delimiters.
// Also, you can't nest strtok
char *strtoke(char *str, const char *toks)
{
	static TLSDEF char *start=0;
	if ( str )
		start = str;
	char *sptr;
	for ( sptr=start; sptr && *sptr; sptr++ )
	{
		const char *tptr;
		for ( tptr=toks; *tptr; tptr++ )
		{
			if ( *sptr==*tptr )
				break;
		}
		if ( *tptr )
		{
			*sptr = 0;
			char *rc = start;
			start = sptr +1;
			return rc;
		}
	}
	if ( start && *start )
	{
		char *rc = start;
		if ( sptr && *sptr )
			start = sptr +1;
		else
			start = sptr;
		return rc;
	}
	return 0;
}
#endif
