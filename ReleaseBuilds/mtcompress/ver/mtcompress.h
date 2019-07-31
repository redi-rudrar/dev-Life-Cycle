
#ifndef _MTCOMPRESS_H
#define _MTCOMPRESS_H
// Thread-safe multi-compression wrapper around zlib2.lib and other compression and encryption algorithms

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef WIN32
#ifdef MAKE_STATIC
#define MTEXPORT extern
#elif defined(MAKE_MTCOMPRESS)
#define MTEXPORT __declspec(dllexport)
#else
#define MTEXPORT __declspec(dllimport)
#endif
#else
#define MTEXPORT extern
#endif

MTEXPORT int mtcompress(int proto, char *dest,unsigned int *destLen,const char *source,unsigned int *sourceLen);
MTEXPORT int mtuncompress(int proto, char *dest,unsigned int *destLen,const char *source,unsigned int *sourceLen);

#ifdef __cplusplus
};//extern "C"
#endif

#endif//_MTCOMPRESS_H
