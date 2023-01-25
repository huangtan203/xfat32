#ifndef XDISK_H
#define XDSIK_H
#include"xtypes.h"
#pragma(1)
typedef struct _mbr_part_t {
	u8_t boot_active;
	//u8_t;
}mbr_part_t;
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



xfat_err_t xdisk_open(xdisk_t* xdisk_t, const char*name,struct _xdisk_driver_t*driver,void* init_data);
xfat_err_t xdisk_close(xdisk_t* xdisk_t);
xfat_err_t xdisk_read_sector(xdisk_t* xdisk_t, u8_t* buffer, u32_t start_sector, u32_t count);
xfat_err_t xdisk_write_sector(xdisk_t* xdisk_t, u8_t* buffer, u32_t start_sector, u32_t count);

#endif