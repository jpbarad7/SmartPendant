//******************************************************************************
//  @file DirectControlScr.cpp
//  @author Nicolai Shlapunov / JPB PCB Mill modifications
//
//  @details Full-screen home layout for the PCB mill pendant.
//           Hides the global Application header and status bar on Show()
//           and restores them on Hide(). All navigation, status, and
//           machine controls are self-contained in this screen.
//
//******************************************************************************

#include "DirectControlScr.h"
#include "Application.h"
#include "MillConfig.h"

// *****************************************************************************
DirectControlScr& DirectControlScr::GetInstance()
{
  static DirectControlScr directcontrolscr;
  return directcontrolscr;
}

// *****************************************************************************
// ***   Setup   ***************************************************************
// *****************************************************************************
Result DirectControlScr::Setup(int32_t y, int32_t height)
{
  // Ignore y and height — home screen uses the full display
  int32_t scr_w = display_drv.GetScreenW(); // 320
  int32_t scr_h = display_drv.GetScreenH(); // 480

  // Window height: Font_8x12 * 5, same as original code
  uint32_t window_height = Font_8x12::GetInstance().GetCharH() * 5u; // 60px

  // Vertical centering:
  // 7 rows of window_height + gaps:
  //   nav→X : BORDER_W + DRO_MARGIN
  //   X→Y   : BORDER_W
  //   Y→Z   : BORDER_W
  //   Z→scl : BORDER_W * 2
  //   scl→ax: BORDER_W
  //   ax→run: BORDER_W
  // Total gaps = 7*BORDER_W + DRO_MARGIN = 28 + 8 = 36
  int32_t total_h = 7 * (int32_t)window_height + 7 * BORDER_W + (int32_t)DRO_MARGIN;
  int32_t nav_y   = (scr_h - total_h) / 2; // = 12 for 480px screen

  // DRO section starts after nav bar + gap + margin
  int32_t dro_y_base = nav_y + (int32_t)window_height + BORDER_W + (int32_t)DRO_MARGIN;

  // *** DRO windows ***
  for(uint32_t i = 0u; i < grbl_comm.GetLimitedNumberOfAxis(NumberOf(dw)); i++)
  {
    int32_t row_y = dro_y_base + (int32_t)i * ((int32_t)window_height + BORDER_W);

    dw[i].SetParams(scr_w / 6, row_y, (scr_w - BORDER_W * 2) * 4 / 6, window_height,
                    8u, grbl_comm.GetReportUnitsPrecision(i));
    dw[i].SetBorder(BORDER_W, COLOR_RED);
    dw[i].SetDataFont(Font_8x12::GetInstance(), 2u);
    dw[i].SetNumber(0);
    dw[i].SetUnits(grbl_comm.GetReportUnits(i), DataWindow::RIGHT);
    dw[i].SetCallback(AppTask::GetCurrent());
    dw[i].SetActive(true);

    axis_names[i].SetParams(grbl_comm.GetAxisName(i), 0, 0, COLOR_WHITE, Font_12x16::GetInstance());
    axis_names[i].SetScale(2u);
    axis_names[i].Move((dw[i].GetStartX() / 2) - (axis_names[i].GetWidth() / 2),
                       (dw[i].GetStartY() + (int32_t)dw[i].GetHeight() / 2) - (axis_names[i].GetHeight() / 2));

    zero_btn[i].SetParams("<0>", dw[i].GetEndX() + BORDER_W, row_y,
                          scr_w - dw[i].GetEndX() - BORDER_W * 2, window_height, true);
    zero_btn[i].SetCallback(AppTask::GetCurrent());
  }

  // *** X mode button (lathe, not used in laser) ***
  x_mode_btn.SetParams("", BORDER_W, dw[GrblComm::AXIS_X].GetStartY(),
                       dw[GrblComm::AXIS_X].GetStartX() - BORDER_W * 2,
                       dw[GrblComm::AXIS_X].GetHeight(), true);
  x_mode_btn.SetCallback(AppTask::GetCurrent());
  x_mode_str.SetParams("", dw[GrblComm::AXIS_X].GetStartX() + BORDER_W * 2,
                       dw[GrblComm::AXIS_X].GetStartY() + BORDER_W * 2,
                       COLOR_WHITE, Font_8x12::GetInstance());

  // *** Scale buttons ***
  uint32_t scale_btn_w = ((uint32_t)scr_w - BORDER_W * (NumberOf(scale_btn) + 1u)) / NumberOf(scale_btn); // 75
  int32_t  scale_y     = dw[grbl_comm.GetLimitedNumberOfAxis(NumberOf(dw)) - 1u].GetEndY() + BORDER_W * 2;

  for(uint32_t i = 0u; i < NumberOf(scale_btn); i++)
  {
    scale_btn[i].SetParams(scale_str[i],
                           BORDER_W + (int32_t)i * ((int32_t)scale_btn_w + BORDER_W),
                           scale_y, scale_btn_w, window_height, true);
    scale_btn[i].SetCallback(AppTask::GetCurrent());
    scale_btn[i].SetSpacing(3u);
    scale_btn[i].SetPressed(false);
    scale_btn[i].SetColor(COLOR_WHITE);
  }
  scale_btn[1u].SetPressed(true); // default 0.0010" - the step that jogs smoothly at $120=100

  // *** Navigation bar: prev_btn | status | next_btn ***
  int32_t arrow_w  = scr_w - dw[0].GetEndX() - BORDER_W * 2; // = zero_btn width = 51
  int32_t status_x = BORDER_W + arrow_w + BORDER_W;

  prev_btn.SetParams("<", BORDER_W, nav_y, arrow_w, window_height, true);
  prev_btn.SetCallback(AppTask::GetCurrent());

  next_btn.SetParams(">", scr_w - BORDER_W - arrow_w, nav_y, arrow_w, window_height, true);
  next_btn.SetCallback(AppTask::GetCurrent());

  hdr_state.SetParams("-----", status_x + BORDER_W,
                      nav_y + ((int32_t)window_height - Font_12x16::GetInstance().GetCharH()) / 2,
                      COLOR_WHITE, Font_12x16::GetInstance());

  hdr_status_sub.SetParams("", status_x + BORDER_W + 5 * Font_12x16::GetInstance().GetCharW() + Font_8x12::GetInstance().GetCharW(),
                            nav_y + (int32_t)window_height - Font_8x12::GetInstance().GetCharH() - BORDER_W / 2,
                            COLOR_WHITE, Font_8x12::GetInstance());

  // *** Aux button row: VAC | SPINDLE | MPG ***
  int32_t aux_y = scale_btn[0].GetEndY() + BORDER_W;

  vac_btn.SetParams("VAC", BORDER_W, aux_y, scale_btn_w, window_height, true);
  vac_btn.SetCallback(AppTask::GetCurrent());

  spindle_btn.SetParams("SPINDLE", BORDER_W + (int32_t)scale_btn_w + BORDER_W,
                        aux_y, scale_btn_w, window_height, true);
  spindle_btn.SetCallback(AppTask::GetCurrent());

  mpg_home_btn.SetParams("MPG", BORDER_W + 2 * ((int32_t)scale_btn_w + BORDER_W),
                          aux_y, scale_btn_w, window_height, true);
  mpg_home_btn.SetCallback(AppTask::GetCurrent());

  // *** Bottom row: RUN | STOP (half the screen each, larger font) ***
  // Derived from the screen width, NOT from scale_btn_w. With three scale
  // buttons scale_btn_w is 101, so the old "2 * scale_btn_w + BORDER_W" would
  // give 206 each and overflow the 320 px display.
  int32_t run_stop_w = (scr_w - 3 * BORDER_W) / 2; // 154
  int32_t run_stop_y = aux_y + (int32_t)window_height + BORDER_W;

  run_btn.SetParams("RUN", BORDER_W, run_stop_y, run_stop_w, window_height, true);
  run_btn.SetFont(Font_12x16::GetInstance());
  run_btn.SetCallback(AppTask::GetCurrent());

  stop_btn.SetParams("STOP", BORDER_W + run_stop_w + BORDER_W,
                     run_stop_y, run_stop_w, window_height, true);
  stop_btn.SetFont(Font_12x16::GetInstance());
  stop_btn.SetCallback(AppTask::GetCurrent());

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Show   ****************************************************************
// *****************************************************************************
Result DirectControlScr::Show()
{
  // Hide the global Application header and status bar
  Application::GetInstance().HideGlobalUI();

  // Navigation bar
  prev_btn.Show(100);
  next_btn.Show(100);
  hdr_state.Show(101);
  hdr_status_sub.Show(101);

  // Scale buttons
  UpdateScaleButtons();

  // DRO windows
  for(uint32_t i = 0u; i < grbl_comm.GetLimitedNumberOfAxis(NumberOf(dw)); i++)
  {
    dw[i].Show(100);
    axis_names[i].Show(100);
    zero_btn[i].Show(100);
  }

  // Reset axis selection
  axis = GrblComm::AXIS_CNT;
  for(uint32_t i = 0u; i < GrblComm::AXIS_CNT; i++) dw[i].SetSelected(false);

  // Aux row
  vac_btn.Show(100); // VAC (M8)
  spindle_btn.SetString("SPINDLE");
  spindle_btn.SetColor(COLOR_WHITE);
  spindle_on = false;
  spindle_btn.Show(100);
  mpg_home_btn.Show(100);

  // Bottom row
  run_btn.Show(100);
  stop_btn.Show(100);

  // Encoder callback
  InputDrv::GetInstance().AddEncoderCallbackHandler(AppTask::GetCurrent(),
    reinterpret_cast<CallbackPtr>(ProcessEncoderCallback), this, enc_cble);

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Hide   ****************************************************************
// *****************************************************************************
Result DirectControlScr::Hide()
{
  InputDrv::GetInstance().DeleteEncoderCallbackHandler(enc_cble);
  change_box.Hide();

  prev_btn.Hide();
  next_btn.Hide();
  hdr_state.Hide();
  hdr_status_sub.Hide();

  for(uint32_t i = 0u; i < NumberOf(dw); i++)
  {
    dw[i].Hide();
    axis_names[i].Hide();
    zero_btn[i].Hide();
  }
  for(uint32_t i = 0u; i < NumberOf(scale_btn); i++) scale_btn[i].Hide();

  x_mode_btn.Hide();
  x_mode_str.Hide();

  vac_btn.Hide();
  spindle_btn.Hide();
  mpg_home_btn.Hide();
  run_btn.Hide();
  stop_btn.Hide();

  Application::GetInstance().ShowGlobalUI();

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   TimerExpired   ********************************************************
// *****************************************************************************
Result DirectControlScr::TimerExpired(uint32_t interval)
{
  Result result = Result::RESULT_OK;

  // Update nav bar status
  hdr_state.SetString(grbl_comm.GetCurrentStateName());
  hdr_status_sub.SetString(grbl_comm.GetCurrentStatusName());
  int32_t status_x = prev_btn.GetEndX() + BORDER_W;
  int32_t status_w = next_btn.GetStartX() - BORDER_W - status_x;
  hdr_state.Move(status_x + (status_w - hdr_state.GetWidth()) / 2, hdr_state.GetStartY());
  hdr_status_sub.Move(status_x + (status_w - hdr_status_sub.GetWidth()) / 2, hdr_status_sub.GetStartY());

  // Update DRO positions
  for(uint32_t i = 0u; i < grbl_comm.GetLimitedNumberOfAxis(NumberOf(dw)); i++)
    dw[i].SetNumber(grbl_comm.GetAxisPosition(i));

  // Process jogging
  for(uint32_t i = 0u; i < GrblComm::AXIS_CNT; i++)
  {
    if(axis_jog_val[i] != 0)
    {
      int32_t distance = axis_jog_val[i] * scale;
      uint32_t feed_x100 = (grbl_comm.IsRotaryAxis(axis) ? 21600u : 600u) * 100u;
      if(((axis_jog_dir[i] < 0) && (axis_jog_val[i] < 0)) ||
         ((axis_jog_dir[i] > 0) && (axis_jog_val[i] > 0)))
      {
        feed_x100 = InputDrv::GetInstance().GetEncoderSpeed();
        if(feed_x100 < 20u) feed_x100 = 20u;
        feed_x100 *= (uint32_t)scale;
        feed_x100 = feed_x100 * 60u / 10u;
      }
      else
      {
        axis_jog_dir[i] = axis_jog_val[i] > 0 ? 1 : -1;
      }
      result = grbl_comm.Jog(i, distance, feed_x100, false);
      axis_jog_val[i] = 0;
      break;
    }
  }

  // VAC button color (M8 = CoolantFlood). State comes from the controller, so
  // this stays correct even if flood is toggled from telnet or a G-code program.
  vac_btn.SetColor(grbl_comm.GetCoolantFlood() ? COLOR_GREEN : COLOR_WHITE);
  // SPINDLE button color
  spindle_btn.SetColor(spindle_on ? COLOR_RED : COLOR_WHITE);

  // MPG button color
  if(grbl_comm.GetMpgModeRequest())
    mpg_home_btn.SetColor(grbl_comm.GetMpgMode() ? COLOR_GREEN : COLOR_RED);
  else
    mpg_home_btn.SetColor(grbl_comm.GetMpgMode() ? COLOR_RED : COLOR_WHITE);

  // RUN button text
  run_btn.SetString(grbl_comm.GetState() == GrblComm::RUN ? "HOLD" : "RUN");

  // STOP button text
  if(grbl_comm.GetState() == GrblComm::ALARM)
  {
    stop_btn.SetString(grbl_comm.GetStatusCode() == GrblComm::Status_NotAllowedCriticalEvent
                       ? "RESET" : "UNLOCK");
  }
  else if((grbl_comm.GetState() == GrblComm::UNKNOWN) || (grbl_comm.GetState() == GrblComm::HOME))
  {
    stop_btn.SetString("RESET");
  }
  else
  {
    stop_btn.SetString("STOP");
  }

  // Enable/disable buttons based on control state
  if(grbl_comm.IsInControl() && ((grbl_comm.GetState() == GrblComm::IDLE) ||
                                  (grbl_comm.GetState() == GrblComm::JOG)))
  {
    x_mode_btn.Enable();
    for(uint32_t i = 0u; i < NumberOf(zero_btn); i++) zero_btn[i].Enable();
    vac_btn.Enable();
    spindle_btn.Enable();
  }
  else
  {
    change_box.Hide();
    x_mode_btn.Disable();
    for(uint32_t i = 0u; i < NumberOf(zero_btn); i++) zero_btn[i].Disable();
    vac_btn.Disable();
    spindle_btn.Disable();
  }

  return result;
}

// *****************************************************************************
// ***   ProcessCallback   *****************************************************
// *****************************************************************************
Result DirectControlScr::ProcessCallback(const void* ptr)
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
  else if(ptr == &mpg_home_btn)
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
  else if(ptr == &spindle_btn)
  {
    // Plain M3/M5. The laser version also toggled both coolants and flipped
    // $32 in and out of laser mode; none of that applies here - $32 is already
    // 0 on this machine and the vacuum is controlled by its own button.
    bool was_in_control = grbl_comm.GetMpgModeRequest();
    if(!was_in_control) grbl_comm.GainControl();
    uint32_t id = 0u;
    if(!spindle_on)
    {
      grbl_comm.SendCmd(SPINDLE_ON_CMD, id);
      spindle_on = true;
    }
    else
    {
      grbl_comm.SendCmd(SPINDLE_OFF_CMD, id);
      spindle_on = false;
    }
    if(!was_in_control) grbl_comm.ReleaseControl();
  }
  else if(ptr == &change_box)
  {
    if(change_box.GetResult())
      grbl_comm.SetAxisPosition(change_box.GetId(), change_box.GetValue());
  }
  else if(ptr == &vac_btn)
  {
    grbl_comm.CoolantFloodToggle(); // VAC = M8
  }
  else
  {
    uint32_t i = 0u;
    for(; i < NumberOf(scale_btn); i++)
    {
      if(ptr == &scale_btn[i])
      {
        UnpressButtons();
        scale_btn[i].SetPressed(true);
        scale_btn[i].SetColor(COLOR_GREEN);
        scale = scale_val[i];
        break;
      }
    }

    if(i == NumberOf(scale_btn))
    {
      for(uint32_t j = 0u; j < GrblComm::AXIS_CNT; j++)
      {
        if((ptr == &dw[j]) && dw[j].IsSelected())
        {
          change_box.Setup(grbl_comm.GetAxisName(j), grbl_comm.GetReportUnits(),
                           dw[j].GetNumber(), -10000000, 10000000,
                           grbl_comm.GetReportUnitsPrecision(j));
          change_box.SetCallback(AppTask::GetCurrent());
          change_box.SetId(j);
          change_box.Show(10000u);
        }
        else if(ptr == &dw[j])
        {
          for(uint32_t k = 0u; k < GrblComm::AXIS_CNT; k++) dw[k].SetSelected(false);
          dw[j].SetSelected(true);
          axis = (GrblComm::Axis_t)j;
          UpdateScaleButtons();
          break;
        }
        else if(ptr == &zero_btn[j])
        {
          grbl_comm.ZeroAxis((GrblComm::Axis_t)j);
        }
      }
    }
  }

  return result;
}

// *****************************************************************************
// ***   ProcessEncoderCallback   **********************************************
// *****************************************************************************
Result DirectControlScr::ProcessEncoderCallback(DirectControlScr* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;
  if(obj_ptr != nullptr)
  {
    DirectControlScr& ths = *obj_ptr;
    int32_t enc_val = (int32_t)ptr;
    if(ths.axis < GrblComm::AXIS_CNT)
      ths.axis_jog_val[ths.axis] += enc_val;
    result = Result::RESULT_OK;
  }
  return result;
}

// *****************************************************************************
// ***   UnpressButtons   ******************************************************
// *****************************************************************************
void DirectControlScr::UnpressButtons(void)
{
  for(uint32_t i = 0u; i < NumberOf(scale_btn); i++)
  {
    scale_btn[i].SetPressed(false);
    scale_btn[i].SetColor(COLOR_WHITE);
  }
}

// *****************************************************************************
// ***   UpdateScaleButtons   **************************************************
// *****************************************************************************
void DirectControlScr::UpdateScaleButtons()
{
  for(uint32_t i = 0u; i < NumberOf(scale_btn); i++)
  {
    memset(scale_str[i], 0, NumberOf(scale_str[i]));

    // Fixed jog scale values, in 1/scaler units. The imperial scaler is 10000
    // (0.0001"), so these render as 0.0005 / 0.0010 / 0.0020 inch.
    // Kept small deliberately: each detent is one $J= increment, and at
    // $120=100 mm/s^2 a coarse step becomes a visible lurch rather than
    // smooth motion. Revisit if acceleration is raised.
    static const uint32_t mill_scale[NumberOf(scale_btn)] = {5u, 10u, 20u};
    scale_val[i] = mill_scale[i];
    grbl_comm.ValueToStringWithScalerAndUnits(scale_str[i], NumberOf(scale_str[i]),
      scale_val[i], grbl_comm.GetReportUnitsScaler(GrblComm::AXIS_X),
      grbl_comm.GetReportUnits(GrblComm::AXIS_X), false);

    // Replace space with newline
    for(uint8_t j = 0; j < NumberOf(scale_str[i]); j++)
    {
      if(scale_str[i][j] == ' ') { scale_str[i][j] = '\n'; break; }
    }

    scale_btn[i].Show(100);
    if(scale_btn[i].GetPressed())
    {
      scale_btn[i].SetColor(COLOR_GREEN);
      scale = scale_val[i];
    }
  }
}

// *****************************************************************************
// ***   Constructor   *********************************************************
// *****************************************************************************
DirectControlScr::DirectControlScr() :
  change_box(Application::GetInstance().GetChangeValueBox()) {}
