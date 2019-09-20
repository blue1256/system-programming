#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <asm/pgtable.h>

struct packet {
    pid_t pid;
    unsigned long vaddr;
    unsigned long paddr;
};

MODULE_LICENSE("GPL");

static struct dentry *dir, *output;
static struct task_struct *task;

static ssize_t read_output(struct file *fp,
                        char __user *user_buffer,
                        size_t length,
                        loff_t *position)
{
    struct packet pac;
    struct mm_struct* mm;
    pgd_t* pgd;
    p4d_t* p4d;
    pud_t* pud;
    pmd_t* pmd;
    pte_t* pte;
    struct page* page;
    unsigned long vaddr;
    unsigned long paddr;

    copy_from_user(&pac, user_buffer, sizeof(struct packet));
    vaddr = pac.vaddr;

    task = pid_task(find_vpid(pac.pid), PIDTYPE_PID);

    mm = task->mm;

    pgd = pgd_offset(mm, vaddr);

    p4d = p4d_offset(pgd, vaddr);

    pud = pud_offset(p4d, vaddr);

    pmd = pmd_offset(pud, vaddr);

    pte = pte_offset_map(pmd, vaddr);

    page = pte_page(*pte);

    paddr = page_to_phys(page);
    paddr = paddr + (vaddr & 0xFFF);

    pac.paddr = paddr;
    copy_to_user(user_buffer, &pac, sizeof(struct packet));

    return length;
}

static const struct file_operations dbfs_fops = {
    .read = read_output,
};

static int __init dbfs_module_init(void)
{
    dir = debugfs_create_dir("paddr", NULL);

    if (!dir) {
        printk("Cannot create paddr dir\n");
        return -1;
    }

    output = debugfs_create_file("output", 0666, dir, NULL, &dbfs_fops);

	printk("dbfs_paddr module initialize done\n");

    return 0;
}

static void __exit dbfs_module_exit(void)
{
    debugfs_remove_recursive(dir);
	printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);
