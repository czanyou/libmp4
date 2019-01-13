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
#ifndef _NS_VISION_MP4_ATOM_H
#define _NS_VISION_MP4_ATOM_H

#include "mp4common.h"
#include "mp4property.h"

namespace mp4 {

//______________________________________________________________________________
////////////////////////////////////////////////////////////////////////////////
// Mp4Atom class

class Mp4Atom;

/** Mp4Atom 智能指针类型. */
typedef SmartPtr<Mp4Atom> Mp4AtomPtr;

/** Mp4Atom 智能指针类型数组. */
typedef Mp4Array<Mp4AtomPtr> Mp4AtomArray;

/** 
 * 代表 MP4 媒体文件的 Atom 对象. 
 * An object-oriented building block defined by a unique type identifier and 
 * length (called ‘atom’ in some specifications, including the first definition 
 * of MP4). 
 * 一个 MP4 文件就是由多个 Atom 组成的, Atom 是组成 MP4 文件的基本单位.
 *
 * Atom 头:
 * - uint (32) size 整个 Atom 的大小, 包括 size 和 type 头, 字段以及所有子 
 *	Atom 大小在内. 单位为字节. 如果为 0 表示这个 Atom 是这个文件的最后一
 * 个 Atom, 一般只用于 mdat Atom. 如果为 1 表示实际大小为 largesize 字段的
 * 值, largesize 为 64 位.
 *
 * - uint (32) type 这个 Atom 的类型. 一般由 4 个可以打印的字符组成.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Atom : public VisionBaseObject
{
public:
	Mp4Atom();
	Mp4Atom(LPCSTR type);
	virtual ~Mp4Atom();
	VISION_BASEREF_METHODS(Mp4Atom); ///< 实现引用计数功能.

	friend class Mp4Factory;

// Attributes -------------------------------------------------
public:
	Mp4AtomPtr GetParent();	
	LPCSTR GetType()  const;		
	INT64  GetSize()  const;		
	INT64  GetStart() const;		
	INT64  GetEnd() const;
	UINT   GetChildrenCount();
	UINT   GetPropertyCount();
	
	void   SetParent(Mp4Atom* parent);
	void   SetSize (INT64 size);
	void   SetStart(INT64 start);

// Operations -------------------------------------------------
public:
	void  Clear();
	INT64 CalculateSize();
	int   Init(BYTE version = 0);
	int   Read(Mp4File* file, UINT flags = 0);
	int   Write(Mp4File* file);
	int   WriteHeader( Mp4File* file );
	int   WriteSize( Mp4File* file );

	BYTE* GetPropertyBytes (LPCSTR name);
	float GetPropertyFloat (LPCSTR name);
	UINT  GetPropertyInt   (LPCSTR name);
	INT64 GetPropertyInt64 (LPCSTR name);
	int   GetPropertyString(LPCSTR name, char* buf, UINT bufLen);

	void  SetPropertyFloat (LPCSTR name, float  value);
	void  SetPropertyInt   (LPCSTR name, INT64  value);
	void  SetPropertyString(LPCSTR name, LPCSTR value);

	Mp4PropertyPtr AddProperty(Mp4PropertyType type, int size = 0, LPCSTR name = NULL);
	Mp4PropertyPtr FindProperty(LPCSTR name);
	Mp4PropertyPtr GetProperty(UINT index);

	void	   AddChildAtom(Mp4Atom* atom);
	Mp4AtomPtr AddChildAtom(LPCSTR name, UINT index = 0xFFFFFFFF);
	void	   ClearChildAtoms();
	Mp4AtomPtr FindAtom    (LPCSTR name);
	Mp4AtomPtr GetChildAtom(LPCSTR name);
	Mp4AtomPtr GetChildAtom(UINT index);
	int		   GetChildAtomIndex (LPCSTR name);
	void       RemoveChildAtom(LPCSTR name);
	void	   SetChildAtom(UINT index, Mp4Atom* atom);

// Implementation ---------------------------------------------
protected:
	void AddVersionAndFlags();
	BOOL IsValidName(LPCSTR path);
	void Reset();

	UINT GetRemaining   (Mp4File* file);
	int  ReadChildAtoms (Mp4File* file, UINT flags = 0);
	int  ReadProperties (Mp4File* file);
	int  ReadSkip	    (Mp4File* file);
	int  WriteChildAtoms(Mp4File* file);
	int  WriteProperties(Mp4File* file);
	int  WriteFreeAtom  (Mp4File* file);

// Data Members -----------------------------------------------
private:
	INT64 fStart;		///< 这个 Atom 在文件中的开始位置
	INT64 fSize;		///< 这个 Atom 总共的大小, 包括头部
	char  fType[5];		///< 这个 Atom 的类型, Atom 类型总是 4 个字节, 不够用 ' ' 补充
	BOOL  fExpectChild;	///< 是否期望有子 Atom 节点

	Mp4AtomPtr		 fParentAtom;	///< 这个 Atom 的父 Atom 节点
	Mp4AtomArray	 fChildAtoms;	///< 这个 Atom 的子 Atom 列表
	Mp4PropertyArray fProperties;	///< 这个 Atom 的属性列表
};

};

#endif // !defined(_NS_VISION_MP4_ATOM_H)
