#include<linux/kernel.h>
#include<linux/syscalls.h>
#include<linux/uaccess.h>

SYSCALL_DEFINE2(revstr, char __user * ,str, size_t ,n) 
{
	char *buf, *rev;
    size_t i;

    // 動態分配記憶體
    buf = kmalloc(n + 1, GFP_KERNEL);
    rev = kmalloc(n + 1, GFP_KERNEL);

    if (!buf || !rev) {
        // 分配失敗，釋放已分配的記憶體
        if (buf) kfree(buf);
        if (rev) kfree(rev);
        return -ENOMEM;
    }

    if (copy_from_user(buf, str, n)) {
        kfree(buf);
        kfree(rev);
        return -EFAULT;
    }
    buf[n] = '\0';

    printk(KERN_INFO "The origin string: %s\n", buf);

    for (i = 0; i < n; i++) {
        rev[i] = buf[n - 1 - i];
    }
    rev[n] = '\0';

    printk(KERN_INFO "The reverse string: %s\n", rev);

    if (copy_to_user(str, rev, n + 1)) {
        kfree(buf);
        kfree(rev);
        return -EFAULT;
    }

    // 釋放動態記憶體
    kfree(buf);
    kfree(rev);

    return 0;
}
