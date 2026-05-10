//******************************************************************************
//  @file SettingsScr.cpp
//  @author Nicolai Shlapunov / JPB Laser modifications
//
//  @details SettingsScr: Full-screen layout matching other screens.
//
//******************************************************************************

#include "SettingsScr.h"
#include "Application.h"

// *****************************************************************************
SettingsScr& SettingsScr::GetInstance()
{
  static SettingsScr settings_scr;
  return settings_scr;
}

// *****************************************************************************
// ***   Setup   ***************************************************************
// *****************************************************************************
Result SettingsScr::Setup(int32_t y, int32_t height)
{
  int32_t  scr_w         = display_drv.GetScreenW(); // 320
  int32_t  scr_h         = display_drv.GetScreenH(); // 480
  uint32_t window_height = Font_8x12::GetInstance().GetCharH() * 5u; // 60px

  // *** Nav bar — same geometry as all other screens ***
  int32_t dro_start_x = scr_w / 6;
  int32_t dro_end_x   = dro_start_x + (scr_w - BORDER_W * 2) * 4 / 6;
  int32_t arrow_w     = scr_w - dro_end_x - BORDER_W * 2; // 51px

  int32_t total_h = 7 * (int32_t)window_height + 7 * BORDER_W + BORDER_W * 2; // 456
  int32_t nav_y   = (scr_h - total_h) / 2; // 12
  int32_t nav_end = nav_y + (int32_t)window_height; // 72

  prev_btn.SetParams("<", BORDER_W, nav_y, arrow_w, window_height, true);
  prev_btn.SetCallback(AppTask::GetCurrent());

  next_btn.SetParams(">", scr_w - BORDER_W - arrow_w, nav_y, arrow_w, window_height, true);
  next_btn.SetCallback(AppTask::GetCurrent());

  title_str.SetParams("SETTINGS", 0, 0, COLOR_WHITE, Font_12x16::GetInstance());
  title_str.Move((scr_w - title_str.GetWidth()) / 2,
                 nav_y + ((int32_t)window_height - Font_12x16::GetInstance().GetCharH()) / 2);

  // *** Shutdown button — same y and height as RUN/STOP on other screens ***
  int32_t shutdown_y = nav_y + total_h - (int32_t)window_height; // 408
  int32_t shutdown_h = (int32_t)window_height;                    // 60 — matches other screens

  shutdown_btn.SetParams("SHUT DOWN", 0, shutdown_y, scr_w, shutdown_h, true);
  shutdown_btn.SetFont(Font_12x16::GetInstance());
  shutdown_btn.SetCallback(AppTask::GetCurrent());
  shutdown_btn.SetColor(COLOR_WHITE);

  // Cover box masks shared soft buttons visible below shutdown button
  int32_t cover_y = shutdown_y + shutdown_h;
  int32_t cover_h = scr_h - cover_y;
  cover_box.SetParams(0, cover_y, scr_w, cover_h, COLOR_DARKGREY, true);

  // *** Tabs below nav bar ***
  int32_t tabs_y = nav_end + BORDER_W; // 76
  tabs.SetParams(0, tabs_y, scr_w, 40, 3u);
  tabs.SetText(0u, "GENERAL", nullptr, Font_10x18::GetInstance());
  tabs.SetText(1u, "MPG",     nullptr, Font_10x18::GetInstance());
  tabs.SetText(2u, "PROBE",   nullptr, Font_10x18::GetInstance());
  tabs.SetCallback(AppTask::GetCurrent());

  // *** Menu fills space between tabs and shutdown button ***
  int32_t menu_y = tabs_y + 40 + BORDER_W; // 120
  int32_t menu_h = shutdown_y - BORDER_W - menu_y; // 284

  for(uint32_t i = 0u; i < NumberOf(menu_items); i++)
  {
    menu_items[i].text = str[i];
    menu_items[i].n    = sizeof(str[i]);
  }
  menu.SetCallback(AppTask::GetCurrent(), this,
                   reinterpret_cast<CallbackPtr>(ProcessMenuCallback), nullptr);
  menu.Setup(0, menu_y, scr_w, menu_h);

  UpdateStrings();

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Show   ****************************************************************
// *****************************************************************************
Result SettingsScr::Show()
{
  // Nav bar at z=3000 — above Application header (z=2000) and tabs (z=100)
  prev_btn.Show(3000);
  next_btn.Show(3000);
  title_str.Show(3000);

  // Tabs and menu at z=100
  tabs.Show(100);

  if(!Application::GetInstance().GetMsgBox().IsShow())
    menu.Show(100);

  // Shutdown button at z=3000 — same level as nav bar
  shutdown_active = false;
  shutdown_btn.SetString("SHUT DOWN");
  shutdown_btn.SetColor(COLOR_WHITE);
  shutdown_btn.Show(3000);
  cover_box.Show(2500);

  UpdateStrings();

  // Physical button callback for tab switching
  // Physical buttons disabled
  // InputDrv::GetInstance().AddButtonsCallbackHandler(AppTask::GetCurrent(),
  //   reinterpret_cast<CallbackPtr>(ProcessButtonCallback), this,
  //   InputDrv::BTNM_LEFT_DOWN | InputDrv::BTNM_RIGHT_DOWN, btn_cble);

  // Hide global UI LAST — ensures Application header and shared buttons
  // are hidden even if tabs.Show() or menu.Show() re-showed them
  Application::GetInstance().HideGlobalUI();
  Application::GetInstance().GetLeftButton().Hide();
  Application::GetInstance().GetRightButton().Hide();
  Application::GetInstance().GetMiddleButton().Hide();

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   Hide   ****************************************************************
// *****************************************************************************
Result SettingsScr::Hide()
{
  // InputDrv::GetInstance().DeleteButtonsCallbackHandler(btn_cble);

  prev_btn.Hide();
  next_btn.Hide();
  title_str.Hide();

  menu.Hide();
  tabs.Hide();
  shutdown_btn.Hide();
  cover_box.Hide();

  nvm.WriteData();

  Application::GetInstance().ShowGlobalUI();

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   TimerExpired   ********************************************************
// *****************************************************************************
Result SettingsScr::TimerExpired(uint32_t interval)
{
  shutdown_btn.SetColor(shutdown_active ? COLOR_GREEN : COLOR_WHITE);
  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   ProcessCallback   *****************************************************
// *****************************************************************************
Result SettingsScr::ProcessCallback(const void* ptr)
{
  if(ptr == &prev_btn)
  {
    Application::GetInstance().PrevScreen();
  }
  else if(ptr == &next_btn)
  {
    Application::GetInstance().NextScreen();
  }
  else if(ptr == &shutdown_btn)
  {
    if(!shutdown_active)
    {
      uint32_t id = 0u;
      bool was_in_control = grbl_comm.GetMpgModeRequest();
      if(!was_in_control)
      {
        grbl_comm.GainControl();
        vTaskDelay(500u / portTICK_PERIOD_MS);
      }
      grbl_comm.SendRealTimeCmd(GrblComm::CMD_STOP);
      grbl_comm.SendCmd("M9\r", id);
      grbl_comm.SendCmd("M64 P0\r", id);
      grbl_comm.SendCmd("M65 P0\r", id);
      shutdown_active = true;
      shutdown_btn.SetString("SAFE TO SHUT DOWN");
      shutdown_btn.SetColor(COLOR_GREEN);
    }
    else
    {
      uint32_t id = 0u;
      grbl_comm.SendCmd("M64 P0\r", id);
      shutdown_active = false;
      shutdown_btn.SetString("SHUT DOWN");
      shutdown_btn.SetColor(COLOR_WHITE);
    }
  }
  else if(ptr == &tabs)
  {
    UpdateStrings();
    menu.Show(100);
    Application::GetInstance().GetLeftButton().Hide();
    Application::GetInstance().GetRightButton().Hide();
    Application::GetInstance().GetMiddleButton().Hide();
  }
  else if(ptr == &change_box)
  {
    if(change_box.GetResult())
    {
      if(tabs.GetSelectedTab() == MPG_TAB)
        nvm.SetValue((NVM::Parameters)(change_box.GetId() + NVM::MPG_METRIC_FEED_1), change_box.GetValue());
      else if(tabs.GetSelectedTab() == PROBE_TAB)
        nvm.SetValue((NVM::Parameters)(change_box.GetId() + NVM::PROBE_SEARCH_FEED),
                     grbl_comm.ConvertUnitsToMetric(change_box.GetValue()));
      else { ; }
      UpdateStrings();
    }
  }

  if(ptr == &Application::GetInstance().GetMsgBox())
  {
    menu.Show(100);
    Application::GetInstance().GetLeftButton().Hide();
    Application::GetInstance().GetRightButton().Hide();
    Application::GetInstance().GetMiddleButton().Hide();
  }
  else { ; }

  return Result::RESULT_OK;
}

// *****************************************************************************
// ***   ProcessMenuCallback   *************************************************
// *****************************************************************************
Result SettingsScr::ProcessMenuCallback(SettingsScr* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;

  if(obj_ptr != nullptr)
  {
    SettingsScr& ths = *obj_ptr;
    uint32_t idx = (uint32_t)ptr;

    if(ths.tabs.GetSelectedTab() == GENERAL_TAB)
    {
      uint32_t nvm_idx = idx + NVM::TX_CONTROL;
      if(nvm_idx == NVM::TX_CONTROL)
      {
        uint8_t val = ths.nvm.GetCtrlTx() + 1u;
        if(val >= GrblComm::CTRL_TX_CNT) val = 0u;
        ths.nvm.SetCtrlTx(val);
      }
      else if(nvm_idx == NVM::SCREEN_INVERT)
      {
        ths.nvm.SetValue(NVM::SCREEN_INVERT, !ths.nvm.GetValue(NVM::SCREEN_INVERT));
        ths.display_drv.InvertDisplay(ths.nvm.GetValue(NVM::SCREEN_INVERT));
      }
      else if(nvm_idx == NVM::AUTO_MPG_ON_START)
        ths.nvm.SetValue(NVM::AUTO_MPG_ON_START, !ths.nvm.GetValue(NVM::AUTO_MPG_ON_START));
      else if(nvm_idx == NVM::SAVE_SCRIPT_RESULT)
        ths.nvm.SetValue(NVM::SAVE_SCRIPT_RESULT, !ths.nvm.GetValue(NVM::SAVE_SCRIPT_RESULT));
      else { ; }
    }
    else if(ths.tabs.GetSelectedTab() == MPG_TAB)
    {
      uint32_t nvm_idx = idx + NVM::MPG_METRIC_FEED_1;
      const char* units = nullptr;
      uint32_t precision = 0;
      if((nvm_idx >= NVM::MPG_METRIC_FEED_1) && (nvm_idx <= NVM::MPG_METRIC_FEED_4))
      { units = ths.grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_METRIC); precision = ths.grbl_comm.GetUnitsPrecision(GrblComm::MEASUREMENT_SYSTEM_METRIC); }
      else if((nvm_idx >= NVM::MPG_IMPERIAL_FEED_1) && (nvm_idx <= NVM::MPG_IMPERIAL_FEED_4))
      { units = ths.grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL); precision = ths.grbl_comm.GetUnitsPrecision(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL); }
      else if((nvm_idx >= NVM::MPG_ROTARY_FEED_1) && (nvm_idx <= NVM::MPG_ROTARY_FEED_4))
      { units = ths.grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_ROTARY); precision = ths.grbl_comm.GetUnitsPrecision(GrblComm::MEASUREMENT_SYSTEM_ROTARY); }
      else { ; }
      if(units != nullptr)
      {
        ths.change_box.Setup(ths.menu_strings[idx], units, ths.nvm.GetValue((NVM::Parameters)(idx + NVM::MPG_METRIC_FEED_1)), 1, 10000, precision, 1u);
        ths.change_box.SetCallback(AppTask::GetCurrent());
        ths.change_box.SetId(idx);
        ths.change_box.Show(10000u);
      }
    }
    else if(ths.tabs.GetSelectedTab() == PROBE_TAB)
    {
      uint32_t nvm_idx = idx + NVM::PROBE_SEARCH_FEED;
      const char* units = nullptr;
      uint32_t precision = 0;
      if((nvm_idx == NVM::PROBE_BALL_TIP) || (nvm_idx == NVM::PROBE_POS_DEVIATION))
      { units = ths.grbl_comm.GetReportUnits(); precision = ths.grbl_comm.GetReportUnitsPrecision(); }
      else if((nvm_idx == NVM::PROBE_SEARCH_FEED) || (nvm_idx == NVM::PROBE_LOCK_FEED))
      { units = ths.grbl_comm.GetReportSpeedUnits(); precision = 0; }
      else { ; }
      if(units != nullptr)
      {
        ths.change_box.Setup(ths.menu_strings[idx], units, ths.grbl_comm.ConvertMetricToUnits(ths.nvm.GetValue((NVM::Parameters)(idx + NVM::PROBE_SEARCH_FEED))), 1, 10000, precision, 1u);
        ths.change_box.SetCallback(AppTask::GetCurrent());
        ths.change_box.SetId(idx);
        ths.change_box.Show(10000u);
      }
    }
    else { ; }

    ths.UpdateStrings();
    result = Result::RESULT_OK;
  }

  return result;
}

// *****************************************************************************
// ***   ProcessButtonCallback   ***********************************************
// *****************************************************************************
Result SettingsScr::ProcessButtonCallback(SettingsScr* obj_ptr, void* ptr)
{
  Result result = Result::ERR_NULL_PTR;
  if(obj_ptr != nullptr)
  {
    SettingsScr& ths = *obj_ptr;
    InputDrv::ButtonCallbackData btn = *((InputDrv::ButtonCallbackData*)ptr);
    if((ths.tabs.IsEnabled()) && (btn.state == false))
    {
      if(btn.btn == InputDrv::BTN_LEFT_DOWN)
        ths.tabs.SetSelectedTab(ths.tabs.GetSelectedTab() - 1u);
      else if(btn.btn == InputDrv::BTN_RIGHT_DOWN)
        ths.tabs.SetSelectedTab(ths.tabs.GetSelectedTab() + 1u);
      else { ; }
      ths.UpdateStrings();
    }
    result = Result::RESULT_OK;
  }
  return result;
}

// *****************************************************************************
// ***   UpdateStrings   *******************************************************
// *****************************************************************************
void SettingsScr::UpdateStrings(void)
{
  char tmp_str[16u] = {0};
  uint32_t cnt = 0;

  if(tabs.GetSelectedTab() == GENERAL_TAB)
  {
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::TX_CONTROL],
      (nvm.GetCtrlTx() == GrblComm::CTRL_GPIO_PIN)       ? "dedicated pin" :
      (nvm.GetCtrlTx() == GrblComm::CTRL_SW_COMMAND)     ? "sw command"    :
      (nvm.GetCtrlTx() == GrblComm::CTRL_PIN_AND_SW_CMD) ? "pin & sw cmd"  : "full control");
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::SCREEN_INVERT],      nvm.GetValue(NVM::SCREEN_INVERT)      ? "inverted"  : "normal");
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::AUTO_MPG_ON_START],  nvm.GetValue(NVM::AUTO_MPG_ON_START)  ? "enabled"   : "disabled");
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::SAVE_SCRIPT_RESULT], nvm.GetValue(NVM::SAVE_SCRIPT_RESULT) ? "enabled"   : "disabled");
  }
  else if(tabs.GetSelectedTab() == MPG_TAB)
  {
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_METRIC_FEED_1],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_METRIC_FEED_1),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_METRIC),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_METRIC)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_METRIC_FEED_2],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_METRIC_FEED_2),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_METRIC),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_METRIC)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_METRIC_FEED_3],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_METRIC_FEED_3),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_METRIC),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_METRIC)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_METRIC_FEED_4],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_METRIC_FEED_4),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_METRIC),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_METRIC)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_IMPERIAL_FEED_1], grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_IMPERIAL_FEED_1), grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL), grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_IMPERIAL_FEED_2], grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_IMPERIAL_FEED_2), grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL), grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_IMPERIAL_FEED_3], grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_IMPERIAL_FEED_3), grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL), grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_IMPERIAL_FEED_4], grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_IMPERIAL_FEED_4), grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL), grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_IMPERIAL)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_ROTARY_FEED_1],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_ROTARY_FEED_1),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_ROTARY),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_ROTARY)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_ROTARY_FEED_2],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_ROTARY_FEED_2),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_ROTARY),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_ROTARY)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_ROTARY_FEED_3],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_ROTARY_FEED_3),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_ROTARY),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_ROTARY)));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::MPG_ROTARY_FEED_4],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), nvm.GetValue(NVM::MPG_ROTARY_FEED_4),   grbl_comm.GetUnitsScaler(GrblComm::MEASUREMENT_SYSTEM_ROTARY),   grbl_comm.GetUnits(GrblComm::MEASUREMENT_SYSTEM_ROTARY)));
  }
  else if(tabs.GetSelectedTab() == PROBE_TAB)
  {
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::PROBE_SEARCH_FEED],   grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), grbl_comm.ConvertMetricToUnits(nvm.GetValue(NVM::PROBE_SEARCH_FEED)),   grbl_comm.GetReportSpeedScaler(), grbl_comm.GetReportSpeedUnits()));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::PROBE_LOCK_FEED],     grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), grbl_comm.ConvertMetricToUnits(nvm.GetValue(NVM::PROBE_LOCK_FEED)),     grbl_comm.GetReportSpeedScaler(), grbl_comm.GetReportSpeedUnits()));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::PROBE_POS_DEVIATION], grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), grbl_comm.ConvertMetricToUnits(nvm.GetValue(NVM::PROBE_POS_DEVIATION)), grbl_comm.GetReportUnitsScaler(), grbl_comm.GetReportUnits()));
    menu.CreateString(menu_items[cnt++], menu_strings[NVM::PROBE_BALL_TIP],      grbl_comm.ValueToStringWithScalerAndUnits(tmp_str, NumberOf(tmp_str), grbl_comm.ConvertMetricToUnits(nvm.GetValue(NVM::PROBE_BALL_TIP)),      grbl_comm.GetReportUnitsScaler(), grbl_comm.GetReportUnits()));
  }
  else { ; }

  menu.SetCount(cnt);
}

// *****************************************************************************
// ***   Constructor   *********************************************************
// *****************************************************************************
SettingsScr::SettingsScr() :
  menu(menu_items, NumberOf(menu_items)),
  change_box(Application::GetInstance().GetChangeValueBox()) {}
