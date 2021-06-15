#ifndef _RAWHID2OSC_H
#define _RAWHID2OSC_H

static char get_keystroke (void);
static void parse_packet (uint8_t* p);
static void parse_command (uint8_t* c);
static void print_help (void);

enum machine_state
{
  STATE_IDLE,
  STATE_CALIBRATING,
  STATE_MEASURING,
  STATE_ERROR
};

#endif /* _RAWHID2OSC_H */