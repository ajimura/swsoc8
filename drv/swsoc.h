/* by Shuhei Ajimura */

#include <linux/ioctl.h>

#define DRV_NAME "swsoc"
#define CSR_BASE 0xff280000
#define CSR_SPAN 0x0100
#define DATA_BASE 0xd0000000
#define DATA_SPAN 0x4000

//struct swio_mem {
//  unsigned int addr;
//  unsigned int pad1;
//  unsigned int port;
//  unsigned int pad2;
//  unsigned int val;
//  unsigned int pad3;
//  unsigned int *ptr;
//};

struct swio_mem {
  unsigned int *ptr;
  unsigned int port;
  unsigned int addr;
  unsigned int val; //size for mem access
  unsigned int tid;
  char cmd;
  char saddr;
  char daddr;
  char key; // status for reply
};

#define IOC_MAGIC 's'

#define SW_REG_READ  _IOR(IOC_MAGIC, 1, struct swio_mem)
#define SW_REG_WRITE _IOW(IOC_MAGIC, 2, struct swio_mem)
#define SW_MEM_READ  _IOR(IOC_MAGIC, 3, struct swio_mem)
#define SW_MEM_WRITE _IOW(IOC_MAGIC, 4, struct swio_mem)
#define SW_PCKT_READ  _IOR(IOC_MAGIC, 5, struct swio_mem)
#define SW_PCKT_WRITE _IOW(IOC_MAGIC, 6, struct swio_mem)
#define RMAP_REQ _IOW(IOC_MAGIC, 7, struct swio_mem)
#define RMAP_RCV _IOR(IOC_MAGIC, 8, struct swio_mem)
#define SW_TIME_MARK _IOW(IOC_MAGIC, 9, struct swio_mem)

#define ADD_CM_REG	0x00000000
#define ADD_ST_REG	0x00000004
#define ADD_CK_REG	0x00000008

#define ADD_RX_CSR	0x00000010
#define ADD_RX_DEBG	0x00000014

#define ADD_TX_CSR	0x00000020
#define ADD_TX_DEBG	0x00000024
