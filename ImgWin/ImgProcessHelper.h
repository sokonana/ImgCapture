#pragma once

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

class ImgProcessHelper
{
protected:
    void Sharpened(double alpha, double gamma, cv::Mat& src, cv::Mat& tgt);
public:
    void LoadImg(unsigned short* array, cv::Mat& Img);
    void SaveImg(std::string Filename, cv::Mat& Src);
};

