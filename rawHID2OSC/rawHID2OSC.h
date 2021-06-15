#ifndef _RAWHID2OSC_H
#define _RAWHID2OSC_H

static char get_keystroke(void);
static void parse_packet(uint8_t* p);
static void parse_command(uint8_t* c);
static void display_help(void);

enum machine_state
{
  STATE_IDLE,
  STATE_CALIB_RANGES,
  STATE_CALIB_TOUCH,
  STATE_MEASURING,
  STATE_ERROR
};

typedef enum command_code_t
{
  STRING_E = 'e',
  STRING_G = 'g',
  STRING_D = 'd',
  STRING_A = 'a',
  CALIB_RANGES = 'r',
  CALIB_TOUCH = 't',
  MEASURE = 'm',
  HELP = 'h',
  EXIT = 'x',
  ERR_NOCMD = 0xff,
  ERR_TIMEOUT = 0xfe
} command_code;

#endif /* _RAWHID2OSC_H */