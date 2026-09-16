#ifndef ZEV_KERNEL_H
#define ZEV_KERNEL_H

#include "../emulator/phone.h"

void zev_kernel_boot(ZevPhone *phone);
void zev_kernel_tick(ZevPhone *phone);

#endif
