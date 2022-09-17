#ifndef MDADM_H_
#define MDADM_H_

#include <stdint.h>
#include "jbod.h"
#include "cache.h"
#include "net.h"

/* Return 1 on success and -1 on failure */
int mdadm_mount(void);

/* Return 1 on success and -1 on failure */
int mdadm_unmount(void);

/* Return the number of bytes read on success, -1 on failure. */
int mdadm_read(uint32_t start_addr, uint32_t read_len, uint8_t *read_buf);

/* Return the number of bytes written on success, -1 on failure. */
int mdadm_write(uint32_t start_addr, uint32_t write_len, const uint8_t *write_buf);

uint32_t op(uint32_t reserved, uint32_t command, uint32_t blockID, uint32_t diskID);

#endif
