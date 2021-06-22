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
  bool status;
};
struct range_cal_s
{
  uint16_t min;
  uint16_t max;
  bool status;
};
struct string_cal_s
{
  struct touch_cal_s cal_touch;
  struct range_cal_s cal_range;
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
  char systime_addr[64];
  char position_addr[64];
  char state_addr[64];
};
struct osc_receiver_s
{
  char port[8];
  char cal_t_addr[64];
  char cal_r_addr[64];
  char meas_addr[64];
  char cmd_addr[64];
};
struct osc_s
{
  struct osc_sender_s sender;
  struct osc_receiver_s receiver;
};

struct hid_s
{
  int8_t dev_id;
  uint16_t vid, pid, page, usage;
  bool open;
};

struct violin_s
{
  struct hid_s device;
  machine_state cur_state;
  struct calib_state_s cal_state;
  struct osc_s osc;
};

uint32_t get_ms(void);
static void parse_notification(uint8_t* p);
static void parse_keystroke(char c, bool dev_open);
static void display_help(void);
static void display_calib_vals(void);
static char get_keystroke(void);
void lo_error(int num, const char* msg, const char* path);
int calib_touch_handler(const char* path, const char* types, lo_arg** argv,
                        int argc, void* data, void* user_data);
int calib_range_handler(const char* path, const char* types, lo_arg** argv,
                        int argc, void* data, void* user_data);
int measure_handler(const char* path, const char* types, lo_arg** argv,
                    int argc, void* data, void* user_data);
int command_handler(const char* path, const char* types, lo_arg** argv,
                    int argc, void* data, void* user_data);
void init(void);
void startup(void);
#endif /* _RAWHID2OSC_H */