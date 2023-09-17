#include "stdafx.h"
#include "pciLab.h"
#include <windows.h>
#include <setupapi.h>
#include <iostream>
#include <iomanip> 

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int main() {
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, L"PCI", NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);

    if (hDevInfo == INVALID_HANDLE_VALUE) {
        std::cerr << "Error opening information about devices" << std::endl;
        return 1;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // Вывод заголовка таблицы
    std::wcout << std::setw(10) << L"Device" << std::setw(12) << L"Vendor ID" << std::setw(12) << L"Device ID" << std::endl;
    std::wcout << std::setw(10) << L"------" << std::setw(12) << L"---------" << std::setw(12) << L"---------" << std::endl;

    for (DWORD devIndex = 0; SetupDiEnumDeviceInfo(hDevInfo, devIndex, &devInfoData); ++devIndex) {
        TCHAR deviceInstanceId[MAX_PATH];
        if (SetupDiGetDeviceInstanceId(hDevInfo, &devInfoData, deviceInstanceId, MAX_PATH, NULL)) {
            std::wstring deviceInstanceIdStr(deviceInstanceId);
            size_t venPos = deviceInstanceIdStr.find(L"VEN_");
            size_t devPos = deviceInstanceIdStr.find(L"DEV_");

            if (venPos != std::wstring::npos && devPos != std::wstring::npos) {
                std::wstring venStr = deviceInstanceIdStr.substr(venPos + 4, 4);
                std::wstring devStr = deviceInstanceIdStr.substr(devPos + 4, 4);

                std::wcout << std::setw(30) << deviceInstanceId << std::setw(8) << venStr.c_str() << std::setw(8) << devStr.c_str() << std::endl;
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);
    getchar();
    return 0;
}

