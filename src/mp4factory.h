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
#ifndef _NS_VISION_MP4_FACTORY_H
#define _NS_VISION_MP4_FACTORY_H

#include "mp4.h"
#include "mp4atom.h"

namespace mp4 {

//______________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Factory class

/** 
 * MP4 atom 辅助类.
 * 
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Factory
{
public:
	Mp4Factory(void);
	~Mp4Factory(void);

// Operations -------------------------------------------------
public:
	int AddProperties ( Mp4Atom* atom, BYTE version );
	int InitProperties( Mp4Atom* atom, BYTE version );
};

}

#endif // _NS_VISION_MP4_FACTORY_H
