#代码说明

##版权说明

本文档为学习过程中抄的代码，源[三维重建\_李迎松\~的博客-CSDN博客](https://blog.csdn.net/rs_lys/category_9701974.html?spm=1001.2014.3001.5482)

##环境

WSL+VS Code

通过CMake建立工程，链接第三方库opencv

##编译

```
mkdir build
cd build
cmake ..
make
```

##执行

```/home/choc01ate/SGM/SGM_Project_Cmake/bin/main
./SGM/SGM_Project_Cmake/bin/main ./SGM/SGM_Project_Cmake/Input_Image/im2.png ./SGM/SGM_Project_Cmake/Input_Image/im6.png ./SGM/SGM_Project_Cmake/result/dis_5.png
```

四个参数分别为：main im2.png im.png dis_5.png

#一些问题

1、没有实现教程中的弱纹理优化。

2、img1执行的效果很好。

3、教程中给出的img2执行的效果也不错，但是我跑出来的结果很“辣眼睛”。不知道怎么回事。
