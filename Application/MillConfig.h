//******************************************************************************
//  @file MillConfig.h
//  @author JPB PCB Mill modifications
//
//  @details Machine-specific constants shared between screens.
//
//           Kept in one place because the same spindle command is issued from
//           both the home screen and the override screen. Duplicating the RPM
//           in two translation units is a trap: change one, miss the other,
//           and the two screens spin the spindle at different speeds.
//
//******************************************************************************

#ifndef MillConfig_h
#define MillConfig_h

// Command issued by the SPINDLE button.
//
// 10000 RPM: this machine cuts PCBs almost exclusively - 0.020" tapered trace
// cutters, 1/8" profile mills and 0.035" drills - and all of them need high
// surface speed for chip clearance at those diameters.
//
// NOTE: the spindle is not wired yet and $30 is still 1000. It must become
// 10800 before this value means anything (10000 is then 93% of full scale).
//
// A string literal rather than a formatted constant: snprintf is not
// guaranteed to be declared in these translation units, and the speed is
// fixed at compile time anyway.
static const char SPINDLE_ON_CMD[]  = "M3 S10000\r";
static const char SPINDLE_OFF_CMD[] = "M5\r";

#endif
