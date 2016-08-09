/* Execution of byte code produced by bytecomp.el.
   Copyright (C) 1985-1988, 1993, 2000-2016 Free Software Foundation,
   Inc.

This file is part of GNU Emacs.

GNU Emacs is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

GNU Emacs is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU Emacs.  If not, see <http://www.gnu.org/licenses/>.  */

#include <config.h>

#include "lisp.h"
#include "blockinput.h"
#include "character.h"
#include "buffer.h"
#include "keyboard.h"
#include "syntax.h"
#include "window.h"

/* Work around GCC bug 54561.  */
#if GNUC_PREREQ (4, 3, 0)
# pragma GCC diagnostic ignored "-Wclobbered"
#endif

/* Define BYTE_CODE_SAFE true to enable some minor sanity checking,
   useful for debugging the byte compiler.  It defaults to false.  */

#ifndef BYTE_CODE_SAFE
# define BYTE_CODE_SAFE false
#endif

/* Define BYTE_CODE_METER to generate a byte-op usage histogram.  */
/* #define BYTE_CODE_METER */

/* If BYTE_CODE_THREADED is defined, then the interpreter will be
   indirect threaded, using GCC's computed goto extension.  This code,
   as currently implemented, is incompatible with BYTE_CODE_SAFE and
   BYTE_CODE_METER.  */
#if (defined __GNUC__ && !defined __STRICT_ANSI__ \
     && !BYTE_CODE_SAFE && !defined BYTE_CODE_METER)
#define BYTE_CODE_THREADED
#endif


#ifdef BYTE_CODE_METER

#define METER_2(code1, code2) \
  (*aref_addr (AREF (Vbyte_code_meter, code1), code2))
#define METER_1(code) METER_2 (0, code)

#define METER_CODE(last_code, this_code)				\
{									\
  if (byte_metering_on)							\
    {									\
      if (XFASTINT (METER_1 (this_code)) < MOST_POSITIVE_FIXNUM)	\
        XSETFASTINT (METER_1 (this_code),				\
		     XFASTINT (METER_1 (this_code)) + 1);		\
      if (last_code							\
	  && (XFASTINT (METER_2 (last_code, this_code))			\
	      < MOST_POSITIVE_FIXNUM))					\
        XSETFASTINT (METER_2 (last_code, this_code),			\
		     XFASTINT (METER_2 (last_code, this_code)) + 1);	\
    }									\
}

#endif /* BYTE_CODE_METER */


/*  Byte codes: */

#define BYTE_CODES							\
DEFINE (Bstack_ref, 0) /* Actually, Bstack_ref+0 is not implemented: use dup.  */ \
DEFINE (Bstack_ref1, 1)							\
DEFINE (Bstack_ref2, 2)							\
DEFINE (Bstack_ref3, 3)							\
DEFINE (Bstack_ref4, 4)							\
DEFINE (Bstack_ref5, 5)							\
DEFINE (Bstack_ref6, 6)							\
DEFINE (Bstack_ref7, 7)							\
DEFINE (Bvarref, 010)							\
DEFINE (Bvarref1, 011)							\
DEFINE (Bvarref2, 012)							\
DEFINE (Bvarref3, 013)							\
DEFINE (Bvarref4, 014)							\
DEFINE (Bvarref5, 015)							\
DEFINE (Bvarref6, 016)							\
DEFINE (Bvarref7, 017)							\
DEFINE (Bvarset, 020)							\
DEFINE (Bvarset1, 021)							\
DEFINE (Bvarset2, 022)							\
DEFINE (Bvarset3, 023)							\
DEFINE (Bvarset4, 024)							\
DEFINE (Bvarset5, 025)							\
DEFINE (Bvarset6, 026)							\
DEFINE (Bvarset7, 027)							\
DEFINE (Bvarbind, 030)							\
DEFINE (Bvarbind1, 031)							\
DEFINE (Bvarbind2, 032)							\
DEFINE (Bvarbind3, 033)							\
DEFINE (Bvarbind4, 034)							\
DEFINE (Bvarbind5, 035)							\
DEFINE (Bvarbind6, 036)							\
DEFINE (Bvarbind7, 037)							\
DEFINE (Bcall, 040)							\
DEFINE (Bcall1, 041)							\
DEFINE (Bcall2, 042)							\
DEFINE (Bcall3, 043)							\
DEFINE (Bcall4, 044)							\
DEFINE (Bcall5, 045)							\
DEFINE (Bcall6, 046)							\
DEFINE (Bcall7, 047)							\
DEFINE (Bunbind, 050)							\
DEFINE (Bunbind1, 051)							\
DEFINE (Bunbind2, 052)							\
DEFINE (Bunbind3, 053)							\
DEFINE (Bunbind4, 054)							\
DEFINE (Bunbind5, 055)							\
DEFINE (Bunbind6, 056)							\
DEFINE (Bunbind7, 057)							\
									\
DEFINE (Bpophandler, 060)						\
DEFINE (Bpushconditioncase, 061)					\
DEFINE (Bpushcatch, 062)						\
									\
DEFINE (Bnth, 070)							\
DEFINE (Bsymbolp, 071)							\
DEFINE (Bconsp, 072)							\
DEFINE (Bstringp, 073)							\
DEFINE (Blistp, 074)							\
DEFINE (Beq, 075)							\
DEFINE (Bmemq, 076)							\
DEFINE (Bnot, 077)							\
DEFINE (Bcar, 0100)							\
DEFINE (Bcdr, 0101)							\
DEFINE (Bcons, 0102)							\
DEFINE (Blist1, 0103)							\
DEFINE (Blist2, 0104)							\
DEFINE (Blist3, 0105)							\
DEFINE (Blist4, 0106)							\
DEFINE (Blength, 0107)							\
DEFINE (Baref, 0110)							\
DEFINE (Baset, 0111)							\
DEFINE (Bsymbol_value, 0112)						\
DEFINE (Bsymbol_function, 0113)						\
DEFINE (Bset, 0114)							\
DEFINE (Bfset, 0115)							\
DEFINE (Bget, 0116)							\
DEFINE (Bsubstring, 0117)						\
DEFINE (Bconcat2, 0120)							\
DEFINE (Bconcat3, 0121)							\
DEFINE (Bconcat4, 0122)							\
DEFINE (Bsub1, 0123)							\
DEFINE (Badd1, 0124)							\
DEFINE (Beqlsign, 0125)							\
DEFINE (Bgtr, 0126)							\
DEFINE (Blss, 0127)							\
DEFINE (Bleq, 0130)							\
DEFINE (Bgeq, 0131)							\
DEFINE (Bdiff, 0132)							\
DEFINE (Bnegate, 0133)							\
DEFINE (Bplus, 0134)							\
DEFINE (Bmax, 0135)							\
DEFINE (Bmin, 0136)							\
DEFINE (Bmult, 0137)							\
									\
DEFINE (Bpoint, 0140)							\
/* Was Bmark in v17.  */						\
DEFINE (Bsave_current_buffer, 0141) /* Obsolete.  */			\
DEFINE (Bgoto_char, 0142)						\
DEFINE (Binsert, 0143)							\
DEFINE (Bpoint_max, 0144)						\
DEFINE (Bpoint_min, 0145)						\
DEFINE (Bchar_after, 0146)						\
DEFINE (Bfollowing_char, 0147)						\
DEFINE (Bpreceding_char, 0150)						\
DEFINE (Bcurrent_column, 0151)						\
DEFINE (Bindent_to, 0152)						\
DEFINE (Beolp, 0154)							\
DEFINE (Beobp, 0155)							\
DEFINE (Bbolp, 0156)							\
DEFINE (Bbobp, 0157)							\
DEFINE (Bcurrent_buffer, 0160)						\
DEFINE (Bset_buffer, 0161)						\
DEFINE (Bsave_current_buffer_1, 0162) /* Replacing Bsave_current_buffer.  */ \
DEFINE (Binteractive_p, 0164) /* Obsolete since Emacs-24.1.  */		\
									\
DEFINE (Bforward_char, 0165)						\
DEFINE (Bforward_word, 0166)						\
DEFINE (Bskip_chars_forward, 0167)					\
DEFINE (Bskip_chars_backward, 0170)					\
DEFINE (Bforward_line, 0171)						\
DEFINE (Bchar_syntax, 0172)						\
DEFINE (Bbuffer_substring, 0173)					\
DEFINE (Bdelete_region, 0174)						\
DEFINE (Bnarrow_to_region, 0175)					\
DEFINE (Bwiden, 0176)							\
DEFINE (Bend_of_line, 0177)						\
									\
DEFINE (Bconstant2, 0201)						\
DEFINE (Bgoto, 0202)							\
DEFINE (Bgotoifnil, 0203)						\
DEFINE (Bgotoifnonnil, 0204)						\
DEFINE (Bgotoifnilelsepop, 0205)					\
DEFINE (Bgotoifnonnilelsepop, 0206)					\
DEFINE (Breturn, 0207)							\
DEFINE (Bdiscard, 0210)							\
DEFINE (Bdup, 0211)							\
									\
DEFINE (Bsave_excursion, 0212)						\
DEFINE (Bsave_window_excursion, 0213) /* Obsolete since Emacs-24.1.  */	\
DEFINE (Bsave_restriction, 0214)					\
DEFINE (Bcatch, 0215)							\
									\
DEFINE (Bunwind_protect, 0216)						\
DEFINE (Bcondition_case, 0217)						\
DEFINE (Btemp_output_buffer_setup, 0220) /* Obsolete since Emacs-24.1.  */ \
DEFINE (Btemp_output_buffer_show, 0221)  /* Obsolete since Emacs-24.1.  */ \
									\
DEFINE (Bunbind_all, 0222)	/* Obsolete.  Never used.  */		\
									\
DEFINE (Bset_marker, 0223)						\
DEFINE (Bmatch_beginning, 0224)						\
DEFINE (Bmatch_end, 0225)						\
DEFINE (Bupcase, 0226)							\
DEFINE (Bdowncase, 0227)						\
									\
DEFINE (Bstringeqlsign, 0230)						\
DEFINE (Bstringlss, 0231)						\
DEFINE (Bequal, 0232)							\
DEFINE (Bnthcdr, 0233)							\
DEFINE (Belt, 0234)							\
DEFINE (Bmember, 0235)							\
DEFINE (Bassq, 0236)							\
DEFINE (Bnreverse, 0237)						\
DEFINE (Bsetcar, 0240)							\
DEFINE (Bsetcdr, 0241)							\
DEFINE (Bcar_safe, 0242)						\
DEFINE (Bcdr_safe, 0243)						\
DEFINE (Bnconc, 0244)							\
DEFINE (Bquo, 0245)							\
DEFINE (Brem, 0246)							\
DEFINE (Bnumberp, 0247)							\
DEFINE (Bintegerp, 0250)						\
									\
DEFINE (BRgoto, 0252)							\
DEFINE (BRgotoifnil, 0253)						\
DEFINE (BRgotoifnonnil, 0254)						\
DEFINE (BRgotoifnilelsepop, 0255)					\
DEFINE (BRgotoifnonnilelsepop, 0256)					\
									\
DEFINE (BlistN, 0257)							\
DEFINE (BconcatN, 0260)							\
DEFINE (BinsertN, 0261)							\
									\
/* Bstack_ref is code 0.  */						\
DEFINE (Bstack_set,  0262)						\
DEFINE (Bstack_set2, 0263)						\
DEFINE (BdiscardN,   0266)						\
									\
DEFINE (Bconstant, 0300)

enum byte_code_op
{
#define DEFINE(name, value) name = value,
    BYTE_CODES
#undef DEFINE

#if BYTE_CODE_SAFE
    Bscan_buffer = 0153, /* No longer generated as of v18.  */
    Bset_mark = 0163, /* this loser is no longer generated as of v18 */
#endif
};

/* Structure describing a value stack used during byte-code execution
   in Fbyte_code.  */

struct byte_stack
{
  /* Program counter.  This points into the byte_string below
     and is relocated when that string is relocated.  */
  const unsigned char *pc;

  /* The string containing the byte-code, and its current address.
     Storing this here protects it from GC.  */
  Lisp_Object byte_string;
  const unsigned char *byte_string_start;

  /* Next entry in byte_stack_list.  */
  struct byte_stack *next;
};

/* A list of currently active byte-code execution value stacks.
   Fbyte_code adds an entry to the head of this list before it starts
   processing byte-code, and it removes the entry again when it is
   done.  Signaling an error truncates the list.  */

struct byte_stack *byte_stack_list;


/* Relocate program counters in the stacks on byte_stack_list.  Called
   when GC has completed.  */

void
relocate_byte_stack (void)
{
  struct byte_stack *stack;

  for (stack = byte_stack_list; stack; stack = stack->next)
    {
      if (stack->byte_string_start != SDATA (stack->byte_string))
	{
	  ptrdiff_t offset = stack->pc - stack->byte_string_start;
	  stack->byte_string_start = SDATA (stack->byte_string);
	  stack->pc = stack->byte_string_start + offset;
	}
    }
}


/* Fetch the next byte from the bytecode stream.  */

#if BYTE_CODE_SAFE
#define FETCH (eassert (stack.byte_string_start == SDATA (stack.byte_string)), *stack.pc++)
#else
#define FETCH *stack.pc++
#endif

/* Fetch two bytes from the bytecode stream and make a 16-bit number
   out of them.  */

#define FETCH2 (op = FETCH, op + (FETCH << 8))

/* Push X onto the execution stack.  The expression X should not
   contain TOP, to avoid competing side effects.  */

#define PUSH(x) (*++top = (x))

/* Pop a value off the execution stack.  */

#define POP (*top--)

/* Discard n values from the execution stack.  */

#define DISCARD(n) (top -= (n))

/* Get the value which is at the top of the execution stack, but don't
   pop it.  */

#define TOP (*top)

/* Check for jumping out of range.  */

#define CHECK_RANGE(ARG) \
  (BYTE_CODE_SAFE && bytestr_length <= (ARG) ? emacs_abort () : (void) 0)

/* A version of the QUIT macro which makes sure that the stack top is
   set before signaling `quit'.  */

#define BYTE_CODE_QUIT					\
  do {							\
    if (quitcounter++)					\
      break;						\
    maybe_gc ();					\
    if (!NILP (Vquit_flag) && NILP (Vinhibit_quit))	\
      {							\
        Lisp_Object flag = Vquit_flag;			\
	Vquit_flag = Qnil;				\
	if (EQ (Vthrow_on_input, flag))			\
	  Fthrow (Vthrow_on_input, Qt);			\
	quit ();					\
      }							\
    else if (pending_signals)				\
      process_pending_signals ();			\
  } while (0)


DEFUN ("byte-code", Fbyte_code, Sbyte_code, 3, 3, 0,
       doc: /* Function used internally in byte-compiled code.
The first argument, BYTESTR, is a string of byte code;
the second, VECTOR, a vector of constants;
the third, MAXDEPTH, the maximum stack depth used in this function.
If the third argument is incorrect, Emacs may crash.  */)
  (Lisp_Object bytestr, Lisp_Object vector, Lisp_Object maxdepth)
{
  return exec_byte_code (bytestr, vector, maxdepth, Qnil, 0, NULL);
}

static void
bcall0 (Lisp_Object f)
{
  Ffuncall (1, &f);
}

/* Execute the byte-code in BYTESTR.  VECTOR is the constant vector, and
   MAXDEPTH is the maximum stack depth used (if MAXDEPTH is incorrect,
   emacs may crash!).  If ARGS_TEMPLATE is non-nil, it should be a lisp
   argument list (including &rest, &optional, etc.), and ARGS, of size
   NARGS, should be a vector of the actual arguments.  The arguments in
   ARGS are pushed on the stack according to ARGS_TEMPLATE before
   executing BYTESTR.  */

Lisp_Object
exec_byte_code (Lisp_Object bytestr, Lisp_Object vector, Lisp_Object maxdepth,
		Lisp_Object args_template, ptrdiff_t nargs, Lisp_Object *args)
{
  USE_SAFE_ALLOCA;
#ifdef BYTE_CODE_METER
  int volatile this_op = 0;
  int prev_op;
#endif
  int op;
  /* Lisp_Object v1, v2; */
  Lisp_Object *vectorp;
  ptrdiff_t const_length;
  ptrdiff_t bytestr_length;
  struct byte_stack stack;
  Lisp_Object *top;
  Lisp_Object result;
  enum handlertype type;

  CHECK_STRING (bytestr);
  CHECK_VECTOR (vector);
  CHECK_NATNUM (maxdepth);

  const_length = ASIZE (vector);

  if (STRING_MULTIBYTE (bytestr))
    /* BYTESTR must have been produced by Emacs 20.2 or the earlier
       because they produced a raw 8-bit string for byte-code and now
       such a byte-code string is loaded as multibyte while raw 8-bit
       characters converted to multibyte form.  Thus, now we must
       convert them back to the originally intended unibyte form.  */
    bytestr = Fstring_as_unibyte (bytestr);

  bytestr_length = SBYTES (bytestr);
  vectorp = XVECTOR (vector)->contents;

  stack.byte_string = bytestr;
  stack.pc = stack.byte_string_start = SDATA (bytestr);
  unsigned char quitcounter = 0;
  EMACS_INT stack_items = XFASTINT (maxdepth) + 1;
  Lisp_Object *stack_base;
  SAFE_ALLOCA_LISP (stack_base, stack_items);
  Lisp_Object *stack_lim = stack_base + stack_items;
  top = stack_base;
  stack.next = byte_stack_list;
  byte_stack_list = &stack;
  ptrdiff_t count = SPECPDL_INDEX ();

  if (!NILP (args_template))
    {
      eassert (INTEGERP (args_template));
      ptrdiff_t at = XINT (args_template);
      bool rest = (at & 128) != 0;
      int mandatory = at & 127;
      ptrdiff_t nonrest = at >> 8;
      ptrdiff_t maxargs = rest ? PTRDIFF_MAX : nonrest;
      if (! (mandatory <= nargs && nargs <= maxargs))
	Fsignal (Qwrong_number_of_arguments,
		 list2 (Fcons (make_number (mandatory), make_number (nonrest)),
			make_number (nargs)));
      ptrdiff_t pushedargs = min (nonrest, nargs);
      for (ptrdiff_t i = 0; i < pushedargs; i++, args++)
	PUSH (*args);
      if (nonrest < nargs)
	PUSH (Flist (nargs - nonrest, args));
      else
	for (ptrdiff_t i = nargs - rest; i < nonrest; i++)
	  PUSH (Qnil);
    }

  while (1)
    {
      if (BYTE_CODE_SAFE && ! (stack_base <= top && top < stack_lim))
	emacs_abort ();

#ifdef BYTE_CODE_METER
      prev_op = this_op;
      this_op = op = FETCH;
      METER_CODE (prev_op, op);
#else
#ifndef BYTE_CODE_THREADED
      op = FETCH;
#endif
#endif

      /* The interpreter can be compiled one of two ways: as an
	 ordinary switch-based interpreter, or as a threaded
	 interpreter.  The threaded interpreter relies on GCC's
	 computed goto extension, so it is not available everywhere.
	 Threading provides a performance boost.  These macros are how
	 we allow the code to be compiled both ways.  */
#ifdef BYTE_CODE_THREADED
      /* The CASE macro introduces an instruction's body.  It is
	 either a label or a case label.  */
#define CASE(OP) insn_ ## OP
      /* NEXT is invoked at the end of an instruction to go to the
	 next instruction.  It is either a computed goto, or a
	 plain break.  */
#define NEXT goto *(targets[op = FETCH])
      /* FIRST is like NEXT, but is only used at the start of the
	 interpreter body.  In the switch-based interpreter it is the
	 switch, so the threaded definition must include a semicolon.  */
#define FIRST NEXT;
      /* Most cases are labeled with the CASE macro, above.
	 CASE_DEFAULT is one exception; it is used if the interpreter
	 being built requires a default case.  The threaded
	 interpreter does not, because the dispatch table is
	 completely filled.  */
#define CASE_DEFAULT
      /* This introduces an instruction that is known to call abort.  */
#define CASE_ABORT CASE (Bstack_ref): CASE (default)
#else
      /* See above for the meaning of the various defines.  */
#define CASE(OP) case OP
#define NEXT break
#define FIRST switch (op)
#define CASE_DEFAULT case 255: default:
#define CASE_ABORT case 0
#endif

#ifdef BYTE_CODE_THREADED

      /* A convenience define that saves us a lot of typing and makes
	 the table clearer.  */
#define LABEL(OP) [OP] = &&insn_ ## OP

#if GNUC_PREREQ (4, 6, 0)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Woverride-init"
#elif defined __clang__
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Winitializer-overrides"
#endif

      /* This is the dispatch table for the threaded interpreter.  */
      static const void *const targets[256] =
	{
	  [0 ... (Bconstant - 1)] = &&insn_default,
	  [Bconstant ... 255] = &&insn_Bconstant,

#define DEFINE(name, value) LABEL (name) ,
	  BYTE_CODES
#undef DEFINE
	};

#if GNUC_PREREQ (4, 6, 0) || defined __clang__
# pragma GCC diagnostic pop
#endif

#endif


      FIRST
	{
	CASE (Bvarref7):
	  op = FETCH2;
	  goto varref;

	CASE (Bvarref):
	CASE (Bvarref1):
	CASE (Bvarref2):
	CASE (Bvarref3):
	CASE (Bvarref4):
	CASE (Bvarref5):
	  op = op - Bvarref;
	  goto varref;

	/* This seems to be the most frequently executed byte-code
	   among the Bvarref's, so avoid a goto here.  */
	CASE (Bvarref6):
	  op = FETCH;
	varref:
	  {
	    Lisp_Object v1, v2;

	    v1 = vectorp[op];
	    if (SYMBOLP (v1))
	      {
		if (XSYMBOL (v1)->redirect != SYMBOL_PLAINVAL
		    || (v2 = SYMBOL_VAL (XSYMBOL (v1)),
			EQ (v2, Qunbound)))
		  {
		    v2 = Fsymbol_value (v1);
		  }
	      }
	    else
	      {
		v2 = Fsymbol_value (v1);
	      }
	    PUSH (v2);
	    NEXT;
	  }

	CASE (Bgotoifnil):
	  {
	    Lisp_Object v1;
	    op = FETCH2;
	    v1 = POP;
	    if (NILP (v1))
	      {
		BYTE_CODE_QUIT;
		CHECK_RANGE (op);
		stack.pc = stack.byte_string_start + op;
	      }
	    NEXT;
	  }

	CASE (Bcar):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    if (CONSP (v1))
	      TOP = XCAR (v1);
	    else if (NILP (v1))
	      TOP = Qnil;
	    else
	      {
		wrong_type_argument (Qlistp, v1);
	      }
	    NEXT;
	  }

	CASE (Beq):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = EQ (v1, TOP) ? Qt : Qnil;
	    NEXT;
	  }

	CASE (Bmemq):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fmemq (TOP, v1);
	    NEXT;
	  }

	CASE (Bcdr):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    if (CONSP (v1))
	      TOP = XCDR (v1);
	    else if (NILP (v1))
	      TOP = Qnil;
	    else
	      {
		wrong_type_argument (Qlistp, v1);
	      }
	    NEXT;
	  }

	CASE (Bvarset):
	CASE (Bvarset1):
	CASE (Bvarset2):
	CASE (Bvarset3):
	CASE (Bvarset4):
	CASE (Bvarset5):
	  op -= Bvarset;
	  goto varset;

	CASE (Bvarset7):
	  op = FETCH2;
	  goto varset;

	CASE (Bvarset6):
	  op = FETCH;
	varset:
	  {
	    Lisp_Object sym, val;

	    sym = vectorp[op];
	    val = TOP;

	    /* Inline the most common case.  */
	    if (SYMBOLP (sym)
		&& !EQ (val, Qunbound)
		&& !XSYMBOL (sym)->redirect
		&& !SYMBOL_CONSTANT_P (sym))
	      SET_SYMBOL_VAL (XSYMBOL (sym), val);
	    else
	      {
		set_internal (sym, val, Qnil, 0);
	      }
	  }
	  (void) POP;
	  NEXT;

	CASE (Bdup):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    PUSH (v1);
	    NEXT;
	  }

	/* ------------------ */

	CASE (Bvarbind6):
	  op = FETCH;
	  goto varbind;

	CASE (Bvarbind7):
	  op = FETCH2;
	  goto varbind;

	CASE (Bvarbind):
	CASE (Bvarbind1):
	CASE (Bvarbind2):
	CASE (Bvarbind3):
	CASE (Bvarbind4):
	CASE (Bvarbind5):
	  op -= Bvarbind;
	varbind:
	  /* Specbind can signal and thus GC.  */
	  specbind (vectorp[op], POP);
	  NEXT;

	CASE (Bcall6):
	  op = FETCH;
	  goto docall;

	CASE (Bcall7):
	  op = FETCH2;
	  goto docall;

	CASE (Bcall):
	CASE (Bcall1):
	CASE (Bcall2):
	CASE (Bcall3):
	CASE (Bcall4):
	CASE (Bcall5):
	  op -= Bcall;
	docall:
	  {
	    DISCARD (op);
#ifdef BYTE_CODE_METER
	    if (byte_metering_on && SYMBOLP (TOP))
	      {
		Lisp_Object v1, v2;

		v1 = TOP;
		v2 = Fget (v1, Qbyte_code_meter);
		if (INTEGERP (v2)
		    && XINT (v2) < MOST_POSITIVE_FIXNUM)
		  {
		    XSETINT (v2, XINT (v2) + 1);
		    Fput (v1, Qbyte_code_meter, v2);
		  }
	      }
#endif
	    TOP = Ffuncall (op + 1, &TOP);
	    NEXT;
	  }

	CASE (Bunbind6):
	  op = FETCH;
	  goto dounbind;

	CASE (Bunbind7):
	  op = FETCH2;
	  goto dounbind;

	CASE (Bunbind):
	CASE (Bunbind1):
	CASE (Bunbind2):
	CASE (Bunbind3):
	CASE (Bunbind4):
	CASE (Bunbind5):
	  op -= Bunbind;
	dounbind:
	  unbind_to (SPECPDL_INDEX () - op, Qnil);
	  NEXT;

	CASE (Bunbind_all):	/* Obsolete.  Never used.  */
	  /* To unbind back to the beginning of this frame.  Not used yet,
	     but will be needed for tail-recursion elimination.  */
	  unbind_to (count, Qnil);
	  NEXT;

	CASE (Bgoto):
	  BYTE_CODE_QUIT;
	  op = FETCH2;    /* pc = FETCH2 loses since FETCH2 contains pc++ */
	  CHECK_RANGE (op);
	  stack.pc = stack.byte_string_start + op;
	  NEXT;

	CASE (Bgotoifnonnil):
	  {
	    Lisp_Object v1;
	    op = FETCH2;
	    v1 = POP;
	    if (!NILP (v1))
	      {
		BYTE_CODE_QUIT;
		CHECK_RANGE (op);
		stack.pc = stack.byte_string_start + op;
	      }
	    NEXT;
	  }

	CASE (Bgotoifnilelsepop):
	  op = FETCH2;
	  if (NILP (TOP))
	    {
	      BYTE_CODE_QUIT;
	      CHECK_RANGE (op);
	      stack.pc = stack.byte_string_start + op;
	    }
	  else DISCARD (1);
	  NEXT;

	CASE (Bgotoifnonnilelsepop):
	  op = FETCH2;
	  if (!NILP (TOP))
	    {
	      BYTE_CODE_QUIT;
	      CHECK_RANGE (op);
	      stack.pc = stack.byte_string_start + op;
	    }
	  else DISCARD (1);
	  NEXT;

	CASE (BRgoto):
	  BYTE_CODE_QUIT;
	  stack.pc += (int) *stack.pc - 127;
	  NEXT;

	CASE (BRgotoifnil):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    if (NILP (v1))
	      {
		BYTE_CODE_QUIT;
		stack.pc += (int) *stack.pc - 128;
	      }
	    stack.pc++;
	    NEXT;
	  }

	CASE (BRgotoifnonnil):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    if (!NILP (v1))
	      {
		BYTE_CODE_QUIT;
		stack.pc += (int) *stack.pc - 128;
	      }
	    stack.pc++;
	    NEXT;
	  }

	CASE (BRgotoifnilelsepop):
	  op = *stack.pc++;
	  if (NILP (TOP))
	    {
	      BYTE_CODE_QUIT;
	      stack.pc += op - 128;
	    }
	  else DISCARD (1);
	  NEXT;

	CASE (BRgotoifnonnilelsepop):
	  op = *stack.pc++;
	  if (!NILP (TOP))
	    {
	      BYTE_CODE_QUIT;
	      stack.pc += op - 128;
	    }
	  else DISCARD (1);
	  NEXT;

	CASE (Breturn):
	  result = POP;
	  goto exit;

	CASE (Bdiscard):
	  DISCARD (1);
	  NEXT;

	CASE (Bconstant2):
	  PUSH (vectorp[FETCH2]);
	  NEXT;

	CASE (Bsave_excursion):
	  record_unwind_protect (save_excursion_restore,
				 save_excursion_save ());
	  NEXT;

	CASE (Bsave_current_buffer): /* Obsolete since ??.  */
	CASE (Bsave_current_buffer_1):
	  record_unwind_current_buffer ();
	  NEXT;

	CASE (Bsave_window_excursion): /* Obsolete since 24.1.  */
	  {
	    ptrdiff_t count1 = SPECPDL_INDEX ();
	    record_unwind_protect (restore_window_configuration,
				   Fcurrent_window_configuration (Qnil));
	    TOP = Fprogn (TOP);
	    unbind_to (count1, TOP);
	    NEXT;
	  }

	CASE (Bsave_restriction):
	  record_unwind_protect (save_restriction_restore,
				 save_restriction_save ());
	  NEXT;

	CASE (Bcatch):		/* Obsolete since 24.4.  */
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = internal_catch (TOP, eval_sub, v1);
	    NEXT;
	  }

	CASE (Bpushcatch):	/* New in 24.4.  */
	  type = CATCHER;
	  goto pushhandler;
	CASE (Bpushconditioncase): /* New in 24.4.  */
	  type = CONDITION_CASE;
	pushhandler:
	  {
	    Lisp_Object tag = POP;
	    int dest = FETCH2;

	    struct handler *c = push_handler (tag, type);
	    c->bytecode_dest = dest;
	    c->bytecode_top = top;

	    if (sys_setjmp (c->jmp))
	      {
		struct handler *c = handlerlist;
		int dest;
		top = c->bytecode_top;
		dest = c->bytecode_dest;
		handlerlist = c->next;
		PUSH (c->val);
		CHECK_RANGE (dest);
		/* Might have been re-set by longjmp!  */
		stack.byte_string_start = SDATA (stack.byte_string);
		stack.pc = stack.byte_string_start + dest;
	      }

	    NEXT;
	  }

	CASE (Bpophandler):	/* New in 24.4.  */
	  {
	    handlerlist = handlerlist->next;
	    NEXT;
	  }

	CASE (Bunwind_protect):	/* FIXME: avoid closure for lexbind.  */
	  {
	    Lisp_Object handler = POP;
	    /* Support for a function here is new in 24.4.  */
	    record_unwind_protect (NILP (Ffunctionp (handler))
				   ? unwind_body : bcall0,
				   handler);
	    NEXT;
	  }

	CASE (Bcondition_case):		/* Obsolete since 24.4.  */
	  {
	    Lisp_Object handlers, body;
	    handlers = POP;
	    body = POP;
	    TOP = internal_lisp_condition_case (TOP, body, handlers);
	    NEXT;
	  }

	CASE (Btemp_output_buffer_setup): /* Obsolete since 24.1.  */
	  CHECK_STRING (TOP);
	  temp_output_buffer_setup (SSDATA (TOP));
	  TOP = Vstandard_output;
	  NEXT;

	CASE (Btemp_output_buffer_show): /* Obsolete since 24.1.  */
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    temp_output_buffer_show (TOP);
	    TOP = v1;
	    /* pop binding of standard-output */
	    unbind_to (SPECPDL_INDEX () - 1, Qnil);
	    NEXT;
	  }

	CASE (Bnth):
	  {
	    Lisp_Object v1, v2;
	    EMACS_INT n;
	    v1 = POP;
	    v2 = TOP;
	    CHECK_NUMBER (v2);
	    n = XINT (v2);
	    immediate_quit = 1;
	    while (--n >= 0 && CONSP (v1))
	      v1 = XCDR (v1);
	    immediate_quit = 0;
	    TOP = CAR (v1);
	    NEXT;
	  }

	CASE (Bsymbolp):
	  TOP = SYMBOLP (TOP) ? Qt : Qnil;
	  NEXT;

	CASE (Bconsp):
	  TOP = CONSP (TOP) ? Qt : Qnil;
	  NEXT;

	CASE (Bstringp):
	  TOP = STRINGP (TOP) ? Qt : Qnil;
	  NEXT;

	CASE (Blistp):
	  TOP = CONSP (TOP) || NILP (TOP) ? Qt : Qnil;
	  NEXT;

	CASE (Bnot):
	  TOP = NILP (TOP) ? Qt : Qnil;
	  NEXT;

	CASE (Bcons):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fcons (TOP, v1);
	    NEXT;
	  }

	CASE (Blist1):
	  TOP = list1 (TOP);
	  NEXT;

	CASE (Blist2):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = list2 (TOP, v1);
	    NEXT;
	  }

	CASE (Blist3):
	  DISCARD (2);
	  TOP = Flist (3, &TOP);
	  NEXT;

	CASE (Blist4):
	  DISCARD (3);
	  TOP = Flist (4, &TOP);
	  NEXT;

	CASE (BlistN):
	  op = FETCH;
	  DISCARD (op - 1);
	  TOP = Flist (op, &TOP);
	  NEXT;

	CASE (Blength):
	  TOP = Flength (TOP);
	  NEXT;

	CASE (Baref):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Faref (TOP, v1);
	    NEXT;
	  }

	CASE (Baset):
	  {
	    Lisp_Object v1, v2;
	    v2 = POP; v1 = POP;
	    TOP = Faset (TOP, v1, v2);
	    NEXT;
	  }

	CASE (Bsymbol_value):
	  TOP = Fsymbol_value (TOP);
	  NEXT;

	CASE (Bsymbol_function):
	  TOP = Fsymbol_function (TOP);
	  NEXT;

	CASE (Bset):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fset (TOP, v1);
	    NEXT;
	  }

	CASE (Bfset):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Ffset (TOP, v1);
	    NEXT;
	  }

	CASE (Bget):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fget (TOP, v1);
	    NEXT;
	  }

	CASE (Bsubstring):
	  {
	    Lisp_Object v1, v2;
	    v2 = POP; v1 = POP;
	    TOP = Fsubstring (TOP, v1, v2);
	    NEXT;
	  }

	CASE (Bconcat2):
	  DISCARD (1);
	  TOP = Fconcat (2, &TOP);
	  NEXT;

	CASE (Bconcat3):
	  DISCARD (2);
	  TOP = Fconcat (3, &TOP);
	  NEXT;

	CASE (Bconcat4):
	  DISCARD (3);
	  TOP = Fconcat (4, &TOP);
	  NEXT;

	CASE (BconcatN):
	  op = FETCH;
	  DISCARD (op - 1);
	  TOP = Fconcat (op, &TOP);
	  NEXT;

	CASE (Bsub1):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    if (INTEGERP (v1))
	      {
		XSETINT (v1, XINT (v1) - 1);
		TOP = v1;
	      }
	    else
	      {
		TOP = Fsub1 (v1);
	      }
	    NEXT;
	  }

	CASE (Badd1):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    if (INTEGERP (v1))
	      {
		XSETINT (v1, XINT (v1) + 1);
		TOP = v1;
	      }
	    else
	      {
		TOP = Fadd1 (v1);
	      }
	    NEXT;
	  }

	CASE (Beqlsign):
	  {
	    Lisp_Object v1, v2;
	    v2 = POP; v1 = TOP;
	    CHECK_NUMBER_OR_FLOAT_COERCE_MARKER (v1);
	    CHECK_NUMBER_OR_FLOAT_COERCE_MARKER (v2);
	    if (FLOATP (v1) || FLOATP (v2))
	      {
		double f1, f2;

		f1 = (FLOATP (v1) ? XFLOAT_DATA (v1) : XINT (v1));
		f2 = (FLOATP (v2) ? XFLOAT_DATA (v2) : XINT (v2));
		TOP = (f1 == f2 ? Qt : Qnil);
	      }
	    else
	      TOP = (XINT (v1) == XINT (v2) ? Qt : Qnil);
	    NEXT;
	  }

	CASE (Bgtr):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = arithcompare (TOP, v1, ARITH_GRTR);
	    NEXT;
	  }

	CASE (Blss):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = arithcompare (TOP, v1, ARITH_LESS);
	    NEXT;
	  }

	CASE (Bleq):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = arithcompare (TOP, v1, ARITH_LESS_OR_EQUAL);
	    NEXT;
	  }

	CASE (Bgeq):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = arithcompare (TOP, v1, ARITH_GRTR_OR_EQUAL);
	    NEXT;
	  }

	CASE (Bdiff):
	  DISCARD (1);
	  TOP = Fminus (2, &TOP);
	  NEXT;

	CASE (Bnegate):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    if (INTEGERP (v1))
	      {
		XSETINT (v1, - XINT (v1));
		TOP = v1;
	      }
	    else
	      {
		TOP = Fminus (1, &TOP);
	      }
	    NEXT;
	  }

	CASE (Bplus):
	  DISCARD (1);
	  TOP = Fplus (2, &TOP);
	  NEXT;

	CASE (Bmax):
	  DISCARD (1);
	  TOP = Fmax (2, &TOP);
	  NEXT;

	CASE (Bmin):
	  DISCARD (1);
	  TOP = Fmin (2, &TOP);
	  NEXT;

	CASE (Bmult):
	  DISCARD (1);
	  TOP = Ftimes (2, &TOP);
	  NEXT;

	CASE (Bquo):
	  DISCARD (1);
	  TOP = Fquo (2, &TOP);
	  NEXT;

	CASE (Brem):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Frem (TOP, v1);
	    NEXT;
	  }

	CASE (Bpoint):
	  {
	    Lisp_Object v1;
	    XSETFASTINT (v1, PT);
	    PUSH (v1);
	    NEXT;
	  }

	CASE (Bgoto_char):
	  TOP = Fgoto_char (TOP);
	  NEXT;

	CASE (Binsert):
	  TOP = Finsert (1, &TOP);
	  NEXT;

	CASE (BinsertN):
	  op = FETCH;
	  DISCARD (op - 1);
	  TOP = Finsert (op, &TOP);
	  NEXT;

	CASE (Bpoint_max):
	  {
	    Lisp_Object v1;
	    XSETFASTINT (v1, ZV);
	    PUSH (v1);
	    NEXT;
	  }

	CASE (Bpoint_min):
	  {
	    Lisp_Object v1;
	    XSETFASTINT (v1, BEGV);
	    PUSH (v1);
	    NEXT;
	  }

	CASE (Bchar_after):
	  TOP = Fchar_after (TOP);
	  NEXT;

	CASE (Bfollowing_char):
	  {
	    Lisp_Object v1;
	    v1 = Ffollowing_char ();
	    PUSH (v1);
	    NEXT;
	  }

	CASE (Bpreceding_char):
	  {
	    Lisp_Object v1;
	    v1 = Fprevious_char ();
	    PUSH (v1);
	    NEXT;
	  }

	CASE (Bcurrent_column):
	  {
	    Lisp_Object v1;
	    XSETFASTINT (v1, current_column ());
	    PUSH (v1);
	    NEXT;
	  }

	CASE (Bindent_to):
	  TOP = Findent_to (TOP, Qnil);
	  NEXT;

	CASE (Beolp):
	  PUSH (Feolp ());
	  NEXT;

	CASE (Beobp):
	  PUSH (Feobp ());
	  NEXT;

	CASE (Bbolp):
	  PUSH (Fbolp ());
	  NEXT;

	CASE (Bbobp):
	  PUSH (Fbobp ());
	  NEXT;

	CASE (Bcurrent_buffer):
	  PUSH (Fcurrent_buffer ());
	  NEXT;

	CASE (Bset_buffer):
	  TOP = Fset_buffer (TOP);
	  NEXT;

	CASE (Binteractive_p):	/* Obsolete since 24.1.  */
	  PUSH (call0 (intern ("interactive-p")));
	  NEXT;

	CASE (Bforward_char):
	  TOP = Fforward_char (TOP);
	  NEXT;

	CASE (Bforward_word):
	  TOP = Fforward_word (TOP);
	  NEXT;

	CASE (Bskip_chars_forward):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fskip_chars_forward (TOP, v1);
	    NEXT;
	  }

	CASE (Bskip_chars_backward):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fskip_chars_backward (TOP, v1);
	    NEXT;
	  }

	CASE (Bforward_line):
	  TOP = Fforward_line (TOP);
	  NEXT;

	CASE (Bchar_syntax):
	  {
	    int c;

	    CHECK_CHARACTER (TOP);
	    c = XFASTINT (TOP);
	    if (NILP (BVAR (current_buffer, enable_multibyte_characters)))
	      MAKE_CHAR_MULTIBYTE (c);
	    XSETFASTINT (TOP, syntax_code_spec[SYNTAX (c)]);
	  }
	  NEXT;

	CASE (Bbuffer_substring):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fbuffer_substring (TOP, v1);
	    NEXT;
	  }

	CASE (Bdelete_region):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fdelete_region (TOP, v1);
	    NEXT;
	  }

	CASE (Bnarrow_to_region):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fnarrow_to_region (TOP, v1);
	    NEXT;
	  }

	CASE (Bwiden):
	  PUSH (Fwiden ());
	  NEXT;

	CASE (Bend_of_line):
	  TOP = Fend_of_line (TOP);
	  NEXT;

	CASE (Bset_marker):
	  {
	    Lisp_Object v1, v2;
	    v1 = POP;
	    v2 = POP;
	    TOP = Fset_marker (TOP, v2, v1);
	    NEXT;
	  }

	CASE (Bmatch_beginning):
	  TOP = Fmatch_beginning (TOP);
	  NEXT;

	CASE (Bmatch_end):
	  TOP = Fmatch_end (TOP);
	  NEXT;

	CASE (Bupcase):
	  TOP = Fupcase (TOP);
	  NEXT;

	CASE (Bdowncase):
	  TOP = Fdowncase (TOP);
	NEXT;

      CASE (Bstringeqlsign):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fstring_equal (TOP, v1);
	    NEXT;
	  }

	CASE (Bstringlss):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fstring_lessp (TOP, v1);
	    NEXT;
	  }

	CASE (Bequal):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fequal (TOP, v1);
	    NEXT;
	  }

	CASE (Bnthcdr):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fnthcdr (TOP, v1);
	    NEXT;
	  }

	CASE (Belt):
	  {
	    Lisp_Object v1, v2;
	    if (CONSP (TOP))
	      {
		/* Exchange args and then do nth.  */
		EMACS_INT n;
		v2 = POP;
		v1 = TOP;
		CHECK_NUMBER (v2);
		n = XINT (v2);
		immediate_quit = 1;
		while (--n >= 0 && CONSP (v1))
		  v1 = XCDR (v1);
		immediate_quit = 0;
		TOP = CAR (v1);
	      }
	    else
	      {
		v1 = POP;
		TOP = Felt (TOP, v1);
	      }
	    NEXT;
	  }

	CASE (Bmember):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fmember (TOP, v1);
	    NEXT;
	  }

	CASE (Bassq):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fassq (TOP, v1);
	    NEXT;
	  }

	CASE (Bnreverse):
	  TOP = Fnreverse (TOP);
	  NEXT;

	CASE (Bsetcar):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fsetcar (TOP, v1);
	    NEXT;
	  }

	CASE (Bsetcdr):
	  {
	    Lisp_Object v1;
	    v1 = POP;
	    TOP = Fsetcdr (TOP, v1);
	    NEXT;
	  }

	CASE (Bcar_safe):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    TOP = CAR_SAFE (v1);
	    NEXT;
	  }

	CASE (Bcdr_safe):
	  {
	    Lisp_Object v1;
	    v1 = TOP;
	    TOP = CDR_SAFE (v1);
	    NEXT;
	  }

	CASE (Bnconc):
	  DISCARD (1);
	  TOP = Fnconc (2, &TOP);
	  NEXT;

	CASE (Bnumberp):
	  TOP = (NUMBERP (TOP) ? Qt : Qnil);
	  NEXT;

	CASE (Bintegerp):
	  TOP = INTEGERP (TOP) ? Qt : Qnil;
	  NEXT;

#if BYTE_CODE_SAFE
	  /* These are intentionally written using 'case' syntax,
	     because they are incompatible with the threaded
	     interpreter.  */

	case Bset_mark:
	  error ("set-mark is an obsolete bytecode");
	  break;
	case Bscan_buffer:
	  error ("scan-buffer is an obsolete bytecode");
	  break;
#endif

	CASE_ABORT:
	  /* Actually this is Bstack_ref with offset 0, but we use Bdup
	     for that instead.  */
	  /* CASE (Bstack_ref): */
	  call3 (Qerror,
		 build_string ("Invalid byte opcode: op=%s, ptr=%d"),
		 make_number (op),
		 make_number ((stack.pc - 1) - stack.byte_string_start));

	  /* Handy byte-codes for lexical binding.  */
	CASE (Bstack_ref1):
	CASE (Bstack_ref2):
	CASE (Bstack_ref3):
	CASE (Bstack_ref4):
	CASE (Bstack_ref5):
	  {
	    Lisp_Object *ptr = top - (op - Bstack_ref);
	    PUSH (*ptr);
	    NEXT;
	  }
	CASE (Bstack_ref6):
	  {
	    Lisp_Object *ptr = top - (FETCH);
	    PUSH (*ptr);
	    NEXT;
	  }
	CASE (Bstack_ref7):
	  {
	    Lisp_Object *ptr = top - (FETCH2);
	    PUSH (*ptr);
	    NEXT;
	  }
	CASE (Bstack_set):
	  /* stack-set-0 = discard; stack-set-1 = discard-1-preserve-tos.  */
	  {
	    Lisp_Object *ptr = top - (FETCH);
	    *ptr = POP;
	    NEXT;
	  }
	CASE (Bstack_set2):
	  {
	    Lisp_Object *ptr = top - (FETCH2);
	    *ptr = POP;
	    NEXT;
	  }
	CASE (BdiscardN):
	  op = FETCH;
	  if (op & 0x80)
	    {
	      op &= 0x7F;
	      top[-op] = TOP;
	    }
	  DISCARD (op);
	  NEXT;

	CASE_DEFAULT
	CASE (Bconstant):
	  if (BYTE_CODE_SAFE
	      && ! (Bconstant <= op && op < Bconstant + const_length))
	    emacs_abort ();
	  PUSH (vectorp[op - Bconstant]);
	  NEXT;
	}
    }

 exit:

  byte_stack_list = byte_stack_list->next;

  /* Binds and unbinds are supposed to be compiled balanced.  */
  if (SPECPDL_INDEX () != count)
    {
      if (SPECPDL_INDEX () > count)
	unbind_to (count, Qnil);
      error ("binding stack not balanced (serious byte compiler bug)");
    }

  SAFE_FREE ();
  return result;
}

/* `args_template' has the same meaning as in exec_byte_code() above.  */
Lisp_Object
get_byte_code_arity (Lisp_Object args_template)
{
  eassert (NATNUMP (args_template));
  EMACS_INT at = XINT (args_template);
  bool rest = (at & 128) != 0;
  int mandatory = at & 127;
  EMACS_INT nonrest = at >> 8;

  return Fcons (make_number (mandatory),
		rest ? Qmany : make_number (nonrest));
}

void
syms_of_bytecode (void)
{
  defsubr (&Sbyte_code);

#ifdef BYTE_CODE_METER

  DEFVAR_LISP ("byte-code-meter", Vbyte_code_meter,
	       doc: /* A vector of vectors which holds a histogram of byte-code usage.
\(aref (aref byte-code-meter 0) CODE) indicates how many times the byte
opcode CODE has been executed.
\(aref (aref byte-code-meter CODE1) CODE2), where CODE1 is not 0,
indicates how many times the byte opcodes CODE1 and CODE2 have been
executed in succession.  */);

  DEFVAR_BOOL ("byte-metering-on", byte_metering_on,
	       doc: /* If non-nil, keep profiling information on byte code usage.
The variable byte-code-meter indicates how often each byte opcode is used.
If a symbol has a property named `byte-code-meter' whose value is an
integer, it is incremented each time that symbol's function is called.  */);

  byte_metering_on = 0;
  Vbyte_code_meter = Fmake_vector (make_number (256), make_number (0));
  DEFSYM (Qbyte_code_meter, "byte-code-meter");
  {
    int i = 256;
    while (i--)
      ASET (Vbyte_code_meter, i,
           Fmake_vector (make_number (256), make_number (0)));
  }
#endif
}
