#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void)
{
    printk("rtframe CM7 hello from C++\n");

    return 0;
}
