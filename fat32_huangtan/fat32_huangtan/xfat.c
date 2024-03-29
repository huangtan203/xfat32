#include"xfat.h"
#include<stdlib.h>
#include<ctype.h>
#include<string.h>
#include"xdisk.h"
extern u8_t temp_buffer[512];
#define xfat_get_disk(xfat) ((xfat)->disk_part->disk)
#define is_path_sep(ch) (((ch)=='\\')||((ch)=='/'))
#define to_sector(disk,offset) ((offset)/(disk)->sector_size)
#define to_sector_offset(disk,offset) ((offset)%(disk)->sector_size)
#define to_cluster_offset(xfat,pos) ((pos)%((xfat)->cluster_byte_size))
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
static xfat_err_t to_sfn(char*dest_name,const char*my_name) {
	int i, name_len;
	char* dest = dest_name;
	const char* ext_dot;
	const char* p;
	int ext_existed;
	memset(dest, ' ', SFN_LEN);
	while (is_path_sep(*my_name)) {
		my_name++;
	}
	ext_dot = my_name;
	p = my_name;
	name_len = 0;
	while ((*p != '\0') && !is_path_sep(*p)) {
		if (*p == '.') {
			ext_dot = p;
		}
		p++;
		name_len++;
	}
	ext_existed = (ext_dot > my_name && ext_dot < my_name + name_len - 1);
	p = my_name;
	for (i = 0; (i < SFN_LEN) && (*p != '\0') && !is_path_sep(*p); i++) {
		if (ext_existed) {
			if (p == ext_dot) {
				dest = dest_name + 8;
				p++;
				i--;
				continue;
			}
			else {
				*dest++ = toupper(*p++);
			}
		}
		else {
			*dest++ = toupper(*p++);
		}
	}
	return FS_ERR_OK;

}
static u8_t is_filename_match(const char* name_in_dir, const char* to_find_name) {
	char temp_name[SFN_LEN];
	to_sfn(temp_name, to_find_name);
	return memcmp(name_in_dir, to_find_name, SFN_LEN) == 0;
}
static const char* skip_first_path_sep(const char* path) {
	const char* p = path;
	while (is_path_sep(p)) {
		p++;
	}
	return p;
}
const char* get_child_path(const char* dir_path) {
	const char* c = skip_first_path_sep(dir_path);
	while ((*c!='\0')&&!is_path_sep(c)) {
		c++;
	}
	return (*c == '\0') ? (const char* )0 : c + 1;
}
static void copy_date_time(xfile_time_t *dest,const diritem_date_t*date,
	const diritem_time_t*time,const u8_t mil_sec) {
	if (date) {
		dest->year = (u16_t)(date->year_from_1980 + 1980);
		dest->month = (u8_t)date->month;
		dest->day = (u8_t)date->day;
	}
	else {
		dest->year = 0;
		dest->month =0;
		dest->day = 0;
	}
	if (time) {
		dest->hour = (u8_t)time->hour;
		dest->minute = (u8_t)time->minute;
		dest->second = (time->second_2 * 2 + mil_sec / 1000);
	}
	else {
		dest->hour = 0;
		dest->minute = 0;
		dest->second = 0;
	}
}
static xfile_type_t get_file_type(const diritem_t* diritem) {
	xfile_type_t type;
	if (diritem->DIR_Attr & DIRITEM_ATTR_VOLUME_ID) {
		type = FAT_VOL;
	}
	else if (diritem->DIR_Attr & DIRITEM_ATTR_DIRECTORY) {
		type = FAT_DIR;
	}
	else {
		type = FAT_FILE;
	}
	return type;
}
static u32_t get_diritem_cluster(diritem_t* diritem) {
	return (diritem->DIR_FstClusHI << 16) | (diritem->DIR_FstClusL0);
}
static void sfn_to_myname(char*dest_name,const diritem_t*diritem) {
	int i;
	char* dest = dest_name, * raw_name = (char*)diritem->DIR_Name;
	u8_t ext_exist = raw_name[8] != 0x20;
	u8_t scan_len = ext_exist ? SFN_LEN + 1 : SFN_LEN;
	memset(dest_name, 0, X_FILEINFO_NAME_SIZE);
	for (i = 0; i < scan_len; i++) {
		if (*raw_name == ' ') {
			raw_name++;
		}
		else if ((i == 8) && ext_exist) {
			*dest++ = '.';
		}
		else {
			u8_t lower = 0;
			if (((i < 8) && diritem->DIR_NTRes & DIRITEM_NTRES_BODY_LOWER) || (i > 8 && diritem->DIR_NTRes & DIRITEM_NTRES_EXT_LOWER)) {
				lower = 1;
			}
			*dest++ = lower ? tolower(*raw_name++) : toupper(*raw_name++);
		}
	}
	*dest = '\0';
}
static void copy_file_info(xfileinfo_t* info, const diritem_t* dir_item) {
	sfn_to_myname(info->file_name,dir_item);
	info->size = dir_item->DIR_FileSize;
	info->attr = dir_item->DIR_Attr;
	info->type = get_file_type(dir_item);
	copy_date_time(&info->create_time,&dir_item->DIR_CrtDate,&dir_item->DIR_CrtTime,dir_item->DIR_CrtTimeTeenth);
	copy_date_time(&info->last_acctime, &dir_item->DIR_LastAccDate, (diritem_time_t*)0, 0);
	copy_date_time(&info->modity_time, &dir_item->DIR_WrtDate, &dir_item->DIR_WrtTime, dir_item->DIR_CrtTimeTeenth);
}
static xfat_err_t locate_file_dir_item(xfat_t*xfat,u32_t*dir_cluster,u32_t *cluster_offset,const char*path,u32_t*move_bytes,
	diritem_t**r_diritem){
	u32_t curr_cluster = *dir_cluster;
	xdisk_t* xdisk = xfat_get_disk(xfat);
	u32_t initial_sector = to_sector(xdisk, *cluster_offset);
	u32_t initial_offset = to_sector_offset(xdisk, *cluster_offset);
	u32_t r_move_bytes = 0;
	do {
		u32_t i;
		xfat_err_t err;
		u32_t start_sector = cluster_fist_sector(xfat, curr_cluster);
		for (i = initial_sector; i < xfat->sec_per_cluster; i++) {
			err = xdisk_read_sector(xdisk, temp_buffer, i + start_sector, 1);
			if (err < 0) {
				return err;
			}
			u32_t j;
			for (j = initial_offset / sizeof(diritem_t); j < xdisk->sector_size / sizeof(diritem_t); j+=sizeof(diritem_t)) {
				diritem_t* dir_item = (diritem_t*)temp_buffer + j;
				if (dir_item->DIR_Name[0] == DIRITEM_NAME_END) {
					return FS_ERR_EOF;
				}
				else if (dir_item->DIR_Name[0] == DIRITEM_NAME_FREE) {
					continue;
				}
				if ((path == (const char*)0) || (*path) == 0 
					|| is_filename_match(path, (const char*)dir_item->DIR_Name)) {
					u32_t total_offset=i* xdisk->sector_size+j*sizeof(diritem_t);
					*dir_cluster=curr_cluster;
					*move_bytes=r_move_bytes+sizeof(diritem_t);
					*cluster_offset=total_offset;
					if (r_diritem) {
						*r_diritem=dir_item;
					}
					return FS_ERR_OK;
				}
				r_move_bytes+=sizeof(diritem_t);
			}
			err = get_next_cluster(xfat, curr_cluster, &curr_cluster);
			if (err < 0) {
				return err;
			}
			initial_offset = 0;
			initial_sector = 0;
		}
	} while (is_cluster_valid(curr_cluster));
	return FS_ERR_EOF;

}
static xfat_err_t open_sub_file(xfat_t*xfat,u32_t dir_cluster,xfile_t*file,const char*path){
	/*file->size = 0;
	file->type = FAT_DIR;
	file->start_cluster = dir_cluster;
	file->curr_cluster = dir_cluster;
	file->xfat = xfat;
	file->pos = 0;
	file->err = FS_ERR_OK;
	file->attr = 0*/
	u32_t parent_cluster = dir_cluster;
	u32_t parent_cluster_offset = 0;
	path = skip_first_path_sep(path);
	if (path != (const char*)0 && (*path != '\0')) {
		diritem_t* dir_item = (diritem_t*)0;
		u32_t file_start_cluster = 0;
		const char* curr_path = path;
		while (curr_path != (const char*)0) {
			u32_t moved_bytes = 0;
			dir_item = (diritem_t*)0;
			xfat_err_t err= locate_file_dir_item(xfat, &parent_cluster, &parent_cluster_offset, 
				curr_path,&moved_bytes,&dir_item);
			if (err < 0) {
				return err;
			}
			if (dir_item == (diritem_t*)0) {
				return FS_ERR_NONE;
			}
			curr_path = get_child_path(curr_path);
			if (curr_path != (const char*)0) {
				parent_cluster = get_diritem_cluster(dir_item);
				parent_cluster_offset = 0;
			}
			else {
				file_start_cluster = get_diritem_cluster(dir_item);
			}
		}
		file->size = dir_item->DIR_FileSize;
		file->type = get_file_type(dir_item);
		file->start_cluster = file_start_cluster;
		file->curr_cluster = file_start_cluster;
		
	}
	else {
		file->size = 0;
		file->type = FAT_DIR;
		file->start_cluster = parent_cluster;
		file->curr_cluster = parent_cluster;
	}
	file->xfat = xfat;
	file->pos = 0;
	file->err = FS_ERR_OK;
	return FS_ERR_OK;

}
xfat_err_t xfile_open(xfat_t* xfat, xfile_t* xfile, const char* path) {
	return open_sub_file(xfat, xfat->root_cluster, xfile, path);
}
xfat_err_t xfile_close(xfile_t* file) {
	return FS_ERR_OK;
}
xfat_err_t xdir_first_file(xfile_t* file, xfileinfo_t* info) {
	diritem_t* diritem = (diritem_t*)0;
	xfat_err_t err;
	u32_t moved_bytes = 0;
	u32_t cluster_offset;
	if (file->type != FAT_DIR) {
		return FS_ERR_PARAM;
	}
	file->curr_cluster = file->start_cluster;
	file->pos = 0;
	cluster_offset = 0;
	err = locate_file_dir_item(file->xfat, &file->curr_cluster, &cluster_offset, "", & moved_bytes, &diritem);
	if (err < 0) {
		return err;
	}
	if (diritem == (diritem_t*)0) {
		return FS_ERR_EOF;
	}
	file->pos += moved_bytes;
	copy_file_info(info, diritem);
	return err;
}
xfat_err_t xdir_next_file(xfile_t* file, xfileinfo_t* info) {
	xfat_err_t err;
	diritem_t* dir_item = (diritem_t*)0;
	u32_t moved_bytes = 0;
	u32_t cluster_offset;
	if (file->type != FAT_DIR) {
		return FS_ERR_PARAM;
	}
	cluster_offset = to_cluster_offset(file->xfat, file->pos);
	err = locate_file_dir_item(file->xfat, &file->curr_cluster, &cluster_offset, "", &moved_bytes, &dir_item);
	if (err != FS_ERR_OK) {
		return err;
	}
	if (dir_item == (diritem_t*)0) {
		return FS_ERR_EOF;
	}
	file->pos += moved_bytes;
	if (cluster_offset + sizeof(diritem_t) >= file->xfat->cluster_byte_size) {
		err = get_next_cluster(file->xfat, file->curr_cluster, &file->curr_cluster);
		if (err < 0) {
			return err;
		}
	}
	copy_file_info(info, dir_item);
	return err;
}