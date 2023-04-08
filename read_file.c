#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "fat12.h"

int file_founded(Fat12Entry *entry, char *file_name, int is_deleted);
void upper(char *file_name);
void trim(char *file_name);
void get_file_name(char *, Fat12Entry *entry);
void script_info();

int main(int argc, char *argv[])
{
    char imagefile[MAX_FAT12_FILENAME_SIZE + 1];
    char searchfile[MAX_FAT12_FILENAME_SIZE + 1];
    char firstCharacter;
    int is_deleted = 0;
    int is_recovered = 0;

    if (argc == 5 && !strcmp(argv[1], "-r"))
    {
        firstCharacter = toupper(argv[2][0]);
        strcpy(imagefile, argv[3]);
        strcpy(searchfile, argv[4]);
        is_deleted = 1;
        is_recovered = 1;
    }
    else if (argc == 4 && !strcmp(argv[1], "-d"))
    {
        strcpy(imagefile, argv[2]);
        strcpy(searchfile, argv[3]);
        is_deleted = 1;
    }
    else if (argc == 3)
    {
        strcpy(imagefile, argv[1]);
        strcpy(searchfile, argv[2]);
    }
    else
    {
        script_info();
        return -1;
    }

    FILE *in = fopen(imagefile, "rb+");
    if (in == NULL)
    {
        printf("File could not be opened %s. \n", imagefile);
    }
    PartitionTable pt[4];
    Fat12BootSector bs;
    Fat12Entry entry;

    // Go to begining of particions
    fseek(in, BEGIN_PARTITION_TABLE, SEEK_SET);
    fread(pt, sizeof(PartitionTable), 4, in);

    int i;
    for (i = 0; i < 4; i++)
    {
        if (pt[i].partition_type == 1)
        {
            break;
        }
    }

    if (i == 4)
    {
        printf("FAT12 filesystem not found, exiting... \n");
        return -1;
    }

    // Go to begining of file
    fseek(in, 0, SEEK_SET);

    // leo el boot sector
    fread(&bs, sizeof(Fat12BootSector), 1, in);

    // Voy hasta el root directory que es BootSector + sectores reservados + las dos fats
    fseek(in,
          (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);

    // Read root dir searching for the file
    Fat12Entry file_entry;
    int is_found = 0;
    for (i = 0; i < bs.root_dir_entries; i++)
    {
        fread(&entry, sizeof(entry), 1, in);
        if (is_found == 0 && file_founded(&entry, searchfile, is_deleted))
        {
            memcpy(&file_entry, &entry, sizeof(entry));
            is_found = 1;
        }
        if (is_recovered && is_found)
        {
            // Fix file name first char
            file_entry.filename[0] = firstCharacter;

            // vuelvo para atras el puntero al file
            fseek(in, -sizeof(entry), SEEK_CUR);

            // Write
            fwrite(&file_entry, sizeof(entry), 1, in);
            fclose(in);
            printf("File recovered. \n");
            return 0;
        }
    }

    // If is not found, return
    if (!is_found)
    {
        printf(
            "File %s not found in the filesystem %s. \n",
            searchfile, imagefile);
        return 0;
    }

    // Go to the file
    // First cluster is number two and its where the file begings
    int clusters_to_file = (file_entry.cluster_lowbytes_address - 2);

    int bytes_to_file = clusters_to_file * bs.sector_size * bs.sectors_by_cluster;

    // Make seek
    fseek(in, bytes_to_file, SEEK_CUR);

    // Read file content
    char buffer[2048];
    fread(buffer, 60, 1, in);
    buffer[60] = 0;
    printf("%s", buffer);

    fclose(in);
    return 0;
}

// Determines if the entry corresponds to the file that was searched
int file_founded(Fat12Entry *entry, char *file_name, int is_delete)
{
    char complete_filename[MAX_FAT12_FILENAME_SIZE + 1] = "";
    char file_name[MAX_FAT12_FILENAME_SIZE + 1];

    // Capitalize searched file name
    strcpy(file_name, file_name);
    upper(file_name);

    // Compare files not deleted
    if (is_delete == 0 && entry->filename[0] != 0x00 && entry->filename[0] != 0xE5 && entry->attributes[0] == 0x20)
    {
        // Get file name
        get_file_name(complete_filename, entry);
        int find = !strcmp(complete_filename, file_name);
        if (find)
        {
            printf("File %s not found.\n", complete_filename);
        }
        return find;
    }
    // Campare deleted files
    if (entry->filename[0] != 0x00 && entry->filename[0] == 0xE5 && entry->attributes[0] == 0x20)
    {
        // Get file name
        get_file_name(complete_filename, entry);
        int find = strstr(&complete_filename[1], file_name) != NULL;
        if (find)
        {
            printf("File %s founded. \n", complete_filename);
        }
        return find;
    }

    return 0;
}

void get_file_name(char *complete_filename, Fat12Entry *entry)
{
    char entry_filename[MAX_FAT12_FILENAME_SIZE + 1];
    char extension[MAX_FAT12_EXTENSION_SIZE + 1];

    // concat trimmed entry->filename with entry->extension and then trim again
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

// Trim file name
void trim(char *filename)
{
    int i = 0;
    while (i < MAX_FAT12_FILENAME_SIZE && filename[i] != 0x20)
    {
        i++;
    }
    filename[i] = 0;
}

// File name to capital letter
void upper(char *filename)
{
    for (int i = 0; filename[i] != '\0' && i < MAX_FAT12_FILENAME_SIZE; ++i)
    {
        filename[i] = toupper(filename[i]);
    }
    return;
}

void script_info()
{
    printf("How to use: read_file -r <char> -d <image> <file>\n");
    printf(" -d: Optional. Search a deleted file.\n");
    printf(" -r: Optional. Recover a deleted file.\n");
    printf(" <char>: Optional. Char argument to select script behavior.\n");
    printf(" <image>: Filesystem Image file..\n");
    printf(" <file>: File name that must be searched.\n");
    printf("Observations: If the file was not deleted, the name must fully match the file name, otherwise you must search for part of the file name without specifying the first character\n");
}
