#include "rawHID2OSC.h"

const bool debug = true;
lo_address addr;
lo_server_thread st;
uint32_t cur_time, prev_time;
struct violin_s violin;
bool app_running;

int main()
{
  int num_bytes;
  char c;
  uint8_t notif[64];

  printf("rawHID2OSC utility for the HAPTEEV e-violin experiment\n"
         "------------------------------------------------------\n"
         "Press 'o' to open the rawHID device, 'c' to close it\n"
         "!!          or use the max OSC interface          !!\n");

  init(); // Initialize variables related to the HID device
  startup(); // Startup threads

  while(app_running)
  {
    if(violin.device.open)
    {
      num_bytes = rawhid_recv(violin.device.dev_id, notif, 64, 220);
      if(num_bytes < 0)
      {
        printf("\nerror reading on device#%d, device went offline\n", violin.device.dev_id);
        rawhid_close(violin.device.dev_id);
        violin.device.open = false;
        violin.device.dev_id--;
        printf("Press 'o' to open the rawHID device, 'c' to close it\n");
      }

      if(num_bytes > 0)
      {
        parse_notification(notif);
      }
    }

    while((c = get_keystroke()) >= 32)
    {
      parse_keystroke(c, violin.device.open);
    }

#if defined(OS_LINUX) || defined(OS_MACOSX)
    usleep(1000);
#elif defined(OS_WINDOWS)
    Sleep(1);
#endif
  }

  return 0;
}

static void parse_keystroke(char c1, bool dev_open)
{
  uint8_t req[64] = {0};
  char c2;

  if(!dev_open)
  {
    if(c1 == 'o')
    {
      int8_t dev_nb = rawhid_open(1, violin.device.vid, violin.device.pid,
                                  violin.device.page, violin.device.usage);

      if(dev_nb == 1)
      {
        violin.device.dev_id++;
        printf("found rawhid device, dev#%d\n", violin.device.dev_id);
        violin.device.open = true;
        display_help();
      }
      else
      {
        printf("No or too many rawhid devices found\n");
      }
    }
    else
    {
      printf("Bad command! Open HID device first.\n");
    }
  }
  else
  {
    bool rts = true;

    req[0] = R_CMD;
    req[2] = (uint8_t)c1;
    switch(c1)
    {
      case 'c':
      {
        req[1] = 2;
        req[2] = R_EXIT;
        req[3] = R_END;
        rawhid_send(violin.device.dev_id, req, 64, 100);

        rawhid_close(violin.device.dev_id);
        violin.device.open = false;
        violin.device.dev_id--;
        rts = false;
        printf("rawHID device closed, press 'o' to open again\n");

        break;
      }

      case R_HELP:
      {
        display_help();
        break;
      }

      case R_MEAS:
      {
        req[1] = 2;
        req[3] = R_END;
        printf("MEASURE requested, sending: 0x%02x %d %d 0x%02x\n", req[0], req[1], req[2], req[3]);
        break;
      }

      case R_CALIB_R:
      {
        printf("Calib RANGES requested, please choose a string: ");
        req[1] = 3;
        while((c2 = get_keystroke()) == 0)
          ;

        switch(c2)
        {
          case 'e':
            printf("got a 'e'\n");
            req[3] = R_STR_E;
            break;

          case 'g':
            printf("got a 'g'\n");
            req[3] = R_STR_G;
            break;

          case 'x':
            printf("got a 'x'... exiting\n");
            req[3] = R_STR_A;
            break;

          default:
            printf("got something else\n");
            req[3] = R_STR_A;
            break;
        }
        req[4] = R_END;
        printf("Calib RANGES requested, sending: 0x%02x %d %d 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
        break;
      }

      case R_CALIB_T:
      {
        printf("Calib TOUCH requested, please choose a string: ");
        req[1] = 3;
        while((c2 = get_keystroke()) == 0)
          ;

        switch(c2)
        {
          case 'e':
            printf("got a 'e'\n");
            req[3] = R_STR_E;
            break;

          case 'g':
            printf("got a 'g'\n");
            req[3] = R_STR_G;
            break;

          case 'x':
            printf("got a 'x'... exiting\n");
            req[3] = R_STR_A;
            break;

          default:
            printf("got something else\n");
            req[3] = R_STR_A;
            break;
        }

        req[4] = R_END;
        printf("Calib TOUCH requested, sending: 0x%02x %d %d 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
        break;
      }

      case R_VIEW:
      {
        display_calib_vals();
        break;
      }

      case R_EXIT:
      {
        req[1] = 2;
        req[3] = R_END;
        printf("EXIT requested, sending: 0x%02x %d %d 0x%02x\n", req[0], req[1], req[2], req[3]);
        break;
      }

      default:
        rts = false;
        break;
    }

    if(rts)
    {
      if(rawhid_send(violin.device.dev_id, req, 64, 100) < 0)
      {
        printf("Error on sending packet to device #%d, closing HID device\n", violin.device.dev_id);
        rawhid_close(violin.device.dev_id);
        violin.device.open = false;
        violin.device.dev_id--;
      }
    }
  }
}

static void parse_notification(uint8_t* p)
{
  uint8_t len = p[1];
  violin.cur_state = p[len];
  printf("Received (len = %d, state = 0x%02x): ", len, violin.cur_state);
  for(uint8_t i = 0; i < (len + 2); i++)
  {
    printf("0x%02x ", p[i]);
  }
  printf("\n");
  if(p[0] == N_MEAS)
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
  if(p[0] == N_CT_DONE)
  {
    char str = (p[2] == N_STR_E) ? 'E' : 'G';
    uint16_t min = (p[3] << 8) | p[4];
    uint16_t max = (p[5] << 8) | p[6];
    uint16_t avg = (p[7] << 8) | p[8];
    uint8_t cal = p[9];
    printf("Touch calib done on string %c, results (min/max/avg): %d/%d/%d, quality: %d\n",
           str, min, max, avg, cal);

    if(str == 'E')
    {
      violin.cal_state.e_str.cal_touch.min = min;
      violin.cal_state.e_str.cal_touch.max = max;
      violin.cal_state.e_str.cal_touch.avg = avg;
      violin.cal_state.e_str.cal_touch.status = cal;
    }
    if(str == 'G')
    {
      violin.cal_state.g_str.cal_touch.min = min;
      violin.cal_state.g_str.cal_touch.max = max;
      violin.cal_state.g_str.cal_touch.avg = avg;
      violin.cal_state.g_str.cal_touch.status = cal;
    }
  }
  if(p[0] == N_CR_DONE)
  {
    char str = (p[2] == N_STR_E) ? 'E' : 'G';
    uint16_t min = (p[3] << 8) | p[4];
    uint16_t max = (p[5] << 8) | p[6];
    uint8_t cal = p[7];
    printf("Range calib done on string %c, results (min/max): %d/%d, quality: %d\n", str, min, max, cal);

    if(str == 'E')
    {
      violin.cal_state.e_str.cal_range.min = min;
      violin.cal_state.e_str.cal_range.max = max;
      violin.cal_state.e_str.cal_range.status = cal;
    }
    if(str == 'G')
    {
      violin.cal_state.g_str.cal_range.min = min;
      violin.cal_state.g_str.cal_range.max = max;
      violin.cal_state.g_str.cal_range.status = cal;
    }
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
  uint16_t v1 = violin.cal_state.e_str.cal_touch.min;
  uint16_t v2 = violin.cal_state.e_str.cal_touch.max;
  uint16_t v3 = violin.cal_state.e_str.cal_touch.avg;
  bool v4 = violin.cal_state.e_str.cal_touch.status;
  uint16_t v5 = violin.cal_state.e_str.cal_range.min;
  uint16_t v6 = violin.cal_state.e_str.cal_range.max;
  bool v7 = violin.cal_state.e_str.cal_range.status;
  printf("Some calib values should be displayed here... %d %d %d %d %d %d %d \n", v1, v2, v3, v4, v5, v6, v7);

  lo_send(addr, "/violin/calib/e", "iiiiiii", v1, v2, v3, v4, v5, v6, v7);
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
    if(cal_string == R_EXIT)
    {
      req[0] = R_CMD;
      req[1] = 2;
      req[2] = R_EXIT;
      req[3] = R_END;
      calibrating = false;
    }
    else
      printf("Comand not valid!\n");
  }
  else
  {
    req[0] = R_CMD;
    req[1] = 3;
    req[2] = R_CALIB_T;
    req[3] = cal_string;
    req[4] = R_END;
    calibrating = true;

    if((cal_string != R_STR_E) && (cal_string != R_STR_G))
    {
      rts = false;
      calibrating = false;
    }
  }

  if(rts)
  {
    printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
    if(rawhid_send(violin.device.dev_id, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(violin.device.dev_id);
      violin.device.open = false;
      violin.device.dev_id--;
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
  req[0] = R_CMD;
  req[1] = 3;
  req[2] = R_CALIB_R;
  req[3] = cal_string;
  req[4] = R_END;

  if((cal_string != R_STR_E) && (cal_string != R_STR_G))
    rts = false;

  if(rts)
  {
    printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);
    if(rawhid_send(violin.device.dev_id, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(violin.device.dev_id);
      violin.device.open = false;
      violin.device.dev_id--;
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

  req[0] = R_CMD;
  req[1] = 2;
  req[3] = R_END;
  if(!measuring && meas_cmd)
  {
    printf("Starting measurement!\n");
    req[2] = R_MEAS;
    measuring = true;
  }
  else if(measuring && !meas_cmd)
  {
    printf("Stopping measurement!\n");
    req[2] = R_EXIT;
    measuring = false;
  }
  else
  {
    printf("Wrong command!\n");
    rts = false;
  }

  if(rts)
  {
    if(rawhid_send(violin.device.dev_id, req, 64, 100) < 0)
    {
      printf("Error on sending packet, closing HID device\n");
      rawhid_close(violin.device.dev_id);
      violin.device.open = false;
      violin.device.dev_id--;
    }
  }

  fflush(stdout);

  return 1;
}

int command_handler(const char* path, const char* types, lo_arg** argv,
                    int argc, void* data, void* user_data)
{
  if(strcmp((const char*)argv[0], "hid") == 0)
  {
    if((strcmp((const char*)argv[1], "open") == 0) && !violin.device.open)
    {
      printf("Opening HID device!\n");
      int8_t dev_nb = rawhid_open(1, violin.device.vid, violin.device.pid,
                                  violin.device.page, violin.device.usage);
      if(dev_nb == 1)
      {
        violin.device.dev_id++;
        printf("found rawhid device, dev#%d\n", violin.device.dev_id);
        violin.device.open = true;
        display_help();
      }
      else
      {
        printf("No or too many rawhid devices found\n");
      }
    }
    else if((strcmp((const char*)argv[1], "close") == 0) && violin.device.open)
    {
      printf("Closing HID device!\n");
      violin.device.open = false;
      violin.device.dev_id--;
    }
    else
    {
      printf("Bad HID command\n");
    }
  }

  if(strcmp((const char*)argv[0], "app") == 0)
  {
    if(strcmp((const char*)argv[1], "close") == 0)
    {
      printf("Stopping app\n");
      app_running = false;
    }
    else
    {
      printf("Bad 'app' command\n");
    }
  }

  if(strcmp((const char*)argv[0], "calib") == 0)
  {
    display_calib_vals();
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

  violin.device.dev_id = -1;
  violin.device.vid = 0x1C57;
  violin.device.pid = 0x1234;
  violin.device.page = 0xFFAB;
  violin.device.usage = 0x0200;
  violin.device.open = false;

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

void startup(void)
{
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

  app_running = true;
}