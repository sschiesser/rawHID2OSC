#ifndef _RAWHID2OSC_H
#define _RAWHID2OSC_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

//#include <time.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#elif defined(OS_WINDOWS)
#include <conio.h>
#endif

#include "hid.h"
#include "lo/lo.h"
#include "messages.h"




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

struct touch_cal_s
{
  uint16_t min;
  uint16_t max;
  uint16_t avg;
  bool st;
};
struct range_cal_s
{
  uint16_t min;
  uint16_t max;
  bool st;
};
struct string_cal_s
{
  struct touch_cal_s calib_t;
  struct range_cal_s calib_r;
};
struct calib_state_s
{
  struct string_cal_s e_str;
  struct string_cal_s g_str;
};

struct osc_sender_s
{
  char host[32];
  char port[8];
  char n_meas_addr[64];
  char n_calib_t_addr[64];
  char n_calib_r_addr[64];
  char n_view_addr[64];
  char n_hid_addr[64];
  char n_app_addr[64];
};
struct osc_receiver_s
{
  char port[8];
  char r_meas_addr[64];
  char r_calib_t_addr[64];
  char r_calib_r_addr[64];
  char r_view_addr[64];
  char r_hid_addr[64];
  char r_app_addr[64];
};
struct osc_s
{
  struct osc_sender_s s;
  struct osc_receiver_s r;
};

struct hid_s
{
  int8_t id;
  uint16_t vid, pid, page, usage;
  bool open;
};

struct violin_s
{
  struct hid_s dev;
  machine_state cur_st;
  struct calib_state_s cal_st;
  struct osc_s osc;
};

uint32_t get_ms(void);
static void parse_hid_notif(uint8_t* p);
static int parse_hid_n_info(uint8_t* p, uint8_t len);
static int parse_hid_n_ack(uint8_t* p, uint8_t len);
static void parse_keystroke(char c, bool dev_open);
static void display_help(void);
static void display_calib_vals(void);
static char get_keystroke(void);
void lo_error(int num, const char* msg, const char* path);
int r_meas_handler(const char* path, const char* types, lo_arg** argv,
                   int argc, void* data, void* user_data);
int r_calib_t_handler(const char* path, const char* types, lo_arg** argv,
                      int argc, void* data, void* user_data);
int r_calib_r_handler(const char* path, const char* types, lo_arg** argv,
                      int argc, void* data, void* user_data);
int r_view_handler(const char* path, const char* types, lo_arg** argv,
                   int argc, void* data, void* user_data);
int r_hid_handler(const char* path, const char* types, lo_arg** argv,
                  int argc, void* data, void* user_data);
int r_app_handler(const char* path, const char* types, lo_arg** argv,
                  int argc, void* data, void* user_data);
void init(void);
void startup(void);
#endif /* _RAWHID2OSC_H */