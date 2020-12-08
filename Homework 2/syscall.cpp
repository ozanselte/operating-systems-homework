/* SPIM S20 MIPS simulator.
   Execute SPIM syscalls, both in simulator and bare mode.
   Execute MIPS syscalls in bare mode, when running on MIPS systems.
   Copyright (c) 1990-2010, James R. Larus.
   All rights reserved.

   Redistribution and use in source and binary forms, with or without modification,
   are permitted provided that the following conditions are met:

   Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation and/or
   other materials provided with the distribution.

   Neither the name of the James R. Larus nor the names of its contributors may be
   used to endorse or promote products derived from this software without specific
   prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
   GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _WIN32
#include <unistd.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <io.h>
#endif

#include "spim.h"
#include "string-stream.h"
#include "inst.h"
#include "reg.h"
#include "mem.h"
#include "sym-tbl.h"
#include "spim-utils.h"
#include "scanner.h"
#include "syscall.h"

#include <iostream>
#include <vector>
#include <stdexcept>
using namespace std;

static vector<struct Process *> processTable;
static struct Process *current = NULL;
static bool isRandomSeeded = false;

#ifdef _WIN32
/* Windows has an handler that is invoked when an invalid argument is passed to a system
   call. https://msdn.microsoft.com/en-us/library/a9yf33zb(v=vs.110).aspx

   All good, except that the handler tries to invoke Watson and then kill spim with an exception.

   Override the handler to just report an error.
*/

#include <stdio.h>
#include <stdlib.h>
#include <crtdbg.h>

void myInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
  if (function != NULL)
    {
      run_error ("Bad parameter to system call: %s\n", function);
    }
  else
    {
      run_error ("Bad parameter to system call\n");
    }
}

static _invalid_parameter_handler oldHandler;

void windowsParameterHandlingControl(int flag )
{
  static _invalid_parameter_handler oldHandler;
  static _invalid_parameter_handler newHandler = myInvalidParameterHandler;

  if (flag == 0)
    {
      oldHandler = _set_invalid_parameter_handler(newHandler);
      _CrtSetReportMode(_CRT_ASSERT, 0); // Disable the message box for assertions.
    }
  else
    {
      newHandler = _set_invalid_parameter_handler(oldHandler);
      _CrtSetReportMode(_CRT_ASSERT, 1);  // Enable the message box for assertions.
    }
}
#endif


/*You implement your handler here */
void SPIM_timerHandler()
{
  // Implement your handler..
  try {
    if (NULL != current) {
      change_process(READY);
    }
  } catch ( exception &e ) {
    cerr <<  endl << "Caught: " << e.what( ) << endl;
  };
}
/* Decides which syscall to execute or simulate.  Returns zero upon
   exit syscall and non-zero to continue execution. */
int
do_syscall ()
{
#ifdef _WIN32
    windowsParameterHandlingControl(0);
#endif

  /* Syscalls for the source-language version of SPIM.  These are easier to
    use than the real syscall and are portable to non-MIPS operating
    systems. */
  switch (R[REG_V0])
    {
    case PRINT_INT_SYSCALL:
      write_output (console_out, "%d", R[REG_A0]);
      break;

    case PRINT_FLOAT_SYSCALL:
      {
	float val = FPR_S (REG_FA0);

	write_output (console_out, "%.8f", val);
	break;
      }

    case PRINT_DOUBLE_SYSCALL:
      write_output (console_out, "%.18g", FPR[REG_FA0 / 2]);
      break;

    case PRINT_STRING_SYSCALL:
      write_output (console_out, "%s", mem_reference (R[REG_A0]));
      break;

    case READ_INT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	R[REG_RES] = atol (str);
	break;
      }

    case READ_FLOAT_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR_S (REG_FRES) = (float) atof (str);
	break;
      }

    case READ_DOUBLE_SYSCALL:
      {
	static char str [256];

	read_input (str, 256);
	FPR [REG_FRES] = atof (str);
	break;
      }

    case READ_STRING_SYSCALL:
      {
	read_input ( (char *) mem_reference (R[REG_A0]), R[REG_A1]);
	data_modified = true;
	break;
      }

    case SBRK_SYSCALL:
      {
	mem_addr x = data_top;
	expand_data (R[REG_A0]);
	R[REG_RES] = x;
	data_modified = true;
	break;
      }

    case PRINT_CHARACTER_SYSCALL:
      write_output (console_out, "%c", R[REG_A0]);
      break;

    case READ_CHARACTER_SYSCALL:
      {
	static char str [2];

	read_input (str, 2);
	if (*str == '\0') *str = '\n';      /* makes xspim = spim */
	R[REG_RES] = (long) str[0];
	break;
      }

    case EXIT_SYSCALL:
      free_all();
      spim_return_value = 0;
      return (0);

    case EXIT2_SYSCALL:
      free_all();
      spim_return_value = R[REG_A0];	/* value passed to spim's exit() call */
      return (0);

    case OPEN_SYSCALL:
      {
#ifdef _WIN32
        R[REG_RES] = _open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#else
	R[REG_RES] = open((char*)mem_reference (R[REG_A0]), R[REG_A1], R[REG_A2]);
#endif
	break;
      }

    case READ_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = read(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	data_modified = true;
	break;
      }

    case WRITE_SYSCALL:
      {
	/* Test if address is valid */
	(void)mem_reference (R[REG_A1] + R[REG_A2] - 1);
#ifdef _WIN32
	R[REG_RES] = _write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#else
	R[REG_RES] = write(R[REG_A0], mem_reference (R[REG_A1]), R[REG_A2]);
#endif
	break;
      }

    case CLOSE_SYSCALL:
      {
#ifdef _WIN32
	R[REG_RES] = _close(R[REG_A0]);
#else
	R[REG_RES] = close(R[REG_A0]);
#endif
	break;
      }

    case FORK_SYSCALL:
      R[REG_RES] = fork_syscall();
      break;

    case WAITPID_SYSCALL:
      waitpid_syscall(R[REG_A0]);
      break;

    case EXECVE_SYSCALL:
      execve_syscall((char *)mem_reference(R[REG_A0]));
      break;

    case PROCESS_EXIT_SYSCALL:
      if (0 == processTable.size() || exit_syscall()) {
        free_all();
        spim_return_value = 0;
        return (0);
      }
      break;

    case RANDOM_INT_SYSCALL:
      if (!isRandomSeeded) {
        srand(time(NULL));
        isRandomSeeded = true;
      }
      R[REG_RES] = rand() % R[REG_A0];
      break;

    default:
      run_error ("Unknown system call: %d\n", R[REG_V0]);
      break;
    }

#ifdef _WIN32
    windowsParameterHandlingControl(1);
#endif
  return (1);
}

struct Process *get_new_process()
{
  struct Process *tmp;
  tmp = (struct Process *)xmalloc(sizeof(struct Process));
  return tmp;
}

void init_spimos_gtu()
{
  current = get_new_process();
  get_current_process(RUNNING);
  processTable.push_back(current);
  strcpy(current->name, "init");
  current->id = 0;
}

void get_current_process(enum State state)
{
  if (0 == processTable.size()) {
    current->id = processTable.size();
    current->waiting = 0;
    current->parent = 0;
    memset(current->name, '\0', NAME_SIZE);
  }
  current->state = state;
  memcpy(current->R, R, sizeof(R));
  current->HI = HI;
  current->LO = LO;
  current->PC = PC;
  current->nPC = nPC;
  current->FPR = FPR;
  current->FGR = FGR;
  current->FWR = FWR;
  memcpy(current->CCR, CCR, sizeof(CCR));
  memcpy(current->CPR, CPR, sizeof(CPR));
  current->text_seg = text_seg;
  current->text_modified = text_modified;
  current->text_top = text_top;
  current->data_seg = data_seg;
  current->data_modified = data_modified;
  current->data_seg_h = data_seg_h;
  current->data_seg_b = data_seg_b;
  current->data_top = data_top;
  current->gp_midpoint = gp_midpoint;
  current->stack_seg = stack_seg;
  current->stack_seg_h = stack_seg_h;
  current->stack_seg_b = stack_seg_b;
  current->stack_bot = stack_bot;
  current->k_text_seg = k_text_seg;
  current->k_text_top = k_text_top;
  current->k_data_seg = k_data_seg;
  current->k_data_seg_h = k_data_seg_h;
  current->k_data_seg_b = k_data_seg_b;
  current->k_data_top = k_data_top;
}

void set_current_process()
{
  memcpy(R, current->R, sizeof(R));
  HI = current->HI;
  LO = current->LO;
  PC = current->PC;
  nPC = current->nPC;
  FPR = current->FPR;
  FGR = current->FGR;
  FWR = current->FWR;
  memcpy(CCR, current->CCR, sizeof(CCR));
  memcpy(CPR, current->CPR, sizeof(CPR));
  text_seg = current->text_seg;
  text_modified = current->text_modified;
  text_top = current->text_top;
  data_seg = current->data_seg;
  data_modified = current->data_modified;
  data_seg_h = current->data_seg_h;
  data_seg_b = current->data_seg_b;
  data_top = current->data_top;
  gp_midpoint = current->gp_midpoint;
  stack_seg = current->stack_seg;
  stack_seg_h = current->stack_seg_h;
  stack_seg_b = current->stack_seg_b;
  stack_bot = current->stack_bot;
  k_text_seg = current->k_text_seg;
  k_text_top = current->k_text_top;
  k_data_seg = current->k_data_seg;
  k_data_seg_h = current->k_data_seg_h;
  k_data_seg_b = current->k_data_seg_b;
  k_data_top = current->k_data_top;
}

struct Process *make_copy_process(const struct Process *parent)
{
  struct Process *child = get_new_process();
  memcpy(child, parent, sizeof(struct Process));
  child->id = processTable.size();
  strcpy(child->name, parent->name);
  child->FPR = (double *)xmalloc(FPR_LENGTH * sizeof(double));
  memcpy(child->FPR, parent->FPR, FPR_LENGTH * sizeof(double));
  child->FGR = (float *)child->FPR;
  child->FWR = (int *)child->FPR;
  child->text_seg = (instruction **)xmalloc(BYTES_TO_INST(TEXT_SIZE));
  memcpy(child->text_seg, parent->text_seg, BYTES_TO_INST(TEXT_SIZE));
  child->data_seg = (mem_word *)xmalloc(DATA_SIZE);
  memcpy(child->data_seg, parent->data_seg, DATA_SIZE);
  child->data_seg_h = (short *)child->data_seg;
  child->data_seg_b = (BYTE_TYPE *)child->data_seg;
  child->stack_seg = (mem_word *)xmalloc(STACK_SIZE);
  memcpy(child->stack_seg, parent->stack_seg, STACK_SIZE);
  child->stack_seg_h = (short *)child->stack_seg;
  child->stack_seg_b = (BYTE_TYPE *)child->stack_seg;
  child->stack_bot = parent->stack_bot;
  child->k_text_seg = (instruction **)xmalloc(BYTES_TO_INST(K_TEXT_SIZE));
  memcpy(child->k_text_seg, parent->k_text_seg, BYTES_TO_INST(K_TEXT_SIZE));
  child->k_data_seg = (mem_word *)xmalloc(K_DATA_SIZE);
  memcpy(child->k_data_seg, parent->k_data_seg, K_DATA_SIZE);
  child->k_data_seg_h = (short *)child->k_data_seg_h;
  child->k_data_seg_b = (BYTE_TYPE *)child->k_data_seg_b;
  return child;
}

void free_all()
{
  for (int i = 0; i < (int)processTable.size(); ++i) {
    struct Process *pro = processTable[i];
    if (pro == current) {
      continue;
    }
    if (TERMINATED != pro->state) {
      free_process(pro);
    }
    free(pro);
  }
}

void free_process(struct Process *process)
{
  free(process->text_seg);
  free(process->data_seg);
  free(process->stack_seg);
  free(process->k_text_seg);
  free(process->k_data_seg);
  free(process->FPR);
  process->FPR = NULL;
  process->FGR = NULL;
  process->FWR = NULL;
  process->text_seg = NULL;
  process->data_seg = NULL;
  process->stack_seg = NULL;
  process->k_text_seg = NULL;
  process->k_data_seg = NULL;
}

int fork_syscall()
{
  if (0 == processTable.size()) {
    init_spimos_gtu();
  } else {
    get_current_process(RUNNING);
  }
  struct Process *child = make_copy_process(current);
  child->state = READY;
  child->parent = current->id;
  child->R[REG_RES] = 0;
  child->PC += BYTES_PER_WORD;
  processTable.push_back(child);
  return child->id;
}

void waitpid_syscall(int waitingProcessId)
{
  if (0 == processTable.size()) {
    init_spimos_gtu();
  } else {
    get_current_process(RUNNING);
  }
  if (-1 == waitingProcessId) {
    // OK
  } else if (0 > waitingProcessId || (int)processTable.size() <= waitingProcessId) {
    throw out_of_range("Waiting process ID is out of range!");
  } else if (TERMINATED == processTable[waitingProcessId]->state) {
    return;
  } else if (waitingProcessId == current->id) {
    return;
  }
  current->waiting = waitingProcessId;
  PC += BYTES_PER_WORD;
  
  change_process(WAITING);
  PC -= BYTES_PER_WORD;
}

void execve_syscall(char *path)
{
  if (0 == processTable.size()) {
    init_spimos_gtu();
  } else {
    get_current_process(RUNNING);
  }
  char *str = str_copy(path);
  int len = strlen(str);
  if (isspace(str[len-1])) {
    str[len-1] = '\0';
  }
  free_process(current);
  set_current_process();
  initialize_world(DEFAULT_EXCEPTION_HANDLER, false);
  initialize_run_stack(0, NULL);
  read_assembly_file(str);
  pop_scanner();
  PC = find_symbol_address("main") - BYTES_PER_WORD;
  int id = current->id;
  get_current_process(RUNNING);
  current->id = id;
  strcpy(current->name, str);
  free(str);
}

void print_table(){
  error("\n--- --- --- --- --- --- --- --- --- --- --- --- ---\n");
  for (int i = 0; i < (int)processTable.size(); ++i) {
    struct Process *pro = processTable[i];
    error("%2d ", pro->id);
    error("%16s ", pro->name);
    error("%x %x ", pro->PC, pro->R[REG_SP]);
    error("%2d ", pro->parent);
    switch (pro->state) {
      case READY:
        error("READY");
        break;
      case RUNNING:
        error("RUNNING");
        break;
      case WAITING:
        error("WAITING(%2d)", pro->waiting);
        break;
      case TERMINATED:
        error("TERMINATED");
        break;
    }
    error("\n");
  }
  error("--- --- --- --- --- --- --- --- --- --- --- --- ---\n");
}

void change_process(enum State state)
{
  int size = processTable.size();
  if (0 == size) {
    return;
  }
  int id = current->id;
  get_current_process(state);
  int i = (size-1 == id) ? 0 : id+1;
  int traversed = -1;
  while (++traversed < size) {
    if (READY == processTable[i]->state) {
      current = processTable[i];
      break;
    }
    if (size-1 == i) {
      i = 0;
    } else {
      ++i;
    }
  }
  current->state = RUNNING;
  set_current_process();
  print_table();
}

int count_active_children(int id)
{
  int count = 0;
  for (int i = 0; i < (int)processTable.size(); ++i) {
    struct Process *pro = processTable[i];
    if (TERMINATED != pro->state && id == pro->parent) {
      ++count;
    }
  }
  if (0 == id) {
    --count;
  }
  return count;
}

bool exit_syscall()
{
  bool canContinue = false;
  for (int i = 0; i < (int)processTable.size(); ++i) {
    struct Process *pro = processTable[i];
    if (WAITING == pro->state) {
      if (current->id == pro->waiting) {
        pro->state = READY;
        pro->waiting = 0;
      } else if (-1 == pro->waiting && current->parent == pro->id) {
        if (1 >= count_active_children(pro->id)) {
          pro->state = READY;
          pro->waiting = 0;
        }
      }
    }
    if (READY == pro->state) {
      canContinue = true;
    }
  }
  free_process(current);
  change_process(TERMINATED);
  PC -= BYTES_PER_WORD;
  return !canContinue;
}

void
handle_exception ()
{
  if (!quiet && CP0_ExCode != ExcCode_Int)
    error ("Exception occurred at PC=0x%08x\n", CP0_EPC);

  exception_occurred = 0;
  PC = EXCEPTION_ADDR;

  switch (CP0_ExCode)
    {
    case ExcCode_Int:
      break;

    case ExcCode_AdEL:
      if (!quiet)
	error ("  Unaligned address in inst/data fetch: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_AdES:
      if (!quiet)
	error ("  Unaligned address in store: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_IBE:
      if (!quiet)
	error ("  Bad address in text read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_DBE:
      if (!quiet)
	error ("  Bad address in data/stack read: 0x%08x\n", CP0_BadVAddr);
      break;

    case ExcCode_Sys:
      if (!quiet)
	error ("  Error in syscall\n");
      break;

    case ExcCode_Bp:
      exception_occurred = 0;
      return;

    case ExcCode_RI:
      if (!quiet)
	error ("  Reserved instruction execution\n");
      break;

    case ExcCode_CpU:
      if (!quiet)
	error ("  Coprocessor unuable\n");
      break;

    case ExcCode_Ov:
      if (!quiet)
	error ("  Arithmetic overflow\n");
      break;

    case ExcCode_Tr:
      if (!quiet)
	error ("  Trap\n");
      break;

    case ExcCode_FPE:
      if (!quiet)
	error ("  Floating point\n");
      break;

    default:
      if (!quiet)
	error ("Unknown exception: %d\n", CP0_ExCode);
      break;
    }
}
