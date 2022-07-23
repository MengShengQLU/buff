#include "Camera.h"

unsigned int g_nPayloadSize = 0;
bool PrintDeviceInfo(MV_CC_DEVICE_INFO* pstMVDevInfo)
{
    cout<<"hello word";
    if (NULL == pstMVDevInfo)
    {
        printf("The Pointer of pstMVDevInfo is NULL!\n");
        return false;
    }
    if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE)
    {
        int nIp1 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

        // ch:打印当前相机ip和用户自定义名字 | en:print current ip and user defined name
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
        printf("CurrentIp: %d.%d.%d.%d\n" , nIp1, nIp2, nIp3, nIp4);
        printf("UserDefinedName: %s\n\n" , pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
    }
    else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE)
    {
        printf("Device Model Name: %s\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);

        printf("UserDefinedName: %s\n\n", pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);

       return true;

    }
    else
    {
        printf("Not support.\n");
    }

    return true;
}

Camera::Camera(){
    nRet = MV_OK;
    handle = NULL;
    cout<<"hello word123";
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));

    //枚举所有设备信息
    nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != nRet){
        printf("MV_CC_EnumDevices fail! nRet [%x]\n", nRet);
        return;
    }

    if (stDeviceList.nDeviceNum > 0)
    {
        for (int i = 0; i < stDeviceList.nDeviceNum; i++)
        {
            printf("[device %d]:\n", i);
            MV_CC_DEVICE_INFO* pDeviceInfo = stDeviceList.pDeviceInfo[i];
            if (NULL == pDeviceInfo)
            {
                break;
            }
            PrintDeviceInfo(pDeviceInfo);

        }
    }
    else{
        printf("Find No Devices!\n");
        return;
    }

    // 选择设备并创建句柄
    unsigned int nIndex = 0; //默认首个设备
    nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[nIndex]);
    if (MV_OK != nRet){
        printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
        return;
    }

    //打开设备

    nRet = MV_CC_OpenDevice(handle);
    if (MV_OK != nRet){
        printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
        return;
    }

    // 设置触发模式为off
    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);
    if (MV_OK != nRet)
    {
        printf("MV_CC_SetTriggerMode fail! nRet [%x]\n", nRet);
        return;
    }

    //开始取流
    nRet = MV_CC_StartGrabbing(handle);
    if (MV_OK != nRet)
    {
        printf("MV_CC_StartGrabbing fail! nRet [%x]\n", nRet);
        return;
    }
    nBufSize = 1280*1024*3;
    pFrameBuf = (unsigned char*)malloc(nBufSize);

}

Camera::~Camera(){
    // 停止取流
    nRet = MV_CC_StopGrabbing(handle);
    if (MV_OK != nRet)
    {
        printf("MV_CC_StopGrabbing fail! nRet [%x]\n", nRet);
        return;
    }

    // 关闭设备
    nRet = MV_CC_CloseDevice(handle);
    if (MV_OK != nRet){
        printf("MV_CC_CloseDevice fail! nRet [%x]\n", nRet);
        return;
    }

    //销毁句柄
    nRet = MV_CC_DestroyHandle(handle);
    if (MV_OK != nRet){
        printf("MV_CC_DestroyHandle fail! nRet [%x]\n", nRet);
        return;
    }
}

bool Camera::getImage(Mat &img){
    MV_FRAME_OUT_INFO_EX stImageInfo = {0};
    memset(&stImageInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    nRet = MV_CC_GetImageForBGR(handle, pFrameBuf, nBufSize, &stImageInfo, 100);
    if (nRet == MV_OK)
    {
      //printf("GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n",
        //     stImageInfo.nWidth, stImageInfo.nHeight, stImageInfo.nFrameNum);
        img = Mat(stImageInfo.nHeight,stImageInfo.nWidth,CV_8UC3,pFrameBuf);
       //imshow("test", img);

    }
    else
    {
       // printf("No data[%x]\n", nRet);
        return false;
    }


    return true;
}

void Camera::start(){
    // ch:获取数据包大小 | en:Get payload size
    cout<<"Camer->start";
    MVCC_INTVALUE stIntvalue = { 0 };
    nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stIntvalue);
    if (nRet != 0)
    {
        printf("Get PayloadSize failed! nRet [%x]\n", nRet);
    }
}
