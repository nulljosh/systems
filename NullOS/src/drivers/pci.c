/* NullOS PCI Bus Driver
 *
 * Enumerates devices on the PCI bus.
 * Uses I/O ports 0xCF8 (address) and 0xCFC (data).
 *
 * Future use: detecting and configuring hardware (NIC, USB, etc.)
 */

/* TODO: Implement PCI enumeration
 *
 * PCI configuration address (port 0xCF8):
 *   Bit 31:    Enable
 *   Bits 16-23: Bus number
 *   Bits 11-15: Device number
 *   Bits 8-10:  Function number
 *   Bits 0-7:   Register offset
 *
 * Functions:
 *   pci_init()                            - Scan all buses
 *   pci_read_config(bus, dev, func, off)  - Read 32-bit config word
 *   pci_find_device(vendor, device)       - Find device by ID
 */
