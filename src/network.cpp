#include "network.h"
#include "libs/ikcp.h"

void InitializeNetwork(){
	ikcpcb *kcp = ikcp_create(1, (void*)0);
	ikcp_release(kcp);
}
