#include<stdio.h>
#include"xdisk.h"
#include"xfat.h"
#include"xtypes.h"

static xfat_err_t xdisk_hw_open(xdisk_t* disk, void* init_data) {
	const char* path = (const char*)init_data;
	FILE* file = fopen(path, "rb+");
	if (file == NULL) {
		printf("open disk failed:%s\n", path);
		return FS_ERR_IO;
	}
	disk->data = file;
	disk->sector_size = 512;
	//ftell用于得到文件位置指针当前位置相对于文件首的偏移字节数
	fseek(file, 0, SEEK_END);
	disk->total_sector = ftell(file) / disk->sector_size;
	return FS_ERR_OK;
}
static xfat_err_t xdisk_hw_close(xdisk_t* disk) {
	FILE* file = (FILE*)disk->data;
	int err = fclose(file);
	if (err == -1) {
		printf("disk close failed\n");
		return FS_ERR_IO;
	}
	return FS_ERR_OK;
}
static xfat_err_t xdisk_hw_read_sector(xdisk_t* disk, u8_t* buffer, u32_t start_sector, u32_t count) {
	u32_t offset = start_sector * disk->sector_size;
	FILE* file = (FILE*)disk->data;
	int err=fseek(file, offset, SEEK_SET);
	if (err == -1) {
		printf("seek disk failed:0x%x\n",(int)offset);
		return FS_ERR_IO;
	}
	err=(int)fread(buffer, disk->sector_size, count,file);
	if (err == -1) {
		printf("read disk failed,sector:0x%x,count:0x%x\n",(int)start_sector,(int)count);
		return FS_ERR_IO;
	}
	return FS_ERR_OK;
}
static xfat_err_t xdisk_hw_write_sector(xdisk_t* disk, u8_t* buffer, u32_t start_sector, u32_t count) {
	u32_t offset = start_sector * disk->sector_size;
	FILE* file = (FILE*)disk->data;
	int err = fseek(file, offset, SEEK_SET);
	if (err == -1) {
		printf("seek disk failed:0x%x\n", (int)offset);
		return FS_ERR_IO;
	}
	err =(int) fwrite(buffer, disk->sector_size, count, file);
	if (err == -1) {
		printf("write disk failed,sector:0x%x,count:0x%x\n", (int)start_sector,(int)count);
		return FS_ERR_IO;
	}
	fflush(file);
	return FS_ERR_OK;
}
xdisk_driver_t vdisk_driver = {
	.open = xdisk_hw_open,
	.close = xdisk_hw_close,
	.read_sector = xdisk_hw_read_sector,
	.write_sector = xdisk_hw_write_sector,
};