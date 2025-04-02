#include "neske.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../../umbox/umka_api/umka_api.h"

uint32_t pallete[] =
{
  0x626262ff, 0x001fb2ff, 0x2404c8ff, 0x5200b2ff,
  0x730076ff, 0x800024ff, 0x730b00ff, 0x522800ff,
  0x244400ff, 0x005700ff, 0x005c00ff, 0x005324ff,
  0x003c76ff, 0x000000ff, 0x000000ff, 0x000000ff,
  0xabababff, 0x0d57ffff, 0x4b30ffff, 0x8a13ffff,
  0xbc08d6ff, 0xd21269ff, 0xc72e00ff, 0x9d5400ff,
  0x607b00ff, 0x209800ff, 0x00a300ff, 0x009942ff,
  0x007db4ff, 0x000000ff, 0x000000ff, 0x000000ff,
  0xffffffff, 0x53aeffff, 0x9085ffff, 0xd365ffff,
  0xff57ffff, 0xff5dcfff, 0xff7757ff, 0xfa9e00ff,
  0xbdc700ff, 0x7ae700ff, 0x43f611ff, 0x26ef7eff,
  0x2cd5f6ff, 0x4e4e4eff, 0x000000ff, 0x000000ff,
  0xffffffff, 0xb6e1ffff, 0xced1ffff, 0xe9c3ffff,
  0xffbcffff, 0xffbdf4ff, 0xffc6c3ff, 0xffd59aff,
  0xe9e681ff, 0xcef481ff, 0xb6fb9aff, 0xa9fac3ff,
  0xa9f0f4ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff,
};

struct frame_result
{
  int32_t state; // 0 = ok, 1 = crashed
  struct nrom_frame_result result;
};

void _noLock(void *_) {}
void _noUnlock(void *_) {}

struct mux_api _getDummyMux()
{
  return (struct mux_api){
    NULL,
    _noLock,
    _noUnlock
  };
}

void dummyFree(UmkaStackSlot *params, UmkaStackSlot *result)
{

}

UMKA_EXPORT void neskeInit(UmkaStackSlot *params, UmkaStackSlot *result)
{
  void *umka = result->ptrVal;
  UmkaAPI *api = umkaGetAPI(umka);

  UmkaDynArray(uint8_t) *source = umkaGetParam(params, 0)->ptrVal;
  size_t size = api->umkaGetDynArrayLen(source);

  if (size < 16) {
    umkaGetResult(params, result)->ptrVal = 0;
    return;
  }

  struct nrom nrom = { 0 };
  nrom.apu_mux = _getDummyMux();
  if (nrom_load(source->data, &nrom)) {
    umkaGetResult(params, result)->ptrVal = 0;
    return;
  }

  void *data = api->umkaAllocData(umka, sizeof nrom, dummyFree);

  memcpy(data, &nrom, sizeof nrom);

  umkaGetResult(params, result)->ptrVal = data;
}

UMKA_EXPORT void neskeFrame(UmkaStackSlot *params, UmkaStackSlot *result)
{
  void *umka = result->ptrVal;
  UmkaAPI *api = umkaGetAPI(result->ptrVal);

  struct nrom *nrom = umkaGetParam(params, 0)->ptrVal;
  
  printf("Test %p\n", nrom);

  struct nrom_frame_result frame = nrom_frame(nrom);

  *((struct frame_result*)umkaGetResult(params, result)->ptrVal) = (struct frame_result){
    nrom->cpu.crash ? 1 : 0,
    frame
  };
}
