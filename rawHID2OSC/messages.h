#ifndef _MESSAGES_H
#define _MESSAGES_H

/******************************************************************************
* OSC socket settings
*******************************************************************************/
#define OSC_SEND_HOST "127.0.0.1"
#define OSC_SEND_PORT "19001"
#define OSC_RECV_PORT "19002"

/******************************************************************************
* OSC requests:
* -----------------------------------------------------------------------------
* OSC client (Max app) >>>>> server (this app)
*******************************************************************************/
#define OSC_REQ_MEAS "/violin/req/measure"
#define OSC_REQ_CALIB_R "/violin/req/calib/range"
#define OSC_REQ_CALIB_T "/violin/req/calib/touch"
#define OSC_REQ_VIEW "/violin/req/view"
#define OSC_REQ_HID "/violin/req/hid"
#define OSC_REQ_APP "/violin/req/app"

#define OSC_REQ_ARGS 1

/******************************************************************************
* OSC notifications:
* -----------------------------------------------------------------------------
* OSC server (this app) >>>>> client (Max app)
*******************************************************************************/
#define OSC_NOTIF_MEAS "/violin/notif/measure"
#define OSC_NOTIF_CALIB_R "/violin/notif/calib/range"
#define OSC_NOTIF_CALIB_T "/violin/notif/calib/touch"
#define OSC_NOTIF_VIEW "/violin/notif/view"
#define OSC_NOTIF_HID "/violin/notif/hid"
#define OSC_NOTIF_APP "/violin/notif/app"

#define OSC_N_MEAS_ARGS 4
#define OSC_N_CR_ARGS 3
#define OSC_N_CT_ARGS 4
#define OSC_N_VIEW_ARGS 8
#define OSC_N_HID_ARGS 1
#define OSC_N_APP_ARGS 1

/******************************************************************************
* HID settings
*******************************************************************************/
#define HID_VID 0x1C57
#define HID_PID 0x1234
#define HID_PAGE 0xFFAB
#define HID_USAGE 0x0200
#define HID_DEV_NONE -1

/******************************************************************************
* HID requests:
* -----------------------------------------------------------------------------
* HID client (this app) >>>>> server (e-violin)
*******************************************************************************/
typedef enum hid_requests_t
{
  // request type headers (0x10 - 0x1F)
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
  // request end delimiter (0xFF)
  R_END = 0xFF
} hid_requests;

/******************************************************************************
* HID notifications:
* ------------------
* HID server (e-violin) >>>>> client (this app)
*******************************************************************************/
typedef enum hid_notifications_t
{
  // notification type headers (0x00 - 0x0F)
  N_MEAS = 0x00,                // measurement values (special string)
  N_ACK = 0x01,                 // acknowledgement of a received request
  N_INFO = 0x02,                // information on running tasks (e.g. calibration)
  // notification value (0x20 - 0x7E)
  N_CALIB_R = R_CALIB_R,
  N_CR_DONE = 0x21,
  N_CALIB_T = R_CALIB_T,
  N_CT_DONE = 0x31,
  N_VIEW = R_VIEW,
  N_EXIT = R_EXIT,
  N_STR_E = R_STR_E,
  N_STR_G = R_STR_G,
  N_STR_D = R_STR_D,
  N_STR_A = R_STR_A,
  N_ERR = 0xE0,
  N_TIMEOUT = 0xE1,
  // notification end delimiter (0xFF)
  N_END = 0xFF
} hid_notifications;

#endif /* _MESSAGES_H */