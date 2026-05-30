#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("rtframe CM4 hello from C++\n");

    while (true) {
        k_sleep(K_MSEC(2000));
    }

    return 0;
}
