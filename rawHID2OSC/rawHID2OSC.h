#ifndef _RAWHID2OSC_H
#define _RAWHID2OSC_H

typedef enum hid_notifications_t
{
  // notification type headers
  NOTIF_MEASUREMENT = 0x00,
  NOTIF_ACKNOWLEDGEMENT = 0x10,
  // notification values
  NOTIF_CALIB_RANGES = 0x20,
  NOTIF_CALIB_RANGES_DONE = 0x21,
  NOTIF_CALIB_TOUCH = 0x30,
  NOTIF_CALIB_TOUCH_DONE = 0x31,
  NOTIF_MEASURE_REQ = 0x40,
  NOTIF_VIEW_VALUES = 0x50,
  NOTIF_STRING_E = 0x60,
  NOTIF_STRING_G = 0x61,
  NOTIF_STRING_D = 0x62,
  NOTIF_STRING_A = 0x63,
  NOTIF_ERROR_BADCMD = 0xE0,
  NOTIF_ERROR_TIMEOUT = 0xE1,
  // notification end delimiter
  NOTIF_END = 0xFF
} hid_notifications;

typedef enum hid_requests_t
{
  // request type headers (val < 0x20)
  REQ_COMMAND = 0x10,
  // request values (0x20 <= val <= 0x7E)
  REQ_STRING_A = 'a',
  REQ_STRING_D = 'd',
  REQ_STRING_E = 'e',
  REQ_STRING_G = 'g',
  REQ_STRING_NONE = 0xe0,
  REQ_HELP = 'h',
  REQ_MEASURE = 'm',
  REQ_CALIB_RANGES = 'r',
  REQ_CALIB_TOUCH = 't',
  REQ_VIEW = 'v',
  REQ_EXIT = 'x',
  // request end delimiter
  REQ_END = 0xFF
} hid_requests;

typedef enum machine_state_t
{
  STATE_IDLE = 0x00,
  STATE_CALIB_RANGES = 0x10,
  STATE_CALIB_RANGES_G = 0x11,
  STATE_CALIB_RANGES_E = 0x12,
  STATE_CALIB_TOUCH = 0x20,
  STATE_CALIB_TOUCH_G = 0x21,
  STATE_CALIB_TOUCH_E = 0x22,
  STATE_MEASURING = 0x50,
  STATE_ERROR = 0xE0
} machine_state;


static char get_keystroke(void);
static void parse_notification(uint8_t* p);
static void parse_command(hid_requests cmd, machine_state state);
static void display_help(void);
static void display_calib_vals(void);

#endif /* _RAWHID2OSC_H */