#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *inputdir, *ptreedir;
static struct task_struct *curr;
static char output[10000];

static ssize_t write_pid_to_input(struct file *fp, 
                                const char __user *user_buffer, 
                                size_t length, 
                                loff_t *position)
{
    pid_t input_pid;
    struct debugfs_blob_wrapper* blob;
    blob = kmalloc(sizeof(struct debugfs_blob_wrapper), GFP_KERNEL);

    char* buff = (char*)kmalloc(sizeof(output), GFP_KERNEL);

    sscanf(user_buffer, "%u", &input_pid);
    curr = pid_task(find_vpid(input_pid), PIDTYPE_PID);

    pid_t cpid = curr->pid;
    sprintf(output, "%s (%d)\n", curr->comm, cpid);
    curr = curr->parent;
    cpid = curr->pid;

    while(cpid!=1){
        strcpy(buff, output);
        sprintf(output, "%s (%d)\n", curr->comm, cpid);
        strcat(output, buff);
        curr = curr->parent;
        cpid = curr->pid;
    }
    strcpy(buff, output);
    sprintf(output, "%s (%d)\n", curr->comm, cpid);
    strcat(output, buff);

    blob->data = (void*)(output);
    blob->size = strlen(output);

    debugfs_remove(ptreedir);
    ptreedir = debugfs_create_blob("ptree", 0666, dir, blob);
    kfree(buff);
    return length;
}

static const struct file_operations dbfs_fops = {
    .write = write_pid_to_input,
};

static int __init dbfs_module_init(void)
{
    dir = debugfs_create_dir("ptree", NULL);
    
    if (!dir) {
        printk("Cannot create ptree dir\n");
        return -1;
    }

    inputdir = debugfs_create_file("input", 0444, dir, NULL, &dbfs_fops);

    ptreedir = debugfs_create_blob("ptree", 0666, dir, NULL);
	
    printk("dbfs_ptree module initialize done\n");

    return 0;
}

static void __exit dbfs_module_exit(void)
{
    debugfs_remove_recursive(dir);
	printk("dbfs_ptree module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);
