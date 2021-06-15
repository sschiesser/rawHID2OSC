#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <conio.h>

#include "hid.h"
#include "lo/lo.h"
#include "rawHID2OSC.h"

const bool debug = true;
enum machine_state state = STATE_IDLE;

int main()
{
  int i, r, num;
  char c, buf[64];

  // C-based example is 16C0:0480:FFAB:0200
  r = rawhid_open(1, 0x16C0, 0x0480, 0xFFAB, 0x0200);
  if(r <= 0)
  {
    // Arduino-based example is 16C0:0486:FFAB:0200
    r = rawhid_open(1, 0x1C57, 0x1234, 0xFFAB, 0x0200);
    if(r <= 0)
    {
      printf("no rawhid device found\n");
      return -1;
    }
  }
  printf("found rawhid device\n");
  display_help();

  while(1)
  {
    // check if any Raw HID packet has arrived
    num = rawhid_recv(0, buf, 64, 220);
    if(num < 0)
    {
      printf("\nerror reading, device went offline\n");
      rawhid_close(0);
      return 0;
    }
    if(num > 0)
    {
      printf("\nrecv %d bytes:\n", num);
      parse_packet(buf);

      //for (i = 0; i < num; i++) {
      //	printf("%02X ", buf[i] & 255);
      //	if (i % 16 == 15 && i < num - 1) printf("\n");
      //}
      //printf("\n");
    }
    // check if any input on stdin
    while((c = get_keystroke()) >= 32)
    {
      printf("\ngot key '%c', sending...\n", c);
      buf[0] = c;
      for(i = 1; i < 64; i++)
      {
        buf[i] = 0;
      }
      rawhid_send(0, buf, 64, 100);
    }
  }
}


static char get_keystroke(void)
{
  if(_kbhit())
  {
    char c = _getch();
    if(c >= 32) return c;
  }
  return 0;
}


static void parse_packet(uint8_t* p)
{
  if(*p == 0xcc)
  {
    printf("Command! ");
    command_code c;
    c = (p + 1);
    parse_command(c);
  }
  else if(*p == 0x00)
  {
    uint8_t len = *(p + 1);
    printf("Measurement! p: 0x%02x, len: %d, p+len: 0x%02x\n", *p, len, *(p + len));
  }
  else
  {
    printf("Not recognized packet format!\n");
  }
}

static void parse_command(command_code cmd)
{
  printf("code: %c... ", cmd);
  switch(cmd)
  {
    case CMD_STRING_E:
      printf("E string!\n");
      break;

    case CMD_STRING_G:
      printf("G string!\n");
      break;

    case CMD_CALIB_RANGES:
      printf("RANGES calib!!\n");
      state = STATE_CALIB_RANGES;
      display_help();
      break;

    case CMD_CALIB_TOUCH:
      printf("TOUCH calib!!\n");
      state = STATE_CALIB_TOUCH;
      break;

    case CMD_MEASURE:
      printf("MEASUREMENT!!\n");
      state = STATE_MEASURING;
      break;

    case CMD_HELP:
      printf("HELP!!\n");
      display_help();
      break;

    case CMD_EXIT:
      printf("EXIT!!\n");
      state = STATE_IDLE;
      break;

    case CMD_ERR_NOCMD:
      printf("ERROR: no valid command!!\n");
      break;

    case CMD_ERR_TIMEOUT:
      printf("ERROR: timeout!!\n");
      break;

    default:
      printf("something else!!\n");
      state = STATE_IDLE;
      break;
  }
}


static void display_help()
{
  switch(state)
  {
    case STATE_MEASURING:
    case STATE_IDLE:
      printf("To navigate here:\n"
             "'r' : calibrate string RANGES\n"
             "'t' : calibrate TOUCH thresholds\n"
             "'m' : start MEASUREMENTS\n"
             "'h' : display HELP\n"
             "'x' : EXIT program\n");
      break;

    case STATE_CALIB_RANGES:
      printf("Ranges calibration:\n"
             "'e' : calibrate E string\n"
             "'g' : calibrate G string\n"
             "'v' : view current calibration values\n"
             "'h' : display this help\n"
             "'x' : exit to main menu\n");
      break;

    case STATE_CALIB_TOUCH:
      printf("Touch calibration:\n"
             "'e' : set touch threshold on E string\n"
             "'g' : set touch threshold on G string\n"
             "'v' : view current threshold values\n"
             "'h' : display this help\n"
             "'x' : exit to main menu\n");
      break;

    case STATE_ERROR:
    default:
      break;
  }
}