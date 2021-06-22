#ifndef _MESSAGES_H
#define _MESSAGES_H

typedef enum hid_requests_t
{
  // request type headers (val < 0x20)
  R_CMD = 0x10,
  // request values (0x20 <= val <= 0x7E)
  R_STR_A = 'A', // 0x41
  R_STR_D = 'D', // 0x44
  R_STR_E = 'E', // 0x45
  R_STR_G = 'G', // 0x47
  R_HELP = 'h', // 0x68
  R_MEAS = 'm', // 0x6D
  R_CALIB_R = 'r', // 0x72
  R_CALIB_T = 't', // 0x74
  R_VIEW = 'v', // 0x76
  R_EXIT = 'x', //0x78
  // request end delimiter
  R_END = 0xFF
} hid_requests;

/******************************************************************************
* HID notifications are sent from the HID server (e-violin) to the client
* (this app)
*******************************************************************************/
typedef enum hid_notifications_t
{
  // notification type headers
  N_MEAS = 0x00,                // measurement values (special string)
  N_ACK = 0x01,                 // acknowledgement of a received request
  N_INFO = 0x02,                // information on running tasks (e.g. calibration)
  // notification values
  N_CALIB_R = 0x20,
  N_CR_DONE = 0x21,
  N_CALIB_T = 0x30,
  N_CT_DONE = 0x31,
  N_VIEW = R_VIEW,
  N_EXIT = R_EXIT,
  N_STR_E = R_STR_E,
  N_STR_G = R_STR_G,
  N_STR_D = R_STR_D,
  N_STR_A = R_STR_A,
  N_ERR = 0xE0,
  N_TIMEOUT = 0xE1,
  // notification end delimiter
  N_END = 0xFF
} hid_notifications;

#endif /* _MESSAGES_H */