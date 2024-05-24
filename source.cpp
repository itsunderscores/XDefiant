#include <iostream>
#include <Windows.h>
#include <chrono>
#include <thread>
#include <random>
#include <algorithm>

using std::string;
using std::cout;
using std::cin;
using std::endl;

bool isr3d(BYTE red, BYTE green, BYTE blue) {
    return red > 200 && green < 100 && blue < 100;
}

typedef int(*pDD_btn)(int btn);
pDD_btn      DD_btn;          // Mouse button

//Had this for testing to see where the fuck it was checking for the pixels lol
void DrawYellowCircle(int x, int y, int radius) {
    HDC hdc = GetDC(NULL);
    HPEN hPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 0)); // Yellow color
    HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hdc, hPen);
    SelectObject(hdc, hBrush);
    Ellipse(hdc, x - radius, y - radius, x + radius, y + radius);
    DeleteObject(hPen);
    ReleaseDC(NULL, hdc);
}

int main()
{
    bool alwayson = false;
    bool toggleBoxSize = false; // Toggle state for boxSize and distY range
    string on;
    cout << "Listen to M4? (Y/N): ";
    cin >> on;
    if (on == "N") { alwayson = true; }

    cout << "\nControls:\nM5: Switches between AR/Sniper mode\n\nLOG:\n";

    // Set the coordinates of the center box
    int centerX = GetSystemMetrics(SM_CXSCREEN) / 2;
    int centerY = GetSystemMetrics(SM_CYSCREEN) / 2 - 10;
    int boxSize = 50; // Initial box size
    int requiredRedPixels = 5; // Adjust the threshold as needed

    // Load Chinese Mouse Driver
    HMODULE hDll = LoadLibraryW(L"3jkbnhhjkb.dll");    // App x64
    if (hDll == nullptr) {
        return -1;
    }
    DD_btn = (pDD_btn)GetProcAddress(hDll, "DD_btn");
    int st = DD_btn(0);
    if (st != 1) {
        // DD Initialize Error
        return st;
    }

    // Random number generator for offset
    std::mt19937_64 eng{ std::random_device{}() };
    std::uniform_int_distribution<> distY{ -100, -50 }; // Initial offset range

    while (true) {
        int arsize = 50;
        int snipersize = 80;
        // Check if Mouse5 (XBUTTON2) is pressed
        if (GetKeyState(VK_XBUTTON2) & 0x8000) {
            toggleBoxSize = !toggleBoxSize;
            if (toggleBoxSize) {
                boxSize = snipersize;
                distY = std::uniform_int_distribution<>(-200, 10); // Update the range
                cout << "[-] Sniper Mode\n";
            }
            else {
                boxSize = arsize;
                distY = std::uniform_int_distribution<>(-100, -50); // Update the range
                cout << "[-] AR Mode\n";
            }
            // Debounce the button press
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        if (GetKeyState(VK_XBUTTON1) & 0x8000 || alwayson == true) {
            // Capture the screen
            HDC hdcScreen = GetDC(NULL);
            HDC hdcMem = CreateCompatibleDC(hdcScreen);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, boxSize, boxSize);
            SelectObject(hdcMem, hBitmap);
            int offsetY = distY(eng);

            BitBlt(hdcMem, 0, 0, boxSize, boxSize, hdcScreen, centerX - boxSize / 2, centerY - boxSize / 2 + offsetY, SRCCOPY);

            // Draw a yellow circle to indicate the active area
            DrawYellowCircle(centerX, centerY + offsetY, boxSize / 2);

            // Check pixels in the captured box
            int redPixelCount = 0;
            for (int x = 0; x < boxSize; ++x) {
                for (int y = 0; y < boxSize; ++y) {
                    COLORREF pixelColor = GetPixel(hdcMem, x, y);
                    BYTE red = GetRValue(pixelColor);
                    BYTE green = GetGValue(pixelColor);
                    BYTE blue = GetBValue(pixelColor);

                    if (isr3d(red, green, blue)) {
                        redPixelCount++;
                        if (redPixelCount >= requiredRedPixels) {
                            break;
                        }
                    }
                }
                if (redPixelCount >= requiredRedPixels) {
                    break;
                }
            }

            // If the required number of red pixels is detected, simulate a left mouse button click
            if (redPixelCount >= requiredRedPixels) {
                DD_btn(1);
                //cout << "[+] Shoot";

                std::uniform_int_distribution<> dist{ 30, 90 };
                std::this_thread::sleep_for(std::chrono::milliseconds{ dist(eng) });

                DD_btn(2);

                if (boxSize == arsize) {
                    std::uniform_int_distribution<> dist1{ 10, 50 };
                    std::this_thread::sleep_for(std::chrono::milliseconds{ dist1(eng) });
                }

                if (boxSize == snipersize) {
                    std::uniform_int_distribution<> dist1{ 200, 800 };
                    std::this_thread::sleep_for(std::chrono::milliseconds{ dist1(eng) });
                }
            }

            DeleteObject(hBitmap);
            DeleteDC(hdcMem);
            ReleaseDC(NULL, hdcScreen);

            std::uniform_int_distribution<> dist1{ 10, 30 };
            std::this_thread::sleep_for(std::chrono::milliseconds{ dist1(eng) });
        }
    }
}
