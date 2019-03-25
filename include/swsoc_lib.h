/* by Shuhei Ajimura */

#include <linux/ioctl.h>

//struct swio_mem {
//  unsigned int addr;
//  unsigned int pad1;
//  unsigned int port;
//  unsigned int pad2;
//  unsigned int val;
//  unsigned int pad3;
//  unsigned int *ptr;
//};
/* changed on May/30 midnight
  REG_WR  : addr=reg address, port=port, val=data, ptr=N.A.
  REG_RD  : addr=reg address, port=port, val=data, ptr=N.A.
  MEM_WR  : addr=NA, port=port, val=size, ptr=data buffer
  MEM_RD  : addr=NA, port=port, val=size, ptr=data buffer
  TIME_MARK  : addr=NA, port=NA, val=marker value, ptr=NA
*/
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

struct swio_dma {
  unsigned int id;
  unsigned int pad1;
  unsigned int length;
  unsigned int pad2;
  unsigned int port;
  unsigned int pad3;
  unsigned int *data;
};

struct swp_dma_db{
  unsigned int ndesc;
  unsigned int pad1;
  unsigned int length[8];
  unsigned int port[8];
  unsigned int *ptr[8];
};

#define IOC_MAGIC 's'

#define SW_REG_READ  _IOR(IOC_MAGIC, 1, struct swio_mem)
#define SW_REG_WRITE _IOW(IOC_MAGIC, 2, struct swio_mem)
#define SW_MEM_READ  _IOR(IOC_MAGIC, 3, struct swio_mem)
#define SW_MEM_WRITE _IOW(IOC_MAGIC, 4, struct swio_mem)
#define SW_TIME_MARK _IOW(IOC_MAGIC, 5, struct swio_mem)
//#define RMAP_MEM_READ  _IOR(IOC_MAGIC, 6, struct swio_mem)
//#define RMAP_MEM_WRITE _IOW(IOC_MAGIC, 7, struct swio_mem)
#define SW_PCKT_READ  _IOR(IOC_MAGIC, 6, struct swio_mem)
#define SW_PCKT_WRITE _IOW(IOC_MAGIC, 7, struct swio_mem)
#define RMAP_REQ _IOW(IOC_MAGIC, 8, struct swio_mem)
#define RMAP_RCV _IOR(IOC_MAGIC, 9, struct swio_mem)

// for SWSOC8 dev name is attatchced to each minor number
// #define DEVFILE "/dev/swsoc"

#define ADD_CM_REG	0x00000000
#define ADD_ST_REG	0x00000004
#define ADD_CK_REG	0x00000008

#define ADD_RX_CSR	0x00000010
#define ADD_RX_DEBG	0x00000014

#define ADD_TX_CSR	0x00000020
#define ADD_TX_DEBG	0x00000024

#define ADD_RX_DATA 	0x00000000
#define ADD_TX_DATA	0x00000000

int sw_open(void);
void sw_close(int);
int sw_w(int, int, unsigned int, unsigned int);
int sw_r(int, int, unsigned int, unsigned int *);
int sw_bw(int, int, unsigned int *, unsigned int);
int sw_br(int, int, unsigned int *, unsigned int);
int sw_put_data(int, int, unsigned int *, unsigned int);
int sw_get_data(int, int, unsigned int *, unsigned int);
int sw_link_test(int, int);
int sw_link_check(int, int);
void sw_link_reset(int, int);
void sw_link_up(int, int);
void sw_link_down(int, int);
int sw_rx_status(int, int);
int sw_rx_flush(int, int);
int sw_wait_data(int, int);
void sw_print_status(int, int);
