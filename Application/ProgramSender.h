//******************************************************************************
//  @file ProgramSender.h
//  @author Nicolai Shlapunov / JPB Laser modifications
//
//  @details ProgramSender: G-code file sender screen.
//           Full-screen self-contained layout matching other screens.
//           Bottom: status | AIR/EXHAUST/FIRE/MPG | RUN/STOP
//           File open triggered by tapping text area.
//
//******************************************************************************

#ifndef ProgramSender_h
#define ProgramSender_h

#include "DevCfg.h"
#include "DisplayDrv.h"
#include "UiEngine.h"
#include "IScreen.h"
#include "DataWindow.h"
#include "GrblComm.h"
#include "InputDrv.h"
#include "Menu.h"
#include "TextBox.h"

#define BG_Z (100)

class ProgramSender : public IScreen
{
  public:
    static ProgramSender& GetInstance();
    virtual Result Setup(int32_t y, int32_t height);
    virtual Result Show();
    virtual Result Hide();
    virtual Result TimerExpired(uint32_t interval);
    virtual Result ProcessCallback(const void* ptr);

    char*    AllocateDataBuffer(uint32_t& size);
    char*    GetDataBufferPtr()    {return p_text;}
    uint32_t GetDataBufferLength() {return strlen(p_text);}
    void     ReleaseDataPointer();

  private:
    static const uint8_t BORDER_W = 4u;

    // Run state
    bool     run      = false;
    bool     finished = false;
    uint32_t idx      = 0u;
    uint32_t id       = 0u;

    // G-code buffer
    char* p_text = nullptr;

    // File browser menu
    char           str[32u][32u + 1u] = {0};
    Menu::MenuItem menu_items[32u];
    Menu           menu;

    // G-code text display
    TextBox text_box;

    // Navigation bar
    UiButton prev_btn;
    UiButton next_btn;
    String   title_str;

    // Status display
    String hdr_state;
    String hdr_status_sub;

    // Aux row: AIR | EXHAUST | FIRE | MPG
    UiButton flood_btn;    // AIR     (M8)
    UiButton mist_btn;     // EXHAUST (M7)
    UiButton fire_pgm_btn; // FIRE test
    UiButton mpg_pgm_btn;  // MPG toggle
    bool     fire_active = false;

    // Bottom row
    UiButton run_btn;
    UiButton stop_btn;

    // Driver instances
    DisplayDrv& display_drv = DisplayDrv::GetInstance();
    GrblComm&   grbl_comm   = GrblComm::GetInstance();

    // Encoder (for scrolling text)
    int32_t enc_val = 0;
    InputDrv::CallbackListEntry enc_cble;

    static Result ProcessMenuOkCallback(ProgramSender* obj_ptr, void* ptr);
    static Result ProcessMenuCancelCallback(ProgramSender* obj_ptr, void* ptr);
    static Result ProcessEncoderCallback(ProgramSender* obj_ptr, void* ptr);
    void          OpenFileMenu();

    ProgramSender() {};
};

#endif
