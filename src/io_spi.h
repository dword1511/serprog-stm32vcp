#ifndef __IO_SPI_H__
#define __IO_SPI_H__

extern uint32_t spi_conf(uint32_t speed_hz);
extern void spi_bulk_write(uint32_t size);
extern void spi_bulk_read(uint32_t size);

#endif /* __IO_SPI_H__ */
