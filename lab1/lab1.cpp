#include <iostream>
#include <windows.h>
#include <powrprof.h>
#include <batclass.h>
#include <setupapi.h>
#include <devguid.h>

#define INITGUID
#define GBS_HASBATTERY 0x1
#define GBS_ONBATTERY  0x2

#pragma comment (lib, "Setupapi.lib")
#pragma comment(lib, "PowrProf.lib")

struct BatterySateInfo {
    DWORD state;
    std::string chemistry; 
    long FullChargedCapacity;
    long DesignedCapacity;
    long CycleCount;
    long Capacity;
    long EstimatedTime;
};

BatterySateInfo GetBatteryState();

void GetPowerInfo() {

    auto batteryInfo = GetBatteryState();

    if (!batteryInfo.state & GBS_HASBATTERY) 
    {
        std::cout << "Системная батарея не найдена" << std::endl;
    }

    SYSTEM_POWER_STATUS powerStatus;

    if (GetSystemPowerStatus(&powerStatus)) {

        std::cout << "Тип энергопитания: ";

        if (batteryInfo.state & GBS_ONBATTERY) 
            std::cout << "Батарея\n";
        else
            std::cout << "Подключено к сети\n";


        std::cout << "Тип батареи: " << batteryInfo.chemistry << std::endl;

        auto charge = (double)batteryInfo.Capacity / batteryInfo.FullChargedCapacity * 100;

        std::cout << "Уровень заряда батареи: " << charge << "%\n";

        std::cout << "Текущий режим энергосбережения: ";
        switch (powerStatus.SystemStatusFlag) {
        case 0:
            std::cout << "Выключен\n";
            break;
        case 1:
            std::cout << "Активен\n";
            break;
        default:
            std::cout << "Неизвестно\n";
        }

        std::cout << "Текущее время работы аккумулятора: " << batteryInfo.EstimatedTime / 60 << " мин\n";

        int wear = (double) batteryInfo.FullChargedCapacity / batteryInfo.DesignedCapacity * 100;

        std::cout << "Состояние батареи: " << wear << std::endl;

        std::cout << "Количество циклов заряда/разряда батареи: " << batteryInfo.CycleCount << std::endl;

    }
    else {
        std::cerr << "Ошибка при получении информации об энергопитании.\n";
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    GetPowerInfo();

    while (true) {
        std::cout << "Выберите действие:\n";
        std::cout << "1. Перейти в спящий режим\n";
        std::cout << "2. Гибернация\n";
        std::cout << "3. Обновить информацию\n";
        std::cout << "4. Выйти из программы\n";

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1:
            SetSuspendState(FALSE, FALSE, FALSE);
            break;
        case 2:
            SetSuspendState(TRUE, FALSE, FALSE);
            break;
        case 3:
            GetPowerInfo();
            break;
        case 4:
            return 0;
        default:
            std::cout << "Некорректный выбор.\n";
        }

    }

    return 0;
}



BatterySateInfo GetBatteryState()
{

    BatterySateInfo batteryStateInfo = {0};

    DWORD dwResult = GBS_ONBATTERY;

    HDEVINFO hdev = SetupDiGetClassDevs(&GUID_DEVCLASS_BATTERY, 0, 0, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE); 
    
    if (INVALID_HANDLE_VALUE != hdev)
    {
        
        int idev = 0;

        SP_DEVICE_INTERFACE_DATA did = { 0 };
        did.cbSize = sizeof(did);

        if (SetupDiEnumDeviceInterfaces(hdev, 0, &GUID_DEVCLASS_BATTERY, idev, &did))
        {
            DWORD cbRequired = 0;

            SetupDiGetDeviceInterfaceDetail(hdev, &did, 0, 0, &cbRequired, 0);
            if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
                PSP_DEVICE_INTERFACE_DETAIL_DATA pdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LPTR, cbRequired);
                if (pdidd)
                {
                    pdidd->cbSize = sizeof(*pdidd);
                    if (SetupDiGetDeviceInterfaceDetail(hdev, &did, pdidd, cbRequired, &cbRequired, 0))
                    {
                        HANDLE hBattery =
                            CreateFile(pdidd->DevicePath,
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL);
                        if (INVALID_HANDLE_VALUE != hBattery)
                        {
                            BATTERY_QUERY_INFORMATION bqi = { 0 };

                            DWORD dwWait = 0;
                            DWORD dwOut;

                            if (DeviceIoControl(hBattery,
                                IOCTL_BATTERY_QUERY_TAG,
                                &dwWait,
                                sizeof(dwWait),
                                &bqi.BatteryTag,
                                sizeof(bqi.BatteryTag),
                                &dwOut,
                                NULL)
                                && bqi.BatteryTag)
                            {
                                BATTERY_INFORMATION bi = { 0 };
                                bqi.InformationLevel = BatteryInformation;

                                if (DeviceIoControl(hBattery,
                                    IOCTL_BATTERY_QUERY_INFORMATION,
                                    &bqi,
                                    sizeof(bqi),
                                    &bi,
                                    sizeof(bi),
                                    &dwOut,
                                    NULL))
                                {
                                    if (bi.Capabilities & BATTERY_SYSTEM_BATTERY)
                                    {
                                        if (!(bi.Capabilities & BATTERY_IS_SHORT_TERM))
                                        {
                                            dwResult |= GBS_HASBATTERY;

                                            std::string chemistry(bi.Chemistry, bi.Chemistry + 4);

                                            batteryStateInfo.chemistry = chemistry;

                                            batteryStateInfo.CycleCount = bi.CycleCount;

                                            batteryStateInfo.DesignedCapacity = bi.DesignedCapacity;

                                            batteryStateInfo.FullChargedCapacity = bi.FullChargedCapacity;

                                            BATTERY_QUERY_INFORMATION timeQuery = { 0 };
                                            timeQuery.BatteryTag = bqi.BatteryTag;
                                            timeQuery.InformationLevel = BatteryEstimatedTime;

                                            DeviceIoControl(hBattery,
                                                IOCTL_BATTERY_QUERY_INFORMATION,
                                                &timeQuery,
                                                sizeof(timeQuery),
                                                &batteryStateInfo.EstimatedTime,
                                                sizeof(batteryStateInfo.EstimatedTime),
                                                &dwOut,
                                                NULL);
                                        }


                                        BATTERY_WAIT_STATUS bws = { 0 };
                                        bws.BatteryTag = bqi.BatteryTag;

                                        BATTERY_STATUS bs;
                                        if (DeviceIoControl(hBattery,
                                            IOCTL_BATTERY_QUERY_STATUS,
                                            &bws,
                                            sizeof(bws),
                                            &bs,
                                            sizeof(bs),
                                            &dwOut,
                                            NULL))
                                        {
                                            if (bs.PowerState & BATTERY_POWER_ON_LINE)
                                            {
                                                dwResult &= ~GBS_ONBATTERY;
                                            }

                                            batteryStateInfo.Capacity = bs.Capacity;
                                        }
                                    }
                                }
                            }
                            CloseHandle(hBattery);
                        }
                    }
                    LocalFree(pdidd);
                }
            }
        }
        SetupDiDestroyDeviceInfoList(hdev);
    }

    if (!(dwResult & GBS_HASBATTERY))
        dwResult &= ~GBS_ONBATTERY;

    batteryStateInfo.state = dwResult;

    return batteryStateInfo;
}