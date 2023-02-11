#include"xtypes.h"
#include"xdisk.h"
#include"xfat.h"
#include<stdio.h>
#include<string.h>
static u32_t read_buffer[160 * 1024];
static u32_t write_buffer[160 * 1024];
extern xdisk_driver_t vdisk_driver;
const char* disk_path_test = "disk_test.img";
const char* disk_path = "disk.img";
xdisk_t disk;
xdisk_part_t disk_part;
xfat_t xfat;
int disk_io_test() {
	int err;
	xdisk_t disk_test;
	memset(read_buffer, 0, sizeof(read_buffer));
	//memset(write_buffer, 0, sizeof(write_buffer));
	err = xdisk_open(&disk_test, "vdisk_test", &vdisk_driver, (void*)disk_path_test);
	if (err) {
		printf("open disk failed...\n");
		return -1;
	}
	err = xdisk_write_sector(&disk_test, (u8_t*)write_buffer, 0, 2);
	if (err) {
		printf("disk write failed...\n");
		return -1;
	}
	err = xdisk_read_sector(&disk_test, (u8_t*)read_buffer, 0, 2);
	if (err) {
		printf("disk read failed...\n ");
		return -1;
	}
	err = memcmp((u8_t*)read_buffer, (u8_t*)write_buffer, disk_test.sector_size * 2);
	if (err != 0) {
		printf("not equal...\n");
		return err;
	}
	err = xdisk_close(&disk_test);
	if (err) {
		printf("disk close failed...\n");
		return -1;
	}
	printf("disk io test ok!\n");
	return err;
}
int open_test() {
	xfat_err_t err;
	xfile_t file;
	printf("fs open test ...\n");
	err=xfile_open(&xfat,&file,"/");
	if (err) {
		printf("open file failed %s\n", "/");
		return err;
	}
	xfile_close(&file);
	printf("file open test ok!\n");
	return 0;
}
int disk_part_test() {
	printf("===========disk_part_test start...===========\n");
	u32_t count, i;
	xfat_err_t err = xdisk_get_part_count(&disk,&count);
	if (err < 0) {
		printf("partion count detect failed...\n");
		return err;
	}
	printf("partion count:%d\n", count);
	for (i = 0; i < count; i++) {
		xdisk_part_t part;
		int err;
		err = xdisk_get_part(&disk, &part, i);
		if (err == -1) {
			printf("read partion in failed:%d\n", i);
			return -1;
		}
		printf("no:%d strat:%d count:%d capacity:%.0f M\n", i, part.start_sector, part.total_sector, 
			part.total_sector * disk.sector_size / 1024 / 1024.0);
	}
	return 0;
}
int main() {
	printf("===========test start...============\n");
	xfat_err_t err;
	int i = 0;
	for (i = 0; i < sizeof(write_buffer) / sizeof(u32_t); i++) {
		write_buffer[i] = i;
	}
	err = disk_io_test();
	if (err) {
		return err;
	}
	err = xdisk_open(&disk, "vdisk", &vdisk_driver, (void*)disk_path);
	//disk_part_test();
	if (err) {
		printf("open disk failed...\n");
		return -1;
	}
	//err = disk_part_test();
	//if (err) {
		//return err;
	//}
	err = xdisk_get_part(&disk, &disk_part, 1);
	if (err < 0) {
		printf("read partion info failed...\n");
	}
	xfat.disk_part = &disk_part;
	err = xfat_open(&xfat, &disk_part);
	if (err < 0) {
		return err;
	}
	err = xdisk_close(&disk);
	if (err < 0) {
		printf("disk close failed...\n");
		return -1;
	}
	open_test();
	printf("==========test end...============\n");
	return 0;
}