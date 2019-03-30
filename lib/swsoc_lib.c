/* by Shuhei Ajimura */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "swsoc_lib.h"

char DevName[8][16]={"/dev/swsoc0","/dev/swsoc1","/dev/swsoc2","/dev/swsoc3",
		     "/dev/swsoc4","/dev/swsoc5","/dev/swsoc6","/dev/swsoc73"};

int sw_open(int port)
{
  int sw_fd;
  
  if (port<0 || port>8) return(-1);

  if((sw_fd = open(DevName[port], O_RDWR)) < 0){
    printf("error: can not open device file.\n");    
  };
  return(sw_fd);
}

void sw_close(int sw_fd)
{
  close(sw_fd);
}

int sw_w(int sw_fd, int port, unsigned int address, unsigned int value)
{
  struct swio_mem swio;
  int ret;

  if(sw_fd < 0) return(-1);
  swio.addr = address;
  swio.port = port;
  swio.val = value;
  ret=ioctl(sw_fd,SW_REG_WRITE,&swio);
  return (ret);
}

int sw_r(int sw_fd, int port, unsigned int address, unsigned int *value)
{
  struct swio_mem swio;
  int ret;

  if(sw_fd < 0) return(-1);
  swio.addr = address;
  swio.port = port;
  ret=ioctl(sw_fd,SW_REG_READ,&swio);
  *value=swio.val;
  return(ret);
}

int sw_bw(int sw_fd, int port, unsigned int *ptr, unsigned int size)
{
  struct swio_mem swio;

  if(sw_fd < 0) return(-1);
  if (port<0||port>7) return(-1);
  swio.port = port;
  swio.val = size;
  swio.ptr = ptr;
  return (ioctl(sw_fd,SW_MEM_WRITE,&swio));
}

int sw_br(int sw_fd, int port, unsigned int *ptr, unsigned int size)
{
  struct swio_mem swio;

  if(sw_fd < 0) return(-1);
  if (port<0||port>7) return(-1);
  swio.port = port;
  swio.val = size;
  swio.ptr = ptr;
  return (ioctl(sw_fd,SW_MEM_READ,&swio));
}

int sw_put_data0(int sw_fd, int port, unsigned int *data, unsigned int size)
{
  int i;
  unsigned int st,max_size,i_size,put_size;

  i_size=size;

  sw_r(sw_fd,port,ADD_TX_CSR,&st);
  if ((st&0x80000000)!=0) return -1;

  max_size = st&0x000FFFFF; /*16bit*/
  if (i_size > max_size) i_size=max_size;
  if (i_size%4==0) put_size=i_size;
  else             put_size=(i_size/4+1)*4;
  i = sw_bw(sw_fd, port, data, put_size);
  if (i){
    printf("Error in block write %X %X\n",i,put_size);
    return -1;
  }
  i = sw_w(sw_fd, port, ADD_TX_CSR,0x80400000+i_size);
  if (i){
    printf("Error in issue GO %X\n",i);
    return -1;
  }
  return put_size; 
}

int sw_put_data(int sw_fd, int port, unsigned int *data, unsigned int size)
{
  struct swio_mem swio;
  int ret;

  swio.val=size;
  swio.ptr=data;
  if ((ret=ioctl(sw_fd,SW_PCKT_WRITE,&swio))<0){
    printf("Error in block write %X %X\n",ret,swio.val);
    return -1;
  }
  return swio.val;
}

int sw_get_data0(int sw_fd, int port, unsigned int *data, unsigned int size)
{
  int i;
  unsigned int st, j_size, get_size;
  
  sw_r(sw_fd, port, ADD_RX_CSR,&st);
  if ((st&0x80000000)==0) return -1;
  if ((st&0x00400000)==0) return -2;

  j_size = st & 0x0FFFFF;
  if (j_size > size) j_size = size;
  if (j_size%4==0) get_size=j_size;
  else             get_size=(j_size/4+1)*4;
  i = sw_br(sw_fd,port,data,get_size);
  if (i){
    printf("Error in block read %X %X\n",i,get_size);
    return -1;
  }
  i = sw_w(sw_fd,port,ADD_RX_CSR,0);
  if (i){
    printf("Error in flush buffer %X\n",i);
    return -1;
  }
  return j_size;
}

int sw_get_data(int sw_fd, int port, unsigned int *data, unsigned int size)
{
  struct swio_mem swio;
  int ret;

  swio.val=size;
  swio.ptr=data;
  if ((ret=ioctl(sw_fd,SW_PCKT_READ,&swio))<0){
    printf("Error in block read %X %X\n",ret,swio.val);
    return -1;
  }
  return swio.val;
}

int sw_req(int sw_fd, int port, int cmd, int saddr, int daddr, int key, int tid, int addr, int size){
  struct swio_mem swio;
  int ret;

  swio.cmd=cmd;
  swio.saddr=saddr;
  swio.daddr=daddr;
  swio.key=key;
  swio.tid=tid;
  swio.addr=addr;
  swio.val=size;
  if ((ret=ioctl(sw_fd,RMAP_REQ,&swio))<0){
    printf("Error in block read %d %X\n",ret,swio.val);
    return -1;
  }
  //  return swio.val;
  return 0;
}  

int sw_rcv(int sw_fd, int port, unsigned int *data, int *status, int tid, int size){
  struct swio_mem swio;
  int ret;

  swio.val=size;
  swio.ptr=data;
  swio.tid=tid;
  if ((ret=ioctl(sw_fd,RMAP_RCV,&swio))<0){
    *status=swio.key;
    printf("Error in block read %d %X\n",ret,swio.key);
    return -1;
  }
  return swio.val;
}  

int sw_link_test(int sw_fd, int port) {
  unsigned int data;
  sw_r(sw_fd,port, ADD_RX_CSR,&data);
  if ((data&0x40000000)==0){
    sw_r(sw_fd,port,ADD_RX_CSR,&data);
    if ((data&0x40000000)==0){
      return -1;
    }
  }
  return 0;
}

int sw_link_check(int sw_fd, int port){
  unsigned int st,tx,rx;
  sw_r(sw_fd,port,ADD_ST_REG,&st);
  sw_r(sw_fd,port,ADD_TX_CSR,&tx);
  sw_r(sw_fd,port,ADD_RX_CSR,&rx);
  if ((st&0x80000000)==0 || (tx&0x40000000)==0 ||(rx&0x40000000)==0)  //check link stats
    return -1;
  if ((tx&0x80000000)>0 || (rx&0x80000000)>0)  //check buffer
    return -1;
  return 0;
}

void sw_link_reset(int sw_fd, int port) {
  sw_w(sw_fd,port,ADD_CM_REG,1);
  usleep(10000);
  sw_w(sw_fd,port,ADD_CM_REG,0);
  usleep(10000);
}

void sw_link_up(int sw_fd, int port) {
  sw_w(sw_fd,port,ADD_CM_REG,0);
  usleep(10000);
}

void sw_link_down(int sw_fd, int port) {
  sw_w(sw_fd,port,ADD_CM_REG,1);
  usleep(10000);
}

int sw_rx_status(int sw_fd, int port) {
  unsigned int data;
  sw_r(sw_fd,port,ADD_RX_CSR,&data);
  if (data&0x80000000) return data&0xfffff;
  else return -1;
}

int sw_rx_flush(int sw_fd, int port) {
  unsigned int st;
  int i;
  for(i=0;i<10;i++) {
    sw_r(sw_fd, port, ADD_RX_CSR,&st);
    if ((st&0x80000000)==0) return 0;
    sw_w(sw_fd,port,ADD_RX_CSR,0);
    usleep(1);
  }
  return -1;
}

int sw_wait_data(int sw_fd, int port)
{
  int st;
  int i;

  st=0;i=0;
  for(i=0;i<4000;i++){
    if ((st=sw_rx_status(sw_fd,port))>0) return st%0xfffff;
  }
  return(-1);
}

void sw_print_status(int sw_fd, int port) {
  unsigned int data;

  sw_r(sw_fd,port,ADD_ST_REG,&data);
  printf("PORT%1d SpW status=%08x  ",port,data);
  sw_r(sw_fd,port,ADD_CK_REG,&data);
  printf("SpW debug=%08x\n",data);

  sw_r(sw_fd,port,ADD_TX_CSR,&data);
  printf("PORT%1d Tx status=%08x   ",port,data);
  sw_r(sw_fd,port,ADD_TX_DEBG,&data);
  printf("Tx debug=%08x\n",data);

  sw_r(sw_fd,port,ADD_RX_CSR,&data);
  printf("PORT%1d Rx status=%08x   ",port,data);
  sw_r(sw_fd,port,ADD_RX_DEBG,&data);
  printf("Rx debug=%08x\n",data);

}
