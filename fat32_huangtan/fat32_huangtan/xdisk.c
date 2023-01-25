#include"xdisk.h"
#include"xfat.h"
#include<stdio.h>
u8_t temp_buffer[512];
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
static xfat_err_t disk_get_extend_part_count(xdisk_t* disk, u32_t start_sector, u32_t* count) {
	u32_t r_count = 0, i = 0;
	u8_t* disk_buffer = temp_buffer;
	u32_t ext_start_sector = start_sector;
	do {
		xfat_err_t err = xdisk_read_sector(disk, disk_buffer, start_sector, 1);
		if (err < 0) {
			return err;
		}
		mbr_part_t* part = ((mbr_t*)disk_buffer)->part_info;
		if (part->system_id == FS_NOT_VALID) {
			break;
		}
		r_count++;
		if ((++part)->system_id != FS_EXTEND) {
			break;
		}
		ext_start_sector += part->relative_sectors;
	} while (1);
	*count = r_count;
	return FS_ERR_OK;
}
xfat_err_t xdisk_get_part_count(xdisk_t* disk, u32_t* count) {
	u32_t r_count = 0, i = 0;
	u8_t* disk_buffer = temp_buffer;
	u8_t extend_part_flag = 0;
	u32_t start_sector[4];
	xfat_err_t err = xdisk_read_sector(disk, disk_buffer, 0, 1);
	if (err < 0) {
		printf("read disk failed\n");
		return err;
	}
	mbr_part_t * part =((mbr_t*)disk_buffer)->part_info;
	for (i=0; i < MBR_PRIMARY_PART_NR; i++, part++) {
		if (part->system_id == FS_NOT_VALID) {
			continue;
		}
		else if(part->system_id==FS_EXTEND){
			extend_part_flag |= 1 << i;
			start_sector[i] = part->relative_sectors;
		}
		else {
			r_count++;
		}
	}
	if (extend_part_flag) {
		for (i = 0; i < MBR_PRIMARY_PART_NR; i++) {
			if (extend_part_flag >> i & 1) {
				u32_t ext_count = 0;
				err = disk_get_extend_part_count(disk, start_sector[i], &ext_count);
				if (err < 0) {
					return err;
				}
				r_count += ext_count;
			}
		}
	}
	*count = r_count;
	return FS_ERR_OK;
}
static xfat_err_t disk_get_exten_part(xdisk_t*disk, xdisk_part_t* xdisk_part,u32_t start_sector,int part_no,u32_t *count)
{
	u32_t r_count = -1, i = 0;
	u8_t* disk_buffer = temp_buffer;
	xfat_err_t err = FS_ERR_OK;
	u32_t ext_start_sector = start_sector;
	do {
		xfat_err_t err = xdisk_read_sector(disk, disk_buffer, start_sector, 1);
		if (err < 0) {
			return err;
		}
		mbr_part_t* part = ((mbr_t*)disk_buffer)->part_info;
		if (part->system_id == FS_NOT_VALID) {
			err = FS_ERR_EOF;
			break;
		}
		if (++r_count == part_no) {
			xdisk_part->disk = disk;
			xdisk_part->start_sector = part->relative_sectors;
			xdisk_part->total_sector = part->total_sector;
			xdisk_part->type =part->system_id;
			break;
		}
		if ((++part)->system_id != FS_EXTEND) {
			err = FS_ERR_EOF;
			break;
		}
		ext_start_sector += part->relative_sectors;
	} while (1);
	*count = r_count+1;
	return err;

}
xfat_err_t xdisk_get_part(xdisk_t* disk, xdisk_part_t* xdisk_part, int part_no) {
	int i = 0;
	int curr_no = -1;
	u8_t* disk_buffer = temp_buffer;
	int err = xdisk_read_sector(disk, temp_buffer, 0, 1);
	if (err < 0) {
		return err;
	}
	mbr_part_t* mbr_part = ((mbr_t*)disk_buffer)->part_info;
	for (i = 0; i < MBR_PRIMARY_PART_NR; i++, mbr_part++) {
		if (mbr_part->system_id == FS_NOT_VALID) {
			continue;
		}
		else if (mbr_part->system_id == FS_EXTEND) {
			u32_t count = 0;
			err = disk_get_exten_part(disk, xdisk_part, mbr_part->relative_sectors, part_no - i, &count);
			if (err < 0) {
				return err;
			}
			if (err == FS_ERR_OK) {
				return FS_ERR_OK;
			}
			else {
				curr_no += count;
				err = xdisk_read_sector(disk, temp_buffer, 0, 1);
				if (err < 0) {
					return err;
				}
			}
		}
		else {
			if (++curr_no == part_no) {
				xdisk_part->disk = disk;
				xdisk_part->start_sector = mbr_part->relative_sectors;
				xdisk_part->total_sector = mbr_part->total_sector;
				xdisk_part->type = mbr_part->system_id;
				return FS_ERR_OK;
			}
		}
	}
	return FS_ERR_NONE;

}