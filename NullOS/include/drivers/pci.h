/* NullOS PCI Header */

#ifndef _DRIVERS_PCI_H
#define _DRIVERS_PCI_H

#include <lib/stdint.h>

void     pci_init(void);
uint32_t pci_read_config(uint8_t bus, uint8_t device, uint8_t func, uint8_t offset);

#endif /* _DRIVERS_PCI_H */
