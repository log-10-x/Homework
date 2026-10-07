#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <limits>
#include <algorithm>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <GxIAPI.h>
#include <DxImageProc.h>

#include "IPConfig.h"

// //图像回调处理
// static void GX_STDC OnFrameCallbackFun(GX_FRAME_CALLBACK_PARAM* pFrame)
// {
//     if(pFrame->status == GX_FRAME_STATUS_SUCCESS)
//     {
//         std::cout << "图像采集成功" << std::endl;
//     }
//     else
//     {
//         std::cerr << "图像采集失败 错误: " << pFrame->status << std::endl;
//     }
// }


GX_IP_CONFIGURE_MODE emIPConfigureMode = GX_IP_CONFIGURE_STATIC_IP;
GX_STATUS emStatus = GX_STATUS_SUCCESS;
uint32_t ui32DeviceNum = 0;
GX_OPEN_PARAM stOpenParam;
GX_DEV_HANDLE hDevice = NULL;
PGX_FRAME_DATA_EX pFrameBuffer;
std::string Classnames = "Inc/classes.txt";


// struct LetterboxInfo
// {
//     float scale;
//     int padX;
//     int padY;
// };

// cv::Mat letterbox(const cv::Mat& src, const cv::Size& newShape, LetterboxInfo& info,
//                   cv::Scalar color = cv::Scalar(114, 114, 114))
// {
//     int w = src.cols, h = src.rows;
//     float r = std::min(newShape.width / (float)w, newShape.height / (float)h);
//     int newW = (int)std::round(w * r);
//     int newH = (int)std::round(h * r);

//     cv::Mat resized;
//     if (w != newW || h != newH)
//         cv::resize(src, resized, cv::Size(newW, newH), 0, 0, cv::INTER_LINEAR);
//     else
//         resized = src;

//     int dw = newShape.width - newW;
//     int dh = newShape.height - newH;
//     int top = dh / 2, bottom = dh - top;
//     int left = dw / 2, right = dw - left;

//     cv::Mat out;
//     cv::copyMakeBorder(resized, out, top, bottom, left, right,
//                        cv::BORDER_CONSTANT, color);

//     info.scale = r;
//     info.padX = left;
//     info.padY = top;
//     return out;
// }
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
    if (image.channels() == 3)
    {
        bgr = image;
    }
    else if (image.channels() == 4)
    {
        cv::cvtColor(image, bgr, cv::COLOR_BGRA2BGR);
    }
    else if (image.channels() == 1)
    {
        cv::cvtColor(image, bgr, cv::COLOR_GRAY2BGR);
    }
    else
    {
        std::cerr << "传统图像识别失败：不支持的图像通道数 "
                  << image.channels() << std::endl;
        return;
    }

    cv::Mat smoothed;
    cv::GaussianBlur(bgr, smoothed, cv::Size(5, 5), 0);
    cv::Mat hsv;
    cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> boardMasks(3);
    cv::inRange(hsv, cv::Scalar(95, 180, 0), cv::Scalar(125, 255, 100),
                boardMasks[0]);

    cv::Mat redLowHue;
    cv::Mat redHighHue;
    cv::inRange(hsv, cv::Scalar(0, 200, 0), cv::Scalar(10, 255, 255),
                redLowHue);
    cv::inRange(hsv, cv::Scalar(170, 200, 0), cv::Scalar(179, 255, 255),
                redHighHue);
    cv::bitwise_or(redLowHue, redHighHue, boardMasks[1]);

    cv::inRange(hsv, cv::Scalar(35, 120, 0), cv::Scalar(85, 255, 220),
                boardMasks[2]);

    const auto percentile = [](std::vector<int>& values, double fraction)
    {
        const size_t index = static_cast<size_t>(
            fraction * static_cast<double>(values.size() - 1));
        std::nth_element(values.begin(), values.begin() + index, values.end());
        return values[index];
    };

    const size_t minimumColorPixels = image.total() / 100;
    bool detected = false;
    for (const cv::Mat& boardMask : boardMasks)
    {
        std::vector<cv::Point> boardPixels;
        cv::findNonZero(boardMask, boardPixels);
        if (boardPixels.size() < minimumColorPixels)
        {
            continue;
        }

        std::vector<int> xCoordinates;
        std::vector<int> yCoordinates;
        xCoordinates.reserve(boardPixels.size());
        yCoordinates.reserve(boardPixels.size());
        for (const cv::Point& pixel : boardPixels)
        {
            xCoordinates.push_back(pixel.x);
            yCoordinates.push_back(pixel.y);
        }

        const int left = percentile(xCoordinates, 0.001);
        const int right = percentile(xCoordinates, 0.98);
        const int top = percentile(yCoordinates, 0.001);
        const int bottom = percentile(yCoordinates, 0.95);
        if (right <= left || bottom <= top)
        {
            continue;
        }

        cv::rectangle(image, cv::Point(left, top), cv::Point(right, bottom),
                      image.channels() == 1 ? cv::Scalar(255)
                                            : cv::Scalar(0, 255, 0),
                      2);
        detected = true;
    }

    if (!detected)
    {
        std::cerr << "未检测到红色、绿色或蓝色板面，请检查光照和相机画面。"
                  << std::endl;
    }
}
}


int main(int argc, char* argv[])
{
    // cv::dnn::Net net = cv::dnn::readNetFromONNX("yolov8.onnx");
    // if (net.empty())
    // {
    //     std::cerr << "加载 ONNX 模型失败，请确认 yolov8.onnx 在当前工作目录" << std::endl;
    //     return -1;
    // }
    // net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
    // net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);

    //获取类名
    std::vector<std::string> classNames;
    std::ifstream file(Classnames);
    std::string line;
    while(std::getline(file, line))
    {
        classNames.push_back(line);
    }
    file.close();
    std::cout << "类的数目: " << classNames.size() << std::endl;

    //初始化GxIAPI库
    emStatus = GXInitLib();
    if(emStatus != GX_STATUS_SUCCESS)
    {
        std::cerr << "初始化失败 错误: " << emStatus << std::endl;
        return -1;
    }

    //获取设备数量，信息
    emStatus = GXUpdateAllDeviceList(&ui32DeviceNum, 1000);
    std::cout << "设备数量: " << ui32DeviceNum << std::endl;
    if((emStatus == GX_STATUS_SUCCESS) && (ui32DeviceNum > 0))
    {
        for (uint32_t i = 1; i <= ui32DeviceNum; i++)
        {
            GX_DEVICE_INFO stDeviceInfo;
            GXGetDeviceInfo(i, &stDeviceInfo);
        }

        emStatus = GXGigEIpConfiguration(GigeConfig::chMAC,
                                         emIPConfigureMode,
                                         GigeConfig::chIpAddress,
                                         GigeConfig::chSubnetMask,
                                         GigeConfig::chDefaultGateway,
                                         GigeConfig::chUserID); 

        stOpenParam.accessMode = GX_ACCESS_EXCLUSIVE;
        stOpenParam.openMode = GX_OPEN_INDEX;
        stOpenParam.pszContent = (char*)"1";


        //打开设备
        emStatus = GXOpenDevice(&stOpenParam, &hDevice);
        if(emStatus == GX_STATUS_SUCCESS)
        {
            std::cout << "打开设备成功" << std::endl;

            emStatus = GXSetEnumValueByString(hDevice, "AcquisitionMode", "Continuous");
            if(emStatus == GX_STATUS_SUCCESS)
            {
                emStatus = GXSetEnumValueByString(hDevice, "TriggerMode", "Off");
            }
            if(emStatus != GX_STATUS_SUCCESS)
            {
                std::cerr << "设置连续采集或关闭触发模式失败 错误: " << emStatus << std::endl;
                GXCloseDevice(hDevice);
                GXCloseLib();
                return -1;
            }

            DX_IMAGE_FORMAT_CONVERT_HANDLE convertHandle = nullptr;
            VxInt32 dxStatus = DxImageFormatConvertCreate(&convertHandle);
            if(dxStatus == DX_OK)
            {
                dxStatus = DxImageFormatConvertSetOutputPixelFormat(convertHandle, GX_PIXEL_FORMAT_BGR8);
            }
            if(dxStatus == DX_OK)
            {
                dxStatus = DxImageFormatConvertSetInterpolationType(convertHandle, RAW2RGB_NEIGHBOUR);
            }
            if(dxStatus != DX_OK)
            {
                std::cerr << "初始化图像格式转换失败 错误: " << dxStatus << std::endl;
                if(convertHandle != nullptr)
                {
                    DxImageFormatConvertDestroy(convertHandle);
                }
                GXCloseDevice(hDevice);
                GXCloseLib();
                return -1;
            }

            int exitCode = 0;
            bool streamStarted = false;

            // DQBuf 图像采集
            #ifdef __linux__
            emStatus = GXStreamOn(hDevice);
            #else
            emStatus = GXSetCommandValue(hDevice, "AcquisitionStart");
            #endif
            if(emStatus == GX_STATUS_SUCCESS)
            {
                streamStarted = true;
                cv::namedWindow("Window", cv::WINDOW_NORMAL | cv::WINDOW_FREERATIO);
                while(true)
                {
                    PGX_FRAME_DATA_EX pFrameBuffer = nullptr;
                    emStatus = GXDQBufEx(hDevice, &pFrameBuffer, 1000);
                    if(emStatus != GX_STATUS_SUCCESS)
                    {
                        if(emStatus != GX_STATUS_TIMEOUT)
                        {
                            std::cerr << "读取相机图像失败 错误: " << emStatus << std::endl;
                            exitCode = 1;
                            break;
                        }
                        continue;
                    }

                    if(pFrameBuffer == nullptr)
                    {
                        std::cerr << "相机返回空图像帧" << std::endl;
                        exitCode = 1;
                        break;
                    }

                    cv::Mat displayImage;
                    if(pFrameBuffer->nStatus == GX_FRAME_STATUS_SUCCESS)
                    {
                        const uint64_t pixelCount =
                            static_cast<uint64_t>(pFrameBuffer->nWidth) * pFrameBuffer->nHeight;
                        if(pFrameBuffer->nWidth == 0 || pFrameBuffer->nHeight == 0 ||
                           pFrameBuffer->nWidth > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
                           pFrameBuffer->nHeight > static_cast<uint32_t>(std::numeric_limits<int>::max()) ||
                           pixelCount > static_cast<uint64_t>(std::numeric_limits<int>::max()) / 3 ||
                           pFrameBuffer->nImgSize > static_cast<uint32_t>(std::numeric_limits<int>::max()))
                        {
                            std::cerr << "相机图像尺寸或缓冲区大小无效" << std::endl;
                            exitCode = 1;
                        }
                        else
                        {
                            const GX_PIXEL_FORMAT_ENTRY pixelFormat =
                                static_cast<GX_PIXEL_FORMAT_ENTRY>(pFrameBuffer->nPixelFormat);
                            dxStatus = DxImageFormatConvertSetValidBits(convertHandle, getValidBits(pixelFormat));
                            if(dxStatus == DX_OK)
                            {
                                std::vector<unsigned char> convertedImage(
                                    static_cast<size_t>(pixelCount) * 3);
                                dxStatus = DxImageFormatConvert(
                                    convertHandle,
                                    reinterpret_cast<void*>(pFrameBuffer->pImgBuf),
                                    static_cast<int>(pFrameBuffer->nImgSize),
                                    convertedImage.data(),
                                    static_cast<int>(convertedImage.size()),
                                    pixelFormat,
                                    pFrameBuffer->nWidth,
                                    pFrameBuffer->nHeight,
                                    false);
                                if(dxStatus == DX_OK)
                                {
                                    displayImage = cv::Mat(
                                        static_cast<int>(pFrameBuffer->nHeight),
                                        static_cast<int>(pFrameBuffer->nWidth),
                                        CV_8UC3,
                                        convertedImage.data()).clone();
                                }
                            }
                            if(dxStatus != DX_OK)
                            {
                                std::cerr << "转换相机图像格式失败 错误: " << dxStatus << std::endl;
                                exitCode = 1;
                            }
                        }
                    }
                    else
                    {
                        std::cerr << "相机图像帧无效 错误: " << pFrameBuffer->nStatus << std::endl;
                    }

                    const GX_STATUS queueStatus = GXQBufEx(hDevice, pFrameBuffer);
                    if(queueStatus != GX_STATUS_SUCCESS)
                    {
                        std::cerr << "归还相机图像缓冲区失败 错误: " << queueStatus << std::endl;
                        exitCode = 1;
                        break;
                    }
                    if(exitCode != 0)
                    {
                        break;
                    }
                    if(!displayImage.empty())
                    {
                        // // ---------- 1. 预处理 ----------
                        // LetterboxInfo lbInfo;
                        // cv::Mat input = letterbox(displayImage, cv::Size(640, 640), lbInfo);

                        // cv::Mat blob;
                        // cv::dnn::blobFromImage(input, blob, 1.0 / 255.0,
                        //                     cv::Size(640, 640), cv::Scalar(), true, false);
                        // net.setInput(blob);

                        // // ---------- 2. 前向传播 ----------
                        // std::vector<cv::Mat> outputs;
                        // net.forward(outputs, net.getUnconnectedOutLayersNames());

                        // // ---------- 3. 解析输出 ----------
                        // // YOLOv8 输出: [1, 84, 8400]  => 84 = 4(box) + 80(classes)
                        // cv::Mat out = outputs[0];
                        // cv::Mat pred = out.reshape(1, out.size[1]);   // [84, 8400]
                        // cv::transpose(pred, pred);                    // [8400, 84]

                        // const int rows = pred.rows;
                        // const int dims = pred.cols;
                        // const int numClasses = dims - 4;

                        // std::vector<cv::Rect> boxes;
                        // std::vector<float> confidences;
                        // std::vector<int> classIds;

                        // for (int i = 0; i < rows; ++i)
                        // {
                        //     const float* row = pred.ptr<float>(i);

                        //     // 找最大类别分数
                        //     int bestId = 0;
                        //     float bestScore = 0.f;
                        //     for (int c = 0; c < numClasses; ++c)
                        //     {
                        //         if (row[4 + c] > bestScore)
                        //         {
                        //             bestScore = row[4 + c];
                        //             bestId = c;
                        //         }
                        //     }
                        //     if (bestScore < 0.25f) continue;   // 置信度阈值

                        //     // 中心点 + 宽高 -> 原图坐标
                        //     float cx = row[0], cy = row[1], bw = row[2], bh = row[3];
                        //     float x = (cx - bw / 2.f - lbInfo.padX) / lbInfo.scale;
                        //     float y = (cy - bh / 2.f - lbInfo.padY) / lbInfo.scale;
                        //     float w = bw / lbInfo.scale;
                        //     float h = bh / lbInfo.scale;

                        //     // 裁剪到图像范围内
                        //     x = std::max(0.f, std::min(x, (float)displayImage.cols - 1));
                        //     y = std::max(0.f, std::min(y, (float)displayImage.rows - 1));
                        //     w = std::min(w, displayImage.cols - x);
                        //     h = std::min(h, displayImage.rows - y);

                        //     boxes.emplace_back(cv::Rect((int)x, (int)y, (int)w, (int)h));
                        //     confidences.push_back(bestScore);
                        //     classIds.push_back(bestId);
                        // }

                        // // ---------- 4. NMS 去重 ----------
                        // std::vector<int> indices;
                        // cv::dnn::NMSBoxes(boxes, confidences, 0.25f, 0.45f, indices);

                        // // ---------- 5. 画框 ----------
                        // for (int idx : indices)
                        // {
                        //     const cv::Rect& box = boxes[idx];
                        //     cv::rectangle(displayImage, box, cv::Scalar(0, 255, 0), 2);

                        //     std::string label = (classIds[idx] < (int)classNames.size())
                        //                         ? classNames[classIds[idx]]
                        //                         : std::to_string(classIds[idx]);
                        //     label += " " + cv::format("%.2f", confidences[idx]);

                        //     int baseLine = 0;
                        //     cv::Size tsize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX,
                        //                                     0.5, 1, &baseLine);
                        //     int top = std::max(box.y, tsize.height + 5);
                        //     cv::rectangle(displayImage,
                        //                 cv::Point(box.x, top - tsize.height - 5),
                        //                 cv::Point(box.x + tsize.width, top),
                        //                 cv::Scalar(0, 255, 0), cv::FILLED);
                        //     cv::putText(displayImage, label,
                        //                 cv::Point(box.x, top - 3),
                        //                 cv::FONT_HERSHEY_SIMPLEX, 0.5,
                        //                 cv::Scalar(0, 0, 0), 1);
                        // }

                        drawTraditionalDetectionBoxes(displayImage);
                        cv::imshow("Window", displayImage);
                    }
                    if(cv::waitKey(1) == 27)
                    {
                        break;
                    }
                }
            }
            else
            {
                std::cerr << "启动相机图像流失败 错误: " << emStatus << std::endl;
                exitCode = 1;
            }

            if(streamStarted)
            {
                #ifdef __linux__
                emStatus = GXStreamOff(hDevice);
                #else
                emStatus = GXSetCommandValue(hDevice, "AcquisitionStop");
                #endif
                if(emStatus != GX_STATUS_SUCCESS)
                {
                    std::cerr << "停止相机采集失败 错误: " << emStatus << std::endl;
                    exitCode = 1;
                }
            }

            dxStatus = DxImageFormatConvertDestroy(convertHandle);
            if(dxStatus != DX_OK)
            {
                std::cerr << "释放图像格式转换器失败 错误: " << dxStatus << std::endl;
                exitCode = 1;
            }

            emStatus = GXCloseDevice(hDevice);
            if(emStatus != GX_STATUS_SUCCESS)
            {
                std::cerr << "关闭相机失败 错误: " << emStatus << std::endl;
                exitCode = 1;
            }
            cv::destroyAllWindows();
            if(exitCode != 0)
            {
                GXCloseLib();
                return exitCode;
            }

        }
        else
        {
            std::cerr << "打开设备失败 错误: " << emStatus << std::endl;
            return -1;
        }
    }
        
    else
    {
        std::cerr << "没有找到设备" << std::endl;
        return -1;
    }

    //关闭GxIAPI库
    emStatus = GXCloseLib();
    if(emStatus != GX_STATUS_SUCCESS)
    {
        std::cerr << "关闭失败 错误: " << emStatus << std::endl;
        return -1;
    }
    else
    {
        std::cout << "关闭成功" << std::endl;
        return 0;
    }
}
