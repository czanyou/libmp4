/***
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to ChengZhen(Anyou).  It is subject to the terms of a
 * License Agreement between Licensee and ChengZhen(Anyou).
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and
 * any derivative work shall retain this copyright notice.
 *
 * Copyright (c) 2014-2015 ChengZhen(Anyou). All Rights Reserved.
 *
 */

#ifndef _NS_VISION_MP4_CONFIG_H
#define _NS_VISION_MP4_CONFIG_H

#ifdef _WIN32
#include <io.h>
#define EINTR	4
#define R_OK	0

#ifndef strcasecmp 
#define strcasecmp stricmp
#define usleep Sleep
#endif // end strcasecmp

#ifndef atoll 
#define atoll _atoi64
#endif // end strcasecmp

#endif // end _WIN32

#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

namespace mp4 {

/** 分配指定的大小的缓存区. */
inline void* Mp4Malloc(size_t size) 
{
	return (size > 0) ? malloc(size) : NULL;
}

/** 重新分配指定的大小的缓存区. */
inline void* Mp4Realloc(void* p, size_t newSize) 
{
	// workaround library bug
	if ((p == NULL) && (newSize == 0)) {
		return NULL;
	}
	return realloc(p, newSize);
}

/** 释放指定的缓存区. */
inline void Mp4Free(void* p) 
{
	if (p != NULL) {
		free(p);
	}
}

/** 指出指定的字符串是否为空. */
inline BOOL Mp4IsEmpty(LPCSTR text) 
{
	return (text == NULL) || (*text == '\0');
}

/** 合并文件名和扩展名. */
inline LPCSTR Mp4MergeExtName(char* buf, UINT bufLen, LPCSTR filename, LPCSTR extName)
{
	strncpy(buf, filename, bufLen);
	strncat(buf, extName,  bufLen);
	return buf;
}

}

#endif // _NS_VISION_MP4_CONFIG_H

