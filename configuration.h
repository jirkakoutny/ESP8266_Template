#ifndef _CONFIGURATION_H_
#define _CONFIGURATION_H_
#define NO_PIN -1 // indikator nepouziteho pinu

#define DEBUG_BUILD // **** ladici varianta s Arduino OTA

#define USE_SPIFFS // **** podpora pro praci s SPIFFS souborovym systemem (webovy server dokaze servirovat stranky z SPIFFS)

#define FORCE_CONFIG_BUTTON_PIN NO_PIN // **** I/O pin, pouzity pro vynuceni konfigurace

#define LED_PIN 2 // pin, ke kteremu je pripojena LED dioda

#define HTTP_PORT 80 // port, na kterem pobezi HTTP (www) server

#ifdef DEBUG_BUILD
 #include "src/trace/trace.h"

 #define TRACE(severity, ...) trace_print(severity, __VA_ARGS__)
 #define TRACE_INIT trace_init()
 #define TRACE_ADDWEB(srv) trace_addweb(srv)
 #define TRACE_POLL trace_poll()
#else
 #define TRACE(...) {}
 #define TRACE_INIT {}
 #define TRACE_ADDWEB(a) {}
 #define TRACE_POLL {}
#endif

#endif
