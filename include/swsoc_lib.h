/* by Shuhei Ajimura */

#include <linux/ioctl.h>

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

// for SWSOC8 dev name is attatchced to each minor number
// #define DEVFILE "/dev/swsoc"

#define ADD_CM_REG	0x00000000
#define ADD_ST_REG	0x00000004
#define ADD_CK_REG	0x0000000C

#define ADD_RX_CSR	0x00000010
#define ADD_RX_DEBG	0x00000014

#define ADD_TX_CSR	0x00000020
#define ADD_TX_DEBG	0x00000024

#define ADD_RX_DATA 	0x00000000
#define ADD_TX_DATA	0x00000000

int sw_open(int);
void sw_close(int);
int sw_w(int, int, unsigned int, unsigned int);
int sw_r(int, int, unsigned int, unsigned int *);
int sw_bw(int, int, unsigned int *, unsigned int);
int sw_br(int, int, unsigned int *, unsigned int);
int sw_put_data0(int, int, unsigned int *, unsigned int);
int sw_put_data(int, int, unsigned int *, unsigned int);
int sw_get_data0(int, int, unsigned int *, unsigned int);
int sw_get_data(int, int, unsigned int *, unsigned int);
int sw_req(int, int, int, int, int, int, int, int, int);
int sw_rcv(int, int, unsigned int *, int *, int, int);
int sw_link_test(int, int);
int sw_link_check(int, int);
void sw_link_reset(int, int);
void sw_link_up(int, int);
void sw_link_down(int, int);
int sw_rx_status(int, int);
int sw_rx_flush(int, int);
int sw_wait_data(int, int);
void sw_print_status(int, int);
