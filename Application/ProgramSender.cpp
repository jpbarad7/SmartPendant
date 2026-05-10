//******************************************************************************
//  @file ProgramSender.cpp
//  @author Nicolai Shlapunov / JPB Laser modifications
//
//  @details ProgramSender: G-code file sender.
//           Full-screen self-contained layout matching other screens.
//           Tap text area to open file browser.
//
//******************************************************************************

#include "ProgramSender.h"
#include "Application.h"

#include "fatfs.h"
#include <cctype>

// *****************************************************************************
ProgramSender& ProgramSender::GetInstance()
{
  static ProgramSender pgmsenderscr;
  return pgmsenderscr;
}

// *****************************************************************************
// ***   Setup   ***************************************************************
// *****************************************************************************
Result ProgramSender::Setup(int32_t y, int32_t height)
{
  int32_t  scr_w         = display_drv.GetScreenW(); // 320
  int32_t  scr_h         = display_drv.GetScreenH(); // 480
  uint32_t window_height = Font_8x12::GetInstance().GetCharH() * 5u; // 60px

  // *** Nav bar — same centering as other screens ***
  int32_t total_h = 7 * (int32_t)window_height + 7 * BORDER_W + BORDER_W * 2; // 456
  int32_t nav_y   = (scr_h - total_h) / 2; // 12
  int32_t nav_end = nav_y + (int32_t)window_height; // 72

  int32_t dro_start_x = scr_w / 6;
  int32_t dro_end_x   = dro_start_x + (scr_w - BORDER_W * 2) * 4 / 6;
  int32_t arrow_w     = scr_w - dro_end_x - BORDER_W * 2; // 51px

  prev_btn.SetParams("<", BORDER_W, nav_y, arrow_w, window_height, true);
  prev_btn.SetCallback(AppTask::GetCurrent());

  next_btn.SetParams(">", scr_w - BORDER_W - arrow_w, nav_y, arrow_w, window_height, true);
  next_btn.SetCallback(AppTask::GetCurrent());

  title_str.SetParams("GCODE SENDER", 0, 0, COLOR_WHITE, Font_12x16::GetInstance());
  title_str.Move((scr_w - title_str.GetWidth()) / 2,
                 nav_y + ((int32_t)window_height - Font_12x16::GetInstance().GetCharH()) / 2);

  // *** Bottom section — same as home/override ***
  int32_t run_stop_y = nav_y + total_h - (int32_t)window_height;      // 416
  int32_t aux_y      = run_stop_y - BORDER_W - (int32_t)window_height; // 352
  int32_t status_y   = aux_y - BORDER_W - (int32_t)window_height;      // 288

  // Status strings
  hdr_state.SetParams("-----", BORDER_W,
                      status_y + ((int32_t)window_height - Font_12x16::GetInstance().GetCharH()) / 2,
                      COLOR_WHITE, Font_12x16::GetInstance());
  hdr_status_sub.SetParams("", BORDER_W,
                            status_y + (int32_t)window_height - Font_8x12::GetInstance().GetCharH() - BORDER_W / 2,
                            COLOR_WHITE, Font_8x12::GetInstance());

  // Aux row: AIR | EXHAUST | FIRE | MPG
  uint32_t aux_btn_w = ((uint32_t)scr_w - BORDER_W * 5u) / 4u; // 75px

  flood_btn.SetParams("AIR",
                      BORDER_W, aux_y, aux_btn_w, window_height, true);
  flood_btn.SetCallback(AppTask::GetCurrent());

  mist_btn.SetParams("EXHAUST",
                     BORDER_W + (int32_t)aux_btn_w + BORDER_W, aux_y, aux_btn_w, window_height, true);
  mist_btn.SetCallback(AppTask::GetCurrent());

  fire_pgm_btn.SetParams("FIRE",
                         BORDER_W + 2*((int32_t)aux_btn_w + BORDER_W), aux_y, aux_btn_w, window_height, true);
  fire_pgm_btn.SetCallback(AppTask::GetCurrent());

  mpg_pgm_btn.SetParams("MPG",
                        BORDER_W + 3*((int32_t)aux_btn_w + BORDER_W), aux_y, aux_btn_w, window_height, true);
  mpg_pgm_btn.SetCallback(AppTask::GetCurrent());

  // Run / Stop
  int32_t run_stop_w = 2 * (int32_t)aux_btn_w + BORDER_W; // 154px

  run_btn.SetParams("RUN", BORDER_W, run_stop_y, run_stop_w, window_height, true);
  run_btn.SetFont(Font_12x16::GetInstance());
  run_btn.SetCallback(AppTask::GetCurrent());

  stop_btn.SetParams("STOP", BORDER_W + run_stop_w + BORDER_W, run_stop_y,
                     run_stop_w, window_height, true);
  stop_btn.SetFont(Font_12x16::GetInstance());
  stop_btn.SetCallback(AppTask::GetCurrent());

  // *** Text box and file browser fill space between nav and status ***
  int32_t content_y = nav_end + BORDER_W;
  int32_t content_h = status_y - BORDER_W - content_y;

  menu.SetCallback(AppTask::GetCurrent(), this,
                   reinterpret_cast<CallbackPtr>(ProcessMenuOkCallback),
                   reinterpret_cast<CallbackPtr>(ProcessMenuCancelCallback));
  menu.Setup(menu_items, NumberOf(menu_items), 0, content_y, scr_w, content_h);

  // Fill menu_items
  for(uint32_t i = 0u; i < NumberOf(menu_items); i++)
  {
    menu_items[i].text = str[i];
    menu_items[i].n    = sizeof(str[i]);
  }

  text_box.Setup(0, content_y, scr_w, content_h);

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Show   ****************************************************************
// *****************************************************************************
Result ProgramSender::Show()
{
  Application::GetInstance().HideGlobalUI();

  InputDrv::GetInstance().AddEncoderCallbackHandler(AppTask::GetCurrent(),
    reinterpret_cast<CallbackPtr>(ProcessEncoderCallback), this, enc_cble);

  Application::GetInstance().ShowMemoryInfo();

  prev_btn.Show(100);
  next_btn.Show(100);
  title_str.Show(101);

  text_box.SetText(p_text);
  text_box.Show(100);

  hdr_state.Show(101);
  hdr_status_sub.Show(101);

  flood_btn.Show(100);
  mist_btn.Show(100);
  fire_pgm_btn.SetString("FIRE");
  fire_pgm_btn.SetColor(COLOR_WHITE);
  fire_active = false;
  fire_pgm_btn.Show(100);
  mpg_pgm_btn.Show(100);

  run_btn.Show(100);
  stop_btn.Show(100);

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Hide   ****************************************************************
// *****************************************************************************
Result ProgramSender::Hide()
{
  InputDrv::GetInstance().DeleteEncoderCallbackHandler(enc_cble);

  menu.Hide();
  text_box.Hide();

  if(p_text == nullptr) f_close(&SDFile);

  prev_btn.Hide();
  next_btn.Hide();
  title_str.Hide();

  hdr_state.Hide();
  hdr_status_sub.Hide();

  flood_btn.Hide();
  mist_btn.Hide();
  fire_pgm_btn.Hide();
  mpg_pgm_btn.Hide();

  run_btn.Hide();
  stop_btn.Hide();

  Application::GetInstance().ShowGlobalUI();

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   TimerExpired   ********************************************************
// *****************************************************************************
Result ProgramSender::TimerExpired(uint32_t interval)
{
  // Status strings centered
  int32_t scr_w = display_drv.GetScreenW();
  hdr_state.SetString(grbl_comm.GetCurrentStateName());
  hdr_status_sub.SetString(grbl_comm.GetCurrentStatusName());
  hdr_state.Move((scr_w - hdr_state.GetWidth()) / 2, hdr_state.GetStartY());
  hdr_status_sub.Move((scr_w - hdr_status_sub.GetWidth()) / 2, hdr_status_sub.GetStartY());

  // Aux button colors
  flood_btn.SetColor(grbl_comm.GetCoolantFlood() ? COLOR_GREEN : COLOR_WHITE);
  mist_btn.SetColor(grbl_comm.GetCoolantMist() ? COLOR_GREEN : COLOR_WHITE);
  fire_pgm_btn.SetColor(fire_active ? COLOR_RED : COLOR_WHITE);

  if(grbl_comm.GetMpgModeRequest())
    mpg_pgm_btn.SetColor(grbl_comm.GetMpgMode() ? COLOR_GREEN : COLOR_RED);
  else
    mpg_pgm_btn.SetColor(grbl_comm.GetMpgMode() ? COLOR_RED : COLOR_WHITE);

  // RUN/STOP text
  if(grbl_comm.GetState() == GrblComm::RUN)
    run_btn.SetString("HOLD");
  else
    run_btn.SetString("RUN");

  if(grbl_comm.GetState() == GrblComm::ALARM)
    stop_btn.SetString(grbl_comm.GetStatusCode() == GrblComm::Status_NotAllowedCriticalEvent
                       ? "RESET" : "UNLOCK");
  else if((grbl_comm.GetState() == GrblComm::UNKNOWN) || (grbl_comm.GetState() == GrblComm::HOME))
    stop_btn.SetString("RESET");
  else
    stop_btn.SetString("STOP");

  if(run)
  {
    if(((grbl_comm.GetState() == GrblComm::IDLE) || (grbl_comm.GetState() == GrblComm::RUN) ||
        (grbl_comm.GetState() == GrblComm::HOLD)) && grbl_comm.IsInControl())
    {
      if(finished)
      {
        if(grbl_comm.GetState() == GrblComm::IDLE)
        {
          run = false;
          Application::GetInstance().EnableScreenChange();
        }
      }
      else
      {
        GrblComm::status_t result = (id != 0u) ? grbl_comm.GetCmdResult(id) : GrblComm::Status_OK;
        if((result == GrblComm::Status_OK) || (result == GrblComm::Status_Next_Cmd_Executed))
        {
          char cmd[128u];
          snprintf(cmd, NumberOf(cmd), "%s\r", text_box.GetSelectedStringText());
          if(grbl_comm.SendCmd(cmd, id) == Result::RESULT_OK)
          {
            int32_t select = text_box.GetSelect();
            int32_t scroll = text_box.GetScroll();
            if(p_text == nullptr)
            {
              if((select < text_box.GetNumberOfVisibleLines() / 2) || f_eof(&SDFile))
              {
                text_box.Select(select + 1);
                if(select == text_box.GetSelect()) finished = true;
              }
              else
              {
                char str[128] = {0};
                if(f_gets(str, NumberOf(str), &SDFile) != nullptr)
                {
                  str[NumberOf(str) - 1] = '\0';
                  if(strlen(str) > 80 + 2)
                    f_lseek(&SDFile, SDFile.obj.objsize);
                  else
                    text_box.AddLine(str);
                }
              }
            }
            else
            {
              if(select - scroll >= text_box.GetNumberOfVisibleLines() / 2)
                text_box.Scroll(scroll + 1);
              text_box.Select(select + 1);
              if(select == text_box.GetSelect()) finished = true;
            }
          }
        }
        else if(result == GrblComm::Status_Cmd_Not_Executed_Yet)
        {
          ; // Wait
        }
        else
        {
          run = false;
          Application::GetInstance().EnableScreenChange();
        }
      }
    }
    else
    {
      run = false;
      Application::GetInstance().EnableScreenChange();
    }
  }
  else if(grbl_comm.GetState() != GrblComm::RUN)
  {
    // Enable run only from beginning of file
    if(text_box.GetSelect() == 0)
      run_btn.Enable();
    else
      run_btn.Disable();

    // Allow scrolling with encoder
    if((enc_val != 0) && (p_text != nullptr))
    {
      text_box.Select(text_box.GetSelect() + enc_val);
      enc_val = 0;
    }
  }

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   ProcessCallback   *****************************************************
// *****************************************************************************
Result ProgramSender::ProcessCallback(const void* ptr)
{
  Result result = Result::RESULT_OK;

  if(ptr == &prev_btn)
  {
    Application::GetInstance().PrevScreen();
  }
  else if(ptr == &next_btn)
  {
    Application::GetInstance().NextScreen();
  }
  else if(ptr == &run_btn)
  {
    if(!run && grbl_comm.IsInControl() && (grbl_comm.GetState() == GrblComm::IDLE))
    {
      id       = 0u;
      run      = true;
      finished = false;
      Application::GetInstance().DisableScreenChange();
    }
    else if(grbl_comm.GetState() == GrblComm::RUN)
    {
      grbl_comm.Hold();
    }
    else
    {
      grbl_comm.Run();
    }
  }
  else if(ptr == &stop_btn)
  {
    run = false;
    Application::GetInstance().EnableScreenChange();
    if(grbl_comm.GetState() == GrblComm::ALARM)
    {
      if(grbl_comm.GetStatusCode() == GrblComm::Status_NotAllowedCriticalEvent)
        grbl_comm.Reset();
      else
        grbl_comm.Unlock();
    }
    else if((grbl_comm.GetState() == GrblComm::UNKNOWN) || (grbl_comm.GetState() == GrblComm::HOME))
    {
      grbl_comm.Reset();
    }
    else
    {
      grbl_comm.Stop();
    }
  }
  else if(ptr == &flood_btn)
  {
    grbl_comm.CoolantFloodToggle(); // AIR = M8
  }
  else if(ptr == &mist_btn)
  {
    grbl_comm.CoolantMistToggle(); // EXHAUST = M7
  }
  else if(ptr == &fire_pgm_btn)
  {
    bool was_in_control = grbl_comm.GetMpgModeRequest();
    if(!was_in_control) grbl_comm.GainControl();
    uint32_t fire_id = 0u;
    if(!fire_active)
    {
      grbl_comm.CoolantFloodToggle();
      grbl_comm.CoolantMistToggle();
      grbl_comm.SendCmd("$32=0\r", fire_id);
      grbl_comm.SendCmd("M3 S30\r", fire_id);
      fire_active = true;
    }
    else
    {
      grbl_comm.SendCmd("M5\r", fire_id);
      grbl_comm.SendCmd("$32=1\r", fire_id);
      grbl_comm.CoolantFloodToggle();
      grbl_comm.CoolantMistToggle();
      fire_active = false;
    }
    if(!was_in_control) grbl_comm.ReleaseControl();
  }
  else if(ptr == &mpg_pgm_btn)
  {
    if(grbl_comm.GetMpgModeRequest())
      grbl_comm.ReleaseControl();
    else
      grbl_comm.GainControl();
  }
  else if(ptr == &text_box)
  {
    // Tap on text area opens file browser when not running
    if(!run) OpenFileMenu();
  }
  else
  {
    ; // Do nothing
  }

  return result;
}

// *****************************************************************************
// ***   OpenFileMenu   ********************************************************
// *****************************************************************************
void ProgramSender::OpenFileMenu()
{
  AppTask::GetCurrent()->StopTimer();

  text_box.SetText(nullptr);
  f_close(&SDFile);
  ReleaseDataPointer();

  BSP_SD_Init();
  FRESULT res = f_mount(&SDFatFS, (TCHAR const*)SDPath, 0);
  DIR dir;
  if(res == FR_OK) res = f_opendir(&dir, "/");

  uint32_t cnt = 0u;
  if(res == FR_OK)
  {
    FILINFO fno;
    for(;;)
    {
      res = f_readdir(&dir, &fno);
      if((res != FR_OK) || (fno.fname[0] == 0)) break;
      bool add_file = false;
      uint32_t i = 0u;
      for(; i < NumberOf(fno.fname); i++) if(fno.fname[i] == '\0') break;
      for(i -= 3u; i > 0; i--)
      {
        if((fno.fname[i] == '.') && (tolower(fno.fname[i+2]) == 'c'))
        {
          if((tolower(fno.fname[i+1]) == 'g') || (tolower(fno.fname[i+1]) == 'n'))
          { add_file = true; break; }
        }
      }
      if(!(fno.fattrib & AM_DIR) && add_file)
      {
        menu_items[cnt].str.SetString(menu_items[cnt].text, menu_items[cnt].n,
                                      "%-19s%12lub", fno.fname, fno.fsize);
        cnt++;
        if(cnt == NumberOf(menu_items)) break;
      }
    }
    f_closedir(&dir);
  }
  for(uint32_t i = cnt; i < NumberOf(menu_items); i++) str[i][0] = '\0';

  AppTask::GetCurrent()->StartTimer();

  text_box.Hide();
  menu.SetCount(cnt);
  menu.Show(100);

  idx = 0u;
}

// *****************************************************************************
// ***   ProcessMenuOkCallback   ***********************************************
// *****************************************************************************
Result ProgramSender::ProcessMenuOkCallback(ProgramSender* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;
  if(obj_ptr != nullptr)
  {
    ProgramSender& ths = *obj_ptr;
    ths.menu.Hide();

    char fn[20u] = {0};
    for(uint32_t i = 0u; i < NumberOf(fn); i++)
    {
      fn[i] = ths.menu_items[(uint32_t)ptr].text[i];
      if(fn[i] == '\0') break;
    }
    for(uint32_t i = NumberOf(fn) - 1u; i > 0u; i--)
    {
      if(fn[i] <= ' ') fn[i] = '\0';
      else break;
    }

    FRESULT fres = f_open(&SDFile, fn, FA_OPEN_EXISTING | FA_READ);
    if(fres == FR_OK)
    {
      uint32_t fsize = f_size(&SDFile) + 1u;
      ths.AllocateDataBuffer(fsize);
      if(ths.p_text != nullptr)
      {
        UINT wbytes = 0u;
        fres = f_read(&SDFile, ths.p_text, fsize, &wbytes);
        ths.p_text[wbytes] = 0x00;
        if(!ths.text_box.SetText(ths.p_text))
          ths.text_box.SetText("; Lines longer than 80 chars");
        fres = f_close(&SDFile);
      }
      else
      {
        ths.text_box.SetText(nullptr);
        char str[128] = {0};
        for(int32_t i = 0; i < ths.text_box.GetNumberOfVisibleLines(); i++)
        {
          f_gets(str, NumberOf(str), &SDFile);
          str[NumberOf(str) - 1] = '\0';
          if(strlen(str) > 80 + 2)
          { f_close(&SDFile); ths.text_box.SetText("; Lines longer than 80 chars"); break; }
          else ths.text_box.AddLine(str);
        }
      }
    }
    else
    {
      ths.text_box.SetText("; Error opening file!");
    }

    ths.text_box.Show(100);
    result = Result::RESULT_OK;
  }
  return result;
}

// *****************************************************************************
// ***   ProcessMenuCancelCallback   *******************************************
// *****************************************************************************
Result ProgramSender::ProcessMenuCancelCallback(ProgramSender* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;
  if(obj_ptr != nullptr)
  {
    ProgramSender& ths = *obj_ptr;
    ths.menu.Hide();
    ths.text_box.SetText("; Cancelled");
    ths.text_box.Show(100);
    result = Result::RESULT_OK;
  }
  return result;
}

// *****************************************************************************
// ***   AllocateDataBuffer   **************************************************
// *****************************************************************************
char* ProgramSender::AllocateDataBuffer(uint32_t& size)
{
  ReleaseDataPointer();
  if(size == 0u)
  {
    HeapStats_t HeapStats;
    vPortGetHeapStats(&HeapStats);
    size = HeapStats.xSizeOfLargestFreeBlockInBytes - 32u;
  }
  p_text = new char[size];
  if(p_text != nullptr) p_text[0] = '\0';
  text_box.SetText(p_text);
  Application::GetInstance().UpdateMemoryInfo();
  return p_text;
}

// *****************************************************************************
// ***   ReleaseDataPointer   **************************************************
// *****************************************************************************
void ProgramSender::ReleaseDataPointer()
{
  if(p_text != nullptr)
  {
    text_box.Hide();
    delete [] p_text;
    p_text = nullptr;
  }
  text_box.SetText(nullptr);
  Application::GetInstance().UpdateMemoryInfo();
}

// *****************************************************************************
// ***   ProcessEncoderCallback   **********************************************
// *****************************************************************************
Result ProgramSender::ProcessEncoderCallback(ProgramSender* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;
  if(obj_ptr != nullptr)
  {
    ProgramSender& ths = *obj_ptr;
    ths.enc_val += (int32_t)ptr;
    result = Result::RESULT_OK;
  }
  return result;
}
