// DahengViewer.cpp
#include <opencv2/opencv.hpp>
#include "GxIAPI.h"
#include "DxImageProc.h"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <cmath>
#include <vector>

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

// 把 GX_FRAME_DATA_EX 转换成 OpenCV BGR Mat
static bool FrameToBGR(PGX_FRAME_DATA_EX pFrame, cv::Mat& outBGR)
{
    const int  w   = pFrame->nWidth;
    const int  h   = pFrame->nHeight;
    const int64_t fmt = pFrame->nPixelFormat;
    void* pBuf = (void*)pFrame->pImgBuf;
    if (!pBuf || w <= 0 || h <= 0) return false;

    switch (fmt)
    {
    // -------------- 黑白 8bit --------------
    case GX_PIXEL_FORMAT_MONO8:
    {
        cv::Mat gray(h, w, CV_8UC1, pBuf);
        cv::cvtColor(gray, outBGR, cv::COLOR_GRAY2BGR);
        return true;
    }

    // -------------- Bayer 8bit --------------
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
        // DxRaw8toRGB24 输出为 RGB24，需要再转 BGR
        VxInt32 dx = DxRaw8toRGB24(pBuf, rgb.data,
                                   (VxUint32)w, (VxUint32)h,
                                   RAW2RGB_NEIGHBOUR, bayerType, false);
        if (dx != DX_OK) return false;
        cv::cvtColor(rgb, outBGR, cv::COLOR_RGB2BGR);
        return true;
    }

    // -------------- Bayer 非8bit（先降位再插值） --------------
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

    // -------------- 已经是 RGB/BGR --------------
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

namespace
{
DX_VALID_BIT getValidBits(GX_PIXEL_FORMAT_ENTRY pixelFormat)
{
    switch (pixelFormat)
    {
    case GX_PIXEL_FORMAT_MONO10:
    case GX_PIXEL_FORMAT_MONO10_P:
    case GX_PIXEL_FORMAT_MONO10_PACKED:
    case GX_PIXEL_FORMAT_BAYER_GR10:
    case GX_PIXEL_FORMAT_BAYER_RG10:
    case GX_PIXEL_FORMAT_BAYER_GB10:
    case GX_PIXEL_FORMAT_BAYER_BG10:
    case GX_PIXEL_FORMAT_BAYER_BG10_PACKED:
    case GX_PIXEL_FORMAT_BAYER_GB10_PACKED:
    case GX_PIXEL_FORMAT_BAYER_GR10_PACKED:
    case GX_PIXEL_FORMAT_BAYER_RG10_PACKED:
    case GX_PIXEL_FORMAT_BAYER_BG10_P:
    case GX_PIXEL_FORMAT_BAYER_GB10_P:
    case GX_PIXEL_FORMAT_BAYER_GR10_P:
    case GX_PIXEL_FORMAT_BAYER_RG10_P:
    case GX_PIXEL_FORMAT_RGB10:
    case GX_PIXEL_FORMAT_BGR10:
        return DX_BIT_2_9;

    case GX_PIXEL_FORMAT_MONO12:
    case GX_PIXEL_FORMAT_MONO12_P:
    case GX_PIXEL_FORMAT_MONO12_PACKED:
    case GX_PIXEL_FORMAT_BAYER_GR12:
    case GX_PIXEL_FORMAT_BAYER_RG12:
    case GX_PIXEL_FORMAT_BAYER_GB12:
    case GX_PIXEL_FORMAT_BAYER_BG12:
    case GX_PIXEL_FORMAT_BAYER_BG12_PACKED:
    case GX_PIXEL_FORMAT_BAYER_GB12_PACKED:
    case GX_PIXEL_FORMAT_BAYER_GR12_PACKED:
    case GX_PIXEL_FORMAT_BAYER_RG12_PACKED:
    case GX_PIXEL_FORMAT_BAYER_BG12_P:
    case GX_PIXEL_FORMAT_BAYER_GB12_P:
    case GX_PIXEL_FORMAT_BAYER_GR12_P:
    case GX_PIXEL_FORMAT_BAYER_RG12_P:
    case GX_PIXEL_FORMAT_RGB12:
    case GX_PIXEL_FORMAT_BGR12:
    case GX_PIXEL_FORMAT_RGBA12:
    case GX_PIXEL_FORMAT_RGBA12_P:
    case GX_PIXEL_FORMAT_BGRA12:
    case GX_PIXEL_FORMAT_BGRA12_P:
        return DX_BIT_4_11;

    case GX_PIXEL_FORMAT_MONO14:
    case GX_PIXEL_FORMAT_MONO14_P:
    case GX_PIXEL_FORMAT_BAYER_GR14:
    case GX_PIXEL_FORMAT_BAYER_RG14:
    case GX_PIXEL_FORMAT_BAYER_GB14:
    case GX_PIXEL_FORMAT_BAYER_BG14:
    case GX_PIXEL_FORMAT_BAYER_GR14_P:
    case GX_PIXEL_FORMAT_BAYER_RG14_P:
    case GX_PIXEL_FORMAT_BAYER_GB14_P:
    case GX_PIXEL_FORMAT_BAYER_BG14_P:
    case GX_PIXEL_FORMAT_RGB14:
    case GX_PIXEL_FORMAT_BGR14:
        return DX_BIT_6_13;

    case GX_PIXEL_FORMAT_MONO16:
    case GX_PIXEL_FORMAT_BAYER_GR16:
    case GX_PIXEL_FORMAT_BAYER_RG16:
    case GX_PIXEL_FORMAT_BAYER_GB16:
    case GX_PIXEL_FORMAT_BAYER_BG16:
    case GX_PIXEL_FORMAT_RGB16:
    case GX_PIXEL_FORMAT_BGR16:
        return DX_BIT_8_15;

    default:
        return DX_BIT_0_7;
    }
}

void drawTraditionalDetectionBoxes(cv::Mat& image)
{
    if (image.empty())
    {
        std::cerr << "传统图像识别失败：输入图像为空" << std::endl;
        return;
    }

    cv::Mat bgr;
    if (image.channels() == 3) bgr = image;
    else if (image.channels() == 4) cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
    else if (image.channels() == 1) cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR);
    else
    {
        std::cerr << "传统图像识别失败：不支持的图像通道数 "
                  << image.channels() << std::endl;
        return;
    }

    const double scale = std::min(
        1.0, 1280.0 / std::max(bgr.cols, bgr.rows));
    cv::Mat detectionImage;
    if (scale < 1.0)
        cv::resize(bgr, detectionImage, cv::Size(), scale, scale, cv::INTER_AREA);
    else
        detectionImage = bgr;

    cv::Mat smoothed;
    cv::GaussianBlur(detectionImage, smoothed, cv::Size(5, 5), 0);
    cv::Mat hsv;
    cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);
    const int minDimension = std::min(hsv.cols, hsv.rows);
    const int closeWidth = std::max(5, minDimension / 80);
    cv::Mat ledMask;
    cv::Mat colorMask;
    cv::inRange(hsv, cv::Scalar(75, 25, 200), cv::Scalar(105, 255, 255),
                ledMask);
    cv::inRange(hsv, cv::Scalar(0, 45, 200), cv::Scalar(12, 255, 255),
                colorMask);
    cv::bitwise_or(ledMask, colorMask, ledMask);
    cv::inRange(hsv, cv::Scalar(168, 45, 200), cv::Scalar(179, 255, 255),
                colorMask);
    cv::bitwise_or(ledMask, colorMask, ledMask);
    cv::inRange(hsv, cv::Scalar(35, 45, 200), cv::Scalar(90, 255, 255),
                colorMask);
    cv::bitwise_or(ledMask, colorMask, ledMask);
    cv::morphologyEx(
        ledMask, ledMask, cv::MORPH_OPEN,
        cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
    cv::morphologyEx(
        ledMask, ledMask, cv::MORPH_CLOSE,
        cv::getStructuringElement(
            cv::MORPH_RECT, cv::Size(closeWidth, std::max(3, closeWidth / 3))));

    struct LedCandidate
    {
        cv::RotatedRect box;
        cv::Point2f axis;
        float length;
        float thickness;
        double area;
    };
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(ledMask, contours, cv::RETR_EXTERNAL,
                     cv::CHAIN_APPROX_SIMPLE);
    std::vector<LedCandidate> leds;
    const float minLength = std::max(14.0f, minDimension * 0.07f);
    const float minThickness = std::max(3.0f, minDimension * 0.006f);
    const float maxThickness = minDimension * 0.14f;
    const double minBrightness =
        std::max(155.0, std::min(205.0, cv::mean(hsv)[2] * 1.30));
    for (const std::vector<cv::Point>& contour : contours)
    {
        const double area = cv::contourArea(contour);
        const cv::RotatedRect box = cv::minAreaRect(contour);
        const float length = std::max(box.size.width, box.size.height);
        const float thickness = std::min(box.size.width, box.size.height);
        if (area < minDimension * minDimension * 0.0002 ||
            length < minLength ||
            length > std::max(hsv.cols, hsv.rows) * 0.75f ||
            thickness < minThickness || thickness > maxThickness ||
            length / thickness < 2.8f ||
            area / (length * thickness) < 0.12)
        {
            continue;
        }

        const cv::Rect bounds = cv::boundingRect(contour);
        if (bounds.x <= 1 || bounds.y <= 1 ||
            bounds.x + bounds.width >= hsv.cols - 1 ||
            bounds.y + bounds.height >= hsv.rows - 1 ||
            cv::mean(hsv(bounds))[2] < minBrightness)
        {
            continue;
        }

        cv::Point2f corners[4];
        box.points(corners);
        cv::Point2f axis = corners[1] - corners[0];
        if (cv::norm(axis) < cv::norm(corners[2] - corners[1]))
            axis = corners[2] - corners[1];
        axis *= 1.0f / cv::norm(axis);
        leds.push_back({box, axis, length, thickness, area});
    }
    std::sort(leds.begin(), leds.end(),
              [](const LedCandidate& first, const LedCandidate& second)
              {
                  return first.area > second.area;
              });
    if (leds.size() > 24) leds.resize(24);

    size_t bestFirst = leds.size();
    size_t bestSecond = leds.size();
    double bestPairScore = 0.0;
    for (size_t first = 0; first < leds.size(); ++first)
    {
        for (size_t second = first + 1; second < leds.size(); ++second)
        {
            const LedCandidate& a = leds[first];
            const LedCandidate& b = leds[second];
            const float axisAgreement = std::abs(a.axis.dot(b.axis));
            const float lengthRatio =
                std::min(a.length, b.length) / std::max(a.length, b.length);
            const cv::Point2f between = b.box.center - a.box.center;
            const float separation = cv::norm(between);
            const float perpendicularity =
                std::abs(a.axis.dot(between)) / std::max(1.0f, separation);
            if (axisAgreement < 0.90f || lengthRatio < 0.45f ||
                separation < std::max(a.length, b.length) * 0.75f ||
                perpendicularity > 0.55f)
            {
                continue;
            }

            const double score = (a.area + b.area) * axisAgreement * lengthRatio;
            if (score > bestPairScore)
            {
                bestPairScore = score;
                bestFirst = first;
                bestSecond = second;
            }
        }
    }

    const cv::Scalar red = image.channels() == 1
                               ? cv::Scalar(255)
                               : cv::Scalar(0, 0, 255);
    const auto toImagePoint = [&image, &hsv](const cv::Point2f& point)
    {
        return cv::Point(cvRound(point.x * image.cols / hsv.cols),
                         cvRound(point.y * image.rows / hsv.rows));
    };

    if (bestPairScore > 0.0)
    {
        for (size_t index = 0; index < 2; ++index)
        {
            cv::Point2f corners[4];
            leds[index == 0 ? bestFirst : bestSecond].box.points(corners);
            cv::Point vertices[4];
            for (int corner = 0; corner < 4; ++corner)
                vertices[corner] = toImagePoint(corners[corner]);
            for (int corner = 0; corner < 4; ++corner)
            {
                cv::line(image, vertices[corner], vertices[(corner + 1) % 4],
                         red, 2, cv::LINE_AA);
            }
        }
    }

    if (bestPairScore == 0.0)
    {
        std::cerr << "未检测到成对灯带，请检查光照和相机画面。"
                  << std::endl;
    }
}
}


int main(int /*argc*/, char** /*argv*/)
{
    GX_STATUS emStatus      = GX_STATUS_SUCCESS;
    GX_DEV_HANDLE hDevice   = NULL;
    uint32_t ui32DeviceNum  = 0;

    // ---------- 1. 初始化 GxIAPI 库 ----------
    emStatus = GXInitLib();
    if (emStatus != GX_STATUS_SUCCESS) {
        std::cerr << "GXInitLib failed" << std::endl;
        return -1;
    }

    // ---------- 2. 枚举设备 ----------
    emStatus = GXUpdateAllDeviceList(&ui32DeviceNum, 1000);
    if (emStatus != GX_STATUS_SUCCESS || ui32DeviceNum <= 0) {
        std::cerr << "No device found" << std::endl;
        GXCloseLib();
        return -1;
    }
    std::cout << "Found " << ui32DeviceNum << " device(s)" << std::endl;

    // ---------- 3. 打开第一台设备 ----------
    GX_OPEN_PARAM stOpenParam;
    stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
    stOpenParam.openMode   = GX_OPEN_INDEX;
    stOpenParam.pszContent = (char*)"1";

    emStatus = GXOpenDevice(&stOpenParam, &hDevice);
    if (emStatus != GX_STATUS_SUCCESS) {
        std::cerr << "GXOpenDevice failed" << std::endl;
        GXCloseLib();
        return -1;
    }

    // ---------- 4. 打印设备信息 ----------
    GX_STRING_VALUE stModel;
    if (GXGetStringValue(hDevice, "DeviceModelName", &stModel) == GX_STATUS_SUCCESS)
        std::cout << "Model: " << stModel.strCurValue << std::endl;

    GX_INT_VALUE stW, stH;
    GXGetIntValue(hDevice, "Width",  &stW);
    GXGetIntValue(hDevice, "Height", &stH);
    std::cout << "Resolution: " << stW.nCurValue << " x " << stH.nCurValue << std::endl;

    GX_ENUM_VALUE stPF;
    if (GXGetEnumValue(hDevice, "PixelFormat", &stPF) == GX_STATUS_SUCCESS)
        std::cout << "PixelFormat: " << stPF.stCurValue.strCurSymbolic << std::endl;

    // ---------- 5. 千兆网优化：设置最优包长 ----------
    GX_NODE_ACCESS_MODE emAccessMode = GX_NODE_ACCESS_MODE_NI;
    if (GXGetNodeAccessMode(hDevice, "GevSCPSPacketSize", &emAccessMode) == GX_STATUS_SUCCESS &&
        emAccessMode == GX_NODE_ACCESS_MODE_RW)
    {
        uint32_t ui32PacketSize = 0;
        if (GXGetOptimalPacketSize(hDevice, &ui32PacketSize) == GX_STATUS_SUCCESS) {
            GXSetIntValue(hDevice, "GevSCPSPacketSize", ui32PacketSize);
            std::cout << "Optimal packet size = " << ui32PacketSize << std::endl;
        }
    }

    // ---------- 6. 设置采集 buffer 个数（可选，推荐 >= 3） ----------
    GXSetAcqusitionBufferNumber(hDevice, 5);

    // //曝光时间
    // GXSetEnumValueByString(hDevice, "ExposureAuto", "Off");
    // GXSetEnumValueByString(hDevice, "ExposureTime", "10000.0");
    // GXSetEnumValueByString(hDevice, "GainAuto", "Off");
    // //帧率
    // GXSetEnumValueByString(hDevice, "AcquisitionFrameRateMode", "Off");
    

    // ---------- 7. 开采 ----------
#ifdef __linux__
    emStatus = GXStreamOn(hDevice);
#else
    // Windows 不支持 GXStreamOn/GXStreamOff，用命令节点代替
    emStatus = GXSetCommandValue(hDevice, "AcquisitionStart");
#endif
    if (emStatus != GX_STATUS_SUCCESS) {
        std::cerr << "Start acquisition failed" << std::endl;
        GXCloseDevice(hDevice);
        GXCloseLib();
        return -1;
    }
    std::cout << "Acquisition started. Press ESC to exit." << std::endl;

    // ---------- 8. 循环取图并显示 ----------
    const char* kWin = "Daheng Camera";
    cv::namedWindow(kWin, cv::WINDOW_NORMAL);
    cv::resizeWindow(kWin, 960, 720);

    while (true)
    {
        PGX_FRAME_DATA_EX pFrame = NULL;
        emStatus = GXDQBufEx(hDevice, &pFrame, 1000);
        if (emStatus != GX_STATUS_SUCCESS || pFrame == NULL)
            continue;   // 超时或出错，继续

        if (pFrame->nStatus == GX_FRAME_STATUS_SUCCESS)
        {
            cv::Mat bgr;
            if (FrameToBGR(pFrame, bgr) && !bgr.empty())
            {
                drawTraditionalDetectionBoxes(bgr);
                cv::imshow(kWin, bgr);
            }
        }
        else
        {
            std::cerr << "Incomplete frame (残帧)" << std::endl;
        }

        // 一定要把 buffer 还回库，否则无法持续采集
        GXQBufEx(hDevice, pFrame);

        if (cv::waitKey(1) == 27) break;   // ESC 退出
    }

    // ---------- 9. 停采 ----------
#ifdef __linux__
    emStatus = GXStreamOff(hDevice);
#else
    emStatus = GXSetCommandValue(hDevice, "AcquisitionStop");
#endif

    // ---------- 10. 关闭设备 & 反初始化 ----------
    cv::destroyAllWindows();
    GXCloseDevice(hDevice);
    GXCloseLib();

    std::cout << "Bye." << std::endl;
    return 0;
}