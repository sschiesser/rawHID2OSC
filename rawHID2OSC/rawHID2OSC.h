#ifndef _RAWHID2OSC_H
#define _RAWHID2OSC_H

typedef enum hid_messages_t
{
  MESS_COMMAND = 0xC0,
  MESS_CALIB_RANGES = 0xD0,
  MESS_CALIB_TOUCH = 0xD1,
  MESS_MEASURE = 0xE0
} hid_messages;

typedef enum machine_state_t
{
  STATE_IDLE = 0x00,
  STATE_CALIB_RANGES = 0x10,
  STATE_CALIB_TOUCH = 0x11,
  STATE_MEASURING = 0x20,
  STATE_ERROR = 0xF0
} machine_state;

typedef enum command_code_t
{
  CMD_STRING_A = 'a',
  CMD_STRING_D = 'd',
  CMD_STRING_E = 'e',
  CMD_STRING_G = 'g',
  CMD_HELP = 'h',
  CMD_MEASURE = 'm',
  CMD_CALIB_RANGES = 'r',
  CMD_CALIB_TOUCH = 't',
  CMD_VIEW = 'v',
  CMD_EXIT = 'x',
  CMD_ERR_NOCMD = 0xff,
  CMD_ERR_TIMEOUT = 0xfe
} command_code;

static char get_keystroke(void);
static void parse_packet(uint8_t* p);
static void parse_command(command_code cmd, machine_state state);
static void display_help(void);

#endif /* _RAWHID2OSC_H */