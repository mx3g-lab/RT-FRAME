#include <zephyr/kernel.h>
#include "core/rtframe_core.h"

int main(void)
{
    rtframe_core_init();
    return 0;
}
