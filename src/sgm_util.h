#pragma once
#include "sgm_types.h"

#ifndef SAFE_DELETE
#define SAFE_DELETE(P) {if(P) delete[](P);(P)=nullptr;}
#endif

namespace sgm_util{
    void census_transform_5x5(const uint8* source, uint32* census, const sint32& width, const sint32& height);
    uint16 Hamming32(const uint32& x, const uint32& y);
    void CostAggregateLeftRight(const uint8* img_data, const sint32& width, const sint32& height, const sint32& min_disparity, const sint32& max_disparity,
        const sint32& p1, const sint32& p2_init, const uint8* cost_init, uint8* cost_aggr, bool is_forward);
    void CostAggregateUpDown(const uint8* img_data, const sint32& width, const sint32& height,const sint32& min_disparity, const sint32& max_disparity, 
        const sint32& p1, const sint32& p2_init, const uint8* cost_init, uint8* cost_aggr, bool is_forward);
    void CostAggregateDagonal_1(const uint8* img_data, const sint32& width, const sint32& height,const sint32& min_disparity, const sint32& max_disparity,
        const sint32& p1, const sint32& p2_init,const uint8* cost_init, uint8* cost_aggr, bool is_forward);
    void CostAggregateDagonal_2(const uint8* img_data, const sint32& width, const sint32& height,const sint32& min_disparity, const sint32& max_disparity, 
        const sint32& p1, const sint32& p2_init,const uint8* cost_init, uint8* cost_aggr, bool is_forward);
    void RemoveSpeckles(float32* disparity_map, const sint32& width, const sint32& height,
	    const sint32& diff_insame, const uint32& min_speckle_aera, const float& invalid_val);
    void sgm_util::MedianFilter(const float32* in, float32* out, const sint32& width, const sint32& height,
        const sint32 wnd_size);

}
