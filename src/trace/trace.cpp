#include "trace.h"
#include "../arduinoWebSockets/src/WebSocketsServer.h"
#include "../interval/interval.h"
extern "C" {
#include "user_interface.h"
  extern struct rst_info resetInfo;
}

#include "eh.h"

#define MAX_TRACE_LINES 15
#define MAX_LINE_LEN  50

struct TraceLine
{
  char _text[MAX_LINE_LEN];
  uint16_t _length;
  uint32_t _time;
  uint8_t _severity;

  TraceLine(uint8_t severity, const char *str, uint16_t len)
  {
    memcpy(_text, str, len + 1);
    _length = len;
    _time = millis();
    _severity = severity;
  }

  TraceLine(void) {}
};

static TraceLine _lines[MAX_TRACE_LINES];
static uint16_t _lines_index = 0;
static uint16_t _lines_count = 0;
static int _modified = 0;
//static std::unique_ptr<ESP8266WebServer> _server;
static ESP8266WebServer *_server;
static WebSocketsServer _wss = WebSocketsServer(TRACE_WEBSOCKET); // webovy soket pro trasovani
static Interval _tint; // interval pro casovani stopare
static int _tc = 0; // priznak pro detekci pripojeneho klienta pro websoket trasovani

static void (*message_cb)(const char *) = NULL;

// Odeslani webove stranky stopare
static void handleTrace(void)
{

  _server->send(200, FPSTR(HTML), FPSTR(PAGE_TRACE));
}

// Osetreni komunikace pres webovy soket
static void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length)
{

  switch (type)
  {
    case WStype_DISCONNECTED:
      _tc = 0; // klient byl odpojeny
      break;

    case WStype_CONNECTED:
      { // zalogovani pripojeni prohlizece
        String info;

        _tc = 1; // pripojeny klient
        ++_modified; // vynutime odeslani dat po pripojeni

        info.reserve(512);
        info = F("{\"type\":\"info\",\"reset\":\"");
        info += ESP.getResetReason();
        info += F("\",\"flash\":\"");
        info += String(ESP.getFlashChipRealSize());
        info += F("\",\"ram\":\"");
        info += String(ESP.getFreeHeap());
        info += F("\"}");
        _wss.broadcastTXT(info); // odesleme informace do klienta
      }
      break;

    case WStype_TEXT:
      if (!strncmp((const char *)payload, "mem", length))
      {
        String info;

        info = F("{\"type\":\"mem\",\"ram\":\"");
        info += String(ESP.getFreeHeap());
        info += F("\"}");
        _wss.broadcastTXT(info); // odesleme informace do klienta
      }
      break;

    case WStype_BIN:
      break;
  }
}

static int modulo(int a, int b)
{
  int r = a % b;

  return ((r < 0) ? r + b : r);
}

static TraceLine &trace_line(uint16_t index)
{
  int start = _lines_index - _lines_count;
  int idx = modulo(start + index, _lines_count);

  return (_lines[idx]);
}

static void print(uint8_t severity, const char *buffer, int length)
{

  if (length >= MAX_LINE_LEN)
  {
    length = strlen(buffer);
  }

  TraceLine line(severity, buffer, length);

  _lines[_lines_index++] = line;
  _lines_index %= MAX_TRACE_LINES;
  if (_lines_count < MAX_TRACE_LINES)
  {
    ++_lines_count;
  }
  ++_modified;
}

static const String html_color(uint8_t severity)
{

  switch (severity)
  {
    case TRACE_ERROR: return F("red");
    case TRACE_WARNING: return F("yellow");
    case TRACE_INFO:   return F("green");
  }
  return F("black");
}

void trace_init(void)
{

  trace_print(TRACE_INFO, F("Trace: Starting..."));
}

static void convert_time(char buf[], uint32_t time)
{
  int   hours = (time % 86400000) / 3600000; // 86400 equals secs per day
  int minutes = (time % 3600000)  /   60000; //  3600 equals secs per minute
  int seconds =  (time % 60000) / 1000;
  int      ms = time % 1000;

  sprintf(buf, "%02i:%02i:%02i.%03i", hours, minutes, seconds, ms);
}

void trace_dumphtml(String &str)
{
  char time[16];

  str += F("<pre>");

  for (int i=0; i<_lines_count; i++)
  {
    TraceLine line = trace_line(i);
    String txt = line._text;

    convert_time(time, line._time);
    str += F("[");
    str += String(time);

    str += F("] <span style='color:");
    str += html_color(line._severity);
    str += F("'>");
    str += txt;
    str += F("</span><br>");
  }
  str += F("</pre>");
}

void trace_clear(void)
{

  _lines_index = 0;
  _lines_count = 0;
  _modified = 1;
}

void trace_end(void)
{

  trace_clear();
}

void trace_print(uint8_t severity, const __FlashStringHelper *fmt, ...)
{
  char buffer[MAX_LINE_LEN];
  va_list args;
  int length;

  va_start(args, fmt);
  length = vsnprintf_P(buffer, sizeof (buffer), (const char *)fmt, args);
  va_end(args);

  print(severity, buffer, length);
}

void trace_print(uint8_t severity, const char *fmt, ...)
{
  char buffer[MAX_LINE_LEN];
  va_list args;
  int length;

  va_start(args, fmt);
  length = vsnprintf(buffer, sizeof (buffer), fmt, args);
  va_end(args);

  print(severity, buffer, length);
}

void trace_registermessagecb(void (*cb)(const char *))
{

  message_cb = cb;
}

void trace_addweb(ESP8266WebServer *server)
{

  _server = server;
  _server->on("/trace", handleTrace);
  _wss.begin();
  _wss.onEvent(webSocketEvent);
}

void trace_poll()
{

  _wss.loop();
  if (_tc)
  {  // klient trasovani je pripojeny
    if (_tint.expired())
    {
      if (_modified)
      {
        String log;

        log.reserve(3 * 1024);
        log += F("{\"type\":\"trace\",\"text\":\"");
        trace_dumphtml(log);
        log += F("\"}");
        _wss.broadcastTXT(log);
        _modified = 0; // rusime pozadavek na odeslani novych dat
      }
      _tint.set(TRACE_CHECK_INTERVAL);
    }
  }
}
// EOF
