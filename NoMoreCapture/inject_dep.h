#pragma once
#include "lilith_is_all_you_need.h"


#define HASH_SEED 19203728

//0xe0 bytes (sizeof)
struct _IMAGE_OPTIONAL_HEADER32
{
    USHORT Magic;                                                           //0x0
    UCHAR MajorLinkerVersion;                                               //0x2
    UCHAR MinorLinkerVersion;                                               //0x3
    ULONG SizeOfCode;                                                       //0x4
    ULONG SizeOfInitializedData;                                            //0x8
    ULONG SizeOfUninitializedData;                                          //0xc
    ULONG AddressOfEntryPoint;                                              //0x10
    ULONG BaseOfCode;                                                       //0x14
    ULONG BaseOfData;                                                       //0x18
    ULONG ImageBase;                                                        //0x1c
    ULONG SectionAlignment;                                                 //0x20
    ULONG FileAlignment;                                                    //0x24
    USHORT MajorOperatingSystemVersion;                                     //0x28
    USHORT MinorOperatingSystemVersion;                                     //0x2a
    USHORT MajorImageVersion;                                               //0x2c
    USHORT MinorImageVersion;                                               //0x2e
    USHORT MajorSubsystemVersion;                                           //0x30
    USHORT MinorSubsystemVersion;                                           //0x32
    ULONG Win32VersionValue;                                                //0x34
    ULONG SizeOfImage;                                                      //0x38
    ULONG SizeOfHeaders;                                                    //0x3c
    ULONG CheckSum;                                                         //0x40
    USHORT Subsystem;                                                       //0x44
    USHORT DllCharacteristics;                                              //0x46
    ULONG SizeOfStackReserve;                                               //0x48
    ULONG SizeOfStackCommit;                                                //0x4c
    ULONG SizeOfHeapReserve;                                                //0x50
    ULONG SizeOfHeapCommit;                                                 //0x54
    ULONG LoaderFlags;                                                      //0x58
    ULONG NumberOfRvaAndSizes;                                              //0x5c
    struct _IMAGE_DATA_DIRECTORY DataDirectory[16];                         //0x60
};

//0x14 bytes (sizeof)
struct _IMAGE_FILE_HEADER32
{
    USHORT Machine;                                                         //0x0
    USHORT NumberOfSections;                                                //0x2
    ULONG TimeDateStamp;                                                    //0x4
    ULONG PointerToSymbolTable;                                             //0x8
    ULONG NumberOfSymbols;                                                  //0xc
    USHORT SizeOfOptionalHeader;                                            //0x10
    USHORT Characteristics;                                                 //0x12
};
//0xf8 bytes (sizeof)
struct _IMAGE_NT_HEADERS32
{
    ULONG Signature;                                                        //0x0
    struct _IMAGE_FILE_HEADER32 FileHeader;                                   //0x4
    struct _IMAGE_OPTIONAL_HEADER32 OptionalHeader;                           //0x18
};

