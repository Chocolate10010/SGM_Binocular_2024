# 代码说明

## 版权说明

本文档为学习过程中抄的代码，源[三维重建\_李迎松\~的博客-CSDN博客](https://blog.csdn.net/rs_lys/category_9701974.html?spm=1001.2014.3001.5482)

## 环境

WSL+VS Code

通过CMake建立工程，链接第三方库opencv

## 编译

```
mkdir build
cd build
cmake ..
make
```

## 执行

```/home/choc01ate/SGM/SGM_Project_Cmake/bin/main
./SGM/SGM_Project_Cmake/bin/main ./SGM/SGM_Project_Cmake/Input_Image/im2.png ./SGM/SGM_Project_Cmake/Input_Image/im6.png ./SGM/SGM_Project_Cmake/result/dis_5.png
```

argv[0] main ; argc[1] img1 ; argc[2] img2 ; argc[3] disp ; argc[4] min_disparity ;argc[5] max_disparity ;

4\5可选。

# 一些问题

1、没有实现教程中的 填补空洞 以及 弱纹理优化 。

2、img1在视差值0-64即可执行的很好；img2需要在0-120的时候效果才不错。如果你的图片效果不好，可以试试调整视差值范围。
