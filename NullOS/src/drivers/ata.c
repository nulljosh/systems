/* NullOS ATA/IDE Disk Driver
 *
 * PIO mode disk access for reading and writing sectors.
 * Primary bus: ports 0x1F0-0x1F7.
 *
 * See: docs/PHASES.md Phase 5.1
 */

/* TODO: Implement ATA PIO driver
 *
 * Read sequence:
 *   1. Select drive + LBA high bits -> port 0x1F6
 *   2. Sector count -> port 0x1F2
 *   3. LBA bytes -> ports 0x1F3, 0x1F4, 0x1F5
 *   4. Read command (0x20) -> port 0x1F7
 *   5. Poll status until BSY clears and DRQ sets
 *   6. Read 256 words from port 0x1F0
 *
 * Functions:
 *   ata_init()                                    - Identify drive
 *   ata_read_sector(uint32_t lba, uint8_t *buf)   - Read one sector (512 bytes)
 *   ata_write_sector(uint32_t lba, uint8_t *buf)  - Write one sector
 *   ata_read(lba, count, buf)                     - Read multiple sectors
 */
