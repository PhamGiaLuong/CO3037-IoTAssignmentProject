#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include <DNSServer.h>
#include <esp_wifi.h>

#include "global.h"

void networkTask(void* pvParameters);

#endif  // __NETWORK_MANAGER_H__