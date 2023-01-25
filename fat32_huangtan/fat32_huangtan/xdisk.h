#ifndef XDISK_H
#define XDSIK_H
#include"xtypes.h"
typedef enum {
	FS_NOT_VALID = 0x00,
	FS_FAT32=0x01,
	FS_EXTEND=0x05,
	FS_WIN95_FAT32_0=0xB,
	FS_WIN95_FAT32_1=0xC,
}xfs_type_t;
#pragma(1)
typedef struct _mbr_part_t {
	u8_t boot_active;
	u8_t start_header;
	u16_t start_sector:6;
	u16_t start_cylinder:10;
	u8_t system_id;
	u8_t end_header;
	u16_t end_sector:6;
	u16_t end_cylinder:10;
	u32_t relative_sectors;
	u32_t total_sector;
}mbr_part_t;
#define MBR_PRIMARY_PART_NR 4
typedef struct _mbr_t {
	u8_t code[446];
	mbr_part_t part_info[MBR_PRIMARY_PART_NR];
	u8_t boot_sig[2];
}mbr_t;
#pragma pack()
typedef struct _xdisk_driver_t {
	xfat_err_t(*open)(struct _xdisk_t*xdisk_t,void*init_data);
	xfat_err_t(*close)(struct _xdisk_t*xdisk_t);
	xfat_err_t(*read_sector)(struct _xdisk_t*xdisk_t,u8_t*buffer,u32_t start_sector,u32_t count);
	xfat_err_t(*write_sector)(struct _xdisk_t*xdisk_t,u8_t*buffer,u32_t start_sector,u32_t count);
}xdisk_driver_t;
typedef struct _xdist_t {
	const char* name;
	u32_t sector_size;
	u32_t total_sector;
	xdisk_driver_t* driver;
	void* data;
}xdisk_t;

typedef struct _xdisk_part_t {
	u32_t start_sector;
	u32_t total_sector;
	xfs_type_t type;
	xdisk_t* disk;
}xdisk_part_t;

xfat_err_t xdisk_open(xdisk_t* xdisk_t, const char*name,struct _xdisk_driver_t*driver,void* init_data);
xfat_err_t xdisk_close(xdisk_t* xdisk_t);
xfat_err_t xdisk_read_sector(xdisk_t* xdisk_t, u8_t* buffer, u32_t start_sector, u32_t count);
xfat_err_t xdisk_write_sector(xdisk_t* xdisk_t, u8_t* buffer, u32_t start_sector, u32_t count);

#endif