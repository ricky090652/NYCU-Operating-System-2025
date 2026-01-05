#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/errno.h>

#define MAX_TEMPBUF_LEN 512

enum mode {
    PRINT = 0,
    ADD = 1,
    REMOVE = 2,
};

struct tempbuf_record {
    struct list_head list;
    char *str;
    size_t len;
};

static LIST_HEAD(tempbuf_list);

SYSCALL_DEFINE3(tempbuf, int, mode, void __user *, data, size_t, size)
{
    struct tempbuf_record *rec, *tmp;
    char kbuf[MAX_TEMPBUF_LEN + 1];
    size_t pos;
    int ret = 0;

    if (!data || size == 0 || size > MAX_TEMPBUF_LEN)
        return -EFAULT;

    if (mode == ADD) {
        if (copy_from_user(kbuf, data, size))
            return -EFAULT;
        kbuf[size] = '\0';

        rec = kmalloc(sizeof(*rec), GFP_KERNEL);
        if (!rec)
            return -ENOMEM;

        rec->str = kmalloc(size + 1, GFP_KERNEL);
        if (!rec->str) {
            kfree(rec);
            return -ENOMEM;
        }
        memcpy(rec->str, kbuf, size + 1);
        rec->len = size;
        list_add_tail(&rec->list, &tempbuf_list);
        printk("[tempbuf] Added: %s\n", rec->str);

        return 0;
    }
    else if (mode == REMOVE) {
        if (copy_from_user(kbuf, data, size))
            return -EFAULT;
        kbuf[size] = '\0';

        ret = -ENOENT;
        list_for_each_entry_safe(rec, tmp, &tempbuf_list, list) {
            if (rec->len == size && !memcmp(rec->str, kbuf, size)) {
                printk("[tempbuf] Removed: %s\n", rec->str);
                list_del(&rec->list);
                kfree(rec->str);
                kfree(rec);
                ret = 0;
                break;
            }
        }
        return ret;
    }
    else if (mode == PRINT) {
        pos = 0;
        list_for_each_entry(rec, &tempbuf_list, list) {
            size_t n = min(rec->len, MAX_TEMPBUF_LEN - pos - 1);
            if (pos > 0)
                kbuf[pos++] = ' ';
            memcpy(kbuf + pos, rec->str, n);
            pos += n;
            if (pos >= MAX_TEMPBUF_LEN - 1)
                break;
        }
        kbuf[pos] = '\0';

        if (copy_to_user(data, kbuf, pos + 1))
            return -EFAULT;

        printk("[tempbuf] %s\n", kbuf);
        return pos;
    }

    return -EINVAL;
}

