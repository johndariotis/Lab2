// #include "address_map_arm.h"
#include <stdio.h>
#include <time.h>

#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000

// Simple 8x8 font for rendering characters
const unsigned char font[96][8] = {
    // Include a simple 8x8 font here (e.g., ASCII characters 32-127)
    // Example: Space, '0', '1', ..., '9', ':', etc.
    // Each character is 8 bytes (8 rows of 8 bits).
    // You can find a basic 8x8 font online or define your own.
};

int main(void)
{
    time_t rawtime;
    struct tm *timeinfo;
    volatile int * KEY_ptr             = (int *) KEY_BASE;
    volatile int * Video_In_DMA_ptr    = (int *) VIDEO_IN_BASE;
    volatile short * Video_Mem_ptr     = (short *) FPGA_ONCHIP_BASE;

    int x, y;
    int offset_stamp;
    int offset_count;
    int count = 0;
    int pic_flag = 0; // 0: video going, 1: picture taken
    int bw_flag = 0; // 0: white&black, 1: black&white

    *(Video_In_DMA_ptr + 3) = 0x4; // Enable the video

    // Wait for a key press to capture a frame
    while (1)
    {
        //retrieve time info
        rawtime = time(NULL);
        timeinfo = gmtime(&rawtime);
        timeinfo->tm_hour-=4;
        char *stamp_ptr = asctime(timeinfo); // pointer to timestamp to display
        stamp_ptr[strlen(stamp_ptr)-1] = '\0'; // changes last character to null terminator
        
        if (*KEY_ptr == 0b0001 && pic_flag == 0) // Check if any KEY was pressed
        {
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            pic_flag = 1; // denotes that picture was taken
            *(Video_In_DMA_ptr + 3) = 0x0; // Disable the video to capture one frame

            // Display the timestamp
            offset_stamp = (1<<7) + 1;
            while(*(stamp_ptr)){
                *((volatile char*) (0xC9000000+offset_stamp)) = *(stamp_ptr);
                ++stamp_ptr;
                ++offset_stamp;
            }

            // Display the shot counter
            offset_count = (58<<7) + 1;
            ++count;
            char count_buffer[12];
            char *count_ptr = count_buffer;
            snprintf(count_ptr, sizeof(count_ptr), "%d", count);
            while(*(count_ptr)){
                *((volatile char*) (0xC9000000+offset_count)) = *(count_ptr);
                ++count_ptr;
                ++offset_count;
            }
        }
        
        if (*KEY_ptr == 0b0001 && pic_flag == 1)
        {
            pic_flag = 0; // denotes that the video resumed
            
            // remove the timestamp
            offset_stamp = (1<<7) + 1;
            while(*(stamp_ptr)){
                *((volatile char*) (0xC9000000+offset_stamp)) = ' ';
                ++stamp_ptr;
                ++offset_stamp;
            }

            *(Video_In_DMA_ptr + 3) = 0x4; // Enable the video
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
        }

        if (*KEY_ptr == 0b0010) // Check if KEY_2 was pressed
        {
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            short temp_frame[240][320]; // temp buffer for all pixels in frame

            // Read and store the mirrored frame into the buffer
            for (y = 0; y < 240; y++) {
                for (x = 0; x < 320; x++) {
                    temp_frame[y][x] = *(Video_Mem_ptr + ((239-y) << 9) + x); // store pixels in reverse horizontal order
                }
            }
            
            // Write the mirrored frame back to the video memory
            for (y = 0; y < 240; y++) {
                for (x = 0; x < 320; x++) {
                    *(Video_Mem_ptr + (y << 9) + x) = temp_frame[y][x];
                }
            }
        }
        if (*KEY_ptr == 0b0100) // Check if KEY_2 was pressed
        {
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            short temp_frame[240][320]; // temp buffer for all pixels in frame

            // Read and store the mirrored frame into the buffer
            for (y = 0; y < 240; y++) {
                for (x = 0; x < 320; x++) {
                    temp_frame[y][x] = *(Video_Mem_ptr + (y << 9) + (319-x)); // store pixels in reverse horizontal order
                }
            }
            
            // Write the mirrored frame back to the video memory
            for (y = 0; y < 240; y++) {
                for (x = 0; x < 320; x++) {
                    *(Video_Mem_ptr + (y << 9) + x) = temp_frame[y][x];
                }
            }
        }

        if (*KEY_ptr == 0b1000 && bw_flag == 0) // Check if KEY_3 was pressed
        {
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            
            bw_flag = 1; // denotes that image is now black&white

            // Display the captured frame in black and white
            for (y = 0; y < 240; y++) {
                for (x = 0; x < 320; x++) {
                    short pixel = *(Video_Mem_ptr + (y << 9) + x); // Read the pixel value
                    // Convert the pixel to black and white using an intensity threshold
                    short intensity = (pixel & 0xFF); // Extract the grayscale intensity (assuming 8-bit grayscale)
                    short bw_pixel;
                    if (intensity > 128){
                        bw_pixel = 0xFFFF;
                    } else {
                        bw_pixel = 0x0000;
                    }
                    *(Video_Mem_ptr + (y << 9) + x) = bw_pixel; // Write the black-and-white pixel
                }
            }
        }

        if (*KEY_ptr == 0b1000) // Check if KEY_3 was pressed
        {
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            
            bw_flag = 0; // denotes that image is now white&black

            // Display the captured frame in white and black
            for (y = 0; y < 240; y++) {
                for (x = 0; x < 320; x++) {
                    short pixel = *(Video_Mem_ptr + (y << 9) + x); // Read the pixel value
                    // Convert the pixel to black and white using an intensity threshold
                    short intensity = (pixel & 0xFF); // Extract the grayscale intensity (assuming 8-bit grayscale)
                    short bw_pixel;
                    if (intensity > 128){
                        bw_pixel = 0x0000;
                    } else {
                        bw_pixel = 0xFFFF;
                    }
                    *(Video_Mem_ptr + (y << 9) + x) = bw_pixel; // Write the black-and-white pixel
                }
            }
        }
    }

    //printf("picture captured!\n");
    // Wait for another key press to display the frame
    


    // Generate the timestamp
    //get_timestamp(timestamp, sizeof(timestamp));

    

    //int text_x = 320 - 150; // Adjust the X position to fit the timestamp
    //int text_y = 240 - 20;  // Adjust the Y position to fit the timestamp

    //for (i = 0; timestamp[i] != '\0'; i++) {
        // Get the font data for the current character
    //    unsigned char *char_data = font[timestamp[i] - 32]; // Adjust for ASCII offset

        // Render the character
    //    for (dy = 0; dy < 8; dy++) {
    //        for (dx = 0; dx < 8; dx++) {
    //            if (char_data[dy] & (1 << (7 - dx))) {
    //                *(Video_Mem_ptr + ((text_y + dy) << 9) + (text_x + i * 8 + dx)) = 0xFFFF; // White pixel
    //            }
    //        }
    //    }
    //}

    return 0; // Explicit return statement to fix the warning
}