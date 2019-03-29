/* by Shuhei Ajimura */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/sched.h>
//#include <linux/jiffies.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/ioctl.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <asm/io.h>
#include "swsoc.h"

#define VERB 1
#define DevsNum 8

struct swsoc_devinfo {
  dev_t cdevno;
  struct cdev cdev;
};
struct swsoc_devinfo *swdevinfo = NULL;;

struct swsoc_dev {
  struct swsoc_devinfo *devinfo;
  void * __iomem csr_ptr;
  void * __iomem data_ptr;
  int major;
  int minor;
};

unsigned long jiffies_open;
int refcount[DevsNum];
static spinlock_t swsoc_spin_lock;

unsigned char rmap_calc_crc(void *,unsigned int );
int rmap_create_buffer(unsigned char, unsigned char, unsigned char, unsigned char, unsigned int, unsigned int, unsigned int, unsigned int);
unsigned char tx_buffer[1024];
unsigned char RM_CRCTbl [] = {
  0x00,0x91,0xe3,0x72,0x07,0x96,0xe4,0x75,  0x0e,0x9f,0xed,0x7c,0x09,0x98,0xea,0x7b,
  0x1c,0x8d,0xff,0x6e,0x1b,0x8a,0xf8,0x69,  0x12,0x83,0xf1,0x60,0x15,0x84,0xf6,0x67,
  0x38,0xa9,0xdb,0x4a,0x3f,0xae,0xdc,0x4d,  0x36,0xa7,0xd5,0x44,0x31,0xa0,0xd2,0x43,
  0x24,0xb5,0xc7,0x56,0x23,0xb2,0xc0,0x51,  0x2a,0xbb,0xc9,0x58,0x2d,0xbc,0xce,0x5f,
  0x70,0xe1,0x93,0x02,0x77,0xe6,0x94,0x05,  0x7e,0xef,0x9d,0x0c,0x79,0xe8,0x9a,0x0b,
  0x6c,0xfd,0x8f,0x1e,0x6b,0xfa,0x88,0x19,  0x62,0xf3,0x81,0x10,0x65,0xf4,0x86,0x17,
  0x48,0xd9,0xab,0x3a,0x4f,0xde,0xac,0x3d,  0x46,0xd7,0xa5,0x34,0x41,0xd0,0xa2,0x33,
  0x54,0xc5,0xb7,0x26,0x53,0xc2,0xb0,0x21,  0x5a,0xcb,0xb9,0x28,0x5d,0xcc,0xbe,0x2f,
  0xe0,0x71,0x03,0x92,0xe7,0x76,0x04,0x95,  0xee,0x7f,0x0d,0x9c,0xe9,0x78,0x0a,0x9b,
  0xfc,0x6d,0x1f,0x8e,0xfb,0x6a,0x18,0x89,  0xf2,0x63,0x11,0x80,0xf5,0x64,0x16,0x87,
  0xd8,0x49,0x3b,0xaa,0xdf,0x4e,0x3c,0xad,  0xd6,0x47,0x35,0xa4,0xd1,0x40,0x32,0xa3,
  0xc4,0x55,0x27,0xb6,0xc3,0x52,0x20,0xb1,  0xca,0x5b,0x29,0xb8,0xcd,0x5c,0x2e,0xbf,
  0x90,0x01,0x73,0xe2,0x97,0x06,0x74,0xe5,  0x9e,0x0f,0x7d,0xec,0x99,0x08,0x7a,0xeb,
  0x8c,0x1d,0x6f,0xfe,0x8b,0x1a,0x68,0xf9,  0x82,0x13,0x61,0xf0,0x85,0x14,0x66,0xf7,
  0xa8,0x39,0x4b,0xda,0xaf,0x3e,0x4c,0xdd,  0xa6,0x37,0x45,0xd4,0xa1,0x30,0x42,0xd3,
  0xb4,0x25,0x57,0xc6,0xb3,0x22,0x50,0xc1,  0xba,0x2b,0x59,0xc8,0xbd,0x2c,0x5e,0xcf
};

/* SW_OPEN */
static int swsoc_open(struct inode *inode, struct file *file)
{
  struct swsoc_dev *swsoc;
  unsigned int *csr;
  unsigned int *data;
  int major,minor;

  spin_lock(&swsoc_spin_lock);

  major=imajor(inode);
  minor=iminor(inode);

  if (refcount[minor]!=0){
    printk(KERN_DEBUG DRV_NAME "%d:%d is already opened\n",major,minor);
    spin_unlock(&swsoc_spin_lock);
    return -EBUSY;
  }else{
    refcount[minor]=1;
  }

  printk(KERN_DEBUG DRV_NAME "_open(): %d:%d %lu/%d\n",major,minor,jiffies,HZ);
  jiffies_open=jiffies;

  swsoc = (struct swsoc_dev *)kzalloc(sizeof(struct swsoc_dev), GFP_KERNEL);
  if (!swsoc) {
    printk(KERN_DEBUG "Could not kzalloc() allocate memory.\n");
    goto fail_kzalloc;
  }
  file->private_data = swsoc;
  swsoc->devinfo=swdevinfo;

  csr=ioremap_nocache(CSR_BASE+CSR_SPAN*minor,CSR_SPAN);
  swsoc->csr_ptr=csr;

  data=ioremap_nocache(DATA_BASE+DATA_SPAN*minor,DATA_SPAN);
  swsoc->data_ptr=data;

  //  printk(KERN_DEBUG "SpW %08x\n",*(csr+1));
  //  printk(KERN_DEBUG "SpW RX %08x %08x\n",*(csr+4), *(csr+5));
  //  printk(KERN_DEBUG "SpW TX %08x %08x\n",*(csr+8), *(csr+9));

  swsoc->major=major;
  swsoc->minor=minor;

  spin_unlock(&swsoc_spin_lock);

  return 0;

 fail_kzalloc:
  return -1;
}

/* SW_CLOSE */
static int swsoc_close(struct inode *inode, struct file *file)
{
  struct swsoc_dev *swsoc = file->private_data;
  int minor;

  spin_lock(&swsoc_spin_lock);

  minor=iminor(inode);

  printk(KERN_DEBUG DRV_NAME "_close(): %lu %lu/%d\n",
	 jiffies,jiffies-jiffies_open,HZ);
  iounmap(swsoc->csr_ptr);
  iounmap(swsoc->data_ptr);
  kfree(swsoc);

  refcount[minor]=0;

  spin_unlock(&swsoc_spin_lock);

  return 0;
}

static long swsoc_ioctl(
  struct file *file,
  unsigned int cmd,
  unsigned long arg)
{
  struct swsoc_dev *swsoc = file->private_data;
  void * __iomem csrtop = swsoc->csr_ptr;
  void * __iomem datatop = swsoc->data_ptr;

  struct swio_mem cmd_mem;
  unsigned int address;
  int real_len;
  unsigned int data;
  unsigned int req_size;
  int get_size;
  int put_size;
  int max_size;
  unsigned char buftop[12];
  unsigned int *buftop32;
  unsigned int ret_tid;
  unsigned char ret_status;
  int i;
  
  int retval = 0;

  if (!access_ok(VERIFY_READ, (void __user *)arg,_IOC_SIZE(cmd))){
    retval=-EFAULT; goto done; }
  if (copy_from_user(&cmd_mem, (int __user *)arg,sizeof(cmd_mem))){
    retval = -EFAULT; goto done; }

  switch(cmd){

  case SW_REG_READ:
    address=cmd_mem.addr & 0x0000003f;
    cmd_mem.val=ioread32(csrtop+address);
    rmb();
    if (copy_to_user((int __user *)arg, &cmd_mem, sizeof(cmd_mem))){
      retval = -EFAULT; goto done; }
#if VERB
    printk(KERN_DEBUG "(%d)IOR_cmd.val/addr %08x %08x(%s)\n",swsoc->minor,cmd_mem.val,address, __func__);
#endif
    break;

  case SW_REG_WRITE:
    address=cmd_mem.addr & 0x0000003f;
    iowrite32(cmd_mem.val,csrtop+address);
    wmb();
#if VERB
    printk(KERN_DEBUG "(%d)IOW_cmd.val/addr %08x %08x(%s)\n",swsoc->minor,cmd_mem.val,address, __func__);
#endif
    break;

  case SW_MEM_READ:
    if (cmd_mem.val>DATA_SPAN-1) real_len=DATA_SPAN-1;
    else real_len=cmd_mem.val;
    if (real_len>0){
      if (!access_ok(VERIFY_WRITE, (void __user *)cmd_mem.ptr,real_len)){
	retval = -EFAULT; goto done; }
      if (copy_to_user((int __user *)cmd_mem.ptr, datatop, real_len)){
	retval = -EFAULT; goto done; }
      rmb();
#if VERB
      printk(KERN_DEBUG "(%d)IORB_cmd.size %x (%s)\n",swsoc->minor,real_len, __func__);
#endif
    }
    break;

  case SW_MEM_WRITE:
    if (cmd_mem.val>DATA_SPAN-1) real_len=DATA_SPAN-1;
    else real_len=cmd_mem.val;
    if (real_len>0){
      if (!access_ok(VERIFY_READ, (void __user *)cmd_mem.ptr,real_len)){
	retval = -EFAULT; goto done; }
      if (copy_from_user(datatop, (void __user *)cmd_mem.ptr,real_len)){
	retval = -EFAULT; goto done; }
      wmb();
#if VERB
      printk(KERN_DEBUG "(%d)IOWB_cmd.size %x (%s)\n",swsoc->minor,real_len, __func__);
#endif
    }
    break;

  case SW_PCKT_READ:
    data=ioread32(csrtop+ADD_RX_CSR); //RX CSR
    if ((data&0x80000000)==0) {retval=-1; goto done;}
    if ((data&0x00400000)==0) {retval=-1; goto done;}
    real_len=data&0x0fffff;
    get_size=(real_len+3)/4*4;
    if (get_size>cmd_mem.val) get_size=cmd_mem.val;
    if (!access_ok(VERIFY_WRITE, (void __user *)cmd_mem.ptr,get_size)){
      retval = -EFAULT; goto done; }
    if (copy_to_user((int __user *)cmd_mem.ptr, datatop, get_size)){
      retval = -EFAULT; goto done; }
    iowrite32(0,csrtop+ADD_RX_CSR);
    //real_len to user
    cmd_mem.val=real_len;
    if (copy_to_user((int __user *)arg, &cmd_mem, sizeof(cmd_mem))){
      retval = -EFAULT; goto done; }
    rmb();
#if VERB
    printk(KERN_DEBUG "(%d)IORMR_cmd.size %x (%s)\n",swsoc->minor,get_size, __func__);
#endif
    break;
    
  case SW_PCKT_WRITE:
    data=ioread32(csrtop+ADD_TX_CSR); //RX CSR
    if ((data&0x80000000)!=0) {retval=-1; goto done;}
    max_size=data&0x000fffff;
    if (cmd_mem.val>max_size) put_size=max_size; else put_size=cmd_mem.val;
    real_len=(put_size+3)/4*4;
    if (!access_ok(VERIFY_READ, (void __user *)cmd_mem.ptr,real_len)){
      retval = -EFAULT; goto done; }
    if (copy_from_user(datatop, (void __user *)cmd_mem.ptr,real_len)){
      retval = -EFAULT; goto done; }
    wmb();
    iowrite32(0x80400000+cmd_mem.val,csrtop+ADD_TX_CSR);
    //put_size to user
    cmd_mem.val=put_size;
    if (copy_to_user((int __user *)arg, &cmd_mem, sizeof(cmd_mem))){
      retval = -EFAULT; goto done; }
#if VERB
    printk(KERN_DEBUG "(%d)IORMW_cmd.size %x (%s)\n",swsoc->minor,real_len, __func__);
#endif
    break;

  case RMAP_REQ:
    data=ioread32(csrtop+ADD_TX_CSR); //RX CSR
    if ((data&0x80000000)!=0) {retval=-1; goto done;}
    if (cmd_mem.val>(data&0x0000ffff)) req_size=data&0x0000ffff; else req_size=cmd_mem.val;
    put_size=rmap_create_buffer(cmd_mem.cmd,cmd_mem.saddr,cmd_mem.daddr,
				cmd_mem.key,cmd_mem.tid,cmd_mem.addr,req_size,cmd_mem.val);
    real_len=(put_size+3)/4*4;
#if VERB
    printk(KERN_DEBUG "(%d)%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	   swsoc->minor,
	   tx_buffer[0],tx_buffer[1],tx_buffer[2],tx_buffer[3],tx_buffer[4],tx_buffer[5],
	   tx_buffer[6],tx_buffer[7],tx_buffer[8],tx_buffer[9],tx_buffer[10],tx_buffer[11],
	   tx_buffer[12],tx_buffer[13],tx_buffer[14],tx_buffer[15]);
#endif
    memcpy_toio(datatop,tx_buffer,real_len);
    wmb();
    iowrite32(0x80400000+put_size,csrtop+ADD_TX_CSR);
    cmd_mem.val=put_size;
    if (copy_to_user((int __user *)arg, &cmd_mem, sizeof(cmd_mem))){
      retval = -EFAULT; goto done; }
#if VERB
    printk(KERN_DEBUG "(%d)IOREQ_cmd.size %x (%s)\n",swsoc->minor,real_len, __func__);
#endif
    break;

  case RMAP_RCV:
    data=ioread32(csrtop+ADD_RX_CSR); //RX CSR
    if ((data&0x80000000)==0) {retval=-1; goto done;}
    if ((data&0x00400000)==0) {retval=-1; goto done;}
    max_size=cmd_mem.val;
    memcpy_fromio(buftop,datatop,12);
#if VERB
    printk(KERN_DEBUG "(%d)%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
	   swsoc->minor,
	   buftop[0],buftop[1],buftop[2],buftop[3],buftop[4],buftop[5],
	   buftop[6],buftop[7],buftop[8],buftop[9],buftop[10],buftop[11]);
#endif
    cmd_mem.key=buftop[3];
    ret_tid=buftop[5]*0x100+buftop[6];
    cmd_mem.val=buftop[8]*0x10000+buftop[9]*0x100+buftop[10];
    if (copy_to_user((int __user *)arg, &cmd_mem, sizeof(cmd_mem))){
      retval = -EFAULT; goto done; }
    if (cmd_mem.tid!=0 && cmd_mem.tid!=ret_tid) {retval=-EFAULT;goto done;}
    if (cmd_mem.key!=0) {
      retval=-EFAULT;goto done;}
    get_size=(cmd_mem.val+3)/4*4;
    if (get_size>max_size) get_size=max_size;
    if (!access_ok(VERIFY_WRITE, (void __user *)cmd_mem.ptr,get_size)){
      retval = -EFAULT; goto done; }
    if (copy_to_user((int __user *)cmd_mem.ptr, datatop+12, get_size)){
      retval = -EFAULT; goto done; }
    rmb();
    iowrite32(0,csrtop+ADD_RX_CSR);
#if VERB
    printk(KERN_DEBUG "(%d)IORCV_cmd.size %x (%s)\n",swsoc->minor,get_size, __func__);
#endif
    break;

  case SW_TIME_MARK:
    printk(KERN_DEBUG "(%d)TimeMark(%4d): %lu/%d\n",swsoc->minor,cmd_mem.val,jiffies,HZ);
    break;

  default:
    retval = -ENOTTY;
    goto done;
    break;
  }

  done:
    return(retval);
}

/* character device file operations */
static struct file_operations swsoc_fops = {
  .owner = THIS_MODULE,
  .open = swsoc_open,
  .release = swsoc_close,
  .unlocked_ioctl = swsoc_ioctl,
  //  .read = swsoc_read,
  //  .write = swsoc_write,
};

/* swsoc_init() */
static int __init swsoc_init(void)
{
  int i;
  int rc = 0;
  dev_t devno = MKDEV(0,0);

  printk(KERN_DEBUG DRV_NAME "_init(): %lu/%d\n",jiffies,HZ);
  printk(KERN_DEBUG "PAGESIZE=%ld\n", PAGE_SIZE);
  printk(KERN_DEBUG "HZ=%d\n", HZ);

  for(i=0;i<DevsNum;i++) refcount[i]=0;
  //  refcount[0]=0;  refcount[1]=0;  refcount[2]=0;  refcount[3]=0;
  spin_lock_init(&swsoc_spin_lock);

  swdevinfo = (struct swsoc_devinfo *)kmalloc(sizeof(struct swsoc_devinfo), GFP_KERNEL);
  if (!swdevinfo) {
    printk(KERN_DEBUG "Could not kzalloc() allocate memory.\n");
    goto fail_kzalloc;
  }
  swdevinfo->cdevno=devno;

  rc = alloc_chrdev_region(&(swdevinfo->cdevno), 0/*requested minor base*/, DevsNum/*count*/, DRV_NAME);
  if (rc < 0) {
    printk("alloc_chrdev_region() = %d\n", rc);
    goto fail_alloc;
  }
  cdev_init(&swdevinfo->cdev, &swsoc_fops);
  swdevinfo->cdev.owner = THIS_MODULE;
  rc = cdev_add(&swdevinfo->cdev, swdevinfo->cdevno, DevsNum/*count*/);
  if (rc < 0) {
    printk("cdev_add() = %d\n", rc);
    goto fail_add;
  }
  printk(KERN_DEBUG "swsoc = %d:%d\n", MAJOR(swdevinfo->cdevno), MINOR(swdevinfo->cdevno));
  return 0;
 fail_add:
  /* free the dynamically allocated character device node */
  unregister_chrdev_region(swdevinfo->cdevno, DevsNum/*count*/);
 fail_alloc:
 fail_kzalloc:
  return -1;
}

/* swsoc_exit() */
static void __exit swsoc_exit(void)
{
  printk(KERN_DEBUG DRV_NAME "_exit(): %lu/%d\n",jiffies,HZ);
  /* remove the character device */
  cdev_del(&swdevinfo->cdev);
  /* free the dynamically allocated character device node */
  unregister_chrdev_region(swdevinfo->cdevno, DevsNum/*count*/);
  /* free mem */
  kfree(swdevinfo);
}

MODULE_LICENSE("Dual BSD/GPL");

module_init(swsoc_init);
module_exit(swsoc_exit);

unsigned char rmap_calc_crc(void *buf,unsigned int len){
  unsigned int i;
  unsigned char crc;
  unsigned char *ptr = (unsigned char *)buf;

  /* initial CRC */
  crc=0;

  /* for each bute */
  for(i=0;i<len;i++){
    crc=RM_CRCTbl[crc ^ *ptr++];
  }
  return crc;
}

int rmap_create_buffer(unsigned char command, unsigned char saddr, unsigned char daddr, unsigned char key,
		       unsigned int tid, unsigned int addr, unsigned int size, unsigned int data){// for "put" size is put_data
  int i;
  int in_length;
  unsigned int header_size;
  unsigned char *ptr, *dptr, *crc_start;
  
  ptr = tx_buffer;
  header_size=15;
  crc_start = ptr;
  *ptr++ = daddr;		//Destination Logical Address
  *ptr++ = 0x01;		//Protocol ID
  *ptr++ = command;		//Packet Type
  *ptr++ = key;			//Destination Key
  *ptr++ = saddr;		//Source Logical Address
  *ptr++ = (tid>>8)&0xff;	//Transaction ID(MS)
  *ptr++ = (tid   )&0xff;	//Transaction ID(LS)
  *ptr++ = 0x00;		//Extended Address
  *ptr++ = (addr>>24)&0xff;	//Address
  *ptr++ = (addr>>16)&0xff;	//Address
  *ptr++ = (addr>> 8)&0xff;	//Address
  *ptr++ = (addr    )&0xff;	//Address
  if ((command & 0x20)>0){ // if RM_PCKT_WRT
    *ptr++=0x00;		//Data Size
    *ptr++=0x00;		//Data Size
    *ptr++=0x04;		//Data Size
    *ptr++ = rmap_calc_crc(crc_start,header_size);	//Header CRC
    dptr=ptr;
    *ptr++ = (data    )&0xff;		//Data
    *ptr++ = (data>> 8)&0xff;		//Data
    *ptr++ = (data>>16)&0xff;		//Data
    *ptr++ = (data>>24)&0xff;		//Data
    *ptr++ = rmap_calc_crc(dptr,4);	//Data CRC
    return header_size + 6;
  }else{
    *ptr++ = (size>>16)&0xff;		//Data Size
    *ptr++ = (size>> 8)&0xff;		//Data Size
    *ptr++ = (size    )&0xff;		//Data Size
    *ptr++ = rmap_calc_crc(crc_start,header_size);	//Header CRC
    return header_size + 1;
  }
}
