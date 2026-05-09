//******************************************************************************
//  @file OverrideCtrlScr.h
//  @author Nicolai Shlapunov / JPB Laser modifications
//
//  @details OverrideCtrlScr: Feed and power override screen.
//           Full-screen self-contained layout matching home screen pattern.
//
//******************************************************************************

#ifndef OverrideCtrlScr_h
#define OverrideCtrlScr_h

#include "DevCfg.h"
#include "DisplayDrv.h"
#include "UiEngine.h"
#include "IScreen.h"
#include "DataWindow.h"
#include "GrblComm.h"
#include "InputDrv.h"

#define BG_Z (100)

class OverrideCtrlScr : public IScreen
{
  public:
    static OverrideCtrlScr& GetInstance();
    virtual Result Setup(int32_t y, int32_t height);
    virtual Result Show();
    virtual Result Hide();
    virtual Result TimerExpired(uint32_t interval);
    virtual Result ProcessCallback(const void* ptr);

  private:
    static const uint8_t BORDER_W = 4u;

    // Navigation bar (same pattern as home screen)
    UiButton prev_btn;
    UiButton next_btn;
    String   title_str;

    // Feed override
    String     feed_name;
    DataWindow feed_dw;
    UiButton   feed_reset_btn;
    int32_t    feed_val = 0;

    // Power override
    String     speed_name;
    DataWindow speed_dw;
    UiButton   speed_reset_btn;
    int32_t    speed_val = 0;

    // Status display
    String hdr_state;
    String hdr_status_sub;

    // Aux row: AIR | EXHAUST | FIRE | MPG
    UiButton flood_btn;    // AIR   (M8)
    UiButton mist_btn;     // EXHAUST (M7)
    UiButton fire_ovr_btn; // FIRE test
    UiButton mpg_ovr_btn;  // MPG toggle
    bool     fire_active = false;

    // Bottom row
    UiButton run_btn;
    UiButton stop_btn;

    // Layout values computed in Setup, used in Show
    int32_t axis_dro_y    = 0;
    int32_t axis_dro_h    = 0;
    int32_t content_start = 0;

    // Driver instances
    DisplayDrv& display_drv = DisplayDrv::GetInstance();
    GrblComm&   grbl_comm   = GrblComm::GetInstance();

    // Encoder callback
    InputDrv::CallbackListEntry enc_cble;

    static Result ProcessEncoderCallback(OverrideCtrlScr* obj_ptr, void* ptr);

    OverrideCtrlScr() {};
};

#endif
