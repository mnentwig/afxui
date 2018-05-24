#include <stdarg.h>
typedef struct _midimsg_t{
  int n;
  int nMax;
  char* buf;
  struct _midimsg_t* next;
} midimsg_t;

midimsg_t* midimsg_new(int nMax){
  midimsg_t* self = (midimsg_t*)malloc(sizeof(midimsg_t));
  self->n = 0;
  self->nMax = nMax;
  self->buf = (char*)malloc(self->nMax); assert(self->buf);
  self->next = NULL;
  return self;
}

void midimsg_delete(midimsg_t* self){
  free(self->buf);
  free(self);
}

void midimsg_add(midimsg_t* self, int count, ...){
  va_list ap;
  va_start(ap, count);
  while (self->n+count > self->nMax){
    int tmpNMax = self->nMax*2;
    char* tmpBuf = (char*)malloc(tmpNMax); assert(tmpBuf);
    memcpy(tmpBuf, self->buf, self->n);
    self->nMax = tmpNMax;
    free(self->buf);
    self->buf = tmpBuf;
  }
  
  for (int ix = 0; ix < count; ++ix){
    *(self->buf+self->n) = (char)va_arg(ap, int);
    ++self->n;
  }
  va_end(ap);
}

void midimsg_sysexHeader(midimsg_t* self){
  assert((self->n == 0) && "message may not have content");
  midimsg_add(self, 5,
	      0xF0, 0x00, 0x01, 0x74, MIDIMSG_MODELNO);
}

void midimsg_sysexTrailer(midimsg_t* self){
  assert(self->n > 5);
  assert(self->buf[0] == 0xF0);

  char chksum = 0;
  for (int ix = 0; ix < self->n; ++ix)
    chksum ^= self->buf[ix];
  midimsg_add(self, 2, 
	      chksum & 0x7F, 0xF7);
}

void midimsg_send(midimsg_t* self, int fd){
  char* tmp = self->buf;
  int nRem = self->n;
  while (nRem > 0){
    int n = write(fd, tmp, nRem);
    if (n > 0){
      nRem -= n;
      tmp += n;
    } else sleep(0.005);
  }
}

void midimsg_clear(midimsg_t* self){
  self->n = 0;
}

void midimsg_dump(midimsg_t* self){
  for (int ix = 0; ix < self->n; ++ix){
    fprintf(stdout, "%02x ", (int)(self->buf[ix]));
  }
  fprintf(stdout, "\n"); fflush(stdout);  
}

// === linked list functionality ===

// note: "self" in this context is a dummy element
int midimsg_queueIsEmpty(midimsg_t* self){
  return self->next == NULL;
}

void midimsg_queuePush(midimsg_t* self, midimsg_t* msg){
  while (self->next != NULL)
    self = self->next;
  self->next = msg;
}

midimsg_t* midimsg_queuePop(midimsg_t* self){
  midimsg_t* retVal = self->next; assert(retVal);
  self->next = retVal->next;
  return retVal;
}

// === sysex message tests ===

int midimsg_isSysex(midimsg_t* self){
  if (self->n < 7)
    return 0;
  char chksum = 0;
  for (int ix = 0; ix < self->n-2; ++ix)
    chksum ^= self->buf[ix];
  chksum &= 0x7F;
  
  return 
    (self->buf[0] == 0xF0) &&
    (self->buf[1] == 0x00) &&
    (self->buf[2] == 0x01) &&
    (self->buf[3] == 0x74) &&
    (self->buf[4] == MIDIMSG_MODELNO) &&
    (self->buf[self->n-2] == chksum) &&
    (self->buf[self->n-1] == 0xF7);
}

int midimsg_isTempo(midimsg_t* self){
  if (self->n != 7)
    return 0;
  return 
    (self->buf[0] == 0xF0) &&
    (self->buf[1] == 0x00) &&
    (self->buf[2] == 0x01) &&
    (self->buf[3] == 0x74) &&
    (self->buf[4] == MIDIMSG_MODELNO) &&
    (self->buf[5] == 0x10) &&
    (self->buf[6] == 0xF7);
}

int midimsg_is_GET_BLOCK_PARAMETER_VALUE(midimsg_t* self, int* blockId, int* paramId, int* val){
  if (self->n < 14)
    return 0;
  if (self->buf[5] != 0x02)
    return 0;  
  *blockId = self->buf[6] | (self->buf[7] << 7);
  *paramId = self->buf[8] | (self->buf[9] << 7);
  *val = self->buf[10] | (self->buf[11] << 7) | (self->buf[12] << 14);
  return 1;
}

int midimsg_is_GET_FIRMWARE_VERSION(midimsg_t* self){
  if (self->n != 14)
    return 0;
  return (self->buf[5] == 0x08);
}

int midimsg_is_FRONTPANEL_CHANGE_DETECTED(midimsg_t* self){
  if (self->n != 8)
    return 0;
  return (self->buf[5] == 0x21);
}

int midimsg_is_GET_PRESET_NUMBER(midimsg_t* self){
  if (self->n != 10)
    return 0;
  return (self->buf[5] == 0x14);
}
