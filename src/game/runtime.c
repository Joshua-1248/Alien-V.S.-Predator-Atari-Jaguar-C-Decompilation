#include "avp_runtime.h"
#include <string.h>
static AvpRuntimeOps g_ops;
void avp_runtime_bind(const AvpRuntimeOps *ops){ if(ops) g_ops=*ops; else memset(&g_ops,0,sizeof(g_ops)); }
const AvpRuntimeOps *avp_runtime_ops(void){ return &g_ops; }
void avp_runtime_reset_ops(void){ memset(&g_ops,0,sizeof(g_ops)); }
