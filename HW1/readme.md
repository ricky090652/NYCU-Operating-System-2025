# Implement System Call

# Compiling a custom linux kernel

### Screenshots of the outputs

![Requirement1.png](Requirement1.png)

### Steps for compiling kernel

編譯需要用到的指令

```c
make mrproper //清理大部分編譯產生文件，包含配置檔跟備份檔
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- menuconfig //圖形化介面
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)//啟用多核心編譯
```

![menuconfig.png](menuconfig.png)

Enter General setup

![image.png](image.png)

Change the local version

![menuconfig_localversion.png](menuconfig_localversion.png)

Save  the config

```c
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)//啟用多核心編譯
```

![compiled.png](compiled.png)

compiled successful

## QA

- **What are the main differences between the RISC-V and x86 architectures?**
    1. CISC v.s. RISC
        1. x86是採用複雜指令集(CISC)，他的設計是想盡可能讓CPU做更多工作。他提供了大量且複雜的指令，一個指令可能可以包含多個操作，可以用更少的程式碼完成任務，但也因為指令複雜，因此長度不固定，解碼複雜，增加了CPU設計的複雜度。
        2. RISC的設計理念就跟x86相反。他只提供部分非常簡單且標準化的指令，在較複雜的操作需要多個指令組合而成，但指令長度都是固定，因此在設計上更簡單，執行效率也較高。
    2. 授權商業模式
        1. x86是封閉的指令集架構，一般人無法自行對指令集修改或變更。且若要設計和製造x86相容的cpu，必須向Intel或AMD取得授權。
        2. RISC-V則是完全開放標準的，任何人都可以依照需求擴展指令，適合嵌入式或是行動裝置應用，是目前較新的生態。
- **Why do the architecture differences matter when building the kernel? What happens if you build the kernel without the correct RISC-V cross-compilation flag?**
    
    在編譯kernel時，必須要產生與目標CPU相同架構的機器碼。不同指令集的架構差異非常大，如果使用錯誤架構編譯會導致kernel無法執行。如果使用不正確的RISC-V cross-compilation flag，編譯器通常會以本地主機的架構x86來編譯，產生kernel就不會是RISC-V的格式，導致系統無法啟動。
    
- **Why is Docker used in this assignment, and what advantages does it provide? Please list at least two of them.**
    
    docker主要的功能是將應用程式與他所有的依賴項(特定版本的函式庫、工具等)打包在一起，確保該應用程式在不同的環境下都能以完全相同的方式執行。
    
    1. 確保環境一致：如果沒有docker，安裝環境可能會變得非常複雜，因為每個人安裝的linux、編譯器，甚至函式庫版本的可能都會不一樣，這就會造成程式可以在自己的裝置上跑，但如果在其他電腦可能就無法執行。透過docker可以確保不管是在我們的電腦以及助教的電腦內都可以用相同的環境執行。
    2. 簡化環境設定流程：設定Linux環境非常複雜，助教已經幫我們最繁瑣跟困難的環境設定工作都完成了，我們只需要安裝好docker就直接配置好環境，可以讓我們把大部分時間都花在作業本身而不是設定環境。

---

# Implementing new System Calls

## Screenshots of the outputs

![test_revstr-result.png](test_revstr-result.png)

execution result of user program for sys_revstr

![dmesg_revstr.png](dmesg_revstr.png)

dmesg output of sys_revstr

![test_tempbuf-result.png](test_tempbuf-result.png)

execution result of user program for sys_tempbuf

![dmesg_tempbuf.png](dmesg_tempbuf.png)

dmesg output of sys_tempbuf

## 新增system call流程

1. system call source code實作
2. 增加system call header
3. 在generic list新增system call
4. Makefile擴充
    1. 增加實作的source code的依賴規則
    2. 在kernel Makefile新增檢索路徑
5. 重新編譯

### 1. system call source code實作

**sys_revstr.c**

```c
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE2(revstr, char __user *, str, size_t, n)
{
    char buf[256];  // buffer size 限制
    char rev[256];
    size_t i;

    // 檢查使用者傳入字串長度是否超過限制
    if (n > 255)
        return -EINVAL;

    // 從 user space 取得字串
    if (copy_from_user(buf, str, n)) //成功return 0
        return -EFAULT;

    buf[n] = '\0';  // 確保 null-terminated

    // 印出原始字串到 kernel ring buffer
    printk(KERN_INFO "The origin string: %s\n", buf);

    // 反轉字串
    for (i = 0; i < n; i++) {
        rev[i] = buf[n - 1 - i];
    }
    rev[n] = '\0';

    // 印出反轉字串
    printk(KERN_INFO "The reversed string: %s\n", rev);

    // 將反轉字串 copy 回 user space
    if (copy_to_user(str, rev, n + 1))
        return -EFAULT;

    return 0;
}
```

- **unsigned long copy_from_user(void  *to , const void  *from , unsigned long n)**：將user space n bytes資料複製到kernel space裡。成功的話return 0，失敗的話return沒有複製成功的資料bytes數。
- **unsigned long copy_to_user(void  *to , const void  *from , unsigned long n)** ：將kernel space n bytes的資料複製到user space裡。成功的話return 0，失敗的話return沒有複製成功的資料bytes數。
- **-EFAULT：**代表「無效的記憶體位址」。當kernel需要存取由user space傳入的指標，但指標指向一個無效或無法存取的記憶體位址，就會回傳此錯誤。
- **-EINVAL**：代表「無效的參數」，代表傳遞給系統呼叫的某個參數值本身是不合法或超出範圍的。

**sys_tempbuf.c**

```c
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
        if (copy_from_user(kbuf, data, size)) //成功return 0
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
        list_add_tail(&rec->list, &tempbuf_list); //
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
```

- **tempbuf_record**：我們自行定義的資料結構，就是linked list內的每一個Node的資料結構。每加入一筆資料就動態配置一個新的tempbuf_record，把字串存進去，然後再掛進linked list。struct list_head list則是linux kernel裡面提供的linked list介面。只要在我們自己宣告的struct內放入struct list_head list就可以使用linked list函數。
- **LIST_HEAD**：宣告一個struct list_head結構並初始化。
- **kmalloc(len, GFP_KERNEL)**：kernel space的malloc，GFP_KERNEL代表正常分配。
- **void *memcpy(void *str1, const void *str2 ,size_t n)**：把str2所指的地址前n個bytes的資料複製到str1指的地址上。
- int memcmp(const void *str1, const void *str2, size_t n)：比較str1和str2地址的前n個bytes，若return值為0，代表str1等於str2。

![image.png](image%201.png)

kernel linked list參考資料：

[https://myao0730.blogspot.com/2016/12/linux.html](https://myao0730.blogspot.com/2016/12/linux.html)

- **list_add_tail(struct list_head *new_list , struct list_head *head)**：第一個參數放的是要新增節點裡面的list地址，第二個參數是head的地址
- **list_for_each_entry_safe(struct list_head *rec, struct list_head *temp, struct list_head *HEAD , list)**：刪除節點時用， 使用一般的list for each entry刪除某個節點時並不會自動把前一個的next指標指向被刪除節點的下一個，使用這個函式會自動處理，需要額外一個temp指標來暫存
- **list_del(struct_list head *rec)**：刪除當前節點
- **-ENOMEM**：記憶體不足

**ADD**：先將user space傳入的字串複製到kernel的暫存kbuf中，再用kmalloc分配一塊記憶體給新Node，再為每一個Node分配一塊記憶體用來儲存傳進來的字串。再使用memcpy把剛剛暫存在kbuf內的字串複製到剛剛新增用來儲存字串的位址，最後再透過**list_add_tail**這個函數把這個Node加到串列尾端。

**REMOVE**：一樣先把user space傳進來的字串暫存在kbuf內，遍歷整個linked list的每一筆node，比對kbuf內的字串與node內的字串，必須長度一樣且內容也相等才算match，接著就直接把找到的節點刪除並且釋放記憶體，最後跳出。

**PRINT**：做法是依次將每個Node內的字串放進kbuf，並且每次放完字串後在尾巴加上一個空白鍵，pos就是用來控制拼接後字串在kbuf的位置。由於作業有指定PRINT內最多不會超過512bytes，因此我們將可最大長度設為512，在每次要把字串放進kbuf之前，我們會先確認新增當前Node的字串進入kbuf內長度是否會超過512，如果會的話，我們最多只會把不超過範圍內的字串印出，多餘的部分會直接捨棄掉。

### 2. 增加system call header

- 在header宣告函式(**include/linux/syscalls.h**)

```c
asmlinkage long sys_revstr(char __user *str, size_t n);

asmlinkage long sys_tempbuf(int mode, void __user *data, size_t size)
```

### 3. 在generic list新增system call

- 在syscall list註冊(**include/uapi/asm-generic/unistd.h**)

```c
#define __NR_revstr 451
__SYSCALL(__NR_revstr, sys_revstr)

#define __NR_tempbuf 452
__SYSCALL(__NR_tempbuf, sys_tempbuf)
```

### 4. Makefile擴充

### 增加實作source code的依賴規則

- 分別在自己新增的資料夾內(revstr, tempbuf)並且在裡面新增各自的Makefile：這個指令會自動把同個目錄下的sys_revstr.c跟sys_tempbuf.c編譯成物件檔

```c
obj-y:=sys_revstr.o
```

```c
obj-y:=sys_tempbuf.o
```

### 在kernel Makefile新增檢索路徑

- 把自己新增的資料夾(revstr)放到Kernel Makefile裡面，這部的目的是告訴kbuild，編譯kernel時要遞迴進入這個目錄編譯裡面的東西。如果沒加這個東西kernel不會編譯到我們自己做的Makefile

```c
ifeq($(KBUILD_EXTMOD),)
core-y += revstr/ tempbuf/
```

### 5. 最後重新編譯

```c
make ARCH=riscv CROSS_COMPILE=riscv64-linux-gnu- -j$(nproc)
```

# QA:

- **What does the `include/linux/syscalls.h` do?**
    
    syscalls.h是linux kernel內用來宣告及存放所有system calls的header檔案。裡面會列出所有system call的名稱以及相對應的參數型態及結構。裡面也定義了system call的metadata，像是SYSCALL_DEFINE和SYSCALL_METADATA等巨集。
    
- **Explain the difference between a system call and a glibc library call, then give one example that maps a glibc function to the specific system call it ultimately invokes.**
    
    System call：一般user space的程式是無法直接存取硬體相關的資源，當今天需要硬體相關的操作，就需要借助作業系統的幫忙，而kernel提供我們可以跟作業系統互動的介面就是system call。執行system call時是在kernel space內。
    
    glibc library call：就像我們在寫C program內的printf()或malloc()等等函式，這些函式都是C library提供的。Glibc把底層較複雜的函式都封裝起來，包裝成一個簡單易使用的API供user space直接執行。有些函式若需要操作到硬體資源或是需要kernel的服務，則會再觸發後續的system call。
    
    EX：當今天我們在application內了call read()，會先連結到C library，接著會把參數傳入暫存器內並且對應system call的編號，再來就會執行syscall指令進入kernel space內。接下來就會由system call handler處理，他會去對應實際上要去執行什麼C code，也就是sys_read。
    
- **Explain the difference between static linking and dynamic linking**
    
    Static linking：在編譯時會把所有用到的library一起嵌入進executable檔案裡，好處是如果要移植到別的機器上不需要再依賴任何外部的library。但缺點也常顯而易見，因為包了所有的library在檔案內，因此檔案容量較大，且如果有多個程式都用相同的library，每一個執行檔都會有library的副本，就會造成空間浪費。
    
    dynamic linking：到執行階段才載入系統分享的動態library，也因為library並沒有被整合進我們的executable內，是在運行時動態調用的，因此較省檔案空間，但我們運行的環境也必須要提供相對應的library。
    
- **In this assignment environment, why do we have to compile the test programs with the `-static` flag? What would happen if we omitted it?**
    
    使用static編譯確保所有必要的library都會被包進執行黨內，不會依賴系統上的動態庫，可以減少因為系統環境不同帶來的各種執行問題。
    

---

# 記錄用指令

## docker

```c
docker exec -it os-main-1 /bin/bash
```

- 撰寫user space測試程式
    - Compile your test program
    
    ```c
    riscv64-linux-gnu-gcc -static -o test_revstr test_revstr.c
    ```
    
    - Place the binary into initramfs directory
    
    ```c
    mkdir -p initramfs
    cd initramfs
    gzip -dc ../initramfs.cpio.gz | cpio -idmv
    ```
    
    - **複製測試程式到檔案系統 任何路徑都可以(`/home`**、**`/root`...要在新增的initramfs內)**
    
    ```c
    cp ../test_revstr ./home/
    ```
    
    - Repack the initramfs(把測試檔加入自己建立的initramfs後，再重新壓縮一次變成**initramfs.cpio.gz**)
    
    ```c
    find . | cpio -o -H newc | gzip > ../initramfs.cpio.gz
    cd ..
    ```
    

## QEMU

```c
qemu-system-riscv64 -nographic -machine virt \
  -kernel linux/arch/riscv/boot/Image \
  -initrd initramfs.cpio.gz \
  -append "console=ttyS0 loglevel=3"
```

## 從本機複製檔案到docker

```jsx
docker cp C:\path\to\initramfs.cpio.gz os-main-1:/root/linux/

```

## patch

- 先看自己更動過哪一些檔案

```c
git status
```

- 自己建立一個branch

```c
git checkout -b my_syscall
```

- 把每一個更動過的檔案都git add跟git commit

```c
//只用Makefile舉例
git add Makefile
git commit -m "Modified make file"
```

- 如果要把所有更動程式打包成一包，可以先全部add完最後一次commit，再打包成patch

```c
git format-patch HEAD~1 --stdout > my_syscall.patch
```

## 測試patch

- 先把原本linux資料夾內所有東西複製到另一個測試的linux資料夾內

```c
git clone linux linux-test
```

- 把patch檔移進測試環境

```c
mv linux/my_syscall.patch linux-test/
```

- 先測試patch檔可不可以用

```c
git apply --check my_syscall.patch
```

- 套用patch

```c
git am < my_syscall.patch
```

## Compile test program

```c
riscv64-linux-gnu-gcc -static -o test_revstr test_revstr.c
```