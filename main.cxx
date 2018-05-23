#include <assert.h>
#include <string.h> // memcpy
#include <unistd.h> // read
#include <fcntl.h> // O_NONBLOCK

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Toggle_Button.H>

// [1] https://wiki.fractalaudio.com/axefx2/index.php?title=MIDI_SysEx#MIDI_SysEx:_Axe-Fx_II
#define MIDIMSG_MODELNO (0x07) // [1] "MIDI SysEx: SysEx Model number per device"
#define MIDIDEV ("/dev/snd/midiC1D0")

#include "midimsg_t.c"
#include "midiEngine_t.c"
#include "app.cxx"

int main(int argc, char **argv) {
  app_t* app = app_new();
  app_run(app);
  app_delete(app);
}
