#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("rtframe CM7 hello from C++\n");

    while (true) {
        k_sleep(K_MSEC(1000));
        printk("tick test\n");
    }

    return 0;
}
