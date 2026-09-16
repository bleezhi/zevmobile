#ifndef ZEV_JVM_H
#define ZEV_JVM_H

#include <stddef.h>
#include <stdint.h>
#include "../emulator/phone.h"

typedef struct { const uint8_t *data; size_t size; } ZevClassImage;
typedef struct { ZevPhone *phone; int running; int exit_code; } ZevJvm;

int zev_jvm_init(ZevJvm *jvm, ZevPhone *phone);
int zev_jvm_load_class(ZevJvm *jvm, ZevClassImage image);
int zev_jvm_run_main(ZevJvm *jvm);
int zev_jvm_run_class(ZevJvm *jvm, ZevClassImage image);
extern const ZevClassImage zev_launcher_class;

#endif
