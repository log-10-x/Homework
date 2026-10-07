// DahengViewer.cpp
#include <opencv2/opencv.hpp>
#include "GxIAPI.h"
#include "DxImageProc.h"
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include <limits>

// ---- GenICam 标准像素格式（GxIAPI.h 已包含时可直接删除这一段） ----
#ifndef GX_PIXEL_FORMAT_MONO8
#define GX_PIXEL_FORMAT_MONO8       0x01080001
#define GX_PIXEL_FORMAT_MONO10      0x01100003
#define GX_PIXEL_FORMAT_MONO12      0x01100005
#define GX_PIXEL_FORMAT_MONO16      0x01100007
#define GX_PIXEL_FORMAT_BAYER_GR8   0x01080008
#define GX_PIXEL_FORMAT_BAYER_RG8   0x01080009
#define GX_PIXEL_FORMAT_BAYER_GB8   0x0108000A
#define GX_PIXEL_FORMAT_BAYER_BG8   0x0108000B
#define GX_PIXEL_FORMAT_BAYER_GR10  0x0110000C
#define GX_PIXEL_FORMAT_BAYER_RG10  0x0110000D
#define GX_PIXEL_FORMAT_BAYER_GB10  0x0110000E
#define GX_PIXEL_FORMAT_BAYER_BG10  0x0110000F
#define GX_PIXEL_FORMAT_BAYER_GR12  0x01100010
#define GX_PIXEL_FORMAT_BAYER_RG12  0x01100011
#define GX_PIXEL_FORMAT_BAYER_GB12  0x01100012
#define GX_PIXEL_FORMAT_BAYER_BG12  0x01100013
#define GX_PIXEL_FORMAT_RGB8        0x02180014
#define GX_PIXEL_FORMAT_BGR8        0x02180015
#endif
// -------------------------------------------------------------------

// ============================================================
// 像素格式转换（保留你原来的逻辑，仅做小修）
// ============================================================
static bool FrameToBGR(PGX_FRAME_DATA_EX pFrame, cv::Mat& outBGR)
{
    const int  w   = pFrame->nWidth;
    const int  h   = pFrame->nHeight;
    const int64_t fmt = pFrame->nPixelFormat;
    void* pBuf = reinterpret_cast<void*>(pFrame->pImgBuf);
    if (!pBuf || w <= 0 || h <= 0) return false;

    switch (fmt)
    {
    case GX_PIXEL_FORMAT_MONO8:
    {
        cv::Mat gray(h, w, CV_8UC1, pBuf);
        cv::cvtColor(gray, outBGR, cv::COLOR_GRAY2BGR);
        return true;
    }

    case GX_PIXEL_FORMAT_BAYER_RG8:
    case GX_PIXEL_FORMAT_BAYER_GR8:
    case GX_PIXEL_FORMAT_BAYER_GB8:
    case GX_PIXEL_FORMAT_BAYER_BG8:
    {
        DX_PIXEL_COLOR_FILTER bayerType = BAYERRG;
        switch (fmt) {
            case GX_PIXEL_FORMAT_BAYER_RG8: bayerType = BAYERRG; break;
            case GX_PIXEL_FORMAT_BAYER_GR8: bayerType = BAYERGR; break;
            case GX_PIXEL_FORMAT_BAYER_GB8: bayerType = BAYERGB; break;
            case GX_PIXEL_FORMAT_BAYER_BG8: bayerType = BAYERBG; break;
        }
        cv::Mat rgb(h, w, CV_8UC3);
        VxInt32 dx = DxRaw8toRGB24(pBuf, rgb.data,
                                   (VxUint32)w, (VxUint32)h,
                                   RAW2RGB_NEIGHBOUR, bayerType, false);
        if (dx != DX_OK) return false;
        cv::cvtColor(rgb, outBGR, cv::COLOR_RGB2BGR);
        return true;
    }

    case GX_PIXEL_FORMAT_BAYER_RG10:
    case GX_PIXEL_FORMAT_BAYER_GR10:
    case GX_PIXEL_FORMAT_BAYER_GB10:
    case GX_PIXEL_FORMAT_BAYER_BG10:
    case GX_PIXEL_FORMAT_BAYER_RG12:
    case GX_PIXEL_FORMAT_BAYER_GR12:
    case GX_PIXEL_FORMAT_BAYER_GB12:
    case GX_PIXEL_FORMAT_BAYER_BG12:
    {
        cv::Mat raw8(h, w, CV_8UC1);
        DX_VALID_BIT validBit =
            (fmt == GX_PIXEL_FORMAT_BAYER_RG12 || fmt == GX_PIXEL_FORMAT_BAYER_GR12 ||
             fmt == GX_PIXEL_FORMAT_BAYER_GB12 || fmt == GX_PIXEL_FORMAT_BAYER_BG12)
                ? DX_BIT_4_11 : DX_BIT_2_9;

        VxInt32 dx = DxRaw16toRaw8(pBuf, raw8.data,
                                   (VxUint32)w, (VxUint32)h, validBit);
        if (dx != DX_OK) return false;

        DX_PIXEL_COLOR_FILTER bayerType = BAYERRG;
        if (fmt == GX_PIXEL_FORMAT_BAYER_RG10 || fmt == GX_PIXEL_FORMAT_BAYER_RG12) bayerType = BAYERRG;
        if (fmt == GX_PIXEL_FORMAT_BAYER_GR10 || fmt == GX_PIXEL_FORMAT_BAYER_GR12) bayerType = BAYERGR;
        if (fmt == GX_PIXEL_FORMAT_BAYER_GB10 || fmt == GX_PIXEL_FORMAT_BAYER_GB12) bayerType = BAYERGB;
        if (fmt == GX_PIXEL_FORMAT_BAYER_BG10 || fmt == GX_PIXEL_FORMAT_BAYER_BG12) bayerType = BAYERBG;

        cv::Mat rgb(h, w, CV_8UC3);
        dx = DxRaw8toRGB24(raw8.data, rgb.data,
                           (VxUint32)w, (VxUint32)h,
                           RAW2RGB_NEIGHBOUR, bayerType, false);
        if (dx != DX_OK) return false;
        cv::cvtColor(rgb, outBGR, cv::COLOR_RGB2BGR);
        return true;
    }

    case GX_PIXEL_FORMAT_BGR8:
        outBGR = cv::Mat(h, w, CV_8UC3, pBuf).clone();
        return true;
    case GX_PIXEL_FORMAT_RGB8:
    {
        cv::Mat rgb(h, w, CV_8UC3, pBuf);
        cv::cvtColor(rgb, outBGR, cv::COLOR_RGB2BGR);
        return true;
    }

    default:
        std::cerr << "Unsupported pixel format: 0x"
                  << std::hex << fmt << std::dec << std::endl;
        return false;
    }
}

// ============================================================
// 传统图像识别框（保留你的实现，仅缩进整理）
// ============================================================
static void drawTraditionalDetectionBoxes(cv::Mat& image)
{
    if (image.empty()) return;

    cv::Mat bgr;
    if (image.channels() == 3)      bgr = image;
    else if (image.channels() == 4) cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
    else if (image.channels() == 1) cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR);
    else return;

    cv::Mat smoothed;
    cv::GaussianBlur(bgr, smoothed, cv::Size(5, 5), 0);
    cv::Mat hsv;
    cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);

    std::vector<cv::Mat> boardMasks(3);
    cv::inRange(hsv, cv::Scalar(95, 180, 0),  cv::Scalar(125, 255, 100), boardMasks[0]);

    cv::Mat redLowHue, redHighHue;
    cv::inRange(hsv, cv::Scalar(0, 200, 0),    cv::Scalar(10, 255, 255),  redLowHue);
    cv::inRange(hsv, cv::Scalar(170, 200, 0),  cv::Scalar(179, 255, 255), redHighHue);
    cv::bitwise_or(redLowHue, redHighHue, boardMasks[1]);

    cv::inRange(hsv, cv::Scalar(35, 120, 0), cv::Scalar(85, 255, 220), boardMasks[2]);

    const auto percentile = [](std::vector<int>& values, double fraction)
    {
        const size_t index = static_cast<size_t>(
            fraction * static_cast<double>(values.size() - 1));
        std::nth_element(values.begin(), values.begin() + index, values.end());
        return values[index];
    };

    const size_t minimumColorPixels =
        std::max<size_t>(100, image.total() / 2000);

    bool detected = false;
    for (const cv::Mat& boardMask : boardMasks)
    {
        std::vector<cv::Point> boardPixels;
        cv::findNonZero(boardMask, boardPixels);
        if (boardPixels.size() < minimumColorPixels) continue;

        std::vector<int> xCoordinates, yCoordinates;
        xCoordinates.reserve(boardPixels.size());
        yCoordinates.reserve(boardPixels.size());
        for (const cv::Point& pixel : boardPixels)
        {
            xCoordinates.push_back(pixel.x);
            yCoordinates.push_back(pixel.y);
        }

        const int left   = percentile(xCoordinates, 0.001);
        const int right  = percentile(xCoordinates, 0.98);
        const int top    = percentile(yCoordinates, 0.001);
        const int bottom = percentile(yCoordinates, 0.95);
        if (right <= left || bottom <= top) continue;

        const double boxArea     = static_cast<double>(right - left) * (bottom - top);
        const double colorDensity = static_cast<double>(boardPixels.size()) / boxArea;
        if (colorDensity < 0.02) continue;

        cv::rectangle(image, cv::Point(left, top), cv::Point(right, bottom),
                      image.channels() == 1 ? cv::Scalar(255) : cv::Scalar(0, 255, 0),
                      2);
        detected = true;
    }

    if (!detected)
        std::cerr << "未检测到红色、绿色或蓝色板面，请检查光照和相机画面。" << std::endl;
}

// ============================================================
// main
// ============================================================
int main(int /*argc*/, char** /*argv*/)
{
    GX_STATUS emStatus     = GX_STATUS_SUCCESS;
    GX_DEV_HANDLE hDevice  = NULL;
    uint32_t ui32DeviceNum = 0;

    // ---------- 1. 初始化 ----------
    emStatus = GXInitLib();
    if (emStatus != GX_STATUS_SUCCESS) { std::cerr << "GXInitLib failed\n"; return -1; }

    // ---------- 2. 枚举 ----------
    emStatus = GXUpdateAllDeviceList(&ui32DeviceNum, 1000);
    if (emStatus != GX_STATUS_SUCCESS || ui32DeviceNum <= 0) {
        std::cerr << "No device found\n";
        GXCloseLib();
        return -1;
    }
    std::cout << "Found " << ui32DeviceNum << " device(s)\n";

    // ---------- 3. 打开 ----------
    GX_OPEN_PARAM stOpenParam;
    stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
    stOpenParam.openMode   = GX_OPEN_INDEX;
    stOpenParam.pszContent = (char*)"1";

    emStatus = GXOpenDevice(&stOpenParam, &hDevice);
    if (emStatus != GX_STATUS_SUCCESS) {
        std::cerr << "GXOpenDevice failed\n";
        GXCloseLib();
        return -1;
    }

    // ---------- 4. 设备信息 ----------
    GX_STRING_VALUE stModel;
    if (GXGetStringValue(hDevice, "DeviceModelName", &stModel) == GX_STATUS_SUCCESS)
        std::cout << "Model: " << stModel.strCurValue << "\n";

    GX_INT_VALUE stW, stH;
    GXGetIntValue(hDevice, "Width",  &stW);
    GXGetIntValue(hDevice, "Height", &stH);
    std::cout << "Resolution: " << stW.nCurValue << " x " << stH.nCurValue << "\n";

    GX_ENUM_VALUE stPF;
    if (GXGetEnumValue(hDevice, "PixelFormat", &stPF) == GX_STATUS_SUCCESS)
        std::cout << "PixelFormat: " << stPF.stCurValue.strCurSymbolic << "\n";

    // ---------- 5. 千兆网最优包长（USB 相机自动跳过） ----------
    GX_NODE_ACCESS_MODE emAccessMode = GX_NODE_ACCESS_MODE_NI;
    if (GXGetNodeAccessMode(hDevice, "GevSCPSPacketSize", &emAccessMode) == GX_STATUS_SUCCESS &&
        emAccessMode == GX_NODE_ACCESS_MODE_RW)
    {
        uint32_t ui32PacketSize = 0;
        if (GXGetOptimalPacketSize(hDevice, &ui32PacketSize) == GX_STATUS_SUCCESS) {
            GXSetIntValue(hDevice, "GevSCPSPacketSize", ui32PacketSize);
            std::cout << "Optimal packet size = " << ui32PacketSize << "\n";
        }
    }

    // ---------- 6. 采集 buffer ----------
    GXSetAcqusitionBufferNumber(hDevice, 5);   // ← 修正拼写

    // ---------- 7. 曝光/增益（正确写法） ----------
    // 如需自动：把 "Off" 改成 "Continuous"
    GXSetEnumValueByString(hDevice, "ExposureAuto", "Off");
    GXSetEnumValueByString(hDevice, "GainAuto",     "Off");
    GXSetEnumValueByString(hDevice, "AcquisitionFrameRateMode", "Off");

    {
        GX_FLOAT_VALUE stExp;
        if (GXGetFloatValue(hDevice, "ExposureTime", &stExp) == GX_STATUS_SUCCESS) {
            double t = 10000.0;
            if (t < stExp.dMin) t = stExp.dMin;
            if (t > stExp.dMax) t = stExp.dMax;
            GXSetFloatValue(hDevice, "ExposureTime", t);
            std::cout << "ExposureTime = " << t << " " << stExp.szUnit << "\n";
        }
        GX_FLOAT_VALUE stGain;
        if (GXGetFloatValue(hDevice, "Gain", &stGain) == GX_STATUS_SUCCESS) {
            double g = 10.0;
            if (g < stGain.dMin) g = stGain.dMin;
            if (g > stGain.dMax) g = stGain.dMax;
            GXSetFloatValue(hDevice, "Gain", g);
            std::cout << "Gain = " << g << " " << stGain.szUnit << "\n";
        }
    }

    // ---------- 8. 开采 ----------
#ifdef __linux__
    emStatus = GXStreamOn(hDevice);
#else
    emStatus = GXSetCommandValue(hDevice, "AcquisitionStart");
#endif
    if (emStatus != GX_STATUS_SUCCESS) {
        std::cerr << "Start acquisition failed\n";
        GXCloseDevice(hDevice);
        GXCloseLib();
        return -1;
    }
    std::cout << "Acquisition started. Press ESC to exit.\n";

    // ---------- 9. 主循环 ----------
    const char* kWin = "Daheng Camera";
    cv::namedWindow(kWin, cv::WINDOW_NORMAL);
    cv::resizeWindow(kWin, 960, 720);

    while (true)
    {
        // ★ 核心：先拿帧
        PGX_FRAME_DATA_EX pFrame = nullptr;
        emStatus = GXDQBufEx(hDevice, &pFrame, 1000);

        if (emStatus != GX_STATUS_SUCCESS || pFrame == nullptr) {
            // 超时或其他错误，继续下一轮（继续前要按键检查，否则窗口卡死）
            if (cv::waitKey(1) == 27) break;
            continue;
        }

        // ★ 拿到帧后再使用
        cv::Mat displayImage;
        if (pFrame->nStatus == GX_FRAME_STATUS_SUCCESS)
        {
            cv::Mat bgr;
            if (FrameToBGR(pFrame, bgr) && !bgr.empty())
            {
                displayImage = bgr;

                // 传统识别画框（不需要可注释掉）
                drawTraditionalDetectionBoxes(displayImage);

                cv::imshow(kWin, displayImage);
            }
        }
        else
        {
            std::cerr << "Incomplete frame (残帧)\n";
        }

        // ★ 一定要归还 buffer
        GXQBufEx(hDevice, pFrame);

        if (cv::waitKey(1) == 27) break;
    }

    // ---------- 10. 停采 ----------
#ifdef __linux__
    emStatus = GXStreamOff(hDevice);
#else
    emStatus = GXSetCommandValue(hDevice, "AcquisitionStop");
#endif

    // ---------- 11. 收尾 ----------
    cv::destroyAllWindows();
    GXCloseDevice(hDevice);
    GXCloseLib();

    std::cout << "Bye.\n";
    return 0;
}