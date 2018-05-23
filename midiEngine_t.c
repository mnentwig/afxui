typedef struct {
  // message FIFO from hardware
  midimsg_t* queueInbound;
  // message FIFO to hardware
  midimsg_t* queueOutbound;

  int fd;
  midimsg_t* msgInbound;
} midiEngine_t;


midiEngine_t* midiEngine_new(const char* midiDev){
  midiEngine_t* self = (midiEngine_t*)calloc(sizeof(midiEngine_t), 1); assert(self);
  self->queueInbound = midimsg_new(1); // dummy element
  self->queueOutbound = midimsg_new(1); // dummy element
  
  self->fd = open(MIDIDEV, O_RDWR|O_NONBLOCK); assert((self->fd > 0) && "failed to open MIDI");
    
  // === queue startup message ===
  midimsg_t* m = midimsg_new(16);
  midimsg_sysexHeader(m);
  midimsg_add(m, /*count*/1, /*data*/0x08); // GET_FIRMWARE_VERSION
  midimsg_sysexTrailer(m);
  midimsg_queuePush(self->queueOutbound, m);

  self->msgInbound = NULL;
  return self;
}

void midiEngine_delete(midiEngine_t* self){
  while (!midimsg_queueIsEmpty(self->queueInbound))
    midimsg_delete(midimsg_queuePop(self->queueInbound));
  midimsg_delete(self->queueInbound);

  while (!midimsg_queueIsEmpty(self->queueOutbound))
    midimsg_delete(midimsg_queuePop(self->queueOutbound));
  midimsg_delete(self->queueOutbound);

  close(self->fd);
}

// return value
// 0: caller should sleep
// 1: did some work
int midiEngine_run(midiEngine_t* self){
  char c;
  int n = read(self->fd, (void*)&c, 1);
  if (n < -1){
    fprintf(stderr, "MIDI read: IO error %i\n", n);
    exit(EXIT_FAILURE);
  }
  if (n < 1){
    if (midimsg_queueIsEmpty(self->queueOutbound))
      return 0;
    midimsg_t* m = midimsg_queuePop(self->queueOutbound);
    midimsg_send(m, self->fd);
    midimsg_delete(m);
    return 1;
  }

  if (self->msgInbound == NULL)
    self->msgInbound = midimsg_new(32);
  
  if (c == 0xF0)
    midimsg_clear(self->msgInbound);
  
  midimsg_add(self->msgInbound, /*count*/1, /*data*/(int)c);
  if (c == 0xF7){
    midimsg_queuePush(self->queueInbound, self->msgInbound);
    self->msgInbound = NULL;
  }
  return 1;
}

void midiEngine_GET_BLOCK_PARAMETER_VALUE(midiEngine_t* self, int blockId, int paramId){
  midimsg_t* m = midimsg_new(32);
  midimsg_sysexHeader(m);
  midimsg_add(m, /*count*/9,
	      0x02, //GET_BLOCK_PARAMETER_VALUE
	      
	      ((blockId >> 0) & 0x7F),
	      ((blockId >> 7) & 0x7F),
	      
	      ((paramId >> 0) & 0x7F),
	      ((paramId >> 7) & 0x7F),
	      
	      0x00, // unused
	      0x00,
	      0x00,
	      
	      0x00 // GET
	      );
	      
  midimsg_sysexTrailer(m);
  midimsg_queuePush(self->queueOutbound, m);  
}

void midiEngine_SET_BLOCK_PARAMETER_VALUE(midiEngine_t* self, int blockId, int paramId, int val){
  assert(val <= 65534);
  midimsg_t* m = midimsg_new(32);
  midimsg_sysexHeader(m);
  midimsg_add(m, /*count*/9,
	      0x02, //GET_BLOCK_PARAMETER_VALUE
	      
	      ((blockId >> 0) & 0x7F),
	      ((blockId >> 7) & 0x7F),
	      
	      ((paramId >> 0) & 0x7F),
	      ((paramId >> 7) & 0x7F),
	      
	      ((val >> 0) & 0x7F),
	      ((val >> 7) & 0x7F),
	      ((val >> 14) & 0x7F),
	      
	      0x01 // SET
	      );
  
  midimsg_sysexTrailer(m);
  midimsg_queuePush(self->queueOutbound, m);  
}

