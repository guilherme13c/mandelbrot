#include "raylib.h"
#include <atomic>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>

// Mandelbrot set boundaries
constexpr float MIN_RE = -2.5;
constexpr float MAX_RE = 1.0;
constexpr float MIN_IM = -1.0;
constexpr float MAX_IM = 1.0;

// Mandelbrot set iterations
constexpr int MAX_ITERATIONS = 100000;

// Threshold
constexpr float THRESHOLD = 4.0;

// Window dimensions
const int screenWidth = fabs(MAX_RE - MIN_RE) * 400;
const int screenHeight = fabs(MAX_IM - MIN_IM) * 400;

// Atomic variables and mutex for work-stealing
std::vector<std::thread> threads;
std::atomic<int> nextRow(0);
std::mutex stealLock;

// Function to compute a segment of the Mandelbrot set
void ComputeMandelbrotRow(float *buffer, int width, int y) {
    const float scaleX = (MAX_RE - MIN_RE) / width;
    const float c_im = MIN_IM + y * (MAX_IM - MIN_IM) / screenHeight;

    for (int x = 0; x < width; ++x) {
        float c_re = MIN_RE + x * scaleX;
        float z_re = c_re, z_im = c_im;
        int iterations = 0;

        while (iterations < MAX_ITERATIONS) {
            float z_re_temp = z_re * z_re - z_im * z_im + c_re;
            z_im = 2.0 * z_re * z_im + c_im;
            z_re = z_re_temp;

            if (z_re * z_re + z_im * z_im > THRESHOLD) {
                break; // The value has shot to infinity
            }

            ++iterations;
        }

        buffer[y * width + x] = iterations;
    }
}

// Function for thread to perform work-stealing
void WorkStealing(int threadId, float *buffer, int width, int height) {
    while (true) {
        // Try to steal work from another thread
        int rowToProcess = -1;
        {
            std::lock_guard<std::mutex> lock(stealLock);
            if (nextRow < height) {
                rowToProcess = nextRow++;
            }
        }

        if (rowToProcess == -1) {
            // No more work to steal, exit
            break;
        }

        // Compute the Mandelbrot set for the row
        ComputeMandelbrotRow(buffer, width, rowToProcess);
    }
}

// Function to compute the Mandelbrot set using multithreading with
// work-stealing
void ComputeMandelbrot(float *buffer, int width, int height) {
    const int numThreads = std::thread::hardware_concurrency();
    threads.reserve(numThreads);

    for (int i = 0; i < numThreads; ++i) {
        threads.emplace_back(WorkStealing, i, buffer, width, height);
    }

    for (auto &thread : threads) {
        thread.join();
    }
}

// Function to map the Mandelbrot iterations to a smooth color
Color GetColorFromIterations(float iterations, int maxIterations) {
    if (iterations == maxIterations)
        return BLACK;

    float t = iterations / maxIterations;
    t = sqrtf(t); // Apply gamma correction to enhance brightness

    unsigned char r = (unsigned char)(9 * (1 - t) * t * t * 255);
    unsigned char g = (unsigned char)(15 * (1 - t) * (1 - t) * t * 255);
    unsigned char b =
        (unsigned char)(12 * (1 - t) * (1 - t) * (1 - t) * t * 255);

    return Color{r, g, b, 255};
}

int main(void) {
    // Initialize Raylib
    InitWindow(screenWidth, screenHeight, "Mandelbrot Set");

    // Allocate buffer to store Mandelbrot set values
    float *buffer = new float[screenWidth * screenHeight];

    // Compute the Mandelbrot set
    ComputeMandelbrot(buffer, screenWidth, screenHeight);

    // Main game loop
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);

        // Draw the Mandelbrot set
        for (int y = 0; y < screenHeight; ++y) {
            for (int x = 0; x < screenWidth; ++x) {
                float iterations = buffer[y * screenWidth + x];
                Color color =
                    GetColorFromIterations(iterations, MAX_ITERATIONS);
                DrawPixel(x, y, color);
            }
        }

        EndDrawing();
    }

    // De-Initialization
    delete[] buffer;
    CloseWindow();

    return 0;
}
