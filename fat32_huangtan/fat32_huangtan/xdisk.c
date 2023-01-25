#include"xdisk.h"
#include"xfat.h"
#include<stdio.h>

xfat_err_t xdisk_open( xdisk_t* disk, const char* name, xdisk_driver_t* driver, void* init_data){
	xfat_err_t err;
	disk->driver = driver;
	//const char* path = (const char*)init_data;
	err=disk->driver->open(disk, init_data);
	if (err < 0) {
		printf("xdisk open failed...\n");
		return err;
	}
	disk->name = name;
	return FS_ERR_OK;
}
xfat_err_t xdisk_close(xdisk_t* disk) {
	xfat_err_t err;
	err=disk->driver->close(disk);
	if (err <0) {
		printf("xdisk close failed...\n");
		return err;
	}
	return err;
}
xfat_err_t xdisk_read_sector(xdisk_t* disk, u8_t* buffer, u32_t start_sector, u32_t count) {
	u32_t total_sector = start_sector + count;
	if (total_sector >=disk->total_sector) {
		return FS_ERR_PARAM;
	}
	xfat_err_t err = disk->driver->read_sector(disk, buffer, start_sector, count);
	if (err < 0) {
		return FS_ERR_IO;
	}
	return err;
}
xfat_err_t xdisk_write_sector(xdisk_t* disk, u8_t* buffer, u32_t start_sector, u32_t count) {
	u32_t total_sector = start_sector + count;
	if (total_sector >= disk->total_sector) {
		return FS_ERR_PARAM;
	}
	xfat_err_t err = disk->driver->write_sector(disk, buffer, start_sector, count);
	if (err < 0) {
		return FS_ERR_IO;
	}
	return err;
}
