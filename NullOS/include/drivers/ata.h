/* NullOS ATA/IDE Header */

#ifndef _DRIVERS_ATA_H
#define _DRIVERS_ATA_H

#include <lib/stdint.h>

#define ATA_PRIMARY_IO   0x1F0
#define ATA_SECTOR_SIZE  512

void ata_init(void);
int  ata_read_sector(uint32_t lba, uint8_t *buffer);
int  ata_write_sector(uint32_t lba, const uint8_t *buffer);
int  ata_read(uint32_t lba, uint32_t count, uint8_t *buffer);

#endif /* _DRIVERS_ATA_H */
