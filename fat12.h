typedef struct {
	unsigned char first_byte;
	unsigned char start_chs[3];
	unsigned char partition_type;
	unsigned char end_chs[3];
	char start_sector[4];
	char length_sectors[4];
} __attribute((packed)) PartitionTable; //32

typedef struct {
	unsigned char jmp[3];
	char oem[8];
	unsigned short sector_size; // 2 bytes

	unsigned char sectors_by_cluster;
	unsigned short reserved_sectors;
	unsigned char number_of_fats;
	unsigned short root_dir_entries;
	unsigned short sector_volumen;
	unsigned char descriptor;
	unsigned short fat_size_sectors;
	unsigned short sector_track;
	unsigned short headers;
	unsigned int sector_hidden;
	unsigned int sector_partition;
	unsigned char physical_device;
	unsigned char current_header;
	unsigned char firm;
	unsigned int volume_id;

	char volume_label[11];
	char fs_type[8]; // Type en ascii
	char boot_code[448];
	unsigned short boot_sector_signature;
} __attribute((packed)) Fat12BootSector; //512

typedef struct {
	unsigned char filename[8];
	unsigned char extension[3];
	unsigned char attributes[1];
	unsigned char reserved[2];
	unsigned char created_time[2];
	unsigned char created_day[2];
	unsigned char accessed_day[2];
	unsigned char cluster_highbytes_address[2];
	unsigned char written_time[2];
	unsigned char written_day[2];
	unsigned short cluster_lowbytes_address;
	unsigned int size_of_file;
} __attribute((packed)) Fat12Entry;

#define BEGIN_PARTITION_TABLE 0x1BE
#define MAX_FAT12_EXTENSION_SIZE 3
#define MAX_FAT12_FILENAME_SIZE 12


