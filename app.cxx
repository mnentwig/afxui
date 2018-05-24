typedef struct _ctrlItem {
  int blockId;
  int paramId;
  Fl_Slider* slider;
  Fl_Toggle_Button* button;
} ctrlItem_t;

typedef struct {
  ctrlItem_t* items;
  int nItems;
  midiEngine_t* e;
  Fl_Window *window;
  int keepRunning;
} app_t;

void app_requestHardwareState(app_t* self){
  for (int ix = 0; ix < self->nItems; ++ix)
    midiEngine_GET_BLOCK_PARAMETER_VALUE(self->e, self->items[ix].blockId, self->items[ix].paramId);
}


void myCallback(Fl_Widget* w, void* pApp){
  app_t* app = (app_t*)pApp;
  for (int ix = 0; ix < app->nItems; ++ix){
    if ((Fl_Widget*)app->items[ix].slider == w){
      Fl_Slider* s = (Fl_Slider*)w;
      if (s->changed()){
	// printf("widget changed to %f\n", s->value());
	int value = (int)(s->value() * 65534.0 + 0.5);
	midiEngine_SET_BLOCK_PARAMETER_VALUE(app->e, app->items[ix].blockId, app->items[ix].paramId, value);
	s->clear_changed();
      }
    } else if ((Fl_Widget*)app->items[ix].button == w){
      Fl_Toggle_Button* b = (Fl_Toggle_Button*)w;
      if (b->changed()){
	int value = b->value();
	midiEngine_SET_BLOCK_PARAMETER_VALUE(app->e, app->items[ix].blockId, app->items[ix].paramId, value);
	b->clear_changed();
      }
    }
  }
}
 
void myWindowCallback(Fl_Widget*, void* pApp) {
#if 0
  // Optional: prevent the ESC key for closing the application
  if (Fl::event()==FL_SHORTCUT && Fl::event_key()==FL_Escape) 
    return; // ignore Escape
#endif
  app_t* app = (app_t*)pApp;
  app->keepRunning = 0;
}


app_t* app_new(){
  app_t* self = (app_t*)malloc(sizeof(app_t));
  int X;
  int Y;
  int w;
  int h;
  Fl::screen_xywh (X, Y, w, h);
  self->keepRunning = 1;
  self->window = new Fl_Window(w, h);

  self->window->callback(myWindowCallback, (void*)self);
  self->window->fullscreen();
  self->nItems = 6;
  self->items = (ctrlItem_t*)calloc(self->nItems, sizeof(ctrlItem_t));
  double dw = (double)w / (double)self->nItems;
    
  // === PUT INTERESTING CONTROL ITEMS HERE ===
  // see https://wiki.fractalaudio.com/axefx2/index.php?title=MIDI_SysEx_blocks_and_parameters_IDs#DRV
  for (int ix = 0; ix < self->nItems; ++ix){
    int blockId;
    int paramId;
    const char* tt;
    switch(ix){
    case 0: blockId = 106; paramId =   1; tt = "input drive"; break;
    case 1: blockId = 106; paramId =   2; tt = "bass"; break;
    case 2: blockId = 106; paramId =   3; tt = "middle"; break;
    case 3: blockId = 106; paramId =   4; tt = "treble"; break;
    case 4: blockId = 106; paramId =   5; tt = "master"; break;
    case 5:
    default: blockId = 133; paramId = 255; tt = "OD BYP"; break;
    }    
    
    self->items[ix].blockId = blockId;
    self->items[ix].paramId = paramId;
    if (paramId == 255){
      self->items[ix].button = new Fl_Toggle_Button(ix*dw, 0, dw, h, "");
      self->items[ix].button->tooltip(tt);
      self->items[ix].button->callback(myCallback, (void*)self);
      self->items[ix].button->color(FL_DARK_RED, FL_GRAY);
    } else {
      self->items[ix].slider = new Fl_Slider(ix*dw, 0, dw, h, "");
      self->items[ix].slider->tooltip(tt);
      self->items[ix].slider->type(FL_VERT_FILL_SLIDER);
      self->items[ix].slider->callback(myCallback, (void*)self);
      self->items[ix].slider->color(FL_GRAY, FL_RED);
    }
  }   

  self->window->end();  
  self->window->show();
  
  self->e = midiEngine_new(MIDIDEV);
  app_requestHardwareState(self);
  return self;
}

void app_delete(app_t* self){
    midiEngine_delete(self->e);
    free(self->items);
    free(self);
}

void app_run(app_t* self){    
  while (self->keepRunning){
    // === handle GUI ===
    Fl::wait(0.001);

    // === send / receive MIDI ===
    while (midiEngine_run(self->e)){}
    
    // === process new inbound data ===
    while (!midimsg_queueIsEmpty(self->e->queueInbound)){
      midimsg_t* m = midimsg_queuePop(self->e->queueInbound);      
      if (midimsg_isSysex(m)){
	//midimsg_dump(m);
	int blockId;
	int paramId;
	int val;
	if (midimsg_is_GET_BLOCK_PARAMETER_VALUE(m, &blockId, &paramId, &val)){
	  for (int ix = 0; ix < self->nItems; ++ix){
	    ctrlItem_t* item = &self->items[ix];
	    if ((item->blockId == blockId) && (item->paramId == paramId)){
	      if (item->slider){
		double fval = ((double)val)/65534.0;		
		// printf("hardware sets to %i\n", val);
		item->slider->value(fval);
		item->slider->clear_changed();
	      } else if (item->button){
		// printf("hardware sets button to %i\n", val);
		item->button->value(val ? 1 : 0);
		item->button->clear_changed();
	      }
	    }
	  }
	} else if (midimsg_is_GET_FIRMWARE_VERSION(m)){
	} else if (midimsg_is_GET_PRESET_NUMBER(m) || midimsg_is_FRONTPANEL_CHANGE_DETECTED(m)){
	  app_requestHardwareState(self);	  
	} else {
	  midimsg_dump(m);
	}
      } else if (midimsg_isTempo(m)){
      } else {
	fprintf(stdout, "unknown message: ");
	midimsg_dump(m);
      }
      midimsg_delete(m);
    }
  }
}
