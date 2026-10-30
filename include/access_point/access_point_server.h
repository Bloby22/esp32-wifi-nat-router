#ifndef ESP_CONNECT_ACCESS_POINT_SERVER_H
#define ESP_CONNECT_ACCESS_POINT_SERVER_H

#include <WebServer.h>

extern WebServer webServer;

void startAccessPointWebServer();
void handleRoot();
void handleStylesheet();
void handleSave();
void handleNotFound();

#endif
