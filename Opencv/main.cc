#include <iostream>
#include <opencv2/opencv.hpp>
#include <GxIAPI.h>
#include <DxImageProc.h>

#include "IPConfig.h"


GX_IP_CONFIGURE_MODE emIPConfigureMode = GX_IP_CONFIGURE_STATIC_IP;
GX_STATUS emStatus = GX_STATUS_SUCCESS;
uint32_t ui32DeviceNum = 0;
GX_OPEN_PARAM stOpenParam;
GX_DEV_HANDLE hDevice = NULL;

int main(int argc, char* argv[])
{

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
    //     for (uint32_t i = 1; i <= ui32DeviceNum; i++)
    //     {
    //         GX_DEVICE_INFO stDeviceInfo;
    //         GXGetDeviceInfo(i, &stDeviceInfo);
    //     }

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
        if(emStatus != GX_STATUS_SUCCESS)
        {
            std::cerr << "打开设备失败 错误: " << emStatus << std::endl;
            return -1;
        }
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
