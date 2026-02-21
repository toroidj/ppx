#include <winioctl.h>

//------------------------------------- File Compression
#ifndef COMPRESSION_FORMAT_NONE
	#define COMPRESSION_FORMAT_NONE 0
	#define COMPRESSION_FORMAT_DEFAULT 0
#endif

//------------------------------------- REPARSE POINT
#ifndef IO_REPARSE_TAG_MOUNT_POINT
	#define IO_REPARSE_TAG_MOUNT_POINT 0xA0000003
#endif
#ifndef IO_REPARSE_TAG_SYMLINK
	#define IO_REPARSE_TAG_SYMLINK  0xA000000C
#endif
#ifndef IO_REPARSE_TAG_LX_SYMLINK
	#define IO_REPARSE_TAG_LX_SYMLINK  0xA000001D
#endif

#ifndef MAXIMUM_REPARSE_DATA_BUFFER_SIZE
	#define MAXIMUM_REPARSE_DATA_BUFFER_SIZE	( 16 * 1024 )
	#define FILE_ANY_ACCESS  0
#endif

#ifndef FILE_FLAG_OPEN_REPARSE_POINT
	#define FILE_SPECIAL_ACCESS  (FILE_ANY_ACCESS)
	#define FILE_FLAG_OPEN_REPARSE_POINT  0x00200000
#endif

// REPARSE_DATA_BUFFER Çâ¡çHÇµÇΩç\ë¢ëÃ
typedef struct {
	DWORD	ReparseTag;
	WORD	ReparseDataLength;
	WORD	Reserved;
	union {
		struct { // GUID ReparseGuid;
			WORD	SubstituteNameOffset;
			WORD	SubstituteNameLength;
			WORD	PrintNameOffset;
			WORD	PrintNameLength;
			WCHAR	PathBuffer[1];
		} ReparseGuid;
		struct {
			DWORD	FileType;
			char	PathBuffer[1];
		} LxSymlinkReparseBuffer;
	} data;
	struct {
		BYTE DataBuffer[/*1*/MAXIMUM_REPARSE_DATA_BUFFER_SIZE];
	} GenericReparseBuffer;
} REPARSE_DATA_IOBUFFER, *PREPARSE_DATA_IOBUFFER;

#ifndef FSCTL_SET_REPARSE_POINT
	#define FSCTL_SET_REPARSE_POINT  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 41, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
	#define FSCTL_GET_REPARSE_POINT  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 42, METHOD_BUFFERED, FILE_ANY_ACCESS)
	#define FSCTL_DELETE_REPARSE_POINT  CTL_CODE(FILE_DEVICE_FILE_SYSTEM, 43, METHOD_BUFFERED, FILE_SPECIAL_ACCESS)
#endif

//------------------------------------- Drive status
#ifndef IOCTL_DISK_GET_DRIVE_GEOMETRY_EX
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

typedef struct tagDISK_GEOMETRY_EX {
	struct {
		LARGE_INTEGER Cylinders;
		int MediaType;
		DWORD TracksPerCylinder;
		DWORD SectorsPerTrack;
		DWORD BytesPerSector;
	} Geometry;
	UINTHL DiskSize;
	BYTE Data[1];
} xDISK_GEOMETRY_EX;

#ifndef IOCTL_CDROM_BASE
#define IOCTL_CDROM_BASE  0x00000002
#endif

#ifndef IOCTL_CDROM_GET_DRIVE_GEOMETRY
	#define IOCTL_CDROM_GET_DRIVE_GEOMETRY  CTL_CODE(IOCTL_CDROM_BASE, 0x0013, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif
#ifndef IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX
	#define IOCTL_CDROM_GET_DRIVE_GEOMETRY_EX  CTL_CODE(IOCTL_CDROM_BASE, 0x0014, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif


#define STORAGE_PROPERTY_SIZE 12

#if !defined(IOCTL_STORAGE_QUERY_PROPERTY) || defined(__GNUC__)
#define IOCTL_VOLUME_BASE ((DWORD)'V')
#define IOCTL_STORAGE_QUERY_PROPERTY CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define StorageDeviceProperty 0

typedef struct {
	int PropertyId;
	int QueryType;
	BYTE AdditionalParameters[1];
} STORAGE_PROPERTY_QUERY;

#endif

#ifndef IOCTL_DISK_GET_LENGTH_INFO
#define IOCTL_DISK_GET_LENGTH_INFO  CTL_CODE(IOCTL_DISK_BASE, 0x0017, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

#ifndef IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS
#define IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS  CTL_CODE(IOCTL_VOLUME_BASE, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct {
	DWORD DiskNumber;
	DWORD aliment_padding;
	LARGE_INTEGER StartingOffset;
	LARGE_INTEGER ExtentLength;
} DISK_EXTENT;

typedef struct {
	DWORD NumberOfDiskExtents;
	DWORD aliment_padding;
	DISK_EXTENT Extents[ANYSIZE_ARRAY];
} VOLUME_DISK_EXTENTS;
#endif

typedef struct {
	DWORD Version;
	DWORD Size;
	BOOLEAN IncursSeekPenalty;
} xDEVICE_SEEK_PENALTY_DESCRIPTOR;

typedef struct {
	WORD    Index;
	SHORT   Temperature;
	SHORT   OverThreshold;
	SHORT   UnderThreshold;
	BOOLEAN OverThresholdChangable;
	BOOLEAN UnderThresholdChangable;
	BOOLEAN EventGenerated;
	BYTE    Reserved0;
	DWORD   Reserved1;
} xSTORAGE_TEMPERATURE_INFO;

typedef struct  {
	DWORD Version;
	DWORD Size;
	SHORT CriticalTemperature;
	SHORT WarningTemperature;
	WORD InfoCount;
	BYTE Reserved0[2];
	DWORD Reserved1[2];
	xSTORAGE_TEMPERATURE_INFO TemperatureInfo[1];
} xSTORAGE_TEMPERATURE_DATA_DESCRIPTOR;

typedef struct tagSTORAGE_HW_ENDURANCE_INFO {
	ULONG Version;
	ULONG Size;
	ULONG ValidFields; // B0;GroupId B1:Flags B2:LifePercentage B3:BytesReadCount B4:ByteWriteCount
	ULONG GroupId;
	ULONG Flags;
	ULONG LifePercentage;
	UINTHL BytesReadCount, BytesReadCountH;
	UINTHL ByteWriteCount, ByteWriteCountH;
} xSTORAGE_HW_ENDURANCE_INFO;

typedef struct tagSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR {
	ULONG Version;
	ULONG Size;
	ULONG LunMaxIoCount;
	ULONG AdapterMaxIoCount;
} xSTORAGE_DEVICE_IO_CAPABILITY_DESCRIPTOR;

typedef struct tagSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR {
	ULONG Version;
	ULONG Size;
	ULONG MediumProductType;
} xSTORAGE_MEDIUM_PRODUCT_TYPE_DESCRIPTOR;

typedef struct tagSTORAGE_ADAPTER_DESCRIPTOR {
	ULONG   Version;
	ULONG   Size;
	ULONG   MaximumTransferLength;
	ULONG   MaximumPhysicalPages;
	ULONG   AlignmentMask;
	BOOLEAN AdapterUsesPio;
	BOOLEAN AdapterScansDown;
	BOOLEAN CommandQueueing;
	BOOLEAN AcceleratedTransfer;
	UCHAR   BusType;
	USHORT  BusMajorVersion;
	USHORT  BusMinorVersion;
	UCHAR   SrbType;
	UCHAR   AddressType;
} xSTORAGE_ADAPTER_DESCRIPTOR;

typedef struct {
	DWORD Version;
	DWORD Size;
	BYTE  DeviceType;
	BYTE  DeviceTypeModifier;
	BOOLEAN RemovableMedia;
	BOOLEAN CommandQueueing;
	DWORD VendorIdOffset;
	DWORD ProductIdOffset;
	DWORD ProductRevisionOffset;
	DWORD SerialNumberOffset;
	int BusType;
	DWORD RawPropertiesLength;
	BYTE RawDeviceProperties[1];
} xSTORAGE_DEVICE_DESCRIPTOR;
