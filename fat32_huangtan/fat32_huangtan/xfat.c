#include"xfat.h"
extern u8_t temp_buffer[512];
#define xfat_get_disk(xfat) ((xfat)->disk_part->disk)
static xfat_err_t parse_fat_header(xfat_t* xfat, dbr_t* dbr) {
	xdisk_part_t* xdisk_part = xfat->disk_part;
	xfat->root_cluster = dbr->fat32.BPB_RootClus;
	xfat->fat_tbl_sectors = dbr->fat32.BPB_FATSz32;
	if (dbr->fat32.BPB_ExtFlags & (1 << 7)) {
		u32_t table = dbr->fat32.BPB_ExtFlags & 0xF;
		xfat->fat_start_sector = dbr->bpb.BPB_RsvdSecCnt + xdisk_part->start_sector + table * xfat->fat_tbl_sectors;
		xfat->fat_tbl_nr = 1;
	}
	else {
		xfat->fat_start_sector = dbr->bpb.BPB_RsvdSecCnt + xdisk_part->start_sector;
		xfat->fat_tbl_nr = dbr->bpb.BPB_NumFATs;
	}
	xfat->total_sectors= dbr->bpb.BPB_TotSec32;
	xfat->sec_per_cluster = dbr->bpb.BPB_SecPerClus;
	xfat->cluster_byte_size = xfat->sec_per_cluster * dbr->bpb.BPB_BytesPerSec;
	return FS_ERR_OK;
}
xfat_err_t xfat_open(xfat_t* xfat, xdisk_part_t* xdisk_part) {
	xfat_err_t err;
	dbr_t* dbr = (dbr_t*)temp_buffer;
	xdisk_t* xdisk = xdisk_part->disk;
	err = xdisk_read_sector(xdisk, (u8_t*)dbr, xdisk_part->start_sector, 1);
	if (err < 0) {
		return err;
	}
	err = parse_fat_header(xfat, dbr);
	if (err < 0) {
		return err;
	}
	xfat->fat_buffer = (u8_t*)malloc(xfat->fat_tbl_sectors * xdisk->sector_size);
	xdisk_read_sector(xdisk, xfat->fat_buffer, xfat->fat_start_sector, xfat->fat_tbl_sectors);
	if (err < 0) {
		return err;
	}
	return FS_ERR_OK;
}
u32_t cluster_fist_sector(xfat_t* xfat, u32_t cluster) {
	u32_t data_start_sector = xfat->fat_start_sector + xfat->fat_tbl_nr * xfat->fat_tbl_sectors;
	return data_start_sector + (cluster - 2) * xfat->sec_per_cluster;
}
xfat_err_t is_cluster_valid(u32_t cluster) {
	cluster &= 0xFFFFFFFF;
	return (cluster < 0xFFFFFFF0) && (cluster >= 0x2);
}
xfat_err_t get_next_cluster(xfat_t* xfat, u32_t curr_cluster_no, u32_t* next_cluster) {
	if (is_cluster_valid(curr_cluster_no)) {
		cluster32_t* cluster32_buf = (cluster32_t*)xfat->fat_buffer;
		*next_cluster = cluster32_buf[curr_cluster_no].s.next;
	}
	else {
		*next_cluster = CLUSTER_INVALID;
	}
	return FS_ERR_OK;
}
xfat_err_t read_cluster(xfat_t* xfat, u8_t* buffer, u32_t cluster, u32_t count) {
	xfat_err_t err=0;
	u8_t* current_buffer = buffer;
	u32_t current_sector=cluster_fist_sector(xfat, cluster);
	for (int i = 0; i < count; i++) {
		xdisk_read_sector(xfat->disk_part->disk, current_buffer, current_sector, xfat->sec_per_cluster);
		if (err < 0) {
			return err;
		}
		current_buffer += xfat->cluster_byte_size;
		current_sector += xfat->sec_per_cluster;
	}
	return FS_ERR_OK;
}