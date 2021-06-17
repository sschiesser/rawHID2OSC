#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <time.h>

#if defined(OS_LINUX) || defined(OS_MACOSX)
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(OS_WINDOWS)
//#include <profileapi.h>
#include <conio.h>
#endif

#include "hid.h"
#include "lo/lo.h"
#include "rawHID2OSC.h"

const bool debug = true;
machine_state current_state = STATE_IDLE;
bool device_open = false;
char sender_host[] = "127.0.0.1";
char sender_port[] = "19001";
lo_address addr;
uint32_t cur_time, prev_time;

int main()
{
  int dev, num_bytes;
  char c;
  uint8_t notif[64];

  printf("rawHID2OSC utility for the HAPTEEV e-violin experiment\n"
         "------------------------------------------------------\n"
         "Press 'o' to open the rawHID device, 'c' to close it\n");

  while(1)
  {
    if(device_open)
    {
      // check if any Raw HID packet has arrived
      num_bytes = rawhid_recv(0, notif, 64, 220);
      if(num_bytes < 0)
      {
        printf("\nerror reading, device went offline\n");
        rawhid_close(0);
        device_open = false;
        printf("Press 'o' to open the rawHID device, 'c' to close it\n");
      }

      if(num_bytes > 0)
      {
        parse_notification(notif);
      }

      while((c = get_keystroke()) >= 32)
      {
        parse_keystroke(c);
      }
    }
    else
    {
      while((c = get_keystroke()) >= 32)
      {
        if(c == 'o')
        {
          // C-based example is 16C0:0480:FFAB:0200
          dev = rawhid_open(1, 0x16C0, 0x0480, 0xFFAB, 0x0200);
          if(dev <= 0)
          {
            // Arduino-based example is 16C0:0486:FFAB:0200
            dev = rawhid_open(1, 0x1C57, 0x1234, 0xFFAB, 0x0200);
            if(dev <= 0)
            {
              printf("no rawhid device found\n");
            }
          }
          printf("found rawhid device\n");
          device_open = true;
          addr = lo_address_new(sender_host, sender_port);
          printf("Sending OSC to host %s on port %s\n", sender_host, sender_port);
          display_help();
        }
        else
        {
          printf("No device connected! Press 'o' to open\n");
        }
      }
    }
  }
}

static void parse_keystroke(char c1)
{
  uint8_t req[64] = {0};
  char c2;

  if(c1 == 'c')
  {
    rawhid_close(0);
    printf("rawHID device closed, press 'o' to open again\n");
    device_open = false;
  }
  else
  {
    req[0] = REQ_COMMAND;
    req[2] = (uint8_t)c1;
    switch(c1)
    {
      case REQ_HELP:
      {
        display_help();
        break;
      }

      case REQ_MEASURE:
      {
        req[1] = 2;
        req[3] = REQ_END;
        printf("MEASURE requested, sending: 0x%02x %d %d 0x%02x\n", req[0], req[1], req[2], req[3]);
        break;
      }

      case REQ_CALIB_RANGES:
      {
        printf("Calib RANGES requested, please choose a string: ");
        req[1] = 3;
        while((c2 = get_keystroke()) == 0)
          ;

        switch(c2)
        {
          case 'e':
            printf("got a 'e'\n");
            req[3] = REQ_STRING_E;
            break;

          case 'g':
            printf("got a 'g'\n");
            req[3] = REQ_STRING_G;
            break;

          case 'x':
            printf("got a 'x'... exiting\n");
            req[3] = REQ_STRING_NONE;
            break;

          default:
            printf("got something else\n");
            req[3] = REQ_STRING_NONE;
            break;
        }
        req[4] = REQ_END;
        printf("Calib RANGES requested, sending: 0x%02x %d %d 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
        break;
      }

      case REQ_CALIB_TOUCH:
      {
        printf("Calib TOUCH requested, please choose a string: ");
        req[1] = 3;
        while((c2 = get_keystroke()) == 0)
          ;

        switch(c2)
        {
          case 'e':
            printf("got a 'e'\n");
            req[3] = REQ_STRING_E;
            break;

          case 'g':
            printf("got a 'g'\n");
            req[3] = REQ_STRING_G;
            break;

          case 'x':
            printf("got a 'x'... exiting\n");
            req[3] = REQ_STRING_NONE;
            break;

          default:
            printf("got something else\n");
            req[3] = REQ_STRING_NONE;
            break;
        }

        req[4] = REQ_END;
        printf("Calib TOUCH requested, sending: 0x%02x %d %d 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
        break;
      }

      case REQ_VIEW:
      {
        display_calib_vals();
        break;
      }

      case REQ_EXIT:
      {
        req[1] = 2;
        req[req[1] + 1] = REQ_END;
        printf("EXIT requested, sending: 0x%02x %d %d 0x%02x\n", req[0], req[1], req[2], req[3]);
        break;
      }

      default:
        break;
    }
    if(rawhid_send(0, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(0);
      device_open = false;
    }
  }
}

static void parse_notification(uint8_t* p)
{
  uint8_t len = *(p + 1);
  current_state = *(p + len);
  //printf("Received (len = %d, state = 0x%02x): ", len, current_state);
  //for(uint8_t i = 0; i < (len + 2); i++)
  //{
  //  printf("0x%02x ", *(p + i));
  //}
  //printf("\n");
  if(*p == NOTIF_MEASUREMENT)
  {
    cur_time = get_ms();
    uint32_t delta_host = cur_time - prev_time;
    prev_time = cur_time;
    double delta_teensy = (double)((uint32_t)((p[2] << 8) | (p[3])) / 1.0);
    uint16_t gVal = (uint16_t)((uint16_t)(p[4] << 8) | (p[5]));
    uint16_t eVal = (uint16_t)((uint16_t)(p[6] << 8) | (p[7]));
    //printf("forwarding to %s on %s: %f\n", sender_host, sender_port, delta_ms);
    lo_send(addr, "/violin/systime", "fi", delta_teensy, delta_host);
    lo_send(addr, "/violin/pos/g", "i", gVal);
    lo_send(addr, "/violin/pos/e", "i", eVal);
  }
  //if(*p == (hid_notifications)MESS_COMMAND)
  //{
  //  printf("Command! ");
  //  hid_requests c;
  //  machine_state s;
  //  c = (hid_requests) * (p + 1);
  //  s = (machine_state) * (p + 2);
  //  parse_command(c, s);
  //}
  //else if(*p == (hid_notifications)MESS_CALIB_RANGES)
  //{
  //}
  //else if(*p == (hid_notifications)MESS_CALIB_TOUCH)
  //{
  //  printf("%c", *(p + 1));
  //  current_state = (machine_state) * (p + 2);
  //}
  //else if(*p == (hid_notifications)MESS_CALIB_TOUCH_DONE)
  //{
  //  uint16_t min, max, avg;
  //  min = (*(p + 1) << 8) | *(p + 2);
  //  max = (*(p + 3) << 8) | *(p + 4);
  //  avg = (*(p + 5) << 8) | *(p + 6);
  //  uint8_t ret = *(p + 7);
  //  printf("\nmin: %d, max: %d, avg: %d, ret: %d\n", min, max, avg, ret);
  //  display_help();
  //}
  //else if(*p == (hid_notifications)MESS_MEASURE)
  //{
  //  uint8_t len = *(p + 1);
  //  printf("Measurement! p: 0x%02x, len: %d, p+len: 0x%02x\n", *p, len, *(p + len));
  //}
  //else
  //{
  //  printf("Not recognized packet format!\n");
  //}
}

static void display_help()
{
  switch(current_state)
  {
    case STATE_IDLE:
    {
      printf("To navigate here:\n"
             "'r' : calibrate string RANGES\n"
             "'t' : calibrate TOUCH thresholds\n"
             "'m' : start MEASUREMENTS\n"
             "'h' : display HELP\n"
             "'x' : EXIT program\n");
      break;
    }

    default:
      break;
  }
}

static void display_calib_vals(void)
{
  printf("Some calib values should be displayed here...\n");
}

uint32_t get_ms(void)
{
  LARGE_INTEGER now;
  LARGE_INTEGER frequency;

  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&now);

  now.QuadPart *= 1000;
  return (now.QuadPart / frequency.QuadPart);
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
