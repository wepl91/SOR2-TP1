#include <stdio.h>
#include <stdlib.h>
#include "fat12.h"

void print_file_info(Fat12Entry *entry);

int main()
{
    FILE *in = fopen("test.img", "rb");
    int i;
    PartitionTable pt[4];
    Fat12BootSector bs;
    Fat12Entry entry;

    // Voy al inicio de la tabla de particiones
    fseek(in, BEGIN_PARTITION_TABLE, SEEK_SET);
    fread(pt, sizeof(PartitionTable), 4, in);

    for (i = 0; i < 4; i++)
    {
        if (pt[i].partition_type == 1)
        {
            printf("Encontrada particion FAT12 %d\n", i);
            break;
        }
    }

    if (i == 4)
    {
        printf("No encontrado filesystem FAT12, saliendo...\n");
        return -1;
    }

    fseek(in, 0, SEEK_SET);
    fread(&bs, sizeof(Fat12BootSector), 1, in);

    printf("En  0x%X, sector size %d, FAT size %d sectors, %d FATs\n\n",
           ftell(in), bs.sector_size, bs.fat_size_sectors, bs.number_of_fats);

    fseek(in,
          (bs.reserved_sectors - 1 + bs.fat_size_sectors * bs.number_of_fats) * bs.sector_size, SEEK_CUR);

    printf("Root dir_entries %d \n", bs.root_dir_entries);
    for (i = 0; i < bs.root_dir_entries; i++)
    {
        print_file_info(&entry);
        fread(&entry, sizeof(entry), 1, in);
    }

    printf("\nLeido Root directory, ahora en 0x%X\n", ftell(in));
    fclose(in);
    return 0;
}

void print_file_info(Fat12Entry *entry)
{

    switch (entry->filename[0])
    {
    case 0x00:
        return; // unused entry
    case 0xE5:
        printf("Archivo borrado: [?%.7s.%.3s]\n", &entry->filename[1], entry->extension);
        return;
        // case ...: // Completar los ...
        //     printf("Archivo que comienza con 0xE5: [%c%.7s.%.3s]\n", 0xE5, entry->filename, entry->extension);
        //     break;
        break;
    default:
        switch (entry->attributes[0])
        {
        case 0x10:
            printf("Directory: [%.8s.%.3s]\n", entry->filename,
                   entry->extension);
            return;
        case 0x20:
            printf("File: [%.8s.%.3s]\n", entry->filename,
                   entry->extension);
            return;
        }
    }
}
