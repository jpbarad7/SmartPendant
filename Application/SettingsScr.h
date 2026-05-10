//******************************************************************************
//  @file SettingsScr.h
//  @author Nicolai Shlapunov / JPB Laser modifications
//
//  @details SettingsScr: Settings screen.
//           Full-screen self-contained layout matching other screens.
//
//******************************************************************************

#ifndef SettingsScr_h
#define SettingsScr_h

#include "DevCfg.h"
#include "DisplayDrv.h"
#include "UiEngine.h"
#include "IScreen.h"
#include "Tabs.h"
#include "InputDrv.h"
#include "NVM.h"
#include "Menu.h"
#include "ChangeValueBox.h"

#define BG_Z (100)

class SettingsScr : public IScreen
{
  public:
    static SettingsScr& GetInstance();
    virtual Result Setup(int32_t y, int32_t height);
    virtual Result Show();
    virtual Result Hide();
    virtual Result TimerExpired(uint32_t interval);
    virtual Result ProcessCallback(const void* ptr);

  private:
    static constexpr uint8_t  BORDER_W   = 4u;
    static constexpr uint32_t MENU_ITEMS = 12u;

    enum { GENERAL_TAB, MPG_TAB, PROBE_TAB, MAX_TABS };

    const char* const menu_strings[NVM::MAX_VALUES] =
    {
      "Version",
      "MPG request", "Display Inversion", "Auto MPG on startup", "Save script result",
      "Metric Feed 1", "Metric Feed 2", "Metric Feed 3", "Metric Feed 4",
      "Imperial Feed 1", "Imperial Feed 2", "Imperial Feed 3", "Imperial Feed 4",
      "Rotary Feed 1", "Rotary Feed 2", "Rotary Feed 3", "Rotary Feed 4",
      "Search speed", "Lock speed", "Position deviation", "Ball tip"
    };

    // Navigation bar
    UiButton prev_btn;
    UiButton next_btn;
    String   title_str;

    // Settings tabs
    Tabs tabs;

    // Menu
    char           str[MENU_ITEMS][32u + 1u] = {0};
    Menu::MenuItem menu_items[MENU_ITEMS];
    Menu           menu;

    // Shutdown button
    UiButton shutdown_btn;
    bool     shutdown_active = false;
    Box      cover_box; // masks shared buttons visible below shutdown button

    // Change value box
    ChangeValueBox& change_box;

    // Driver instances
    DisplayDrv& display_drv = DisplayDrv::GetInstance();
    GrblComm&   grbl_comm   = GrblComm::GetInstance();
    NVM&        nvm         = NVM::GetInstance();

    InputDrv::CallbackListEntry btn_cble;

    static Result ProcessMenuCallback(SettingsScr* obj_ptr, void* ptr);
    static Result ProcessButtonCallback(SettingsScr* obj_ptr, void* ptr);
    void UpdateStrings();

    SettingsScr();
};

#endif
