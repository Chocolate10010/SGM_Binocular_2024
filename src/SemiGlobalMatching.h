#pragma once

#include "sgm_types.h"

class SemiGlobalMatching
{
public:
	SemiGlobalMatching();
	~SemiGlobalMatching();

public :
	enum CensusSize {
		Census5x5 = 0,
		Census9x7
	};
	struct SGMOption {
		uint8		num_paths			; // 聚合路径数
		sint32		min_disparity		; // 最小视差
		sint32		max_disparity		; // 最大视差
		CensusSize	census_size			; // census窗口尺寸
		bool		is_check_unique		; // 是否检查唯一性
		float32		uniqueness_ratio	; // 唯一性约束阈值 （最小代价-次最小代价)/最小代价 > 阈值 为有效像素
		bool		is_check_lr			; // 是否检查左右一致性
		float32		lrcheck_thres		; // 左右一致性约束阈值
		bool		is_remove_speckles	; // 是否移除小的连通区
		int			min_speckle_aera	; // 最小的连通区面积（像素数）

		// P1,P2 
		// P2 = P2_int / (Ip-Iq)
		sint32  p1						; // 惩罚项参数P1
		sint32  p2_init					; // 惩罚项参数P2

		SGMOption(): num_paths(8), 
					 min_disparity(0), max_disparity(640),
					 census_size(Census5x5),
					 is_check_unique(true), uniqueness_ratio(0.95f),
					 is_check_lr(true), lrcheck_thres(1.0f),
					 is_remove_speckles(true), min_speckle_aera(20),
					 p1(10), p2_init(150) { }
	};

public:
		/**
	 * \brief 类的初始化，完成一些内存的预分配、参数的预设置等
	 * \param width		输入，核线像对影像宽
	 * \param height	输入，核线像对影像高
	 * \param option	输入，SemiGlobalMatching参数
	 */
	bool Initialize(const uint32& width, const uint32& height, const SGMOption& option);

		/**
	 * \brief 执行匹配
	 * \param img_left		输入，左影像数据指针 
	 * \param img_right		输入，右影像数据指针
	 * \param disp_left		输出，左影像视差图指针，预先分配和影像等尺寸的内存空间
	 */
	bool Match(const uint8* img_left, const uint8* img_right, float32* disp_left);

	/**
	 * \brief 重设
	 * \param width		输入，核线像对影像宽
	 * \param height	输入，核线像对影像高
	 * \param option	输入，SemiGlobalMatching参数
	 */
	bool Reset(const uint32& width, const uint32& height, const SGMOption& option);

private:
	void CensusTransform() const		; //Census变换
	void ComputeCost() const			; //代价计算
	void CostAggregation() const		; //代价聚合
	void ComputeDisparity() const		; //视差计算
	void ComputeDisparityRight() const	; //右视差计算
	void LRCheck() const				; //一致性检查
	void Release()						; //释放空间

	// ↘ ↓ ↙   5  3  7
	// →    ←	 1    2
	// ↗ ↑ ↖   8  4  6
	uint8* cost_aggr_1_		; //代价聚合方向1
	uint8* cost_aggr_2_		; //代价聚合方向2
	uint8* cost_aggr_3_		; //代价聚合方向3
	uint8* cost_aggr_4_		; //代价聚合方向4
	uint8* cost_aggr_5_		; //代价聚合方向5
	uint8* cost_aggr_6_		; //代价聚合方向6
	uint8* cost_aggr_7_		; //代价聚合方向7
	uint8* cost_aggr_8_		; //代价聚合方向8
	
private:
	SGMOption option_		; //SGM参数
	sint32 width_			; //影像宽
	sint32 height_			; //影像高
	const uint8* img_left_	; //左影像数据
	const uint8* img_right_	; //右影像数据
	void* census_left_		; //左影像census值
	void* census_right_		; //右影像census值
	uint8* cost_init_		; //初始匹配代价
	uint16* cost_aggr_		; //聚合匹配代价
	float32* disp_left_		; //左影像视差图
	float32* disp_right_	; //右影像视差图
	bool is_initialized_	; //是否初始化标志
};

