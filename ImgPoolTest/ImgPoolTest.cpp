// ImgPoolTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include "ImgCapture.h"

using namespace cv;

int main()
{
    ImgCapture *ImgObj = new ImgCapture();

    std::cout << "Calling init()";
    if (ImgObj->init() == 0)
    {
        std::cout << "Successfully called init()\n";

        // Retrieve image dimensions
        int height = ImgObj->get_height();
        int width = ImgObj->get_width();

		int TOTAL_LOOP = 5;
		for (int loop = 1; loop <= TOTAL_LOOP; loop++)
		{
			Mat image(height, width, CV_16U);

			ImgObj->shoot(1);

			unsigned short* array = ImgObj->get_img();
			if (array == NULL)
				std::cout << "Failure in getting image\n";
			else
			{
				std::cout << "Image acquired!\n";
				//std::cout << "Average value = " << Get_Avg(array) << endl;
			}

			double pixel = 0.0;
			// Acquire image into Mat
			for (int ht = 0; ht < image.rows; ht++)
				for (int wd = 0; wd < image.cols; wd++)
				{
					pixel = static_cast<double>(array[ht * image.cols + wd]);
					pixel = pixel / 1022. * 65535.0;

					image.at<ushort>(ht, wd) = static_cast<unsigned short>(pixel);

					//if (ht < 10)
						//cout << array[ht * image.cols + wd] << " ";
				}

			//imshow("Dislpay Window", image);
			//waitKey(1000);
			//destroyAllWindows();

			// Save file as png
			std::stringstream fileout;

			fileout << "Captured_" << loop <<  ".png";
			imwrite(fileout.str(), image);
		}
        std::cout << "Calling close()";
        if (ImgObj->close() == 0)
            std::cout << "Successfully called close()\n";
    }
    delete ImgObj;
}

