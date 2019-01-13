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
#ifndef _NS_VISION_MP4_AVC_H
#define _NS_VISION_MP4_AVC_H

namespace mp4 {

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// H264NalType class

/** H.264 网络抽象层单元 (NALU) 类型定义. */
enum H264NalType 
{
	H264_NAL_TYPE_NON_IDR_SLICE		= 0x01,
	H264_NAL_TYPE_DP_A_SLICE		= 0x02,
	H264_NAL_TYPE_DP_B_SLICE		= 0x03,
	H264_NAL_TYPE_DP_C_SLICE		= 0x04,
	H264_NAL_TYPE_IDR_SLICE			= 0x05,
	H264_NAL_TYPE_SEI				= 0x06,	// 
	H264_NAL_TYPE_SEQ_PARAM			= 0x07,	// 序列参数集
	H264_NAL_TYPE_PIC_PARAM			= 0x08,	// 图像参数集
	H264_NAL_TYPE_ACCESS_UNIT		= 0x09, // 
	H264_NAL_TYPE_END_OF_SEQ		= 0x0a,
	H264_NAL_TYPE_END_OF_STREAM		= 0x0b,
	H264_NAL_TYPE_FILLER_DATA		= 0x0c,
	H264_NAL_TYPE_SEQ_EXTENSION		= 0x0d
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// H264SliceType class

/** H.264 Slice 类型. */
enum H264SliceType 
{
	H264_SLICE_TYPE_P		= 0,
	H264_SLICE_TYPE_B		= 1,
	H264_SLICE_TYPE_I		= 2,
	H264_SLICE_TYPE_SP		= 3,
	H264_SLICE_TYPE_SI		= 4,
	H264_SLICE_TYPE2_P		= 5,
	H264_SLICE_TYPE2_B		= 6,
	H264_SLICE_TYPE2_I		= 7,
	H264_SLICE_TYPE2_SP		= 8,
	H264_SLICE_TYPE2_SI		= 9
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4BitBuffer class

/** 
 * 代表一个比特流解析工具类. 
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4BitBuffer 
{
public:
	/** 错误类型. */
	typedef enum BitstreamErr_t {
		BITSTREAM_TOO_MANY_BITS, 
		BITSTREAM_PAST_END,
	} BitstreamErr_t;

public:
	Mp4BitBuffer(void) {};
	Mp4BitBuffer(const BYTE *buffer, UINT bit_len);
	~Mp4BitBuffer (void) {};

// Operations -------------------------------------------------
public:
	int  GetRemainBits (void);
	UINT GetBits(UINT bitsCount);
	UINT PeekBits(UINT bits);

	void Bookmark(int bSet);
	void Init(const BYTE *buffer, UINT bit_len);
	void SetBit(int bit);
	
// Data Members -----------------------------------------------
private:
	UINT fBitsInBuffer;			///< 
	UINT fBitsBuffer;			///< 
	UINT fBufferSize;			///< 缓存区中数据的比特数
	const BYTE *fBuffer;		///< 比特流缓存区
	
	int  fBookmarkOn;			///< 
	UINT fBitsBufferMark;		///< 未注释
	UINT fBitsInBufferMark;		///< 未注释
	UINT fBufferSizeMark;		///< 未注释
	const BYTE *fBufferMark;	///< 未注释
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// H264SliceHeader struct

/** H264SliceHeader 代表 H.264 条带头信息. */
struct H264SliceHeader 
{	
	BOOL is_slice;						///< 当前单元是否是条带数据
	BYTE nal_unit_type;					///< 当前单元的类型
	UINT slice_type;					///< 当前条带的类型
	BYTE field_pic_flag;				///< 未注释
	BYTE bottom_field_flag;				///< 未注释
	UINT frame_num;						///< 帧序列
	UINT idr_pic_id;					///< 未注释
	UINT pic_order_cnt_lsb;				///< 未注释
	int  delta_pic_order_cnt_bottom;	///< 未注释
	int  delta_pic_order_cnt[2];		///< 未注释
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// H264SeqParams struct

/** H264SeqParams 代表 H.264 序列参数集. */
struct H264SeqParams
{
	BYTE profile;							///< 未注释
	BYTE level;								///< 未注释
	UINT chroma_format_idc;					///< 未注释
	BYTE residual_colour_transform_flag;	///< 未注释
	UINT bit_depth_luma_minus8;				///< 未注释
	UINT bit_depth_chroma_minus8;			///< 未注释
	BYTE qpprime_y_zero_transform_bypass_flag;	///< 未注释
	BYTE seq_scaling_matrix_present_flag;	///< 未注释
	UINT log2_max_frame_num_minus4;			///< 未注释
	UINT log2_max_pic_order_cnt_lsb_minus4;	///< 未注释
	UINT pic_order_cnt_type;				///< 未注释
	BYTE pic_order_present_flag;			///< 未注释
	BYTE delta_pic_order_always_zero_flag;	///< 未注释
	int  offset_for_non_ref_pic;			///< 未注释
	int  offset_for_top_to_bottom_field;	///< 未注释
	UINT pic_order_cnt_cycle_length;		///< 未注释
	short offset_for_ref_frame[256];		///< 未注释
	UINT pic_width;							///< 视频的宽度
	UINT pic_height;						///< 视频的高度
	BYTE frame_mbs_only_flag;				///< 是否只包括了帧
};

//_____________________________________________________________________________
///////////////////////////////////////////////////////////////////////////////
// Mp4Video class 

/** 
 * Mp4Video 代表一个 H.264 头结构解析器.
 *
 * @author ChengZhen (anyou@msn.com)
 */
class Mp4Video
{
public:
	Mp4Video();
	virtual ~Mp4Video();

// Operations -------------------------------------------------
public:
	int  GetFrameNum()	{ return fSliceHeader.frame_num; }	///< 返回当前的帧序号
	int  GetHeight()	{ return fSeqParams.pic_height;  }	///< 返回图像的高度
	int  GetWidth()		{ return fSeqParams.pic_width;   }	///< 返回图像的宽度
	BOOL IsSlice()		{ return fSliceHeader.is_slice;  }	///< 指出当前包是否是一个 Slice

public:
	void Clear();
	int  ParseHeader(const BYTE* nalu, int length);

	static BYTE GetNaluType  (const BYTE *sample);
	static int  GetSliceType (const BYTE *sample, UINT length, BYTE *slice_type);
	static int  ReadSliceInfo(const BYTE *sample, UINT length, H264SeqParams *seq, H264SliceHeader *dec);
	static int  ReadSeqParams(const BYTE *sample, UINT length, H264SeqParams *dec);
	static void ScalingList  (UINT sizeOfScalingList, Mp4BitBuffer *buffer);
	static int  h264_se (Mp4BitBuffer *buffer);
	static UINT h264_ue (Mp4BitBuffer *buffer);
	static UINT GetStartCodeLength(const BYTE* sample);

// Data Members -----------------------------------------------
private:
	H264SliceHeader fSliceHeader;	///< 当前条带的头信息
	H264SeqParams   fSeqParams;		///< 当前序列参数集
};

};

#endif // !defined(_NS_VISION_MP4_AVC_H)
