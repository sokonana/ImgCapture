// C++ class library to prepare and fire image capturing of Thorlab sensor
// Created 

#include <iostream>
#include "ImgCapture.h"

extern "C"
{
#include "tl_camera_sdk.h"
#include "tl_camera_sdk_load.h"
}

using namespace std;


unsigned short *ImgCapture::get_img()
{
    if (image_buffer == 0)
        return NULL;

    return image_buffer;
}

int ImgCapture::initialize_camera_resources()
{
    // Initializes camera dll
    if (tl_camera_sdk_dll_initialize())
        return report_error_and_cleanup_resources("Failed to initialize dll!\n");
    cout << "Successfully initialized dll" << endl;
    is_camera_dll_open = 1;

    // Open the camera SDK
    if (tl_camera_open_sdk())
        return report_error_and_cleanup_resources("Failed to open SDK!\n");
    
    cout << "Successfully opened SDK" << endl;
    is_camera_sdk_open = 1;

    char camera_ids[1024];
    camera_ids[0] = 0;

    // Discover cameras.
    if (tl_camera_discover_available_cameras(camera_ids, 1024))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    cout << "camera IDs: " << camera_ids << endl;

    // Check for no cameras.
    if (!strlen(camera_ids))
        return report_error_and_cleanup_resources("Did not find any cameras!\n");

    // Camera IDs are separated by spaces.
    char* p_space = strchr(camera_ids, ' ');
    if (p_space)
        *p_space = '\0'; // isolate the first detected camera
    char first_camera[256];

    strcpy_s(first_camera, 256, camera_ids);

    cout << "First camera_id = " << first_camera << endl;

    // Connect to the camera (get a handle to it).
    if (tl_camera_open_camera(first_camera, &camera_handle))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    cout.setf(ios::hex, ios::basefield);
    cout << "Camera handle = " << camera_handle << endl;
    cout.setf(ios::dec, ios::basefield);

    return 0;
}

int ImgCapture::report_error_and_cleanup_resources(const char* error_string)
{
    int num_errors = 0;

    if (error_string)
    {
        cout << "Error: " << error_string << endl;
        num_errors++;
    }

    cout << "Closing all resources..." << endl;

    if (camera_handle)
    {
        if (tl_camera_close_camera(camera_handle))
        {
            cout << "Failed to close camera!" << endl <<  tl_camera_get_last_error() << endl;
            num_errors++;
        }
        camera_handle = 0;
    }
    if (is_camera_sdk_open)
    {
        if (tl_camera_close_sdk())
        {
            cout << "Failed to close SDK!" << endl;
            num_errors++;
        }
        is_camera_sdk_open = 0;
    }
    if (is_camera_dll_open)
    {
        if (tl_camera_sdk_dll_terminate())
        {
            cout << "Failed to close dll!" << endl;
            num_errors++;
        }
        is_camera_dll_open = 0;
    }

    cout << "Closing resources finished." << endl;
    return num_errors;
}

int ImgCapture::SetGain(double NewGain)
{
    //const double gain_dB = 30.0;
    int gain_index;
    if (tl_camera_convert_decibels_to_gain(camera_handle, NewGain, &gain_index))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    tl_camera_set_gain(camera_handle, gain_index);

    return 0;
}

int ImgCapture::init()
{
    if (initialize_camera_resources())
        return 1;

    // Set the exposure
    long long const exposure = 10000; // 10 ms
    if (tl_camera_set_exposure_time(camera_handle, exposure))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    cout << "Camera exposure set to " << exposure << endl;

    // Set the gain
    int gain_min;
    int gain_max;
    if (tl_camera_get_gain_range(camera_handle, &gain_min, &gain_max))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    if (gain_max > 0)
    {
        // this camera supports gain, set it to 6.0 decibels
        const double gain_dB = 30.0;
        int gain_index;
        if (tl_camera_convert_decibels_to_gain(camera_handle, gain_dB, &gain_index))
            return report_error_and_cleanup_resources(tl_camera_get_last_error());
        tl_camera_set_gain(camera_handle, gain_index);
    }


    // Configure camera for continuous acquisition by setting the number of frames to 0.
    if (tl_camera_set_frames_per_trigger_zero_for_unlimited(camera_handle, 0))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    // Set camera to wait 100 ms for a frame to arrive during a poll.
    // If an image is not received in 100ms, the returned frame will be null
    if (tl_camera_set_image_poll_timeout(camera_handle, 100))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    if (tl_camera_arm(camera_handle, 2))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    cout << "Camera armed" << endl;

    // Get image width and height
    if (tl_camera_get_image_width(camera_handle, &image_width))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    if (tl_camera_get_image_height(camera_handle, &image_height))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    return 0;
}

int ImgCapture::close()
{
    // Stop the camera.
    if (tl_camera_disarm(camera_handle))
        cout << "Failed to stop the camera!" << endl;

    // Clean up and exit
    return report_error_and_cleanup_resources(0);
}

int ImgCapture::shoot(int frame)
{
    if (tl_camera_issue_software_trigger(camera_handle))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    cout << "Software trigger sent" << endl;


    //initialize frame variables
    
    int frame_count = 0;
    unsigned char* metadata = 0;
    int metadata_size_in_bytes = 0;

    //Poll for specific number of images
    int count = 0;

    while (count < frame)
    {
        if (tl_camera_get_pending_frame_or_null(camera_handle, &image_buffer, &frame_count, &metadata, &metadata_size_in_bytes))
            return report_error_and_cleanup_resources(tl_camera_get_last_error());
        if (!image_buffer)
            continue; //timeout

        //printf("Pointer to image: 0x%p\n", image_buffer);
        //printf("Frame count: %d\n", frame_count);
        //printf("Pointer to metadata: 0x%p\n", metadata);
        //printf("Metadata size in bytes: %d\n", metadata_size_in_bytes);

        count++;
    }

    printf("Images received! Closing camera...\n");
    return 0;
}

int ImgCapture::get_width()
{
    return image_width;
}

int ImgCapture::get_height()
{
    return image_height;
}