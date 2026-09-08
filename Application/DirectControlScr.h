//******************************************************************************
//  @file DirectControlScr.h
//  @author Nicolai Shlapunov / JPB PCB Mill modifications
//
//  @details DirectControlScr: Mill home screen - full screen self-contained layout
//
//******************************************************************************

#ifndef DirectControlScr_h
#define DirectControlScr_h

#include "DevCfg.h"
#include "DisplayDrv.h"
#include "UiEngine.h"
#include "IScreen.h"
#include "DataWindow.h"
#include "GrblComm.h"
#include "InputDrv.h"
#include "ChangeValueBox.h"
#include "Version.h"

#define BG_Z (100)

class DirectControlScr : public IScreen
{
  public:
    static DirectControlScr& GetInstance();
    virtual Result Setup(int32_t y, int32_t height);
    virtual Result Show();
    virtual Result Hide();
    virtual Result TimerExpired(uint32_t interval);
    virtual Result ProcessCallback(const void* ptr);

  private:
    static constexpr uint8_t  BORDER_W   = 4u;
    static constexpr uint32_t DRO_MARGIN = BORDER_W * 2u; // extra space above/below DRO section

    // Jogging values
    int32_t axis_jog_val[GrblComm::AXIS_CNT] = {0};
    int32_t axis_jog_dir[GrblComm::AXIS_CNT] = {0};

    // Current selected axis
    GrblComm::Axis_t axis = GrblComm::AXIS_CNT;
    // Jog scale
    int32_t scale = 1u;

    // Axis name strings and DRO windows
    String     axis_names[GrblComm::AXIS_CNT];
    DataWindow dw[GrblComm::AXIS_CNT];
    UiButton   zero_btn[GrblComm::AXIS_CNT];

    // X mode (lathe, not used in laser but kept for compatibility)
    UiButton x_mode_btn;
    String   x_mode_str;

    // Scale buttons
    UiButton scale_btn[3u];
    char     scale_str[NumberOf(scale_btn)][12u] = {0};
    uint32_t scale_val[NumberOf(scale_btn)]      = {0};

    // Navigation bar (replaces global header on home screen)
    UiButton prev_btn;        // < previous screen
    UiButton next_btn;        // > next screen
    String   hdr_state;       // machine state text
    String   hdr_status_sub;  // machine status sub-text

    // Aux button row: VAC | SPINDLE | MPG
    UiButton vac_btn;          // VAC - dust collector (M8, coolant flood)
    UiButton spindle_btn;      // Spindle on/off (M3/M5)
    UiButton mpg_home_btn;     // MPG toggle

    // Spindle state. Tracked locally because grblHAL reports spindle state only
    // in the full status report, which this screen does not request.
    bool spindle_on = false;

    // Bottom row
    UiButton run_btn;   // Run / Hold
    UiButton stop_btn;  // Stop / Reset / Unlock

    // Change value box (shared from Application)
    ChangeValueBox& change_box;

    // Driver instances
    DisplayDrv& display_drv = DisplayDrv::GetInstance();
    GrblComm&   grbl_comm   = GrblComm::GetInstance();

    // Encoder callback
    InputDrv::CallbackListEntry enc_cble;

    void UnpressButtons();
    static Result ProcessEncoderCallback(DirectControlScr* obj_ptr, void* ptr);
    void UpdateScaleButtons();

    DirectControlScr();
};

#endif
