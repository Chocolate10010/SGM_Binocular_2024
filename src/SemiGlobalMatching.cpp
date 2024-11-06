#include "stdafx.h"
#include "SemiGlobalMatching.h"
#include "sgm_util.h"

SemiGlobalMatching::SemiGlobalMatching(): width_(0), height_(0), img_left_(nullptr), img_right_(nullptr),
                                          census_left_(nullptr), census_right_(nullptr),
                                          cost_init_(nullptr), cost_aggr_(nullptr),
                                          cost_aggr_1_(nullptr), cost_aggr_2_(nullptr),
                                          cost_aggr_3_(nullptr), cost_aggr_4_(nullptr),
                                          cost_aggr_5_(nullptr), cost_aggr_6_(nullptr),
                                          cost_aggr_7_(nullptr), cost_aggr_8_(nullptr),
                                          disp_left_(nullptr), disp_right_(nullptr),
                                          is_initialized_(false)
{
}


SemiGlobalMatching::~SemiGlobalMatching()
{
    Release();
    is_initialized_ = false;
}

bool SemiGlobalMatching::Initialize(const uint32& width, const uint32& height, const SGMOption& option)
{
    width_ = width;
    height_ = height;
    option_ = option;

    if(width == 0 || height == 0) {
        return false;
    }

    //··· 开辟内存空间

    // census值（左右影像）
    const sint32 img_size = width * height;
    census_left_ = new uint32[img_size]();
    census_right_ = new uint32[img_size]();

    // 视差范围
    const sint32 disp_range = option.max_disparity - option.min_disparity;
    if (disp_range <= 0) {
        return false;
    }

    const sint32 size = width * height * disp_range;
    cost_init_ = new uint8[size]();
    cost_aggr_ = new uint16[size]();
    cost_aggr_1_ = new uint8[size]();
    cost_aggr_2_ = new uint8[size]();
    cost_aggr_3_ = new uint8[size]();
    cost_aggr_4_ = new uint8[size]();
    cost_aggr_5_ = new uint8[size]();
    cost_aggr_6_ = new uint8[size]();
    cost_aggr_7_ = new uint8[size]();
    cost_aggr_8_ = new uint8[size]();

    // 视差图
    disp_left_ = new float32[img_size]();
    disp_right_ = new float32[img_size]();
    // 是否初始化成功
    is_initialized_ = census_left_ && census_right_ && cost_init_ && cost_aggr_ && disp_left_;

    return is_initialized_;
}

void SemiGlobalMatching::Release()
{
    // 释放内存
    SAFE_DELETE(census_left_);
    SAFE_DELETE(census_right_);
    SAFE_DELETE(cost_init_);
    SAFE_DELETE(cost_aggr_);
    SAFE_DELETE(cost_aggr_1_);
    SAFE_DELETE(cost_aggr_2_);
    SAFE_DELETE(cost_aggr_3_);
    SAFE_DELETE(cost_aggr_4_);
    SAFE_DELETE(cost_aggr_5_);
    SAFE_DELETE(cost_aggr_6_);
    SAFE_DELETE(cost_aggr_7_);
    SAFE_DELETE(cost_aggr_8_);
    SAFE_DELETE(disp_left_);
    SAFE_DELETE(disp_right_);
}

bool SemiGlobalMatching::Reset(const uint32& width, const uint32& height, const SGMOption& option)
{
    Release();

    // 重置初始化标记
    is_initialized_ = false;

    // 初始化
    return Initialize(width, height, option);
}

bool SemiGlobalMatching::Match(const uint8* img_left, const uint8* img_right, float32* disp_left)
{
    if (!is_initialized_){
        return false;
    }
    if (img_left == nullptr || img_right == nullptr) {
        return false;
    }
    img_left_ = img_left;
    img_right_ = img_right;
    CensusTransform();
    ComputeCost();
    CostAggregation();
    ComputeDisparity();

    // 左右一致性检查
    if (option_.is_check_lr) {
        // 视差计算（右影像）
        ComputeDisparityRight();
        // 一致性检查
        LRCheck();
    }

    // 移除小连通区
    if (option_.is_remove_speckles) {
        sgm_util::RemoveSpeckles(disp_left_, width_, height_, 1, option_.min_speckle_aera, Invalid_Float);
    }

    // 中值滤波
    sgm_util::MedianFilter(disp_left_, disp_left_, width_, height_, 3);

    // 输出视差图
    memcpy(disp_left, disp_left_, width_ * height_ * sizeof(float32));
    return true;
}


void SemiGlobalMatching::CensusTransform() const{
    sgm_util::census_transform_5x5(img_left_, census_left_, width_, height_);
    sgm_util::census_transform_5x5(img_right_, census_right_, width_, height_);
}

void SemiGlobalMatching::ComputeCost() const{
    const sint32& min_disparity = option_.min_disparity;
    const sint32& max_disparity = option_.max_disparity;
    const sint32& disp_range = max_disparity - min_disparity;
    if (disp_range <= 0) {
        return;
    }

	// 计算代价（基于Hamming距离）【核线矫正过，左右图像i相同，j-d就是对应像素的位置】
    for (sint32 i = 0; i < height_; i++) {
        for (sint32 j = 0; j < width_; j++) {

            // 左影像census值
            const uint32 census_val_l = census_left_[i * width_ + j];   //从(0,0)开始，边边区域census值为0

            // 逐视差计算代价值
        	for (sint32 d = min_disparity; d < max_disparity; d++) {
                //cost是对按代价主序的三维数组的第i层，第j行，第d列的【引用】
                auto& cost = cost_init_[i * width_ * disp_range + j * disp_range + (d - min_disparity)];    //cost_init_初值为0
                
                //超出图像范围，则给一个很大的代价值
                if (j - d < 0 || j - d >= width_) {
                    cost = UINT8_MAX/2; 
                    continue;
                }
                // 右影像对应像点的census值
                const uint32 census_val_r = census_right_[i * width_ + j - d];
                
        		// 计算匹配代价
                cost = sgm_util::Hamming32(census_val_l, census_val_r); 
            }
        }
    }
}

void SemiGlobalMatching::ComputeDisparity() const
{
	// 最小最大视差
    const sint32& min_disparity = option_.min_disparity;
    const sint32& max_disparity = option_.max_disparity;
    const sint32& disp_range = max_disparity - min_disparity;
    if(disp_range <= 0) {
        return;
    }

    // 左影像视差图
    const auto disparity = disp_left_;
	// 左影像聚合代价数组
	const auto cost_ptr = cost_aggr_;

    const sint32 width = width_;
    const sint32 height = height_;
    const bool is_check_unique = option_.is_check_unique;
	const float32 uniqueness_ratio = option_.uniqueness_ratio;

	// 为了加快读取效率，把单个像素的所有代价值存储到局部数组里
    std::vector<uint16> cost_local(disp_range);
    
    // 逐像素计算最优视差
    for (sint32 i = 0; i < height; i++) {
        for (sint32 j = 0; j < width; j++) {
            
            uint16 min_cost = UINT16_MAX;
            uint16 sec_min_cost = UINT16_MAX;
            uint16 max_cost = 0;
            sint32 best_disparity = 0;

            // 遍历视差范围内的所有代价值，输出最小代价值及对应的视差值
            for (sint32 d = min_disparity; d < max_disparity; d++) {
                const sint32 d_idx = d - min_disparity;
                // const auto 注意
                const auto& cost = cost_local[d_idx] = cost_ptr[i * width * disp_range + j * disp_range + d_idx];
                if(min_cost > cost) {
                    min_cost = cost;
                    best_disparity = d;
                }
                max_cost = std::max(max_cost, static_cast<uint16>(cost));
            }

            if (is_check_unique) {
                // 再遍历一次，输出次最小代价值
                for (sint32 d = min_disparity; d < max_disparity; d++) {
                    if (d == best_disparity) {
                        // 跳过最小代价值
                        continue;
                    }
                    const auto& cost = cost_local[d - min_disparity];
                    sec_min_cost = std::min(sec_min_cost, cost);
                }

                // 判断唯一性约束
                // 若(min-sec)/min < min*(1-uniquness)，则为无效估计
                if (sec_min_cost - min_cost <= static_cast<uint16>(min_cost * (1 - uniqueness_ratio))) {
                    disparity[i * width + j] = Invalid_Float;
                    continue;
                }
            }

            if (best_disparity == min_disparity || best_disparity == max_disparity - 1) {
                disparity[i * width + j] = Invalid_Float;
                continue;
            }
            // 最优视差前一个视差的代价值cost_1，后一个视差的代价值cost_2
            const sint32 idx_1 = best_disparity - 1 - min_disparity;
            const sint32 idx_2 = best_disparity + 1 - min_disparity;
            const uint16 cost_1 = cost_local[idx_1];
            const uint16 cost_2 = cost_local[idx_2];
            // 解一元二次曲线极值
            const uint16 denom = std::max(1, cost_1 + cost_2 - 2 * min_cost);
            disparity[i * width + j] = static_cast<float32>(best_disparity) + static_cast<float32>(cost_1 - cost_2) / (denom * 2.0f);
        }
    }
}

void SemiGlobalMatching::ComputeDisparityRight() const
{
    const sint32& min_disparity = option_.min_disparity;
    const sint32& max_disparity = option_.max_disparity;
    const sint32 disp_range = max_disparity - min_disparity;
    if (disp_range <= 0) {
        return;
    }

    // 右影像视差图
    const auto disparity = disp_right_;
    // 左影像聚合代价数组
	const auto cost_ptr = cost_aggr_;

    const sint32 width = width_;
    const sint32 height = height_;
    const bool is_check_unique = option_.is_check_unique;
    const float32 uniqueness_ratio = option_.uniqueness_ratio;

    // 为了加快读取效率，把单个像素的所有代价值存储到局部数组里
    std::vector<uint16> cost_local(disp_range);

    // ---逐像素计算最优视差
    // 通过左影像的代价，获取右影像的代价
    // 右cost(xr,yr,d) = 左cost(xr+d,yl,d)
    for (sint32 i = 0; i < height; i++) {
        for (sint32 j = 0; j < width; j++) {
            uint16 min_cost = UINT16_MAX;
            uint16 sec_min_cost = UINT16_MAX;
            sint32 best_disparity = 0;

            // ---统计候选视差下的代价值
        	for (sint32 d = min_disparity; d < max_disparity; d++) {
                const sint32 d_idx = d - min_disparity;
        		const sint32 col_left = j + d;
        		if (col_left >= 0 && col_left < width) {
                    const auto& cost = cost_local[d_idx] = cost_ptr[i * width * disp_range + col_left * disp_range + d_idx];
                    if (min_cost > cost) {
                        min_cost = cost;
                        best_disparity = d;
                    }
        		}
                else {
                    cost_local[d_idx] = UINT16_MAX;
                }
            }

            if (is_check_unique) {
                // 再遍历一次，输出次最小代价值
                for (sint32 d = min_disparity; d < max_disparity; d++) {
                    if (d == best_disparity) {
                        // 跳过最小代价值
                        continue;
                    }
                    const auto& cost = cost_local[d - min_disparity];
                    sec_min_cost = std::min(sec_min_cost, cost);
                }

                // 判断唯一性约束
                // 若(min-sec)/min < min*(1-uniquness)，则为无效估计
                if (sec_min_cost - min_cost <= static_cast<uint16>(min_cost * (1 - uniqueness_ratio))) {
                    disparity[i * width + j] = Invalid_Float;
                    continue;
                }
            }
          
            // ---子像素拟合
            if (best_disparity == min_disparity || best_disparity == max_disparity - 1) {
                disparity[i * width + j] = Invalid_Float;
                continue;
            }

            // 最优视差前一个视差的代价值cost_1，后一个视差的代价值cost_2
            const sint32 idx_1 = best_disparity - 1 - min_disparity;
            const sint32 idx_2 = best_disparity + 1 - min_disparity;
            const uint16 cost_1 = cost_local[idx_1];
            const uint16 cost_2 = cost_local[idx_2];
            // 解一元二次曲线极值
            const uint16 denom = std::max(1, cost_1 + cost_2 - 2 * min_cost);
            disparity[i * width + j] = static_cast<float32>(best_disparity) + static_cast<float32>(cost_1 - cost_2) / (denom * 2.0f);
        }
    }
}

void SemiGlobalMatching::CostAggregation() const{
    // 路径聚合
    // 1、左->右/右->左
    // 2、上->下/下->上
    // 3、左上->右下/右下->左上
    // 4、右上->左上/左下->右上
    //
    // ↘ ↓ ↙   5  3  7
    // →    ←	 1    2
    // ↗ ↑ ↖   8  4  6
    //
    const auto& min_disparity = option_.min_disparity;
    const auto& max_disparity = option_.max_disparity;
    assert(max_disparity > min_disparity);

    const sint32 size = width_ * height_ * (max_disparity - min_disparity);
    if(size <= 0) {
        return;
    }

    const auto& P1 = option_.p1;
    const auto& P2_Int = option_.p2_init;


    if (option_.num_paths == 4 || option_.num_paths == 8) {
        // 左右聚合
        sgm_util::CostAggregateLeftRight(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_1_, true);
        sgm_util::CostAggregateLeftRight(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_2_, false);
        // 上下聚合
		sgm_util::CostAggregateUpDown(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_3_, true);
        sgm_util::CostAggregateUpDown(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_4_, false);
    }

    if (option_.num_paths == 8) {
        // 对角线1聚合
        sgm_util::CostAggregateDagonal_1(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_5_, true);
        sgm_util::CostAggregateDagonal_1(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_6_, false);
        // 对角线2聚合
        sgm_util::CostAggregateDagonal_2(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_7_, true);
        sgm_util::CostAggregateDagonal_2(img_left_, width_, height_, min_disparity, max_disparity, P1, P2_Int, cost_init_, cost_aggr_8_, false);
    }

    // 把4/8个方向加起来
    for (sint32 i = 0; i < size; i++) {
        if (option_.num_paths == 4 || option_.num_paths == 8) {
            cost_aggr_[i] = cost_aggr_1_[i] + cost_aggr_2_[i] + cost_aggr_3_[i] + cost_aggr_4_[i];
        }
        if (option_.num_paths == 8) {
            cost_aggr_[i] += cost_aggr_5_[i] + cost_aggr_6_[i] + cost_aggr_7_[i] + cost_aggr_8_[i];
        }
    }
}

// ---左右一致性检查
void SemiGlobalMatching::LRCheck() const
{
    const int width = width_;
    const int height = height_;

    const float32& threshold = option_.lrcheck_thres;

    // ---左右一致性检查
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            
            // 左影像视差值
        	auto& disp = disp_left_[i * width + j];

            /* 根据视差值找到右影像上对应的同名像素
            加上 0.5 是为了确保视差计算是基于像素中心而不是像素角，这样可以提高匹配的精度*/
        	const auto col_right = static_cast<sint32>(j - disp + 0.5);
            
        	if(col_right >= 0 && col_right < width) {

                // 右影像上同名像素的视差值
                const auto& disp_r = disp_right_[i * width + col_right];
                
        		// 判断两个视差值是否一致（差值在阈值内）
        		if (abs(disp - disp_r) > threshold) {
                    // 左右不一致
                    disp = Invalid_Float;
                }
            }
            else{
                // 通过视差值在右影像上找不到同名像素（超出影像范围）
                disp = Invalid_Float;
            }
        }
    }
}
