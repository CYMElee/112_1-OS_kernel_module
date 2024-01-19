#include <linux/atomic.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/module.h> //need for all modules
#include <linux/slab.h> //allocate kernel memory
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/file.h>
#include <linux/cpumask.h>
#include <linux/string.h>
#include <linux/utsname.h>
//====== parameter_for_buffer====//
char *kfetch_buf;
char *kfetch_buf_temp;
char *tmp;

char *mem_buf;
char *thr_buf;

long mem_kb_total = 0;
long mem_kb_free = 0;
//=====parameter_for_copy======//
int current_row = 1;

int kftech_buf_index = 0;
int notation = 0;

//=====struct for character device=====//
static dev_t kfetch_number;
static struct cdev kfetch_cdev;
struct device *kfetch_device;
static struct class *kfetch_class;

//=====struct for proc ======//
struct file *host_file;
struct file *kernel_file;
struct file *cpu_file;
struct file *cpus_file;
struct file *mem_file;
struct file *threads_file;
struct file *uptime_file;
// ======struct for host name and kernel release ====//
struct new_utsname *uts;
//====== number of thread ====//
struct task_struct *task;
//===== offect=====//
loff_t cpu_info_offset =0;
loff_t mem_info_offset =0;
loff_t uptime_info_offset = 0;
loff_t threads_info_offset =0;

//===== target row=====//
int cpu_info_target_row = 3;

//==== search specific ====//
int kfetch_buf_temp_index = 0;
int mam_buf_index = 0;
int thr_buf_index = 0;

// ==== uptime ====//
int uptime = 0;

// ===process ===///
long total_threads= 0;



//======= function declear for kfetch device =====//
static int kfetch_open(struct inode *, struct file *);
static int kfetch_release(struct inode *, struct file *);
static ssize_t kfetch_read(struct file *,char __user *, size_t , loff_t *);
static ssize_t kfetch_write(struct file *, const char __user *, size_t, loff_t *);



//====== function or variable decalear for proc ======//
unsigned int online_cpus = 0;
unsigned int possible_cpus = 0;


// ====== variable of mask ====//
int mask_info = 0;
int mask_info_prev =0;
int total_line =8;
int current_line = 0;

const static struct file_operations kfetch_fops = {
    .owner   = THIS_MODULE,
    .read    = kfetch_read,
    .write   = kfetch_write,
    .open    = kfetch_open,
    .release = kfetch_release,
};

//============ proc ===========//









// ======= initialize the kernel mode ========//
static int __init kfetch_init(void) 
{ 
   cdev_init(&kfetch_cdev, &kfetch_fops);
   //-------- register_device--------//
   if(alloc_chrdev_region(&kfetch_number,0,1,"kfetch")<0){
   	pr_err("Failed to allocate device numbers\n");
   	return -ENOENT;
   }
   
   
   //-------- initialize the data structure struct cdev for our char device and associate it with the device numbers--------//
   
   if(cdev_add(&kfetch_cdev, kfetch_number, 1)!=0)
   {
   pr_info("fail to register character device");
   }
   
   
   kfetch_class = class_create(THIS_MODULE, "kfetch_class");
   kfetch_device = device_create(kfetch_class, NULL, kfetch_number, NULL, "kfetch");
   
  
   
   
   
    //------- send penguin to kfetch_buf--------//
    
    
    pr_info("init_the_module\n"); 
    return 0; 
} 



// ======= exit the kernel mode ========//
static void __exit kfetch_exit(void) 
{ 	device_destroy(kfetch_class, kfetch_number);
	class_destroy(kfetch_class);
	cdev_del(&kfetch_cdev);
	unregister_chrdev_region(kfetch_number, 1);
	
	
	
	
	pr_info("exit_module\n");
	
} 


// ========open character device ========//
static int kfetch_open(struct inode *inode, struct file *file) {

	
	
	//-------- allocate the memory on kernel space --------//
    	kfetch_buf = kmalloc(1024, GFP_KERNEL);
    	tmp = kmalloc(1, GFP_KERNEL);
    	kfetch_buf_temp = kmalloc(1024, GFP_KERNEL);
    	mem_buf = kmalloc(512, GFP_KERNEL);
    	thr_buf = kmalloc(100, GFP_KERNEL);
    	//===== get the uts_namespace infromation ====//
    	uts = init_utsname();
    	// ==== init mask =====//
    	mask_info = 0;
    	// ===== init offset ====//
    	cpu_info_offset = 0;
    	mem_info_offset = 0;
    	uptime_info_offset = 0;
    	threads_info_offset = 0;
    	// ==== init buffer index ===//
    	current_row = 1;
    	kftech_buf_index = 0;
    	mam_buf_index = 0;
    	thr_buf_index = 0;
    	notation = 0;
    	total_threads = 0;
    	current_line = 0;
    	//====init variable ===//
    	mem_kb_total =0;
    	mem_kb_free = 0;
    	
    	uptime = 0;
        return 0;
}





static int kfetch_release(struct inode *inode, struct file *file) {
    kfree(kfetch_buf);
    kfree(kfetch_buf_temp);
    kfree(tmp);
    kfree(mem_buf);
    kfree(thr_buf);	
    
    return 0;
}





static ssize_t kfetch_read(struct file *filp,char __user *buffer, size_t length, loff_t *offset)
{
   if((mask_info == 0) && (mask_info_prev ==0))
   {
    mask_info  = (1 << 6) - 1;
    }
    
   if((mask_info == 0) && (mask_info_prev !=0))
   {
    mask_info =mask_info_prev ;
   }
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\n");
    //=======write penguin to buffer =======//
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"        .-.        \n"
        "       (.. |       \n"
        "       <>  |       \n"
        "      / --- \\      \n"
        "     ( |   | |     \n"
        "   |\\\\_)___/\\)/\\   \n"
        "  <__)------(__/   ");
    /* setting the information mask */
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "\033[1A""\033[1A""\033[1A""\033[1A""\033[1A""\033[1A""\033[1A");
    kfetch_buf[kftech_buf_index++] = '\r';
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    current_line++;
    // ======== host-name =======//
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "%s", uts->nodename);
    kfetch_buf[kftech_buf_index++] = '\n';
    //========add the notation '-'//
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    while(strlen(uts->nodename)-notation>0)
    {
    kfetch_buf[kftech_buf_index++] = '-';
    notation++;
    }
    kfetch_buf[kftech_buf_index++] = '\n';
    current_line++;
   // =======identify the mask ====// 		
    	if((mask_info >> 0 ) & 1){
    	current_line++;
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "Kernel:   %s", uts->release);
    	kfetch_buf[kftech_buf_index++] = '\n';
    	
    
    	}
    	
    	if((mask_info >> 2 ) & 1){
    	current_line++;
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    	kfetch_buf_temp_index = 0;
    	//memset(kfetch_buf_temp, 0, sizeof(kfetch_buf_temp));
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"CPU:      ");
    	cpus_file = filp_open("/proc/cpuinfo", O_RDONLY,0);
    	
    	while (current_row < cpu_info_target_row) {
    		kernel_read(cpus_file,tmp,1,&cpu_info_offset);
    		if (tmp[0] == '\n') {
        	current_row++;
        	tmp[0] = 0;
   	 	}
		tmp[0] = 0;
    		cpu_info_offset++;
	}
    	cpu_info_offset += 12;
    	kernel_read(cpus_file, kfetch_buf_temp, 50 , &cpu_info_offset);
    	while(kfetch_buf_temp[kfetch_buf_temp_index]!='\n')
    	{
    	kfetch_buf[kftech_buf_index] = kfetch_buf_temp[kfetch_buf_temp_index];
    	kftech_buf_index++;
    	kfetch_buf_temp_index++;
    	}
    	kfetch_buf[kftech_buf_index++] = '\n';
    	filp_close(cpus_file, NULL);
    	}
    	
    	if((mask_info >> 1 ) & 1){
    	current_line++;
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    	
    	online_cpus = num_online_cpus();
    	possible_cpus = num_possible_cpus();

	
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "CPUs:     %d", online_cpus);
    	kfetch_buf[kftech_buf_index++] = '/';
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "%d", possible_cpus);
    	kfetch_buf[kftech_buf_index++] = '\n';	
    
    	}
    	
    	
    	if((mask_info >> 3 ) & 1){
    	current_line++;
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    	kfetch_buf_temp_index = 0;
    	mam_buf_index = 0;
    	memset(kfetch_buf_temp, 0, 1024);
    	memset(mem_buf,0,512);
    	mem_info_offset = 12;
    	
    	mem_file = filp_open("/proc/meminfo", O_RDONLY,0);
    	kernel_read(mem_file, kfetch_buf_temp ,70, &mem_info_offset);
    	while(kfetch_buf_temp[kfetch_buf_temp_index]==' ')
    	{
    		kfetch_buf_temp_index++;
    		
    	}
    	
    	while(kfetch_buf_temp[kfetch_buf_temp_index]!=' ')
    	{
    		mem_buf[mam_buf_index] = kfetch_buf_temp[kfetch_buf_temp_index];
    	       
    		kfetch_buf_temp_index++;
    		mam_buf_index++;
    	}
    	mem_buf[mam_buf_index] = '\0';
    	
    	if(kstrtol( mem_buf, 10, &mem_kb_total) == 0)
    	{
    	}
    	mem_kb_total = mem_kb_total/1024;
    	//======== init the index of mam_buf_index =======//
    	mam_buf_index = 0;
    	memset(mem_buf,0,512);
    	kfetch_buf_temp_index= kfetch_buf_temp_index+15;
   
    	while(kfetch_buf_temp[kfetch_buf_temp_index]==' ')
    	{
    		kfetch_buf_temp_index++;
    	}
    	
    	while(kfetch_buf_temp[kfetch_buf_temp_index]!=' ')
    	{
    		mem_buf[mam_buf_index] = kfetch_buf_temp[kfetch_buf_temp_index];
    		
    		kfetch_buf_temp_index++;
    		mam_buf_index++;
    	}
    	
    	mem_buf[mam_buf_index] = '\0';
    	if(kstrtol( mem_buf, 10, &mem_kb_free ) == 0)
    	{
    	}
    	
    	mem_kb_free = mem_kb_free /1024;
    	
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "Mem:      %ld MB /", mem_kb_free);
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], " %ld MB\n", mem_kb_total);
    	filp_close(mem_file, NULL);
    	
    	}
    	
    	if((mask_info >> 5 ) & 1){
    	current_line++;
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    	
    	kfetch_buf_temp_index = 0;
    	memset(kfetch_buf_temp, 0, 1024);
    	threads_file=filp_open("/proc/loadavg", O_RDONLY,0);
    	kernel_read(threads_file, kfetch_buf_temp ,30, &threads_info_offset); 
    	while(kfetch_buf_temp[kfetch_buf_temp_index++]!='/')
    	{
    	}
    	while(kfetch_buf_temp[kfetch_buf_temp_index]!=' ')
    	{
    	thr_buf[thr_buf_index++] = kfetch_buf_temp[kfetch_buf_temp_index];
    	kfetch_buf_temp_index++;
    	}
    	thr_buf[thr_buf_index] ='\0';
    	if(kstrtol( thr_buf, 10, &total_threads ) == 0)
    	{
    		
    	}
    	
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "Procs:    %ld\n", total_threads);
    	filp_close(threads_file, NULL);
    	}
    	
    	if((mask_info >> 4 ) & 1){
    	current_line++;
    	kfetch_buf_temp_index = 0;
    	
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    	uptime_file = filp_open("/proc/uptime", O_RDONLY,0);
    	kernel_read(uptime_file, kfetch_buf_temp, 10 , &uptime_info_offset );
    	
    	
    	
    	
    	while(kfetch_buf_temp[kfetch_buf_temp_index++]!='.')
    	{
    	
    	}
    	kfetch_buf_temp[kfetch_buf_temp_index] = '\0';
    	sscanf(kfetch_buf_temp,"%d",&uptime);
    	uptime =(uptime/60);
    	kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index], "Uptime:   %d mins\n", uptime);
    	filp_close(uptime_file, NULL);
    	}
  
  
  
  
  
    while(current_line<total_line)  
    {
    kfetch_buf[kftech_buf_index++] = '\n';
    kftech_buf_index += sprintf(&kfetch_buf[kftech_buf_index],"\t""\t""\t");
    current_line++;
    
    }
    	
    kfetch_buf[kftech_buf_index] = '\0'; 	
    mask_info_prev = mask_info;
    if (copy_to_user( buffer,kfetch_buf, kftech_buf_index)) {
        pr_alert("Failed to copy data to user");
        return 0;
    }
    else
    {
    pr_alert("success to copy data to user");
    
    }
    
    return kftech_buf_index;
    /* cleaning up */
}


static ssize_t kfetch_write(struct file *filp,const char __user *buffer, size_t length, loff_t *offset)
{


    if (copy_from_user(&mask_info, buffer, length)) {
        pr_alert("Failed to copy data from user");
        return 0;
    }

    	
    	return 0;
    	}
module_init(kfetch_init);
module_exit(kfetch_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("NCRL,LEE,YA-JIE");
MODULE_DESCRIPTION("Show the system information");

