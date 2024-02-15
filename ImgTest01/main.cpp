#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <iostream>
#include <Windows.h>

extern "C"
{
#include "ImgLib.h"
}

using namespace std;
using namespace cv;

unsigned short Get_Avg(unsigned short* Src);

int main()
{
	Mat image (HEIGHT, WIDTH, CV_16U);

	if (init() == 0)
		cout << "Camera initialised." << endl;

	if (fire() == 0)
		cout << "Camera fired!" << endl;

	release();

	unsigned short* array = get_image();

	if (array == NULL)
		cout << "Failure in getting image" << endl;
	else
	{
		cout << "Image acquired!" << endl;
		cout << "Average value = " << Get_Avg(array) << endl;
	}

	double pixel = 0.0;
	// Acquire image into Mat
	for (int ht = 0; ht < image.rows; ht++)
		for (int wd = 0; wd < image.cols; wd++)
		{
			pixel = static_cast<double>(array[ht * image.cols + wd]);
			pixel = pixel / 1022. * 65535.0;
			
			image.at<ushort>(ht, wd) = static_cast<unsigned short>(pixel) ;

			//if (ht < 10)
				//cout << array[ht * image.cols + wd] << " ";
		}

	imshow("Dislpay Window", image);
	waitKey(1000);
	destroyAllWindows();

	// Save file as png
	imwrite("Captured.png", image);

	/*image = imread("test.jpg");
	if (image.empty())
	{
		cout << "Cannot open image file" << endl;
		return -1;
	}

	namedWindow("Display", WINDOW_AUTOSIZE);
	imshow("Display", image);
	waitKey(0);*/

	//HANDLE hHeap = GetProcessHeap();
	//HeapFree(hHeap, 0, array);

	return 0;
}

unsigned short Get_Avg(unsigned short *Src)
{
	unsigned long tmp = 0;
	unsigned short answer = 0;
	unsigned int cnt = 0;

	for (int ht = 0; ht < HEIGHT; ht++)
	{
		for (int wd = 0; wd < WIDTH; wd++)
		{
			tmp += Src[ht * WIDTH + wd];
			cnt++;
		}
	}

	return static_cast<unsigned short>(tmp / cnt);
}