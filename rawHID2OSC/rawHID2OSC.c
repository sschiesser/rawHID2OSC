#include "rawHID2OSC.h"

struct violin_s v = {
  .osc.s.host = OSC_SEND_HOST,
  .osc.s.port = OSC_SEND_PORT,
  .osc.s.n_meas_addr = OSC_NOTIF_MEAS,
  .osc.s.n_calib_t_addr = OSC_NOTIF_CALIB_T,
  .osc.s.n_calib_r_addr = OSC_NOTIF_CALIB_R,
  .osc.s.n_view_addr = OSC_NOTIF_VIEW,
  .osc.s.n_hid_addr = OSC_NOTIF_HID,
  .osc.s.n_app_addr = OSC_NOTIF_APP,
  .osc.r.port = OSC_RECV_PORT,
  .osc.r.r_meas_addr = OSC_REQ_MEAS,
  .osc.r.r_calib_t_addr = OSC_REQ_CALIB_T,
  .osc.r.r_calib_r_addr = OSC_REQ_CALIB_R,
  .osc.r.r_view_addr = OSC_REQ_VIEW,
  .osc.r.r_hid_addr = OSC_REQ_HID,
  .osc.r.r_app_addr = OSC_REQ_APP,
  .dev.vid = HID_VID,
  .dev.pid = HID_PID,
  .dev.page = HID_PAGE,
  .dev.usage = HID_USAGE,
  .dev.id = HID_DEV_NONE,
  .dev.open = false,
  .cal_st.e_str.calib_r.min = UINT16_MAX,
  .cal_st.e_str.calib_r.max = 0,
  .cal_st.e_str.calib_r.st = false,
  .cal_st.e_str.calib_t.min = UINT16_MAX,
  .cal_st.e_str.calib_t.max = 0,
  .cal_st.e_str.calib_t.avg = UINT16_MAX,
  .cal_st.e_str.calib_t.st = false,
  .cal_st.g_str.calib_r.min = UINT16_MAX,
  .cal_st.g_str.calib_r.max = 0,
  .cal_st.g_str.calib_r.st = false,
  .cal_st.g_str.calib_t.min = UINT16_MAX,
  .cal_st.g_str.calib_t.max = 0,
  .cal_st.g_str.calib_t.avg = UINT16_MAX,
  .cal_st.g_str.calib_t.st = false
};

const bool debug = true;
lo_address addr;
lo_server_thread st;
uint32_t cur_time, prev_time;
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
    if(v.dev.open)
    {
      num_bytes = rawhid_recv(v.dev.id, notif, 64, 220);
      if(num_bytes < 0)
      {
        printf("\nerror reading on device#%d, device went offline\n", v.dev.id);
        rawhid_close(v.dev.id);
        v.dev.open = false;
        v.dev.id--;
        printf("Press 'o' to open the rawHID device, 'c' to close it\n");
      }

      if(num_bytes > 0)
      {
        parse_hid_notif(notif);
      }
    }

    while((c = get_keystroke()) >= 32)
    {
      parse_keystroke(c, v.dev.open);
    }

#if defined(OS_LINUX) || defined(OS_MACOSX)
    usleep(1000);
#elif defined(OS_WINDOWS)
    Sleep(1);
#endif
  }

  return 0;
}



/******************************************************************************
* HID ZONE
*******************************************************************************/
static void parse_hid_notif(uint8_t* p)
{
  uint8_t len = p[1];
  v.cur_st = p[len];
  if(debug)
  {
    printf("Received (len = %d, state = 0x%02x): ", len, v.cur_st);
    for(uint8_t i = 0; i < (len + 2); i++)
    {
      printf("0x%02x ", p[i]);
    }
    printf("\n");
  }

  switch(p[0])
  {
    case N_MEAS:
    {
      cur_time = get_ms();
      uint32_t delta_host = cur_time - prev_time;
      prev_time = cur_time;
      uint32_t delta_teensy = ((uint32_t)((p[2] << 8) | (p[3])) / 1.0);
      uint16_t g = (uint16_t)((uint16_t)(p[4] << 8) | (p[5]));
      uint16_t e = (uint16_t)((uint16_t)(p[6] << 8) | (p[7]));
      //printf("forwarding to %s on %s: %d %d %d %d\n", v.osc.s.host, v.osc.s.port, delta_teensy, delta_host, e, g);
      lo_send(addr, v.osc.s.n_meas_addr, "iiii",
              delta_teensy, delta_host, e, g);
      break;
    }

    case N_INFO:
      parse_hid_n_info(&p[2], p[1]);
      break;

    case N_ACK:
      parse_hid_n_ack(&p[2], p[1]);
      break;

    default:
      break;
  }
}

static int parse_hid_n_info(uint8_t* p, uint8_t len)
{
  uint8_t str;
  uint16_t r_min, r_max, t_min, t_max, t_avg;
  bool t_cal, r_cal;

  switch(p[0])
  {
    case N_CALIB_R:
      if(len != 8) return -1;
      str = p[1];
      r_min = (p[2] << 8) | p[3];
      r_max = (p[4] << 8) | p[5];
      if(debug) printf("Sending %c 0x%02x 0x%02x\n",
                       (char)str, r_min, r_max);
      lo_send(addr, v.osc.s.n_calib_r_addr, "iii", str, r_min, r_max);
      break;

    case N_CALIB_T:
      if(len != 10) return -1;
      str = p[1];
      t_min = (p[2] << 8) | p[3];
      t_max = (p[4] << 8) | p[5];
      t_avg = (p[6] << 8) | p[7];
      if(debug) printf("Sending %c 0x%02x 0x%02x 0x%02x\n",
                       (char)str, t_min, t_max, t_avg);
      lo_send(addr, v.osc.s.n_calib_t_addr, "iiii", str, t_min, t_max, t_avg);
      break;

    case N_VIEW:
      if(len != 16) return -1;
      str = (char)p[1];
      t_min = (p[2] << 8) | p[3];
      t_max = (p[4] << 8) | p[5];
      t_avg = (p[6] << 8) | p[7];
      t_cal = (bool)p[8];
      r_min = (p[9] << 8) | p[10];
      r_max = (p[11] << 8) | p[12];
      r_cal = (bool)p[13];
      break;

    case N_ERR:
      if(len != 3) return -1;
      break;

    case N_TIMEOUT:
      if(len != 3) return -1;
      break;
  }

  //if(p[0] == N_CT_DONE)
  //{
  //  switch(str)
  //  {
  //    case 'E':
  //      v.cal_st.e_str.calib_t.min = min;
  //      v.cal_st.e_str.calib_t.max = max;
  //      v.cal_st.e_str.calib_t.avg = avg;
  //      v.cal_st.e_str.calib_t.st = cal;
  //      break;

  //    case 'G':
  //      v.cal_st.g_str.calib_t.min = min;
  //      v.cal_st.g_str.calib_t.max = max;
  //      v.cal_st.g_str.calib_t.avg = avg;
  //      v.cal_st.g_str.calib_t.st = cal;
  //      break;

  //    case 'A':
  //    case 'D':
  //    default:
  //      break;
  //  }
  //  if(debug) printf("Touch calib done on string %c, "
  //                   "results (min/max/avg): %d/%d/%d, quality: %d\n",
  //                   str, min, max, avg, cal);
  //}

  //if(p[0] == N_CR_DONE)
  //{
  //  char str = (char)p[2];
  //  uint16_t min = (p[3] << 8) | p[4];
  //  uint16_t max = (p[5] << 8) | p[6];
  //  uint8_t cal = p[7];
  //  switch(str)
  //  {
  //    case 'E':
  //      v.cal_st.e_str.calib_r.min = min;
  //      v.cal_st.e_str.calib_r.max = max;
  //      v.cal_st.e_str.calib_r.st = cal;
  //      break;

  //    case 'G':
  //      v.cal_st.g_str.calib_r.min = min;
  //      v.cal_st.g_str.calib_r.max = max;
  //      v.cal_st.g_str.calib_r.st = cal;
  //      break;

  //    case 'A':
  //    case 'D':
  //    default:
  //      break;
  //  }
  //  if(debug) printf("Range calib done on string %c,"
  //                   " results (min/max): %d/%d, quality: %d\n",
  //                   str, min, max, cal);
  //}
}

static int parse_hid_n_ack(uint8_t* p, uint8_t len)
{

}

/******************************************************************************
* OSC ZONE
*******************************************************************************/
int r_meas_handler(const char* path, const char* types, lo_arg** argv,
                   int argc, void* data, void* user_data)
{
  static bool measuring = false;
  uint8_t req[64];
  bool rts = true;

  if(debug)
  {
    printf("MEASURE handler!\n");
    printf("%s <- i:%d\n", path, argv[0]->i);
  }

  if(argc != OSC_REQ_ARGS) return -1;

  bool meas_cmd = (bool)argv[0]->i;

  req[0] = R_CMD;
  req[1] = 2;
  req[3] = R_END;
  if(!measuring && meas_cmd)
  {
    if(debug) printf("Starting measurement!\n");
    req[2] = R_MEAS;
    measuring = true;
  }
  else if(measuring && !meas_cmd)
  {
    if(debug) printf("Stopping measurement!\n");
    req[2] = R_EXIT;
    measuring = false;
  }
  else
  {
    if(debug) printf("Wrong command!\n");
    rts = false;
  }

  if(rts)
  {
    if(rawhid_send(v.dev.id, req, 64, 100) < 0)
    {
      if(debug) printf("Error on sending packet, closing HID device\n");
      rawhid_close(v.dev.id);
      v.dev.open = false;
      v.dev.id--;
    }
  }

  fflush(stdout);

  return 0;
}

int r_calib_t_handler(const char* path, const char* types, lo_arg** argv,
                      int argc, void* data, void* user_data)
{
  static bool calibrating = false;
  uint8_t req[64];
  bool rts = true;

  if(debug)
  {
    printf("TOUCH handler!\n");
    printf("%s <- c:%c\n", path, argv[0]->c);
  }
  if(argc != OSC_REQ_ARGS) return -1;


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
    else if(debug)
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
    if(debug) printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                     req[0], req[1], req[2], req[3], req[4]);

    if(rawhid_send(v.dev.id, req, 64, 100) < 0)
    {
      if(debug) printf("Error on sending packet, closing HID device\n");

      rawhid_close(v.dev.id);
      v.dev.open = false;
      v.dev.id--;
    }
  }

  fflush(stdout);

  return 1;
}

int r_calib_r_handler(const char* path, const char* types, lo_arg** argv,
                      int argc, void* data, void* user_data)
{
  static bool calibrating = false;
  uint8_t req[64];
  bool rts = true;

  if(debug)
  {
    printf("RANGE handler!\n");
    printf("%s <- c:%c\n", path, argv[0]->c);
  }

  if(argc != OSC_REQ_ARGS) return -1;

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
    if(debug) printf("Sending 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", req[0], req[1], req[2], req[3], req[4]);

    if(rawhid_send(v.dev.id, req, 64, 100) < 0)
    {
      if(debug) printf("Error on sending packet, closing HID device\n");

      rawhid_close(v.dev.id);
      v.dev.open = false;
      v.dev.id--;
    }
  }

  fflush(stdout);

  return 1;
}

int r_view_handler(const char* path, const char* types, lo_arg** argv,
                   int argc, void* data, void* user_data)
{
  if(argc != OSC_REQ_ARGS) return -1;

  if(strcmp((const char*)argv[0], "r") == 0)
  {
    if(debug) printf("Viewing range values\n");
  }
  else if(strcmp((const char*)argv[0], "t") == 0)
  {
    if(debug) printf("Viewing touch values\n");
  }
  else if(strcmp((const char*)argv[0], "all") == 0)
  {
    if(debug) printf("Viewing all values\n");
  }

  return 0;
}

int r_hid_handler(const char* path, const char* types, lo_arg** argv,
                  int argc, void* data, void* user_data)
{
  if(argc != OSC_REQ_ARGS) return -1;

  if((strcmp((const char*)argv[0], "open") == 0) && !v.dev.open)
  {
    printf("Opening HID device!\n");
    int8_t dev_nb = rawhid_open(1, v.dev.vid, v.dev.pid,
                                v.dev.page, v.dev.usage);
    if(dev_nb == 1)
    {
      v.dev.id++;
      printf("found rawhid device, dev#%d\n", v.dev.id);
      v.dev.open = true;
      display_help();
    }
    else
    {
      printf("No or too many rawhid devices found\n");
    }
  }
  else if((strcmp((const char*)argv[0], "close") == 0) && v.dev.open)
  {
    if(debug) printf("Closing HID device!\n");
    v.dev.open = false;
    v.dev.id--;
  }
  else if(strcmp((const char*)argv[0], "state") == 0)
  {
    if(debug) printf("Sending hid state\n");
    lo_send(addr, v.osc.s.n_hid_addr, "i", v.dev.open);
  }
  else
  {
    if(debug) printf("Bad HID command\n");
  }

  return 0;
}

int r_app_handler(const char* path, const char* types, lo_arg** argv,
                  int argc, void* data, void* user_data)
{
  if(argc != OSC_REQ_ARGS) return -1;

  if(strcmp((const char*)argv[0], "close") == 0)
  {
    if(debug) printf("Stopping app\n");
    app_running = false;
  }
  else if(strcmp((const char*)argv[0], "state") == 0)
  {
    if(debug) printf("Sending app state\n");
    lo_send(addr, v.osc.s.n_app_addr, "i", 1);
  }
  else
  {
    if(debug) printf("Bad 'app' command\n");
  }
  return 0;
}

void lo_error(int num, const char* msg, const char* path)
{
  printf("liblo server error %d in path %s: %s\n", num, path, msg);
  fflush(stdout);
}



static void display_calib_vals(void)
{
  uint16_t v1 = v.cal_st.e_str.calib_t.min;
  uint16_t v2 = v.cal_st.e_str.calib_t.max;
  uint16_t v3 = v.cal_st.e_str.calib_t.avg;
  bool v4 = v.cal_st.e_str.calib_t.st;
  uint16_t v5 = v.cal_st.e_str.calib_r.min;
  uint16_t v6 = v.cal_st.e_str.calib_r.max;
  bool v7 = v.cal_st.e_str.calib_r.st;
  if(debug) printf("Some calib values should be displayed here... "
                   "%d %d %d %d %d %d %d\n",
                   v1, v2, v3, v4, v5, v6, v7);

  lo_send(addr, "/violin/calib/e", "iiiiiii", v1, v2, v3, v4, v5, v6, v7);
}


/******************************************************************************
* UI ZONE
*******************************************************************************/
static void parse_keystroke(char c1, bool dev_open)
{
  uint8_t req[64] = {0};
  char c2;

  if(!dev_open)
  {
    if(c1 == 'o')
    {
      int8_t dev_nb = rawhid_open(1, v.dev.vid, v.dev.pid,
                                  v.dev.page, v.dev.usage);

      if(dev_nb == 1)
      {
        v.dev.id++;
        printf("found rawhid device, dev#%d\n", v.dev.id);
        v.dev.open = true;
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
        rawhid_send(v.dev.id, req, 64, 100);

        rawhid_close(v.dev.id);
        v.dev.open = false;
        v.dev.id--;
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
      if(rawhid_send(v.dev.id, req, 64, 100) < 0)
      {
        printf("Error on sending packet to device #%d, closing HID device\n", v.dev.id);
        rawhid_close(v.dev.id);
        v.dev.open = false;
        v.dev.id--;
      }
    }
  }
}

static void display_help()
{
  switch(v.cur_st)
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

static char get_keystroke(void)
{
  if(_kbhit())
  {
    char c = _getch();
    if(c >= 32) return c;
  }
  return 0;
}


/******************************************************************************
* UTILS ZONE
*******************************************************************************/
uint32_t get_ms(void)
{
  LARGE_INTEGER now;
  LARGE_INTEGER frequency;

  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&now);

  now.QuadPart *= 1000;
  return (now.QuadPart / frequency.QuadPart);
}

void init()
{}

void startup(void)
{
  addr = lo_address_new(v.osc.s.host, v.osc.s.port);
  printf("Sending OSC to host %s on port %s\n", v.osc.s.host, v.osc.s.port);

  st = lo_server_thread_new(v.osc.r.port, lo_error);
  lo_server_thread_add_method(st, v.osc.r.r_meas_addr,
                              "i", r_meas_handler, NULL);
  lo_server_thread_add_method(st, v.osc.r.r_calib_t_addr,
                              "s", r_calib_t_handler, NULL);
  lo_server_thread_add_method(st, v.osc.r.r_calib_r_addr,
                              "s", r_calib_r_handler, NULL);
  lo_server_thread_add_method(st, v.osc.r.r_view_addr,
                              "s", r_view_handler, NULL);
  lo_server_thread_add_method(st, v.osc.r.r_hid_addr,
                              "s", r_hid_handler, NULL);
  lo_server_thread_add_method(st, v.osc.r.r_app_addr,
                              "s", r_app_handler, NULL);
  printf("OSC server thread & method added on port 19002\n");

  lo_server_thread_start(st);
  printf("OSC server thread started\n");

  app_running = true;
}