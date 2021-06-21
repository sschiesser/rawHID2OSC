#include "rawHID2OSC.h"

const bool debug = true;
machine_state current_state = STATE_IDLE;
bool device_open = false;
char sender_host[] = "127.0.0.1";
char sender_port[] = "19001";
lo_address addr;
uint32_t cur_time, prev_time;
lo_server_thread st;

int main()
{
  int dev, num_bytes;
  char c;
  uint8_t notif[64];

  printf("rawHID2OSC utility for the HAPTEEV e-violin experiment\n"
         "------------------------------------------------------\n"
         "Press 'o' to open the rawHID device, 'c' to close it\n");

  addr = lo_address_new(sender_host, sender_port);
  printf("Sending OSC to host %s on port %s\n", sender_host, sender_port);

  st = lo_server_thread_new("19002", lo_error);
  lo_server_thread_add_method(st, "/violin/calib/touch", "s", calib_touch_handler, NULL);
  lo_server_thread_add_method(st, "/violin/calib/range", "s", calib_range_handler, NULL);
  lo_server_thread_add_method(st, "/violin/measure", "i", measure_handler, NULL);
  printf("OSC server thread & method added on port 19002\n");

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

        lo_server_thread_stop(st);
        printf("OSC server thread stopped\n");
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

          lo_server_thread_start(st);
          printf("OSC server thread started\n");

          display_help();
        }
        else
        {
          printf("No device connected! Press 'o' to open\n");
        }
      }
    }
#if defined(OS_LINUX) || defined(OS_MACOSX)
    usleep(1000);
#elif defined(OS_WINDOWS)
    Sleep(1);
#endif
  }
}

static void parse_keystroke(char c1)
{
  uint8_t req[64] = {0};
  char c2;

  if(c1 == 'c')
  {
    req[0] = REQ_COMMAND;
    req[1] = 2;
    req[2] = REQ_EXIT;
    req[3] = REQ_END;
    rawhid_send(0, req, 64, 100);

    rawhid_close(0);
    device_open = false;
    printf("rawHID device closed, press 'o' to open again\n");

    lo_server_thread_stop(st);
    printf("OSC server thread stopped\n");
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

      case REQ_CALIB_RANGE:
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
        req[3] = REQ_END;
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
  printf("Received (len = %d, state = 0x%02x): ", len, current_state);
  for(uint8_t i = 0; i < (len + 2); i++)
  {
    printf("0x%02x ", *(p + i));
  }
  printf("\n");
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

void lo_error(int num, const char* msg, const char* path)
{
  printf("liblo server error %d in path %s: %s\n", num, path, msg);
  fflush(stdout);
}

int calib_touch_handler(const char* path, const char* types, lo_arg** argv,
                        int argc, void* data, void* user_data)
{
  static bool calibrating = false;
  uint8_t req[64];

  printf("TOUCH handler!\n");
  printf("%s <- c:%c\n", path, argv[0]->c);

  char cal_string = argv[0]->c;
  if(calibrating)
  {
    if(cal_string == REQ_EXIT)
    {
      req[0] = REQ_COMMAND;
      req[1] = 2;
      req[2] = REQ_EXIT;
      req[3] = REQ_END;
      calibrating = false;
    }
    else
      printf("Comand not valid!\n");
  }
  else
  {
    req[0] = REQ_COMMAND;
    req[1] = 3;
    req[2] = REQ_CALIB_TOUCH;
    req[4] = REQ_END;
    if((cal_string == REQ_STRING_E) || (cal_string == REQ_STRING_G))
      req[3] = cal_string;
    else
      req[3] = REQ_STRING_NONE;
    calibrating = true;
  }

  printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);

  if(rawhid_send(0, req, 64, 100) < 0)
  {
    printf("Error on sending packet, closing HID device\n");
    rawhid_close(0);
    device_open = false;
  }

  fflush(stdout);

  return 1;
}

int calib_range_handler(const char* path, const char* types, lo_arg** argv,
                        int argc, void* data, void* user_data)
{
  uint8_t req[64];

  printf("RANGE handler!\n");
  printf("%s <- c:%c\n", path, argv[0]->c);

  char cal_string = argv[0]->c;
  req[0] = REQ_COMMAND;
  req[1] = 3;
  req[2] = REQ_CALIB_RANGE;
  req[4] = REQ_END;
  if((cal_string == REQ_STRING_E) || (cal_string == REQ_STRING_G))
    req[3] = cal_string;
  else
    req[3] = REQ_STRING_NONE;

  printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);

  if(rawhid_send(0, req, 64, 100) < 0)
  {
    printf("Error on sending packet, closing HID device\n");
    rawhid_close(0);
    device_open = false;
  }

  fflush(stdout);

  return 1;
}

int measure_handler(const char* path, const char* types, lo_arg** argv,
                    int argc, void* data, void* user_data)
{
  static bool measuring = false;
  uint8_t req[64];
  bool rts = true;

  printf("MEASURE handler!\n");
  printf("%s <- i:%d\n", path, argv[0]->i);
  bool meas_cmd = (bool)argv[0]->i;

  req[0] = REQ_COMMAND;
  req[1] = 2;
  req[3] = REQ_END;
  if(!measuring && meas_cmd)
  {
    printf("Starting measurement!\n");
    req[2] = REQ_MEASURE;
    measuring = true;
  }
  else if(measuring && !meas_cmd)
  {
    printf("Stopping measurement!\n");
    req[2] = REQ_EXIT;
    measuring = false;
  }
  else
  {
    printf("Wrong command!\n");
    rts = false;
  }

  if(rts)
  {
    if(rawhid_send(0, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(0);
      device_open = false;
    }
  }


  //printf("GENERIC handler! path: <%s>\n", path);
  //for(i = 0; i < argc; i++)
  //{
  //  printf("arg %d '%c' ", i, types[i]);
  //  lo_arg_pp((lo_type)types[i], argv[i]);
  //  printf("\n");
  //}
  //printf("\n");
  fflush(stdout);

  return 1;
}
