//******************************************************************************
//  @file OverrideCtrlScr.cpp
//  @author Nicolai Shlapunov / JPB Laser modifications
//
//  @details OverrideCtrlScr: Full-screen self-contained layout.
//           Nav bar matches home screen. Content centered between nav and
//           bottom section. Bottom buttons match home screen text size.
//
//******************************************************************************

#include "OverrideCtrlScr.h"
#include "Application.h"

// *****************************************************************************
OverrideCtrlScr& OverrideCtrlScr::GetInstance()
{
  static OverrideCtrlScr overridectrlscr;
  return overridectrlscr;
}

// *****************************************************************************
// ***   Setup   ***************************************************************
// *****************************************************************************
Result OverrideCtrlScr::Setup(int32_t y, int32_t height)
{
  int32_t  scr_w         = display_drv.GetScreenW(); // 320
  int32_t  scr_h         = display_drv.GetScreenH(); // 480
  uint32_t window_height = Font_8x12::GetInstance().GetCharH() * 5u; // 60px

  // *** Nav bar — identical dimensions to home screen ***
  int32_t dro_start_x = scr_w / 6;
  int32_t dro_end_x   = dro_start_x + (scr_w - BORDER_W * 2) * 4 / 6;
  int32_t arrow_w     = scr_w - dro_end_x - BORDER_W * 2; // 51px

  // Vertical centering — same formula as home screen (7 rows + gaps)
  int32_t total_h = 7 * (int32_t)window_height + 7 * BORDER_W + BORDER_W * 2; // 456
  int32_t nav_y   = (scr_h - total_h) / 2; // = 12
  int32_t nav_end = nav_y + (int32_t)window_height;

  prev_btn.SetParams("<", BORDER_W, nav_y, arrow_w, window_height, true);
  prev_btn.SetCallback(AppTask::GetCurrent());

  next_btn.SetParams(">", scr_w - BORDER_W - arrow_w, nav_y, arrow_w, window_height, true);
  next_btn.SetCallback(AppTask::GetCurrent());

  title_str.SetParams("OVERRIDE", 0, 0, COLOR_WHITE, Font_12x16::GetInstance());
  title_str.Move((scr_w - title_str.GetWidth()) / 2,
                 nav_y + ((int32_t)window_height - Font_12x16::GetInstance().GetCharH()) / 2);

  // *** Bottom section — fixed at screen bottom ***
  int32_t run_stop_y = nav_y + total_h - (int32_t)window_height;
  int32_t aux_y      = run_stop_y - BORDER_W - (int32_t)window_height;
  int32_t status_y   = aux_y - BORDER_W - (int32_t)window_height;

  // *** Status strings ***
  hdr_state.SetParams("-----", BORDER_W,
                      status_y + ((int32_t)window_height - Font_12x16::GetInstance().GetCharH()) / 2,
                      COLOR_WHITE, Font_12x16::GetInstance());
  hdr_status_sub.SetParams("", BORDER_W,
                            status_y + (int32_t)window_height - Font_8x12::GetInstance().GetCharH() - BORDER_W / 2,
                            COLOR_WHITE, Font_8x12::GetInstance());

  // *** Aux row: 4 equal buttons matching home screen ***
  uint32_t aux_btn_w = ((uint32_t)scr_w - BORDER_W * 5u) / 4u; // 75px

  flood_btn.SetParams("AIR", BORDER_W, aux_y, aux_btn_w, window_height, true);
  flood_btn.SetCallback(AppTask::GetCurrent());

  mist_btn.SetParams("EXHAUST", BORDER_W + (int32_t)aux_btn_w + BORDER_W,
                     aux_y, aux_btn_w, window_height, true);
  mist_btn.SetCallback(AppTask::GetCurrent());

  fire_ovr_btn.SetParams("FIRE", BORDER_W + 2 * ((int32_t)aux_btn_w + BORDER_W),
                          aux_y, aux_btn_w, window_height, true);
  fire_ovr_btn.SetCallback(AppTask::GetCurrent());

  mpg_ovr_btn.SetParams("MPG", BORDER_W + 3 * ((int32_t)aux_btn_w + BORDER_W),
                         aux_y, aux_btn_w, window_height, true);
  mpg_ovr_btn.SetCallback(AppTask::GetCurrent());

  // *** Run / Stop — larger font, uppercase, same width as home screen ***
  int32_t run_stop_w = 2 * (int32_t)aux_btn_w + BORDER_W; // 154px

  run_btn.SetParams("RUN", BORDER_W, run_stop_y, run_stop_w, window_height, true);
  run_btn.SetFont(Font_12x16::GetInstance());
  run_btn.SetCallback(AppTask::GetCurrent());

  stop_btn.SetParams("STOP", BORDER_W + run_stop_w + BORDER_W, run_stop_y,
                     run_stop_w, window_height, true);
  stop_btn.SetFont(Font_12x16::GetInstance());
  stop_btn.SetCallback(AppTask::GetCurrent());

  // *** Content: centered between nav_end and status_y ***
  // Axis DRO height — slightly taller than default
  axis_dro_h = Font_10x18::GetInstance().GetCharH()
               + Font_6x8::GetInstance().GetCharH() * 2
               + BORDER_W + 12; // ~50px

  int32_t name_h         = Font_10x18::GetInstance().GetCharH(); // 18
  int32_t axis_section_h = name_h + BORDER_W + axis_dro_h;
  int32_t content_h      = axis_section_h
                           + BORDER_W * 3
                           + (int32_t)window_height
                           + BORDER_W * 2
                           + (int32_t)window_height;
  int32_t available      = status_y - nav_end;
  int32_t padding        = (available - content_h) / 2;
  content_start          = nav_end + padding + BORDER_W * 2;
  axis_dro_y             = content_start + BORDER_W + name_h;
  int32_t axis_dro_end   = axis_dro_y + axis_dro_h;
  int32_t feed_start_y   = axis_dro_end + BORDER_W * 3;

  // *** Feed override ***
  feed_dw.SetParams(scr_w / 4, feed_start_y,
                    (scr_w - scr_w / 4 - BORDER_W) / 2, window_height, 3u, 0u);
  feed_dw.SetBorder(BORDER_W, COLOR_RED);
  feed_dw.SetDataFont(Font_8x12::GetInstance(), 2u);
  feed_dw.SetNumber(0);
  feed_dw.SetUnits("%", DataWindow::RIGHT);
  feed_dw.SetCallback(AppTask::GetCurrent());
  feed_dw.SetActive(true);
  feed_name.SetParams("FEED:", 0, 0, COLOR_WHITE, Font_12x16::GetInstance());
  feed_name.Move((feed_dw.GetStartX() / 2) - (feed_name.GetWidth() / 2),
                 (feed_dw.GetStartY() + (int32_t)feed_dw.GetHeight() / 2) - (feed_name.GetHeight() / 2));
  feed_reset_btn.SetParams("100%", feed_dw.GetEndX() + BORDER_W, feed_dw.GetStartY(),
                            scr_w - feed_dw.GetEndX() - BORDER_W * 2, feed_dw.GetHeight(), true);
  feed_reset_btn.SetCallback(AppTask::GetCurrent());

  // *** Power override ***
  speed_dw.SetParams(feed_dw.GetStartX(), feed_dw.GetEndY() + BORDER_W * 2,
                     feed_dw.GetWidth(), feed_dw.GetHeight(), 3u, 0u);
  speed_dw.SetBorder(BORDER_W, COLOR_RED);
  speed_dw.SetDataFont(Font_8x12::GetInstance(), 2u);
  speed_dw.SetNumber(0);
  speed_dw.SetUnits("%", DataWindow::RIGHT);
  speed_dw.SetCallback(AppTask::GetCurrent());
  speed_dw.SetActive(true);
  speed_name.SetParams("POWER:", 0, 0, COLOR_WHITE, Font_12x16::GetInstance());
  speed_name.Move((speed_dw.GetStartX() / 2) - (speed_name.GetWidth() / 2),
                  (speed_dw.GetStartY() + (int32_t)speed_dw.GetHeight() / 2) - (speed_name.GetHeight() / 2));
  speed_reset_btn.SetParams("100%", speed_dw.GetEndX() + BORDER_W, speed_dw.GetStartY(),
                             scr_w - speed_dw.GetEndX() - BORDER_W * 2, speed_dw.GetHeight(), true);
  speed_reset_btn.SetCallback(AppTask::GetCurrent());

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Show   ****************************************************************
// *****************************************************************************
Result OverrideCtrlScr::Show()
{
  Application::GetInstance().HideGlobalUI();

  prev_btn.Show(100);
  next_btn.Show(100);
  title_str.Show(101);

  int32_t scr_w = display_drv.GetScreenW();
  for(uint32_t i = 0u; i < grbl_comm.GetLimitedNumberOfAxis(3u); i++)
  {
    DataWindow& dw_real      = Application::GetInstance().GetRealDataWindow(i);
    String&     dw_real_name = Application::GetInstance().GetRealDataWindowNameString(i);

    int32_t col_w = (scr_w - BORDER_W * 4) / 3;
    dw_real.SetParams(BORDER_W + (col_w + BORDER_W) * (int32_t)i,
                      axis_dro_y, col_w, axis_dro_h,
                      8u, grbl_comm.GetReportUnitsPrecision(i));
    dw_real.SetBorder(BORDER_W / 2, COLOR_GREY);
    dw_real.SetDataFont(Font_10x18::GetInstance());
    dw_real.SetUnits(grbl_comm.GetReportUnits(), DataWindow::BOTTOM_RIGHT, Font_6x8::GetInstance());

    dw_real_name.SetParams(grbl_comm.GetAxisName(i), 0, 0, COLOR_WHITE, Font_10x18::GetInstance());
    dw_real_name.Move(dw_real.GetStartX() + (dw_real.GetWidth() - dw_real_name.GetWidth()) / 2,
                      content_start);

    dw_real.Show(100);
    dw_real_name.Show(100);
  }

  feed_dw.Show(100);
  feed_name.Show(100);
  feed_reset_btn.Show(100);

  speed_dw.Show(100);
  speed_name.Show(100);
  speed_reset_btn.Show(100);

  hdr_state.Show(101);
  hdr_status_sub.Show(101);

  flood_btn.Show(100);
  mist_btn.Show(100);
  fire_ovr_btn.SetString("FIRE");
  fire_ovr_btn.SetColor(COLOR_WHITE);
  fire_active = false;
  fire_ovr_btn.Show(100);
  mpg_ovr_btn.Show(100);

  run_btn.Show(100);
  stop_btn.Show(100);

  InputDrv::GetInstance().AddEncoderCallbackHandler(AppTask::GetCurrent(),
    reinterpret_cast<CallbackPtr>(ProcessEncoderCallback), this, enc_cble);

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Hide   ****************************************************************
// *****************************************************************************
Result OverrideCtrlScr::Hide()
{
  InputDrv::GetInstance().DeleteEncoderCallbackHandler(enc_cble);

  prev_btn.Hide();
  next_btn.Hide();
  title_str.Hide();

  for(uint32_t i = 0u; i < GrblComm::AXIS_CNT; i++)
  {
    Application::GetInstance().GetRealDataWindow(i).Hide();
    Application::GetInstance().GetRealDataWindowNameString(i).Hide();
  }

  feed_dw.Hide();
  feed_name.Hide();
  feed_reset_btn.Hide();

  speed_dw.Hide();
  speed_name.Hide();
  speed_reset_btn.Hide();

  hdr_state.Hide();
  hdr_status_sub.Hide();

  flood_btn.Hide();
  mist_btn.Hide();
  fire_ovr_btn.Hide();
  mpg_ovr_btn.Hide();

  run_btn.Hide();
  stop_btn.Hide();

  Application::GetInstance().ShowGlobalUI();

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   TimerExpired   ********************************************************
// *****************************************************************************
Result OverrideCtrlScr::TimerExpired(uint32_t interval)
{
  Result result = Result::RESULT_OK;

  for(uint32_t i = 0u; i < grbl_comm.GetLimitedNumberOfAxis(3u); i++)
    Application::GetInstance().GetRealDataWindow(i).SetNumber(grbl_comm.GetAxisPosition(i));

  int32_t scr_w = display_drv.GetScreenW();
  hdr_state.SetString(grbl_comm.GetCurrentStateName());
  hdr_status_sub.SetString(grbl_comm.GetCurrentStatusName());
  hdr_state.Move((scr_w - hdr_state.GetWidth()) / 2, hdr_state.GetStartY());
  hdr_status_sub.Move((scr_w - hdr_status_sub.GetWidth()) / 2, hdr_status_sub.GetStartY());

  feed_dw.SetNumber(grbl_comm.GetFeedOverride());
  speed_dw.SetNumber(grbl_comm.GetSpeedOverride());

  flood_btn.SetColor(grbl_comm.GetCoolantFlood() ? COLOR_GREEN : COLOR_WHITE);
  mist_btn.SetColor(grbl_comm.GetCoolantMist() ? COLOR_GREEN : COLOR_WHITE);
  fire_ovr_btn.SetColor(fire_active ? COLOR_RED : COLOR_WHITE);

  if(grbl_comm.GetMpgModeRequest())
    mpg_ovr_btn.SetColor(grbl_comm.GetMpgMode() ? COLOR_GREEN : COLOR_RED);
  else
    mpg_ovr_btn.SetColor(grbl_comm.GetMpgMode() ? COLOR_RED : COLOR_WHITE);

  run_btn.SetString(grbl_comm.GetState() == GrblComm::RUN ? "HOLD" : "RUN");

  if(grbl_comm.GetState() == GrblComm::ALARM)
    stop_btn.SetString(grbl_comm.GetStatusCode() == GrblComm::Status_NotAllowedCriticalEvent
                       ? "RESET" : "UNLOCK");
  else if((grbl_comm.GetState() == GrblComm::UNKNOWN) || (grbl_comm.GetState() == GrblComm::HOME))
    stop_btn.SetString("RESET");
  else
    stop_btn.SetString("STOP");

  if(feed_val > 0)
  {
    result = (feed_val > 10) ? grbl_comm.FeedCoarsePlus() : grbl_comm.FeedFinePlus();
    feed_val -= (feed_val > 10) ? 10 : 1;
  }
  else if(feed_val < 0)
  {
    result = (feed_val < -10) ? grbl_comm.FeedCoarseMinus() : grbl_comm.FeedFineMinus();
    feed_val += (feed_val < -10) ? 10 : 1;
  }

  if(speed_val > 0)
  {
    result = (speed_val > 10) ? grbl_comm.SpeedCoarsePlus() : grbl_comm.SpeedFinePlus();
    speed_val -= (speed_val > 10) ? 10 : 1;
  }
  else if(speed_val < 0)
  {
    result = (speed_val < -10) ? grbl_comm.SpeedCoarseMinus() : grbl_comm.SpeedFineMinus();
    speed_val += (speed_val < -10) ? 10 : 1;
  }

  return result;
}

// *****************************************************************************
// ***   ProcessCallback   *****************************************************
// *****************************************************************************
Result OverrideCtrlScr::ProcessCallback(const void* ptr)
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
  else if(ptr == &feed_dw)
  {
    speed_dw.SetSelected(false);
    feed_dw.SetSelected(true);
  }
  else if(ptr == &feed_reset_btn)
  {
    grbl_comm.FeedReset();
  }
  else if(ptr == &speed_dw)
  {
    feed_dw.SetSelected(false);
    speed_dw.SetSelected(true);
  }
  else if(ptr == &speed_reset_btn)
  {
    grbl_comm.SpeedReset();
  }
  else if(ptr == &flood_btn)
  {
    grbl_comm.CoolantFloodToggle(); // AIR = M8
  }
  else if(ptr == &mist_btn)
  {
    grbl_comm.CoolantMistToggle(); // EXHAUST = M7
  }
  else if(ptr == &fire_ovr_btn)
  {
    bool was_in_control = grbl_comm.GetMpgModeRequest();
    if(!was_in_control) grbl_comm.GainControl();
    uint32_t id = 0u;
    if(!fire_active)
    {
      grbl_comm.CoolantFloodToggle();
      grbl_comm.CoolantMistToggle();
      grbl_comm.SendCmd("$32=0\r", id);
      grbl_comm.SendCmd("M3 S30\r", id);
      fire_active = true;
    }
    else
    {
      grbl_comm.SendCmd("M5\r", id);
      grbl_comm.SendCmd("$32=1\r", id);
      grbl_comm.CoolantFloodToggle();
      grbl_comm.CoolantMistToggle();
      fire_active = false;
    }
    if(!was_in_control) grbl_comm.ReleaseControl();
  }
  else if(ptr == &mpg_ovr_btn)
  {
    if(grbl_comm.GetMpgModeRequest())
      grbl_comm.ReleaseControl();
    else
      grbl_comm.GainControl();
  }
  else if(ptr == &run_btn)
  {
    if(grbl_comm.GetState() != GrblComm::RUN)
      grbl_comm.Run();
    else
      grbl_comm.Hold();
  }
  else if(ptr == &stop_btn)
  {
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
  else
  {
    result = Result::ERR_UNHANDLED_REQUEST;
  }

  return result;
}

// *****************************************************************************
// ***   ProcessEncoderCallback   **********************************************
// *****************************************************************************
Result OverrideCtrlScr::ProcessEncoderCallback(OverrideCtrlScr* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;
  if(obj_ptr != nullptr)
  {
    OverrideCtrlScr& ths = *obj_ptr;
    int32_t enc_val = (int32_t)ptr;
    if(ths.feed_dw.IsSelected())  ths.feed_val  += enc_val;
    if(ths.speed_dw.IsSelected()) ths.speed_val += enc_val;
    result = Result::RESULT_OK;
  }
  return result;
}
