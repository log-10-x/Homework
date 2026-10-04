// #include <iostream>
// #include <opencv2/opencv.hpp>
// #include <GxIAPI.h>

// namespace
// {
// int getBayerConversionCode(int pixelFormat)
// {
//     switch (pixelFormat)
//     {
//     case GX_PIXEL_FORMAT_BAYER_RG8:
//     case GX_PIXEL_FORMAT_BAYER_RG10:
//     case GX_PIXEL_FORMAT_BAYER_RG12:
//     case GX_PIXEL_FORMAT_BAYER_RG14:
//     case GX_PIXEL_FORMAT_BAYER_RG16:
//         return cv::COLOR_BayerRG2BGR;
//     case GX_PIXEL_FORMAT_BAYER_GR8:
//     case GX_PIXEL_FORMAT_BAYER_GR10:
//     case GX_PIXEL_FORMAT_BAYER_GR12:
//     case GX_PIXEL_FORMAT_BAYER_GR14:
//     case GX_PIXEL_FORMAT_BAYER_GR16:
//         return cv::COLOR_BayerGR2BGR;
//     case GX_PIXEL_FORMAT_BAYER_GB8:
//     case GX_PIXEL_FORMAT_BAYER_GB10:
//     case GX_PIXEL_FORMAT_BAYER_GB12:
//     case GX_PIXEL_FORMAT_BAYER_GB14:
//     case GX_PIXEL_FORMAT_BAYER_GB16:
//         return cv::COLOR_BayerGB2BGR;
//     case GX_PIXEL_FORMAT_BAYER_BG8:
//     case GX_PIXEL_FORMAT_BAYER_BG10:
//     case GX_PIXEL_FORMAT_BAYER_BG12:
//     case GX_PIXEL_FORMAT_BAYER_BG14:
//     case GX_PIXEL_FORMAT_BAYER_BG16:
//         return cv::COLOR_BayerBG2BGR;
//     default:
//         return -1;
//     }
// }

// cv::Mat makeDisplayImage(const PGX_FRAME_DATA_EX frame)
// {
//     const int width = frame->nWidth;
//     const int height = frame->nHeight;
//     const void* buffer = reinterpret_cast<const void*>(frame->pImgBuf);
//     const int pixelFormat = frame->nPixelFormat;

//     if (pixelFormat == GX_PIXEL_FORMAT_MONO8)
//     {
//         return cv::Mat(height, width, CV_8UC1, const_cast<void*>(buffer)).clone();
//     }
//     if (pixelFormat == GX_PIXEL_FORMAT_RGB8)
//     {
//         cv::Mat rgb(height, width, CV_8UC3, const_cast<void*>(buffer));
//         cv::Mat bgr;
//         cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
//         return bgr;
//     }
//     if (pixelFormat == GX_PIXEL_FORMAT_BGR8)
//     {
//         return cv::Mat(height, width, CV_8UC3, const_cast<void*>(buffer)).clone();
//     }

//     const int bayerCode = getBayerConversionCode(pixelFormat);
//     if (bayerCode >= 0)
//     {
//         cv::Mat gray;
//         if (pixelFormat == GX_PIXEL_FORMAT_BAYER_RG8 ||
//             pixelFormat == GX_PIXEL_FORMAT_BAYER_GR8 ||
//             pixelFormat == GX_PIXEL_FORMAT_BAYER_GB8 ||
//             pixelFormat == GX_PIXEL_FORMAT_BAYER_BG8)
//         {
//             gray = cv::Mat(height, width, CV_8UC1, const_cast<void*>(buffer));
//         }
//         else
//         {
//             cv::Mat raw16(height, width, CV_16UC1, const_cast<void*>(buffer));
//             double scale = 1.0;
//             // 根据 pixelFormat 判断位深并设置 scale
//             if (pixelFormat == GX_PIXEL_FORMAT_BAYER_RG10 || ...) scale = 0.25;
//             else if (pixelFormat == GX_PIXEL_FORMAT_BAYER_RG12 || ...) scale = 0.0625;
//             else if (pixelFormat == GX_PIXEL_FORMAT_BAYER_RG14 || ...) scale = 1.0/64;
//             else if (pixelFormat == GX_PIXEL_FORMAT_BAYER_RG16 || ...) scale = 1.0/256;
//             raw16.convertTo(gray, CV_8UC1, scale);
//         }

//         cv::Mat bgr;
//         cv::cvtColor(gray, bgr, bayerCode);
//         return bgr;
//     }

//     if (pixelFormat == GX_PIXEL_FORMAT_MONO10 ||
//         pixelFormat == GX_PIXEL_FORMAT_MONO12 ||
//         pixelFormat == GX_PIXEL_FORMAT_MONO14 ||
//         pixelFormat == GX_PIXEL_FORMAT_MONO16)
//     {
//         cv::Mat raw16(height, width, CV_16UC1, const_cast<void*>(buffer));
//         cv::Mat gray;
//         cv::normalize(raw16, gray, 0, 255, cv::NORM_MINMAX, CV_8UC1);
//         return gray;
//     }

//     std::cerr << "Unsupported camera pixel format: 0x" << std::hex
//               << pixelFormat << std::dec << std::endl;
//     return cv::Mat();
// }
// }

// int main()
// {
//     GX_STATUS status = GXInitLib();
//     if (status != GX_STATUS_SUCCESS)
//     {
//         std::cerr << "Failed to initialize Galaxy SDK: " << status << std::endl;
//         return 1;
//     }

//     uint32_t deviceCount = 0;
//     status = GXUpdateAllDeviceList(&deviceCount, 1000);
//     if (status != GX_STATUS_SUCCESS || deviceCount == 0)
//     {
//         std::cerr << (deviceCount == 0 ? "No camera found." : "Failed to enumerate cameras.") << std::endl;
//         GXCloseLib();
//         return 1;
//     }
//     std::cout << "Found " << deviceCount << " camera(s)." << std::endl;

//     GX_DEV_HANDLE device = nullptr;
//     status = GXOpenDeviceByIndex(1, &device);
//     if (status != GX_STATUS_SUCCESS)
//     {
//         std::cerr << "Failed to open camera: " << status << std::endl;
//         GXCloseLib();
//         return 1;
//     }

//     GX_STRING_VALUE serialNumber = {};
//     GX_STRING_VALUE vendorName = {};
//     status = GXGetStringValue(device, "DeviceSerialNumber", &serialNumber);
//     if (status == GX_STATUS_SUCCESS)
//     {
//         status = GXGetStringValue(device, "DeviceVendorName", &vendorName);
//     }
//     if (status != GX_STATUS_SUCCESS)
//     {
//         std::cerr << "Failed to read camera information: " << status << std::endl;
//         GXCloseDevice(device);
//         GXCloseLib();
//         return 1;
//     }
//     std::cout << "Camera SN: " << serialNumber.strCurValue << std::endl;
//     std::cout << "Camera Vendor: " << vendorName.strCurValue << std::endl;

//     status = GXSetEnumValueByString(device, "AcquisitionMode", "Continuous");
//     if (status == GX_STATUS_SUCCESS)
//     {
//         status = GXSetEnumValueByString(device, "TriggerMode", "Off");
//     }
//     if (status == GX_STATUS_SUCCESS)
//     {
//         status = GXStreamOn(device);
//     }
//     if (status != GX_STATUS_SUCCESS)
//     {
//         std::cerr << "Failed to start acquisition: " << status << std::endl;
//         GXCloseDevice(device);
//         GXCloseLib();
//         return 1;
//     }

//     PGX_FRAME_DATA_EX frame = nullptr;
//     status = GXDQBufEx(device, &frame, 1000);
//     const bool frameValid = status == GX_STATUS_SUCCESS &&
//                             frame != nullptr &&
//                             frame->nStatus == GX_FRAME_STATUS_SUCCESS;
//     cv::Mat capturedImage;
//     if (frameValid)
//     {
//         std::cout << "Image Width: " << frame->nWidth
//                   << ", Height: " << frame->nHeight
//                   << ", Pixel Format: 0x" << std::hex << frame->nPixelFormat
//                   << std::dec << std::endl;
//         capturedImage = makeDisplayImage(frame);
//     }
//     else
//     {
//         std::cerr << "Failed to acquire a valid image: " << status << std::endl;
//     }

//     if (frame != nullptr)
//     {
//         GXQBufEx(device, frame);
//     }
//     GXStreamOff(device);
//     GXCloseDevice(device);
//     GXCloseLib();

//     if (capturedImage.empty())
//     {
//         return 1;
//     }

//     cv::imshow("Captured Image", capturedImage);
//     cv::waitKey(0);
//     return frameValid ? 0 : 1;
// }


#include <iostream>
#include <opencv2/opencv.hpp>
#include <GxIAPI.h>

namespace
{
// Bayer 格式 → OpenCV 转换码
int getBayerConversionCode(int pixelFormat)
{
    switch (pixelFormat)
    {
    case GX_PIXEL_FORMAT_BAYER_RG8:
    case GX_PIXEL_FORMAT_BAYER_RG10:
    case GX_PIXEL_FORMAT_BAYER_RG12:
    case GX_PIXEL_FORMAT_BAYER_RG14:
    case GX_PIXEL_FORMAT_BAYER_RG16:
        return cv::COLOR_BayerRG2BGR;
    case GX_PIXEL_FORMAT_BAYER_GR8:
    case GX_PIXEL_FORMAT_BAYER_GR10:
    case GX_PIXEL_FORMAT_BAYER_GR12:
    case GX_PIXEL_FORMAT_BAYER_GR14:
    case GX_PIXEL_FORMAT_BAYER_GR16:
        return cv::COLOR_BayerGR2BGR;
    case GX_PIXEL_FORMAT_BAYER_GB8:
    case GX_PIXEL_FORMAT_BAYER_GB10:
    case GX_PIXEL_FORMAT_BAYER_GB12:
    case GX_PIXEL_FORMAT_BAYER_GB14:
    case GX_PIXEL_FORMAT_BAYER_GB16:
        return cv::COLOR_BayerGB2BGR;
    case GX_PIXEL_FORMAT_BAYER_BG8:
    case GX_PIXEL_FORMAT_BAYER_BG10:
    case GX_PIXEL_FORMAT_BAYER_BG12:
    case GX_PIXEL_FORMAT_BAYER_BG14:
    case GX_PIXEL_FORMAT_BAYER_BG16:
        return cv::COLOR_BayerBG2BGR;
    default:
        return -1;
    }
}

// 提取 Bayer 格式的位深；非 Bayer 返回 0
int getBayerBitDepth(int pixelFormat)
{
    switch (pixelFormat)
    {
    case GX_PIXEL_FORMAT_BAYER_RG8:
    case GX_PIXEL_FORMAT_BAYER_GR8:
    case GX_PIXEL_FORMAT_BAYER_GB8:
    case GX_PIXEL_FORMAT_BAYER_BG8:
        return 8;

    case GX_PIXEL_FORMAT_BAYER_RG10:
    case GX_PIXEL_FORMAT_BAYER_GR10:
    case GX_PIXEL_FORMAT_BAYER_GB10:
    case GX_PIXEL_FORMAT_BAYER_BG10:
        return 10;

    case GX_PIXEL_FORMAT_BAYER_RG12:
    case GX_PIXEL_FORMAT_BAYER_GR12:
    case GX_PIXEL_FORMAT_BAYER_GB12:
    case GX_PIXEL_FORMAT_BAYER_BG12:
        return 12;

    case GX_PIXEL_FORMAT_BAYER_RG14:
    case GX_PIXEL_FORMAT_BAYER_GR14:
    case GX_PIXEL_FORMAT_BAYER_GB14:
    case GX_PIXEL_FORMAT_BAYER_BG14:
        return 14;

    case GX_PIXEL_FORMAT_BAYER_RG16:
    case GX_PIXEL_FORMAT_BAYER_GR16:
    case GX_PIXEL_FORMAT_BAYER_GB16:
    case GX_PIXEL_FORMAT_BAYER_BG16:
        return 16;

    default:
        return 0;
    }
}

// 提取 Mono 格式的位深；非 Mono 返回 0
int getMonoBitDepth(int pixelFormat)
{
    switch (pixelFormat)
    {
    case GX_PIXEL_FORMAT_MONO8:  return 8;
    case GX_PIXEL_FORMAT_MONO10: return 10;
    case GX_PIXEL_FORMAT_MONO12: return 12;
    case GX_PIXEL_FORMAT_MONO14: return 14;
    case GX_PIXEL_FORMAT_MONO16: return 16;
    default:                     return 0;
    }
}

// 位深 → 映射到 8bit 的固定线性系数（避免帧间闪烁）
double bitDepthToScale(int bitDepth)
{
    if (bitDepth <= 0 || bitDepth > 16)
    {
        return 1.0;
    }
    return 255.0 / ((1 << bitDepth) - 1);
}

cv::Mat makeDisplayImage(const PGX_FRAME_DATA_EX frame)
{
    const int width       = frame->nWidth;
    const int height      = frame->nHeight;
    const void* buffer    = reinterpret_cast<const void*>(frame->pImgBuf);
    const int pixelFormat = frame->nPixelFormat;

    // ---------- Mono8 ----------
    if (pixelFormat == GX_PIXEL_FORMAT_MONO8)
    {
        return cv::Mat(height, width, CV_8UC1, const_cast<void*>(buffer)).clone();
    }

    // ---------- RGB8 ----------
    if (pixelFormat == GX_PIXEL_FORMAT_RGB8)
    {
        cv::Mat rgb(height, width, CV_8UC3, const_cast<void*>(buffer));
        cv::Mat bgr;
        cv::cvtColor(rgb, bgr, cv::COLOR_RGB2BGR);
        return bgr;
    }

    // ---------- BGR8 ----------
    if (pixelFormat == GX_PIXEL_FORMAT_BGR8)
    {
        return cv::Mat(height, width, CV_8UC3, const_cast<void*>(buffer)).clone();
    }

    // ---------- Bayer ----------
    const int bayerCode = getBayerConversionCode(pixelFormat);
    if (bayerCode >= 0)
    {
        const int bitDepth = getBayerBitDepth(pixelFormat);
        cv::Mat gray;

        if (bitDepth == 8)
        {
            gray = cv::Mat(height, width, CV_8UC1, const_cast<void*>(buffer));
        }
        else
        {
            cv::Mat raw16(height, width, CV_16UC1, const_cast<void*>(buffer));
            raw16.convertTo(gray, CV_8UC1, bitDepthToScale(bitDepth));
        }

        cv::Mat bgr;
        cv::cvtColor(gray, bgr, bayerCode);
        return bgr;
    }

    // ---------- Mono 10/12/14/16 ----------
    const int monoBitDepth = getMonoBitDepth(pixelFormat);
    if (monoBitDepth > 0)
    {
        if (monoBitDepth == 8)
        {
            return cv::Mat(height, width, CV_8UC1, const_cast<void*>(buffer)).clone();
        }

        cv::Mat raw16(height, width, CV_16UC1, const_cast<void*>(buffer));
        cv::Mat gray;
        raw16.convertTo(gray, CV_8UC1, bitDepthToScale(monoBitDepth));
        return gray;
    }

    std::cerr << "Unsupported camera pixel format: 0x" << std::hex
              << pixelFormat << std::dec << std::endl;
    return cv::Mat();
}
} // namespace

int main()
{
    GX_STATUS status = GXInitLib();
    if (status != GX_STATUS_SUCCESS)
    {
        std::cerr << "Failed to initialize Galaxy SDK: " << status << std::endl;
        return 1;
    }

    uint32_t deviceCount = 0;
    status = GXUpdateAllDeviceList(&deviceCount, 1000);
    if (status != GX_STATUS_SUCCESS || deviceCount == 0)
    {
        std::cerr << (deviceCount == 0 ? "No camera found."
                                       : "Failed to enumerate cameras.")
                  << std::endl;
        GXCloseLib();
        return 1;
    }
    std::cout << "Found " << deviceCount << " camera(s)." << std::endl;

    GX_DEV_HANDLE device = nullptr;
    status = GXOpenDeviceByIndex(1, &device);
    if (status != GX_STATUS_SUCCESS)
    {
        std::cerr << "Failed to open camera: " << status << std::endl;
        GXCloseLib();
        return 1;
    }

    GX_STRING_VALUE serialNumber = {};
    GX_STRING_VALUE vendorName   = {};
    status = GXGetStringValue(device, "DeviceSerialNumber", &serialNumber);
    if (status == GX_STATUS_SUCCESS)
    {
        status = GXGetStringValue(device, "DeviceVendorName", &vendorName);
    }
    if (status != GX_STATUS_SUCCESS)
    {
        std::cerr << "Failed to read camera information: " << status << std::endl;
        GXCloseDevice(device);
        GXCloseLib();
        return 1;
    }
    std::cout << "Camera SN: "     << serialNumber.strCurValue << std::endl;
    std::cout << "Camera Vendor: " << vendorName.strCurValue   << std::endl;

    status = GXSetEnumValueByString(device, "AcquisitionMode", "Continuous");
    if (status == GX_STATUS_SUCCESS)
    {
        status = GXSetEnumValueByString(device, "TriggerMode", "Off");
    }
    if (status == GX_STATUS_SUCCESS)
    {
        status = GXStreamOn(device);
    }
    if (status != GX_STATUS_SUCCESS)
    {
        std::cerr << "Failed to start acquisition: " << status << std::endl;
        GXCloseDevice(device);
        GXCloseLib();
        return 1;
    }

    PGX_FRAME_DATA_EX frame = nullptr;
    status = GXDQBufEx(device, &frame, 1000);
    const bool frameValid = status == GX_STATUS_SUCCESS &&
                            frame != nullptr &&
                            frame->nStatus == GX_FRAME_STATUS_SUCCESS;
    cv::Mat capturedImage;
    if (frameValid)
    {
        std::cout << "Image Width: "  << frame->nWidth
                  << ", Height: "     << frame->nHeight
                  << ", Pixel Format: 0x" << std::hex << frame->nPixelFormat
                  << std::dec << std::endl;
        capturedImage = makeDisplayImage(frame);
    }
    else
    {
        std::cerr << "Failed to acquire a valid image: " << status << std::endl;
    }

    if (frame != nullptr)
    {
        GXQBufEx(device, frame);
    }
    GXStreamOff(device);
    GXCloseDevice(device);
    GXCloseLib();

    if (capturedImage.empty())
    {
        return 1;
    }

    cv::imshow("Captured Image", capturedImage);
    cv::waitKey(0);
    return frameValid ? 0 : 1;
}
