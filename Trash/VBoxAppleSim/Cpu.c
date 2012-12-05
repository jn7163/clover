/* $Id: Cpu.c $ */
/** @file
 * Cpu.c - VirtualBox CPU descriptors
 */

/*
 * Copyright (C) 2009-2010 Oracle Corporation
 *
 * This file is part of VirtualBox Open Source Edition (OSE), as
 * available from http://www.virtualbox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualBox OSE distribution. VirtualBox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */


/*******************************************************************************
 *   Header Files                                                               *
 *******************************************************************************/
#include <Framework/FrameworkInternalFormRepresentation.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/BaseLib.h>

#include "DataHubRecords.h"
//#include <Guid/DataHubRecords.h>

#include <Protocol/Cpu.h>
#include <Protocol/DataHub.h>
#include <Protocol/FrameworkHii.h>
#include <Protocol/CpuIo.h>

#define EFI_CPU_DATA_MAXIMUM_LENGTH 0x100

typedef union {
    EFI_CPU_DATA_RECORD *DataRecord;
    UINT8               *Raw;
} EFI_CPU_DATA_RECORD_BUFFER;

EFI_SUBCLASS_TYPE1_HEADER mCpuDataRecordHeader = {
    EFI_PROCESSOR_SUBCLASS_VERSION,       // Version
    sizeof (EFI_SUBCLASS_TYPE1_HEADER),   // Header Size
    0,                                    // Instance, Initialize later
    EFI_SUBCLASS_INSTANCE_NON_APPLICABLE, // SubInstance
    0                                     // RecordType, Initialize later
};
/*
EFI_GUID gDataHubPlatformGuid = {
    0x64517cc8, 0x6561, 0x4051, {0xb0, 0x3c, 0x59, 0x64, 0xb6, 0x0f, 0x4c, 0x7a }
};
*/
extern EFI_GUID gDataHubPlatformGuid;
#pragma pack(1)
typedef struct {
    EFI_SUBCLASS_TYPE1_HEADER   Hdr;			/* 0x48 */
    UINT32						NameLen;         /* 0x58 , in bytes */
    UINT32						ValLen;          /* 0x5c */
    UINT8						Data[1];         /* 0x60 Name Value */
} PLATFORM_DATA;
#pragma pack()

UINT32
CopyRecord(PLATFORM_DATA* Rec, const CHAR16* Name, VOID* Val, UINT32 ValLen)
{
	CopyMem(&Rec->Hdr, &mCpuDataRecordHeader, sizeof(EFI_SUBCLASS_TYPE1_HEADER));
    Rec->NameLen = (UINT32)StrLen(Name) * sizeof(CHAR16);
    Rec->ValLen = ValLen;
    CopyMem(Rec->Data, Name, Rec->NameLen);
    CopyMem(Rec->Data + Rec->NameLen, Val, ValLen);

    return (sizeof(EFI_SUBCLASS_TYPE1_HEADER) + 8 + Rec->NameLen + Rec->ValLen);
}

EFI_STATUS EFIAPI
LogData(EFI_DATA_HUB_PROTOCOL       *DataHub,
		EFI_GUID					*Guid1,
		EFI_GUID					*Guid2,
        CHAR16                      *Name,
        VOID                        *Data,
        UINT32                       DataSize)
{
    UINT32                      RecordSize;
    EFI_STATUS                  Status;
	PLATFORM_DATA               *PlatformData;
	
    PlatformData = (PLATFORM_DATA*)AllocatePool (0x200);
    if (PlatformData == NULL) {
        return EFI_OUT_OF_RESOURCES;
    }

    RecordSize = CopyRecord(PlatformData, Name, Data, DataSize);
    Status = DataHub->LogData (
							   DataHub,
							   Guid1,					/* DataRecordGuid */
							   Guid2,					/* ProducerName */
							   //&gDataHubPlatformGuid,     							   
							   EFI_DATA_RECORD_CLASS_DATA,
							   PlatformData,
							   RecordSize
                               );
    ASSERT_EFI_ERROR (Status);
	
    FreePool (PlatformData);
    return Status;
}

EFI_STATUS EFIAPI
CpuUpdateDataHub(EFI_DATA_HUB_PROTOCOL       *DataHub,
                 UINT64              FSBFrequency,
                 UINT64              TSCFrequency,
                 UINT64              CPUFrequency)
{
    // Log data in format some OSes like
    LogData(DataHub, &gEfiProcessorSubClassGuid, &gDataHubPlatformGuid,
			L"FSBFrequency", &FSBFrequency, sizeof(FSBFrequency));
    // do that twice, as last variable read not really accounted for
    LogData(DataHub, &gEfiProcessorSubClassGuid, &gDataHubPlatformGuid,
			L"FSBFrequency", &FSBFrequency, sizeof(FSBFrequency));
    LogData(DataHub, &gEfiProcessorSubClassGuid, &gDataHubPlatformGuid,
			L"TSCFrequency", &TSCFrequency, sizeof(TSCFrequency));
    LogData(DataHub, &gEfiProcessorSubClassGuid, &gDataHubPlatformGuid,
			L"CPUFrequency", &CPUFrequency, sizeof(CPUFrequency));

    return EFI_SUCCESS;
}