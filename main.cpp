#include <iostream>
#include <sys/resource.h>
#include "device.h"
#include "common.h"


//-----------------------------------------------------------------------------

int main(int argc, char ** argv)
{
    // run this program only at first CPU
    cpu_set_t set;
    CPU_ZERO(&set);                                 // clear cpu mask
    CPU_SET(0, &set);                               // set cpu 0
    sched_setaffinity(0, sizeof(cpu_set_t), &set);  // 0 is the calling process

    // set maximum process priority
    setpriority(PRIO_PROCESS, 0, -20);

    MultipleDevice devs(argc, argv);
    devs.showRegions();

    return 0;
}
