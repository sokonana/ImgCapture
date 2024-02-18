#pragma once

class ImgCapture
{
protected:
    int is_camera_sdk_open = 0;
    int is_camera_dll_open = 0;
    void* camera_handle = 0;
    int image_width = 0;
    int image_height = 0;

    int initialize_camera_resources();
    int report_error_and_cleanup_resources(const char* error_string);

    unsigned short* image_buffer = 0;
public:
    int init();
    int shoot(int frame);
    int close();
    int SetGain(double);

    unsigned short* get_img();
    int get_width();
    int get_height();
};