#include"xtypes.h"
#include"xdisk.h"
#include"xfat.h"
#include<stdio.h>
#include<string.h>
static u32_t read_buffer[160 * 1024];
static u32_t write_buffer[160 * 1024];
extern xdisk_driver_t vdisk_driver;
const char* disk_path_test = "disk_test.img";
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
	printf("==========test end...============\n");
	return 0;
}