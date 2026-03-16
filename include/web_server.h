#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>

#include "global.h"
#include "hardware_manager.h"

void webServerTask(void* pvParameters);

#endif  // __WEB_SERVER_H__