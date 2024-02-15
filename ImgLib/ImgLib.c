#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "windows.h"
#include "ImgLib.h"

#define _CRT_SECURE_NO_WARNINGS


unsigned short *image = NULL;

#include "tl_camera_sdk.h"
#include "tl_camera_sdk_load.h"

HANDLE frame_acquired_event = 0;
int is_camera_sdk_open = 0;
int is_camera_dll_open = 0;
void* camera_handle = 0;
int clock = 0;

int report_error_and_cleanup_resources(const char* error_string);
int initialize_camera_resources();
void frame_available_callback(void* sender, unsigned short* image_buffer, int frame_count, unsigned char* metadata, int metadata_size_in_bytes, void* context);
void camera_connect_callback(char* cameraSerialNumber, enum TL_CAMERA_USB_PORT_TYPE usb_bus_speed, void* context);
void camera_disconnect_callback(char* cameraSerialNumber, void* context);

volatile int is_first_frame_finished = 0;

int init()
{
    if (initialize_camera_resources())
        return 1;

    // Set the camera connect event callback. This is used to register for run time camera connect events.
    if (tl_camera_set_camera_connect_callback(camera_connect_callback, 0))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    // Set the camera disconnect event callback. This is used to register for run time camera disconnect events.
    if (tl_camera_set_camera_disconnect_callback(camera_disconnect_callback, 0))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    // Set the exposure
    long long const exposure = 20000; // 10 ms
    if (tl_camera_set_exposure_time(camera_handle, exposure))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    printf("Camera exposure set to %lld\n", exposure);

    // Set the gain
    int gain_min;
    int gain_max;
    if (tl_camera_get_gain_range(camera_handle, &gain_min, &gain_max))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    if (gain_max > 0)
    {
        // this camera supports gain, set it to 6.0 decibels
        const double gain_dB = 6.0;
        int gain_index;
        if (tl_camera_convert_decibels_to_gain(camera_handle, gain_dB, &gain_index))
            return report_error_and_cleanup_resources(tl_camera_get_last_error());
        tl_camera_set_gain(camera_handle, gain_index);
    }

    // Configure camera for continuous acquisition by setting the number of frames to 0.
    // This project only waits for the first frame before exiting
    if (tl_camera_set_frames_per_trigger_zero_for_unlimited(camera_handle, 0))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    // Set the frame available callback
    if (tl_camera_set_frame_available_callback(camera_handle, frame_available_callback, 0))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());

    /* Allocate Heapr memory to hold the image here */
    HANDLE hHeap = HeapCreate(0, 0, 0);

    if (hHeap == NULL)
        return -1;

    image = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, WIDTH * HEIGHT * sizeof(unsigned short));

    if (image == NULL)
        return -1;


    return 0;
}

int fire()
{
    // Arm the camera.
    // if Hardware Triggering, make sure to set the operation mode before arming the camera.
    if (tl_camera_arm(camera_handle, 2))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    printf("Camera armed\n");


    /* Get clock frequency */
    tl_camera_get_timestamp_clock_frequency(camera_handle, &clock);

    /**SOFTWARE TRIGGER**/
    /*
        Once the camera is initialized and armed, this function sends a trigger command to the camera over USB, GE, or CL.
        Camera will return images using a worker thread to call frame_available_callback.
        Continuous acquisition is specified by setting the number of frames to 0 and issuing a single software trigger request.

        Comment out the following code block if using Hardware Triggering.
    */
    if (tl_camera_issue_software_trigger(camera_handle))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    printf("Software trigger sent\n");

    // Wait to get an image from the frame available callback
    printf("Waiting for an image...\n");

    for (;;)
    {
        WaitForSingleObject(frame_acquired_event, INFINITE);

        // break after 1 frame capture
        if (is_first_frame_finished) break;
    }

    printf("Image received! Closing camera...\n");

    // Stop the camera.
    if (tl_camera_disarm(camera_handle))
        printf("Failed to stop the camera!\n");

   
    return 0;
}

int release()
{
    // Clean up and exit
    return report_error_and_cleanup_resources(0);
}

unsigned short *get_image()
{
    if (image != NULL)
        return image;
    else
        return NULL;

}

int initialize_camera_resources()
{
    // Initializes camera dll
    if (tl_camera_sdk_dll_initialize())
        return report_error_and_cleanup_resources("Failed to initialize dll!\n");
    printf("Successfully initialized dll\n");
    is_camera_dll_open = 1;

    // Open the camera SDK
    if (tl_camera_open_sdk())
        return report_error_and_cleanup_resources("Failed to open SDK!\n");
    printf("Successfully opened SDK\n");
    is_camera_sdk_open = 1;

    char camera_ids[1024];
    camera_ids[0] = 0;

    // Discover cameras.
    if (tl_camera_discover_available_cameras(camera_ids, 1024))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    printf("camera IDs: %s\n", camera_ids);

    // Check for no cameras.
    if (!strlen(camera_ids))
        return report_error_and_cleanup_resources("Did not find any cameras!\n");

    // Camera IDs are separated by spaces.
    char* p_space = strchr(camera_ids, ' ');
    if (p_space)
        *p_space = '\0'; // isolate the first detected camera
    char first_camera[256];

    // Copy the ID of the first camera to separate buffer (for clarity)
    strcpy_s(first_camera, 256, camera_ids);
    printf("First camera_id = %s\n", first_camera);

    // Connect to the camera (get a handle to it).
    if (tl_camera_open_camera(first_camera, &camera_handle))
        return report_error_and_cleanup_resources(tl_camera_get_last_error());
    printf("Camera handle = 0x%p\n", camera_handle);

    return 0;
}

/*
    Reports the given error string if it is not null and closes any opened resources. Returns the number of errors that occured during cleanup, +1 if error string was not null.
 */
int report_error_and_cleanup_resources(const char* error_string)
{
    int num_errors = 0;

    if (error_string)
    {
        printf("Error: %s\n", error_string);
        num_errors++;
    }

    printf("Closing all resources...\n");

    if (camera_handle)
    {
        if (tl_camera_close_camera(camera_handle))
        {
            printf("Failed to close camera!\n%s\n", tl_camera_get_last_error());
            num_errors++;
        }
        camera_handle = 0;
    }
    if (is_camera_sdk_open)
    {
        if (tl_camera_close_sdk())
        {
            printf("Failed to close SDK!\n");
            num_errors++;
        }
        is_camera_sdk_open = 0;
    }
    if (is_camera_dll_open)
    {
        if (tl_camera_sdk_dll_terminate())
        {
            printf("Failed to close dll!\n");
            num_errors++;
        }
        is_camera_dll_open = 0;
    }

    if (frame_acquired_event)
    {
        if (!CloseHandle(frame_acquired_event))
        {
            printf("Failed to close concurrent data structure!\n");
            num_errors++;
        }
    }


    printf("Closing resources finished.\n");
    return num_errors;
}

// The callback that is registered with the camera
void frame_available_callback(void* sender, unsigned short* image_buffer, int frame_count, unsigned char* metadata, int metadata_size_in_bytes, void* context)
{
    if (is_first_frame_finished)
        return;

    printf("image buffer = 0x%p\n", image_buffer);
    printf("frame_count = %d\n", frame_count);
    printf("meta data buffer = 0x%p\n", metadata);
    printf("metadata size in bytes = %d\n", metadata_size_in_bytes);

    is_first_frame_finished = 1; // signal to stop after 1 frame
    
    /* Copy out the pixel value */
    for (int idx = 0; idx < HEIGHT * WIDTH; idx++)
    {      
        image[idx] = image_buffer[idx];
     
    }

    SetEvent(frame_acquired_event);

}

void camera_connect_callback(char* cameraSerialNumber, enum TL_CAMERA_USB_PORT_TYPE usb_bus_speed, void* context)
{
    printf("camera %s connected with bus speed = %d!\n", cameraSerialNumber, usb_bus_speed);
}

void camera_disconnect_callback(char* cameraSerialNumber, void* context)
{
    printf("camera %s disconnected!\n", cameraSerialNumber);
}
