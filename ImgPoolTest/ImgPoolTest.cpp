// ImgPoolTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "ImgCapture.h"

int main()
{
    ImgCapture *ImgObj = new ImgCapture();

    std::cout << "Calling init()";
    ImgObj->init();

    // Calling image capture here

    std::cout << "Calling close()";
    ImgObj->close();

    delete ImgObj;
}

