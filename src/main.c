/*************** t.c file *************/
#include <unistd.h>

#include "defines.h"
#include "exceptions.h"
#include "timer.h"
#include "vid.h"

extern char _binary____image0_bmp_start;

/*************** C3.3. t.c file *******************/
void copy_vectors(void) {
  extern u32 vectors_start, vectors_end;
  u32* vectors_src = &vectors_start;
  u32* vectors_dst = (u32*)0;
  while (vectors_src < &vectors_end) *vectors_dst++ = *vectors_src++;
}

void timer0_handler() { timer_handler(0); }

// Use vectored interrupts of PL190 VIC
int vectorInt_init() {
  printf("vectorInterrupt_init()\n");
  /*********** set up vectored interrupts *****************
  (1). write to vectoraddr0 (0x100) with ISR of timer0
  *********************************************************/
  *((int*)(VIC_BASE_ADDR + 0x100)) = (int)timer0_handler;
  // (2). write to intControlRegs = E=1|IRQ#=1xxxxx
  *((int*)(VIC_BASE_ADDR + 0x200)) = 0x24;  // 100100 at IRQ 4
  // (3). write 0's to IntSelectReg to generate IRQ interrupts
  //(any bit=1 generates FIQ interrupt)
  *((int*)(VIC_BASE_ADDR + 0x0C)) = 0;
}

extern char _binary____image0_bmp_start;

void IRQ_handler() {
  void* (*f)();  // f as a function pointer
  int status = *(int*)(VIC_BASE_ADDR + 0x30);
  f = (void*)*((int*)(VIC_BASE_ADDR + 0x30));
  f();                                  // call the ISR function
  *((int*)(VIC_BASE_ADDR + 0x30)) = 1;  // write to vectorAddr as EOI
}

int wall_clock(TIMER* t) {
  int i, ss, mm, hh;
  lock();
  ss = t->ss;
  mm = t->mm;
  hh = t->hh;  // copy from timer struct
  unlock();
  for (i = 0; i < 8; i++) unkpchar(t->clock[i], 0, 70 + i);
  t->clock[7] = '0' + (ss % 10);
  t->clock[6] = '0' + (ss / 10);
  t->clock[4] = '0' + (mm % 10);
  t->clock[3] = '0' + (mm / 10);
  t->clock[1] = '0' + (hh % 10);
  t->clock[0] = '0' + (hh / 10);
  for (i = 0; i < 8; i++)
    kpchar(t->clock[i], 0, 70 + i);  // kputchr(char, row, col)
}

extern volatile int one_second, five_seconds;

int main() {
  fbuf_init();

  VIC_INTENABLE = (1 << 4);  // timer0 at bit4

  vectorInt_init();

  timer_init();
  timer_start(0);

  char* p = &_binary____image0_bmp_start;
  show_bmp(p, 0, 0);

  printf("C4.3: Periodic Events Program\n");
  for (uint32_t itr = 0; 1; itr++) {
    if (one_second) {
      wall_clock(&timer[0]);
      one_second = 0;
    }
    if (five_seconds) {
      printf("five seconds event\n");
      five_seconds = 0;
    }

    asm("MOV r0, #0; MCR p15, 0, R0, c7, c0, 4");  // enter WFI mode
    printf("CPU come out WFI state (%d)\n", itr);
  }
}