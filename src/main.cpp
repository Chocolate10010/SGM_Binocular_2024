#include "stdafx.h"
#include "SemiGlobalMatching.h"
#include "sgm_types.h"
using namespace std::chrono;
#include <opencv2/opencv.hpp>

/**
 * \brief 
 * \param argv 3
 * \param argc argc[1]:左影像路径 argc[2]: 右影像路径 argc[3]: 视差图路径
 * \return 
*/

int main(int argv,char** argc)
{
    if(argv < 3) {
        std::cout << "Insufficient Parameters,Task Failed." << std::endl;
        return 0;
    }

    std::string path_left  = argc[1]    ;       // ··· 读取影像
    std::string path_right = argc[2]    ;

    cv::Mat img_left   = cv::imread(path_left , cv::IMREAD_GRAYSCALE);
    cv::Mat img_right  = cv::imread(path_right, cv::IMREAD_GRAYSCALE);

    if (img_left.data == nullptr || img_right.data == nullptr) {
        std::cout << "Reading image failed."     << std::endl       ;
        return -1                                                   ;
    }
    if (img_left.rows != img_right.rows || img_left.cols != img_right.cols) {
        std::cout << "Image sizes inconsistent." << std::endl       ;
        return -1                                                   ;
    }

    const uint32 width  = static_cast<uint32>(img_left.cols )       ;
    const uint32 height = static_cast<uint32>(img_right.rows)       ;

    SemiGlobalMatching::SGMOption sgm_option                        ; //【匹配参数设计】
    sgm_option.num_paths          = 8                               ;
    sgm_option.min_disparity      = argv < 4 ? 0 : atoi(argc[3])    ; //候选视差范围
    sgm_option.max_disparity      = argv < 5 ? 64 : atoi(argc[4])   ;
    sgm_option.census_size        = SemiGlobalMatching::Census5x5   ; // census窗口类型
    sgm_option.is_check_lr        = true                            ; // 一致性检查
    sgm_option.lrcheck_thres      = 1.0f                            ;
    sgm_option.is_check_unique    = true                            ; // 唯一性约束
    sgm_option.uniqueness_ratio   = 0.99                            ;
    sgm_option.p1                 = 10                              ; // 惩罚项参数
    sgm_option.p2_init            = 150                             ;
    sgm_option.is_remove_speckles = true                            ; // 剔除小连通区
    sgm_option.min_speckle_aera   = 50                              ;
    SemiGlobalMatching              sgm                             ; //【定义sgm匹配类实例】
    printf("w = %d, h = %d, d = [%d,%d]\n\n", 
            width, height, sgm_option.min_disparity, sgm_option.max_disparity);

    printf("SGM Initializing...\n")                                 ; //【SGM Initializing】
    auto start      = std::chrono::steady_clock::now()              ; //计时开始
    if (!sgm.Initialize(width, height, sgm_option)) {                 // 初始化
        std::cout << "SGM initialization failed!" << std::endl      ;
        return -2;
    }                                                                 
    auto end = std::chrono::steady_clock::now()                     ;
    auto tt = duration_cast<std::chrono::milliseconds>(end - start) ;
    printf("SGM Initializing Done! Timing : %lf s\n\n", 
                                    tt.count() / 1000.0)            ;


    printf("SGM Matching...\n")                                     ;
    start = std::chrono::steady_clock::now()                        ;
    auto disparity  = new float32[width * height]()                 ; //初始化视差图
    if (!sgm.Match(img_left.data, img_right.data,disparity)) {          //执行匹配
        std::cout << "SGM matching failed!" << std::endl            ;
        return -2;
    }
    end = std::chrono::steady_clock::now()                          ;
    tt  = duration_cast<std::chrono::milliseconds>(end - start)     ;
    printf("\nSGM Matching...Done! Timing :   %lf s\n", 
                                    tt.count() / 1000.0)            ;

    cv::Mat disp_mat = cv::Mat(height, width, CV_8UC1)              ;
    float min_disp = 0, max_disp = 0                                ; //不知道初值应该赋什么，源代码得出结果很大，有问题
    for (sint32 i = 0; i < height; i++) {
        for (sint32 j = 0; j < width; j++) {
            const float32 disp = disparity[i * width + j]           ;
            if (disp != Invalid_Float) {
                min_disp = std::min(min_disp, disp)                 ;
                max_disp = std::max(max_disp, disp)                 ;
            }
        }
    }

    for (uint32 i=0;i<height;i++) {
	    for(uint32 j=0;j<width;j++) {
            const float32 disp = disparity[i * width + j];
            if (disp == Invalid_Float) {
                disp_mat.data[i * width + j] = 0;
            }
            else {
                disp_mat.data[i * width + j] = static_cast<uchar>((disp - min_disp) / (max_disp - min_disp) * 255);
            }
	    }
    }

    cv::Mat       disp_color                                                     ;
    applyColorMap(disp_mat, disp_color, cv::COLORMAP_JET)                        ;
    std::string disp_map_path = argc[3]; disp_map_path += ".d.png"               ;
    std::string disp_color_map_path = argc[3]; disp_color_map_path += ".c.png"   ;
    cv::imwrite(disp_map_path, disp_mat)                                         ; //视差图写入指定路径
    cv::imwrite(disp_color_map_path, disp_color)                                 ; //彩色视差图写入指定路径

    delete[]    disparity               ;
    disparity   = nullptr               ;
    
    std::cout << "Task Finished" << std::endl;
	return 0;
}
