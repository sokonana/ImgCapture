#include "ImgProcessHelper.h"

// Apply sharpening using unsharp masking
void ImgProcessHelper::Sharpened(double alpha, double gamma, cv::Mat& src, cv::Mat& tgt)
{
    cv::Mat ImgBlur;
    cv::Size Kernel(5, 5);
    cv::GaussianBlur(src, ImgBlur, Kernel, 3.0, 3.0);
    cv::addWeighted(src, alpha, ImgBlur, -1.0, gamma, tgt, -1);

}

// For testing use only

void ImgProcessHelper::LoadImg(unsigned short* array, cv::Mat& Img)
{
    cv::Mat Src;
    cv::Mat Src2;
    Src = cv::imread(cv::samples::findFile("test01.png"), cv::IMREAD_COLOR);

    // Flip the image vertically
    cv::flip(Src, Src2, 0);

    // Apply sharpening
    Sharpened(2.0, 1.5, Src2, Img);
}

void ImgProcessHelper::SaveImg(std::string Filename, cv::Mat& Src)
{
    cv::Mat SharpenedImg;
    Sharpened(2.0, 0, Src, SharpenedImg);
    cv::imwrite(Filename, SharpenedImg);
}