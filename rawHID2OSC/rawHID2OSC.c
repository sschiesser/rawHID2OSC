#include "rawHID2OSC.h"

const bool debug = true;
bool device_open = false;
//char sender_host[] = "127.0.0.1";
//char sender_port[] = "19001";
lo_address addr;
uint32_t cur_time, prev_time;
lo_server_thread st;
struct violin_s violin;

int main()
{
  int num_bytes;
  char c;
  uint8_t notif[64];

  printf("rawHID2OSC utility for the HAPTEEV e-violin experiment\n"
         "------------------------------------------------------\n"
         "Press 'o' to open the rawHID device, 'c' to close it\n");

  init();

  addr = lo_address_new(violin.osc.sender.host, violin.osc.sender.port);
  printf("Sending OSC to host %s on port %s\n", violin.osc.sender.host, violin.osc.sender.port);

  st = lo_server_thread_new(violin.osc.receiver.port, lo_error);
  lo_server_thread_add_method(st, violin.osc.receiver.cal_t_addr, "s", calib_touch_handler, NULL);
  lo_server_thread_add_method(st, violin.osc.receiver.cal_r_addr, "s", calib_range_handler, NULL);
  lo_server_thread_add_method(st, violin.osc.receiver.meas_addr, "i", measure_handler, NULL);
  lo_server_thread_add_method(st, violin.osc.receiver.cmd_addr, NULL, command_handler, NULL);
  printf("OSC server thread & method added on port 19002\n");

  lo_server_thread_start(st);
  printf("OSC server thread started\n");

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

        //lo_server_thread_stop(st);
        //printf("OSC server thread stopped\n");
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
          violin.device.dev_id = rawhid_open(1, violin.device.vid, violin.device.pid,
                                             violin.device.page, violin.device.usage);
          if(violin.device.dev_id <= 0)
          {
            printf("no rawhid device found\n");
          }
          printf("found rawhid device\n");
          device_open = true;
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

    //lo_server_thread_stop(st);
    //printf("OSC server thread stopped\n");
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
  violin.cur_state = *(p + len);
  printf("Received (len = %d, state = 0x%02x): ", len, violin.cur_state);
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
    lo_send(addr, violin.osc.sender.systime_addr, "fi", delta_teensy, delta_host);
    lo_send(addr, "/violin/pos/g", "i", gVal);
    lo_send(addr, "/violin/pos/e", "i", eVal);
  }
}

static void display_help()
{
  switch(violin.cur_state)
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
  bool rts = true;

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
    req[3] = cal_string;
    req[4] = REQ_END;
    calibrating = true;

    if((cal_string != REQ_STRING_E) && (cal_string != REQ_STRING_G))
    {
      rts = false;
      calibrating = false;
    }
  }

  if(rts)
  {
    printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
    if(rawhid_send(0, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(0);
      device_open = false;
    }
  }

  fflush(stdout);

  return 1;
}

int calib_range_handler(const char* path, const char* types, lo_arg** argv,
                        int argc, void* data, void* user_data)
{
  static bool calibrating = false;
  uint8_t req[64];
  bool rts = true;

  printf("RANGE handler!\n");
  printf("%s <- c:%c\n", path, argv[0]->c);

  char cal_string = argv[0]->c;
  req[0] = REQ_COMMAND;
  req[1] = 3;
  req[2] = REQ_CALIB_RANGE;
  req[3] = cal_string;
  req[4] = REQ_END;

  if((cal_string != REQ_STRING_E) && (cal_string != REQ_STRING_G))
    rts = false;

  if(rts)
  {
    printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
    if(rawhid_send(0, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(0);
      device_open = false;
    }
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

int command_handler(const char* path, const char* types, lo_arg** argv,
                    int argc, void* data, void* user_data)
{
  if(strcmp(argv[0], "hid") == 0)
  {
    if((strcmp(argv[1], "open") == 0) && !device_open)
    {
      printf("Opening HID device!\n");
      violin.device.dev_id = rawhid_open(1, violin.device.vid, violin.device.pid,
                                         violin.device.page, violin.device.usage);
      if(violin.device.dev_id <= 0)
      {
        printf("no rawhid device found\n");
      }
      printf("found rawhid device\n");
      device_open = true;
      display_help();
    }
    else if((strcmp(argv[1], "close") == 0) && device_open)
    {
      printf("Closing HID device!\n");
      device_open = false;
    }
    else
    {
      printf("Bad HID command\n");
    }
  }

}

void init()
{
  char sh[] = "127.0.0.1";
  char sp[] = "19001";
  char ssyst[] = "/violin/systime";
  char spos[] = "/violin/pos";
  char rp[] = "19002";
  char rct[] = "/violin/calib/touch";
  char rcr[] = "/violin/calib/range";
  char rm[] = "/violin/measure";
  char rcmd[] = "/violin/command";

  violin.device.vid = 0x1C57;
  violin.device.pid = 0x1234;
  violin.device.page = 0xFFAB;
  violin.device.usage = 0x0200;

  violin.cur_state = STATE_IDLE;

  memcpy(violin.osc.sender.host, sh, sizeof(sh));
  memcpy(violin.osc.sender.port, sp, sizeof(sp));
  memcpy(violin.osc.sender.systime_addr, ssyst, sizeof(ssyst));
  memcpy(violin.osc.sender.position_addr, spos, sizeof(spos));

  memcpy(violin.osc.receiver.port, rp, sizeof(rp));
  memcpy(violin.osc.receiver.cal_t_addr, rct, sizeof(rct));
  memcpy(violin.osc.receiver.cal_r_addr, rcr, sizeof(rcr));
  memcpy(violin.osc.receiver.meas_addr, rm, sizeof(rm));
  memcpy(violin.osc.receiver.cmd_addr, rcmd, sizeof(rcmd));
}
