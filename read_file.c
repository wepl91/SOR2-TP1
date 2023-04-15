#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fat12.h"

int is_file(Fat12Entry *entry, char *file_name, int is_deleted);
void upper(char *filename);
void trim(char *filename);
void get_file_name(char*, Fat12Entry *entry);
void usage();

int main(int argc, char *argv[]) {
	char imagefile[MAX_FAT12_FILENAME_SIZE + 1];
	char searchfile[MAX_FAT12_FILENAME_SIZE + 1];
	char firstCharacter;
	int is_deleted = 0;
	int is_recovered = 0;

	if (argc == 5 && !strcmp(argv[1], "-r")) {
		firstCharacter=toupper(argv[2][0]);
		strcpy(imagefile, argv[3]);
		strcpy(searchfile, argv[4]);
		is_deleted = 1;
		is_recovered = 1;
	} else if (argc == 4 && !strcmp(argv[1], "-d")) {
		strcpy(imagefile, argv[2]);
		strcpy(searchfile, argv[3]);
		is_deleted = 1;
	} else if (argc == 3) {
		strcpy(imagefile, argv[1]);
		strcpy(searchfile, argv[2]);
	} else {
		usage();
		return -1;
	}

	FILE *in = fopen(imagefile, "rb+");
	if (in == NULL) {
		printf("File could not be opened %s\n", imagefile);
	}
	PartitionTable pt[4];
	Fat12BootSector bs;
	Fat12Entry entry;

    // Go to begining of partition table
	fseek(in, BEGIN_PARTITION_TABLE, SEEK_SET);
	fread(pt, sizeof(PartitionTable), 4, in);

	// Go to begining of file
	fseek(in, 0, SEEK_SET);
	// leo el boot sector
	fread(&bs, sizeof(Fat12BootSector), 1, in);

	// Root directory: BootSector + reserved sectors + number of fats
	fseek(in,
			(bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats)
					* bs.sector_size, SEEK_CUR);

    // Read root directory searching for the file
	Fat12Entry file_entry;
	int is_found = 0;
    int i;

	for (i = 0; i < bs.root_dir_entries; i++) {
		fread(&entry, sizeof(entry), 1, in);
		if (is_found==0 && is_file(&entry, searchfile, is_deleted)) {
			memcpy(&file_entry, &entry, sizeof(entry));
			is_found = 1;
		}
		if(is_recovered && is_found){
			//corrijo el primer caracter del nombre de archivo
			file_entry.filename[0]=firstCharacter;

			//vuelvo para atras el puntero al file
			fseek(in,-sizeof(entry), SEEK_CUR);
			//escribo
			fwrite(&file_entry,sizeof(entry), 1, in);
			fclose(in);
			printf("File recovered \n");
			return 0;
		}
	}

	if (!is_found) {
		printf(
				"File %s not found. \n",
				searchfile);
		return 0;
	}

    // Go to the file: First cluster is number 2
	int clusters_to_file = (file_entry.cluster_lowbytes_address - 2);

	//Los bytespor son los cluster * la cantidad de sectores por cluster * el size en bytes de un sector
	int bytes_to_file = clusters_to_file * bs.sector_size
			* bs.sectors_by_cluster;

	// Make fseek
	fseek(in, bytes_to_file, SEEK_CUR);

	// Read file content
	char buffer[2048];
	fread(buffer, 60, 1, in);
	buffer[60] = 0;
	printf("File content: %s", buffer);

	fclose(in);
	return 0;
}

// Determines if the entry corresponds to the file that has the requested name
int is_file(Fat12Entry *entry, char *file_name, int is_deleted) {
	char complete_filename[MAX_FAT12_FILENAME_SIZE + 1] = "";
	char filename[MAX_FAT12_FILENAME_SIZE + 1];

	//paso a mayusculas el archivo que se buscado
	strcpy(filename, file_name);
	upper(filename);

	// Compare non deleted files
	if (is_deleted == 0 && entry->filename[0] != 0x00
			&& entry->filename[0] != 0xE5 && entry->attributes[0] == 0x20) {
		
        // Get file name
		get_file_name(complete_filename, entry);
		int find=!strcmp(complete_filename, filename);
		if(find){
			printf("File %s founded\n",complete_filename);
		}
		return find;
	}
	// Compare deleted files
	if (entry->filename[0] != 0x00 && entry->filename[0] == 0xE5
			&& entry->attributes[0] == 0x20) {

		//Get file name
		get_file_name(complete_filename, entry);
		int find=strstr(&complete_filename[1], filename) != NULL;
		if(find){
			printf("File %s founded\n",complete_filename);
		}
		return find;
	}

	return 0;
}

void get_file_name(char *complete_filename, Fat12Entry *entry) {
	char entry_filename[MAX_FAT12_FILENAME_SIZE + 1];
	char extension[MAX_FAT12_EXTENSION_SIZE + 1];

	//concat trim entry->filename + entry->extension and trim again
	strncpy(entry_filename, entry->filename, sizeof(entry->filename));
	entry_filename[MAX_FAT12_FILENAME_SIZE] = 0;
	trim(entry_filename);
	strncpy(extension, entry->extension, sizeof(entry->extension));
	extension[MAX_FAT12_EXTENSION_SIZE] = 0;
	strcat(complete_filename, entry_filename);
	strcat(complete_filename, ".");
	strcat(complete_filename, extension);
	trim(complete_filename);
}

// Trims a file name
void trim(char *filename) {
	int i = 0;
	while (i < MAX_FAT12_FILENAME_SIZE && filename[i] != 0x20) {
		i++;
	}
	filename[i] = 0;
}

// Make a file name uppercase
void upper(char *filename) {
	for (int i = 0; filename[i] != '\0' && i < MAX_FAT12_FILENAME_SIZE; ++i) {
		filename[i] = toupper(filename[i]);
	}
	return;
}

void usage() {
    printf("How to use\n");
    printf("./read_file  test.img <filename>: Read a not deleted file\n");
	printf("./read_file -d test.img -d <filename>: Read deleted file\n");
    printf("./read_file -r L test.img <filename>: Recover deleted file\n");
	printf("<filename>: File name that want to search in filesystem\n");
	printf("Observations: If file is not deleted, name must match completely\n");
    printf("Observations: If file is deleted, search must be by file partial name without specifying the first character\n");

}

