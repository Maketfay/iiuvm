// Maketfay, 150504
#include "hexioctrl.h"
#include "pci_codes.h"
#include <stdio.h>
#include <conio.h>
#include <iostream>
#include <iomanip> 

#define PCI_ADDRESS_PORT    0x0CF8
#define PCI_DATA_PORT      0x0CFC
#define PCI_MAX_BUSES      128
#define PCI_MAX_DEVICES      32


typedef struct _PCI_CONFIG_ADDRESS {
    union {
        struct {
            UINT32 Zero : 2;
            UINT32 RegisterNumber : 6;
            UINT32 FunctionNumber : 3;
            UINT32 DeviceNumber : 5;
            UINT32 BusNumber : 8;
            UINT32 Reserved : 7;
            UINT32 Enable : 1;
        } s1;
        UINT32 Value;
    } u1;
} PCI_CONFIG_ADDRESS;

void putDeviceInfo(ULONG devId, ULONG venId, ULONG baseClass, ULONG subClass, ULONG prog) {

    bool fl = false;
    for (int i = 0; i < PCI_CLASSCODETABLE_LEN; i++) {
        if (PciClassCodeTable[i].BaseClass == baseClass &&
            PciClassCodeTable[i].SubClass == subClass &&
            PciClassCodeTable[i].ProgIf == prog)
        {
            fl = true;
            printf("| %-25s %-20s %-4s |", PciClassCodeTable[i].BaseDesc, PciClassCodeTable[i].SubDesc, PciClassCodeTable[i].ProgDesc);
            break;
        }
    }
    if (!fl)
    {
        printf("|                                                     |");
    }
    fl = false;

    for (int i = 0; i < PCI_VENTABLE_LEN; i++) {
        if (PciVenTable[i].VenId == venId) {
            printf(" %-27s |", PciVenTable[i].VenFull); break;
        }
    }


    for (int i = 0; i < PCI_DEVTABLE_LEN; i++) {
        if (PciDevTable[i].VenId == venId && PciDevTable[i].DevId == devId) {
            fl = true;
            printf(" %-18s %-40s |", PciDevTable[i].Chip, PciDevTable[i].ChipDesc); break;
        }
    }

    if (!fl)
    {
        printf("                                                             |");
    }
}


int main() {

    PCI_CONFIG_ADDRESS cfg;
    USHORT bus = 0, dev = 0;
    ULONG val = 0;
    ULONG devId, venId;
    ULONG class0, class1, class2, class3;
    int count = 0;

    ALLOW_IO_OPERATIONS;

    std::wcout << L"--------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
    std::wcout << L"| ClassName" << std::setw(55) << L"| VendorName" << std::setw(30) << L"| DeviceName" << std::setw(60) << L"| DeviceId" << std::setw(14) << L"| VendorID |" << std::endl;
    std::wcout << L"--------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;

    for (bus = 0; bus < PCI_MAX_BUSES; bus++) {
        for (dev = 0; dev < PCI_MAX_DEVICES; dev++) {

            cfg.u1.s1.Enable = 1;
            cfg.u1.s1.BusNumber = bus;
            cfg.u1.s1.DeviceNumber = dev;
            cfg.u1.s1.FunctionNumber = 0;
            cfg.u1.s1.RegisterNumber = 0;

            _outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
            val = _inpd(PCI_DATA_PORT);

            if (val == 0 || val == -1) continue;
            count++;


            devId = val >> 16;
            venId = val - (devId << 16);


            cfg.u1.s1.RegisterNumber = 0x08 >> 2;
            _outpd(PCI_ADDRESS_PORT, cfg.u1.Value);
            val = _inpd(PCI_DATA_PORT);
            class0 = val >> 8;
            class1 = class0 >> 16;
            class3 = class0 - (class1 << 16);
            class2 = class3 >> 8;
            class3 = class3 - (class2 << 8);

            putDeviceInfo(devId, venId, class1, class2, class3);
            std::wcout << std::setw(10) << std::hex << devId << " | " << std::setw(8) << venId << " |" << std::dec << std::endl;


            printf("\n");

        }
    }
    std::wcout << L"--------------------------------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
    printf("Total PCI devices discovered: %d", count);
    getchar();
    return 0;
}