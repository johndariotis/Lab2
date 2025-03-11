// #include "address_map_arm.h"
#include <stdio.h>
#include <time.h>

#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000

// Function to get the current timestamp as a string
void get_timestamp(char *timestamp, int size) {
    time_t rawtime;
    struct tm *timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(timestamp, size, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Simple 8x8 font for rendering characters
const unsigned char font[96][8] = {
    // Include a simple 8x8 font here (e.g., ASCII characters 32-127)
    // Example: Space, '0', '1', ..., '9', ':', etc.
    // Each character is 8 bytes (8 rows of 8 bits).
    // You can find a basic 8x8 font online or define your own.
};

int main(void)
{
    volatile int * KEY_ptr             = (int *) KEY_BASE;
    volatile int * Video_In_DMA_ptr    = (int *) VIDEO_IN_BASE;
    volatile short * Video_Mem_ptr     = (short *) FPGA_ONCHIP_BASE;

    int x, y;
    char timestamp[20]; // Buffer to store the timestamp
    int i, dy, dx; // Variables for loop counters (C89 compatibility)

    *(Video_In_DMA_ptr + 3) = 0x4; // Enable the video

    // Wait for a key press to capture a frame
    while (1)
    {
        if (*KEY_ptr != 0) // Check if any KEY was pressed
        {
            *(Video_In_DMA_ptr + 3) = 0x0; // Disable the video to capture one frame
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            break;
        }
    }

    // Wait for another key press to display the frame
    while (1)
    {
        if (*KEY_ptr != 0) // Check if any KEY was pressed
        {
            while (*KEY_ptr != 0); // Wait for pushbutton KEY release
            break;
        }
    }

    // Generate the timestamp
    get_timestamp(timestamp, sizeof(timestamp));

    // Display the captured frame in black and white
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            short pixel = *(Video_Mem_ptr + (y << 9) + x); // Read the pixel value
            // Convert the pixel to black and white using a threshold
            short intensity = (pixel & 0xFF); // Extract the grayscale intensity (assuming 8-bit grayscale)
            short bw_pixel = (intensity > 128) ? 0xFFFF : 0x0000; // Apply threshold
            *(Video_Mem_ptr + (y << 9) + x) = bw_pixel; // Write the black-and-white pixel
        }
    }

    // Display the timestamp in the bottom-right corner
    int text_x = 320 - 150; // Adjust the X position to fit the timestamp
    int text_y = 240 - 20;  // Adjust the Y position to fit the timestamp

    for (i = 0; timestamp[i] != '\0'; i++) {
        // Get the font data for the current character
        unsigned char *char_data = font[timestamp[i] - 32]; // Adjust for ASCII offset

        // Render the character
        for (dy = 0; dy < 8; dy++) {
            for (dx = 0; dx < 8; dx++) {
                if (char_data[dy] & (1 << (7 - dx))) {
                    *(Video_Mem_ptr + ((text_y + dy) << 9) + (text_x + i * 8 + dx)) = 0xFFFF; // White pixel
                }
            }
        }
    }

    return 0; // Explicit return statement to fix the warning
}
