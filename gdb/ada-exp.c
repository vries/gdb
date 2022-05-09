/* A Bison parser, made by GNU Bison 3.5.1.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2020 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Undocumented macros, especially those whose name start with YY_,
   are private implementation details.  Do not rely on them.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "3.5.1"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1




/* First part of user prologue.  */
#line 36 "ada-exp.y"


#include "defs.h"
#include <ctype.h>
#include "expression.h"
#include "value.h"
#include "parser-defs.h"
#include "language.h"
#include "ada-lang.h"
#include "bfd.h" /* Required by objfiles.h.  */
#include "symfile.h" /* Required by objfiles.h.  */
#include "objfiles.h" /* For have_full_symbols and have_partial_symbols */
#include "frame.h"
#include "block.h"
#include "ada-exp.h"

#define parse_type(ps) builtin_type (ps->gdbarch ())

/* Remap normal yacc parser interface names (yyparse, yylex, yyerror,
   etc).  */
#define GDB_YY_REMAP_PREFIX ada_
#include "yy-remap.h"

struct name_info {
  struct symbol *sym;
  struct minimal_symbol *msym;
  const struct block *block;
  struct stoken stoken;
};

/* The state of the parser, used internally when we are parsing the
   expression.  */

static struct parser_state *pstate = NULL;

/* If expression is in the context of TYPE'(...), then TYPE, else
 * NULL.  */
static struct type *type_qualifier;

int yyparse (void);

static int yylex (void);

static void yyerror (const char *);

static void write_int (struct parser_state *, LONGEST, struct type *);

static void write_object_renaming (struct parser_state *,
				   const struct block *, const char *, int,
				   const char *, int);

static struct type* write_var_or_type (struct parser_state *,
				       const struct block *, struct stoken);

static void write_name_assoc (struct parser_state *, struct stoken);

static const struct block *block_lookup (const struct block *, const char *);

static LONGEST convert_char_literal (struct type *, LONGEST);

static void write_ambiguous_var (struct parser_state *,
				 const struct block *, const char *, int);

static struct type *type_int (struct parser_state *);

static struct type *type_long (struct parser_state *);

static struct type *type_long_long (struct parser_state *);

static struct type *type_long_double (struct parser_state *);

static struct type *type_char (struct parser_state *);

static struct type *type_boolean (struct parser_state *);

static struct type *type_system_address (struct parser_state *);

using namespace expr;

/* Handle Ada type resolution for OP.  DEPROCEDURE_P and CONTEXT_TYPE
   are passed to the resolve method, if called.  */
static operation_up
resolve (operation_up &&op, bool deprocedure_p, struct type *context_type)
{
  operation_up result = std::move (op);
  ada_resolvable *res = dynamic_cast<ada_resolvable *> (result.get ());
  if (res != nullptr
      && res->resolve (pstate->expout.get (),
		       deprocedure_p,
		       pstate->parse_completion,
		       pstate->block_tracker,
		       context_type))
    result
      = make_operation<ada_funcall_operation> (std::move (result),
					       std::vector<operation_up> ());

  return result;
}

/* Like parser_state::pop, but handles Ada type resolution.
   DEPROCEDURE_P and CONTEXT_TYPE are passed to the resolve method, if
   called.  */
static operation_up
ada_pop (bool deprocedure_p = true, struct type *context_type = nullptr)
{
  /* Of course it's ok to call parser_state::pop here... */
  return resolve (pstate->pop (), deprocedure_p, context_type);
}

/* Like parser_state::wrap, but use ada_pop to pop the value.  */
template<typename T>
void
ada_wrap ()
{
  operation_up arg = ada_pop ();
  pstate->push_new<T> (std::move (arg));
}

/* Create and push an address-of operation, as appropriate for Ada.
   If TYPE is not NULL, the resulting operation will be wrapped in a
   cast to TYPE.  */
static void
ada_addrof (struct type *type = nullptr)
{
  operation_up arg = ada_pop (false);
  operation_up addr = make_operation<unop_addr_operation> (std::move (arg));
  operation_up wrapped
    = make_operation<ada_wrapped_operation> (std::move (addr));
  if (type != nullptr)
    wrapped = make_operation<unop_cast_operation> (std::move (wrapped), type);
  pstate->push (std::move (wrapped));
}

/* Handle operator overloading.  Either returns a function all
   operation wrapping the arguments, or it returns null, leaving the
   caller to construct the appropriate operation.  If RHS is null, a
   unary operator is assumed.  */
static operation_up
maybe_overload (enum exp_opcode op, operation_up &lhs, operation_up &rhs)
{
  struct value *args[2];

  int nargs = 1;
  args[0] = lhs->evaluate (nullptr, pstate->expout.get (),
			   EVAL_AVOID_SIDE_EFFECTS);
  if (rhs == nullptr)
    args[1] = nullptr;
  else
    {
      args[1] = rhs->evaluate (nullptr, pstate->expout.get (),
			       EVAL_AVOID_SIDE_EFFECTS);
      ++nargs;
    }

  block_symbol fn = ada_find_operator_symbol (op, pstate->parse_completion,
					      nargs, args);
  if (fn.symbol == nullptr)
    return {};

  if (symbol_read_needs_frame (fn.symbol))
    pstate->block_tracker->update (fn.block, INNERMOST_BLOCK_FOR_SYMBOLS);
  operation_up callee = make_operation<ada_var_value_operation> (fn);

  std::vector<operation_up> argvec;
  argvec.push_back (std::move (lhs));
  if (rhs != nullptr)
    argvec.push_back (std::move (rhs));
  return make_operation<ada_funcall_operation> (std::move (callee),
						std::move (argvec));
}

/* Like parser_state::wrap, but use ada_pop to pop the value, and
   handle unary overloading.  */
template<typename T>
void
ada_wrap_overload (enum exp_opcode op)
{
  operation_up arg = ada_pop ();
  operation_up empty;

  operation_up call = maybe_overload (op, arg, empty);
  if (call == nullptr)
    call = make_operation<T> (std::move (arg));
  pstate->push (std::move (call));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands, and then pushes a new Ada-wrapped operation of the
   template type T.  */
template<typename T>
void
ada_un_wrap2 (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();

  operation_up wrapped = maybe_overload (op, lhs, rhs);
  if (wrapped == nullptr)
    {
      wrapped = make_operation<T> (std::move (lhs), std::move (rhs));
      wrapped = make_operation<ada_wrapped_operation> (std::move (wrapped));
    }
  pstate->push (std::move (wrapped));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands.  Unlike ada_un_wrap2, ada_wrapped_operation is not
   used.  */
template<typename T>
void
ada_wrap2 (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();
  operation_up call = maybe_overload (op, lhs, rhs);
  if (call == nullptr)
    call = make_operation<T> (std::move (lhs), std::move (rhs));
  pstate->push (std::move (call));
}

/* A variant of parser_state::wrap2 that uses ada_pop to pop both
   operands.  OP is also passed to the constructor of the new binary
   operation.  */
template<typename T>
void
ada_wrap_op (enum exp_opcode op)
{
  operation_up rhs = ada_pop ();
  operation_up lhs = ada_pop ();
  operation_up call = maybe_overload (op, lhs, rhs);
  if (call == nullptr)
    call = make_operation<T> (op, std::move (lhs), std::move (rhs));
  pstate->push (std::move (call));
}

/* Pop three operands using ada_pop, then construct a new ternary
   operation of type T and push it.  */
template<typename T>
void
ada_wrap3 ()
{
  operation_up rhs = ada_pop ();
  operation_up mid = ada_pop ();
  operation_up lhs = ada_pop ();
  pstate->push_new<T> (std::move (lhs), std::move (mid), std::move (rhs));
}

/* Pop NARGS operands, then a callee operand, and use these to
   construct and push a new Ada function call operation.  */
static void
ada_funcall (int nargs)
{
  /* We use the ordinary pop here, because we're going to do
     resolution in a separate step, in order to handle array
     indices.  */
  std::vector<operation_up> args = pstate->pop_vector (nargs);
  /* Call parser_state::pop here, because we don't want to
     function-convert the callee slot of a call we're already
     constructing.  */
  operation_up callee = pstate->pop ();

  ada_var_value_operation *vvo
    = dynamic_cast<ada_var_value_operation *> (callee.get ());
  int array_arity = 0;
  struct type *callee_t = nullptr;
  if (vvo == nullptr
      || SYMBOL_DOMAIN (vvo->get_symbol ()) != UNDEF_DOMAIN)
    {
      struct value *callee_v = callee->evaluate (nullptr,
						 pstate->expout.get (),
						 EVAL_AVOID_SIDE_EFFECTS);
      callee_t = ada_check_typedef (value_type (callee_v));
      array_arity = ada_array_arity (callee_t);
    }

  for (int i = 0; i < nargs; ++i)
    {
      struct type *subtype = nullptr;
      if (i < array_arity)
	subtype = ada_index_type (callee_t, i + 1, "array type");
      args[i] = resolve (std::move (args[i]), true, subtype);
    }

  std::unique_ptr<ada_funcall_operation> funcall
    (new ada_funcall_operation (std::move (callee), std::move (args)));
  funcall->resolve (pstate->expout.get (), true, pstate->parse_completion,
		    pstate->block_tracker, nullptr);
  pstate->push (std::move (funcall));
}

/* The components being constructed during this parse.  */
static std::vector<ada_component_up> components;

/* Create a new ada_component_up of the indicated type and arguments,
   and push it on the global 'components' vector.  */
template<typename T, typename... Arg>
void
push_component (Arg... args)
{
  components.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Examine the final element of the 'components' vector, and return it
   as a pointer to an ada_choices_component.  The caller is
   responsible for ensuring that the final element is in fact an
   ada_choices_component.  */
static ada_choices_component *
choice_component ()
{
  ada_component *last = components.back ().get ();
  ada_choices_component *result = dynamic_cast<ada_choices_component *> (last);
  gdb_assert (result != nullptr);
  return result;
}

/* Pop the most recent component from the global stack, and return
   it.  */
static ada_component_up
pop_component ()
{
  ada_component_up result = std::move (components.back ());
  components.pop_back ();
  return result;
}

/* Pop the N most recent components from the global stack, and return
   them in a vector.  */
static std::vector<ada_component_up>
pop_components (int n)
{
  std::vector<ada_component_up> result (n);
  for (int i = 1; i <= n; ++i)
    result[n - i] = pop_component ();
  return result;
}

/* The associations being constructed during this parse.  */
static std::vector<ada_association_up> associations;

/* Create a new ada_association_up of the indicated type and
   arguments, and push it on the global 'associations' vector.  */
template<typename T, typename... Arg>
void
push_association (Arg... args)
{
  associations.emplace_back (new T (std::forward<Arg> (args)...));
}

/* Pop the most recent association from the global stack, and return
   it.  */
static ada_association_up
pop_association ()
{
  ada_association_up result = std::move (associations.back ());
  associations.pop_back ();
  return result;
}

/* Pop the N most recent associations from the global stack, and
   return them in a vector.  */
static std::vector<ada_association_up>
pop_associations (int n)
{
  std::vector<ada_association_up> result (n);
  for (int i = 1; i <= n; ++i)
    result[n - i] = pop_association ();
  return result;
}


#line 441 "ada-exp.c.tmp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTRPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTRPTR nullptr
#   else
#    define YY_NULLPTRPTR 0
#   endif
#  else
#   define YY_NULLPTRPTR ((void*)0)
#  endif
# endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    INT = 258,
    NULL_PTR = 259,
    CHARLIT = 260,
    FLOAT = 261,
    TRUEKEYWORD = 262,
    FALSEKEYWORD = 263,
    COLONCOLON = 264,
    STRING = 265,
    NAME = 266,
    DOT_ID = 267,
    DOT_ALL = 268,
    DOLLAR_VARIABLE = 269,
    ASSIGN = 270,
    _AND_ = 271,
    OR = 272,
    XOR = 273,
    THEN = 274,
    ELSE = 275,
    NOTEQUAL = 276,
    LEQ = 277,
    GEQ = 278,
    IN = 279,
    DOTDOT = 280,
    UNARY = 281,
    MOD = 282,
    REM = 283,
    STARSTAR = 284,
    ABS = 285,
    NOT = 286,
    VAR = 287,
    ARROW = 288,
    TICK_ACCESS = 289,
    TICK_ADDRESS = 290,
    TICK_FIRST = 291,
    TICK_LAST = 292,
    TICK_LENGTH = 293,
    TICK_MAX = 294,
    TICK_MIN = 295,
    TICK_MODULUS = 296,
    TICK_POS = 297,
    TICK_RANGE = 298,
    TICK_SIZE = 299,
    TICK_TAG = 300,
    TICK_VAL = 301,
    NEW = 302,
    OTHERS = 303
  };
#endif
/* Tokens.  */
#define INT 258
#define NULL_PTR 259
#define CHARLIT 260
#define FLOAT 261
#define TRUEKEYWORD 262
#define FALSEKEYWORD 263
#define COLONCOLON 264
#define STRING 265
#define NAME 266
#define DOT_ID 267
#define DOT_ALL 268
#define DOLLAR_VARIABLE 269
#define ASSIGN 270
#define _AND_ 271
#define OR 272
#define XOR 273
#define THEN 274
#define ELSE 275
#define NOTEQUAL 276
#define LEQ 277
#define GEQ 278
#define IN 279
#define DOTDOT 280
#define UNARY 281
#define MOD 282
#define REM 283
#define STARSTAR 284
#define ABS 285
#define NOT 286
#define VAR 287
#define ARROW 288
#define TICK_ACCESS 289
#define TICK_ADDRESS 290
#define TICK_FIRST 291
#define TICK_LAST 292
#define TICK_LENGTH 293
#define TICK_MAX 294
#define TICK_MIN 295
#define TICK_MODULUS 296
#define TICK_POS 297
#define TICK_RANGE 298
#define TICK_SIZE 299
#define TICK_TAG 300
#define TICK_VAL 301
#define NEW 302
#define OTHERS 303

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 408 "ada-exp.y"

    LONGEST lval;
    struct {
      LONGEST val;
      struct type *type;
    } typed_val;
    struct {
      gdb_byte val[16];
      struct type *type;
    } typed_val_float;
    struct type *tval;
    struct stoken sval;
    const struct block *bval;
    struct internalvar *ivar;
  

#line 603 "ada-exp.c.tmp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);





#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))

/* Stored state numbers (used for stacks). */
typedef yytype_uint8 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif

#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(E) ((void) (E))
#else
# define YYUSE(E) /* empty */
#endif

#if defined __GNUC__ && ! defined __ICC && 407 <= __GNUC__ * 100 + __GNUC_MINOR__
/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                            \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or xmalloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined xmalloc) \
             && (defined YYFREE || defined xfree)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC xmalloc
#   if ! defined xmalloc && ! defined EXIT_SUCCESS
void *xmalloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE xfree
#   if ! defined xfree && ! defined EXIT_SUCCESS
void xfree (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  57
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   751

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  69
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  27
/* YYNRULES -- Number of rules.  */
#define YYNRULES  118
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  229

#define YYUNDEFTOK  2
#define YYMAXUTOK   303


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_int8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    32,    64,
      58,    63,    34,    30,    65,    31,    57,    35,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    62,
      23,    21,    24,     2,    29,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    59,     2,    68,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    66,    43,    67,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    22,    25,    26,    27,
      28,    33,    36,    37,    38,    39,    40,    41,    42,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    60,    61
};

#if YYDEBUG
  /* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   473,   473,   477,   478,   480,   495,   499,   507,   509,
     524,   524,   537,   541,   543,   551,   562,   568,   572,   579,
     582,   586,   602,   609,   613,   616,   618,   620,   622,   626,
     639,   643,   647,   651,   655,   659,   663,   667,   671,   675,
     678,   682,   686,   690,   692,   699,   707,   710,   718,   729,
     733,   737,   741,   742,   743,   744,   745,   746,   750,   753,
     759,   762,   768,   771,   777,   779,   783,   786,   799,   801,
     803,   809,   815,   821,   823,   825,   827,   829,   831,   837,
     847,   849,   854,   863,   866,   870,   874,   882,   893,   901,
     908,   910,   914,   918,   920,   922,   930,   941,   943,   948,
     959,   960,   966,   971,   977,   986,   987,   988,   992,   999,
    1012,  1018,  1024,  1033,  1038,  1043,  1057,  1059,  1061
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || 0
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "INT", "NULL_PTR", "CHARLIT", "FLOAT",
  "TRUEKEYWORD", "FALSEKEYWORD", "COLONCOLON", "STRING", "NAME", "DOT_ID",
  "DOT_ALL", "DOLLAR_VARIABLE", "ASSIGN", "_AND_", "OR", "XOR", "THEN",
  "ELSE", "'='", "NOTEQUAL", "'<'", "'>'", "LEQ", "GEQ", "IN", "DOTDOT",
  "'@'", "'+'", "'-'", "'&'", "UNARY", "'*'", "'/'", "MOD", "REM",
  "STARSTAR", "ABS", "NOT", "VAR", "ARROW", "'|'", "TICK_ACCESS",
  "TICK_ADDRESS", "TICK_FIRST", "TICK_LAST", "TICK_LENGTH", "TICK_MAX",
  "TICK_MIN", "TICK_MODULUS", "TICK_POS", "TICK_RANGE", "TICK_SIZE",
  "TICK_TAG", "TICK_VAL", "'.'", "'('", "'['", "NEW", "OTHERS", "';'",
  "')'", "'\\''", "','", "'{'", "'}'", "']'", "$accept", "start", "exp1",
  "primary", "$@1", "save_qualifier", "simple_exp", "arglist", "relation",
  "exp", "and_exp", "and_then_exp", "or_exp", "or_else_exp", "xor_exp",
  "tick_arglist", "type_prefix", "opt_type_prefix", "var_or_type", "block",
  "aggregate", "aggregate_component_list", "positional_list",
  "component_groups", "others", "component_group",
  "component_associations", YY_NULLPTRPTR
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[NUM] -- (External) token number corresponding to the
   (internal) symbol number NUM (which must be that of a token).  */
static const yytype_int16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,    61,   276,    60,    62,   277,   278,   279,   280,    64,
      43,    45,    38,   281,    42,    47,   282,   283,   284,   285,
     286,   287,   288,   124,   289,   290,   291,   292,   293,   294,
     295,   296,   297,   298,   299,   300,   301,    46,    40,    91,
     302,   303,    59,    41,    39,    44,   123,   125,    93
};
# endif

#define YYPACT_NINF (-102)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-83)

#define yytable_value_is_error(Yyn) \
  0

  /* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
     STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     410,  -102,  -102,  -102,  -102,  -102,  -102,  -102,    25,  -102,
     410,   410,   134,   134,   410,   410,   269,    -3,     4,    20,
     -38,   509,   648,    35,  -102,     9,    13,    11,    27,    56,
     -46,    61,   255,    83,  -102,  -102,  -102,   546,   137,   137,
      14,    14,   137,   137,    21,    57,   -51,   585,    49,    37,
     269,  -102,  -102,    51,  -102,  -102,    62,  -102,   410,  -102,
    -102,   410,  -102,  -102,    70,    70,    70,  -102,  -102,   257,
     410,   410,   410,   410,   410,   410,   410,   410,   410,   410,
     410,   410,   410,   410,   410,   410,   410,   103,   333,   372,
     410,   410,   113,   410,   114,   410,  -102,    75,    78,    88,
      91,   257,  -102,    58,   410,   448,   410,  -102,   410,   410,
     448,  -102,  -102,    82,  -102,   269,   134,  -102,  -102,   147,
    -102,  -102,  -102,    -2,   608,    12,  -102,    84,   713,   713,
     713,   713,   713,   713,     1,   692,   170,    89,   137,   137,
     137,   116,   116,   116,   116,   116,   410,   410,  -102,   410,
    -102,  -102,  -102,   410,  -102,   410,  -102,   410,   410,   410,
     410,   628,    42,  -102,  -102,  -102,  -102,   661,  -102,  -102,
     676,  -102,  -102,  -102,  -102,    14,    92,   410,   410,  -102,
     486,  -102,    70,   410,   525,   703,   191,  -102,  -102,  -102,
    -102,    93,    86,    96,    99,   410,  -102,   105,   410,   448,
    -102,  -102,   324,    24,  -102,  -102,   713,    70,   410,  -102,
     410,   410,  -102,   498,   410,  -102,  -102,  -102,   410,  -102,
     713,   101,   104,  -102,   106,  -102,  -102,  -102,  -102
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
      84,    85,    88,    86,    87,    90,    91,    89,    93,    17,
      84,    84,    84,    84,    84,    84,    84,     0,     0,     0,
       2,    19,    39,    52,     3,    53,    54,    55,    56,    57,
      83,     0,    16,     0,    18,    97,    95,    19,    21,    20,
     117,   116,    23,    22,    93,     0,     0,    39,     3,     0,
      84,   100,   105,   106,   109,    92,     0,     1,    84,     7,
       6,    84,    68,    69,    80,    80,    80,    73,    74,    84,
      84,    84,    84,    84,    84,    84,    84,    84,    84,    84,
      84,    84,    84,    84,    84,    84,    84,     0,    84,    84,
      84,    84,     0,    84,     0,    84,    79,     0,     0,     0,
       0,    84,    12,    94,    84,    84,    84,    15,    84,    84,
      84,   103,    99,   101,   102,    84,    84,     4,     5,     0,
      70,    71,    72,    93,    39,     0,    25,     0,    40,    41,
      50,    51,    42,    49,    19,     0,    16,    35,    36,    38,
      37,    31,    32,    34,    33,    30,    84,    84,    58,    84,
      62,    66,    59,    84,    63,    84,    67,    84,    84,    84,
      84,    39,     0,    10,    98,    96,   110,     0,   113,   108,
       0,   111,   114,   104,   107,    29,     0,    84,    84,     8,
      84,   118,    80,    84,    19,     0,    16,    60,    64,    61,
      65,     0,     0,     0,     0,    84,     9,     0,    84,    84,
      81,    26,     0,    93,    27,    44,    43,    80,    84,    78,
      84,    84,    77,     0,    84,   112,   115,    13,    84,    47,
      46,     0,     0,    14,     0,    28,    76,    75,    11
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -102,  -102,   160,    19,  -102,  -102,     7,    77,   -52,     0,
    -102,  -102,  -102,  -102,  -102,   -64,  -102,  -102,   -15,  -102,
    -102,  -102,  -102,   -44,  -102,  -102,  -101
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    19,    20,    37,   197,   163,    22,   125,    23,   126,
      25,    26,    27,    28,    29,   120,    30,    31,    32,    33,
      34,    49,    50,    51,    52,    53,    54
};

  /* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
     positive, shift that token.  If negative, reduce the rule whose
     number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      24,   121,   122,    56,   168,    96,   114,    35,    55,   172,
      97,    58,   107,    59,    60,     8,    48,    38,    39,    21,
      57,    42,    43,    47,    58,    91,    59,    60,    93,    92,
      35,    40,    41,    35,    35,    21,   148,   150,   151,   152,
     177,   154,    36,   156,    94,    62,    63,    64,    65,    66,
     113,    88,    89,    90,   182,    67,    68,    47,   117,    69,
      70,   118,   136,   104,   105,    36,   218,   164,    36,    36,
     127,   174,    69,    70,    95,   179,   124,   180,   128,   129,
     130,   131,   132,   133,   135,   137,   138,   139,   140,   141,
     142,   143,   144,   145,   103,   187,   134,   188,   216,   106,
     112,   189,   165,   190,   166,   196,   169,   180,   161,   171,
      98,    99,   167,   100,   111,   170,   115,   167,   205,    79,
      80,    81,   167,    82,    83,    84,    85,    86,   119,   116,
     146,   186,   153,   157,   155,   175,   158,     1,     2,     3,
       4,     5,     6,   219,     7,     8,   159,   173,     9,   160,
     176,   210,   181,   185,    86,   200,   209,   191,   192,   193,
     194,   211,   212,   214,   226,   184,    12,   227,    13,   228,
     -45,    82,    83,    84,    85,    86,    46,   201,   162,     0,
     204,     0,     0,     0,     0,   202,   -45,   -45,   -45,     0,
     206,   -48,    16,     0,    17,     0,     0,     0,   215,     0,
      18,     0,   213,     0,     0,     0,   167,   -48,   -48,   -48,
     221,   222,     0,     0,   224,   220,     0,     0,   225,   -82,
     -82,   -82,   -82,     0,     0,     0,   -82,     0,   101,     0,
       0,     0,   -45,   -45,   102,   -45,     0,     0,   -45,     0,
     -82,   -82,   -82,   -82,     0,     0,     0,   -82,     0,   101,
       0,     0,     0,   -48,   -48,   102,   -48,     0,     0,   -48,
       1,     2,     3,     4,     5,     6,     0,     7,   123,     0,
       0,     9,     1,     2,     3,     4,     5,     6,     0,     7,
      44,     0,     0,     9,     0,     0,     0,    10,    11,    12,
       0,    13,     0,     0,     0,     0,    14,    15,     0,    10,
      11,    12,     0,    13,   -82,   -82,   -82,   -82,    14,    15,
       0,   -82,     0,   101,     0,    16,     0,    17,     0,   102,
     -24,     0,   -24,    18,     0,     0,     0,    16,     0,    17,
      45,     0,     0,     0,     0,    18,     1,     2,     3,     4,
       5,     6,     0,     7,     8,     0,     0,     9,     0,     0,
       0,     0,   147,    78,    79,    80,    81,     0,    82,    83,
      84,    85,    86,    10,    11,    12,     0,    13,     0,     0,
       0,     0,    14,    15,     0,     1,     2,     3,     4,     5,
       6,     0,     7,     8,     0,     0,     9,   217,     0,     0,
       0,    16,   149,    17,     0,     0,     0,     0,     0,    18,
       0,     0,    10,    11,    12,     0,    13,     0,     0,     0,
       0,    14,    15,     1,     2,     3,     4,     5,     6,     0,
       7,     8,     0,     0,     9,     0,     0,     0,     0,     0,
      16,     0,    17,     0,     0,     0,     0,     0,    18,     0,
      10,    11,    12,     0,    13,     0,     0,     0,     0,    14,
      15,     1,     2,     3,     4,     5,     6,     0,     7,    44,
       0,     0,     9,     0,     0,     0,     0,     0,    16,     0,
      17,     0,     0,     0,     0,     0,    18,     0,    10,    11,
      12,     0,    13,     0,     0,     0,     0,    14,    15,     1,
       2,     3,     4,     5,     6,     0,     7,   203,     0,     0,
       9,     0,     0,     0,     0,     0,    16,     0,    17,     0,
       0,     0,     0,     0,    18,     0,    10,    11,    12,     0,
      13,    59,    60,     0,    61,    14,    15,    78,    79,    80,
      81,     0,    82,    83,    84,    85,    86,    59,    60,     0,
       0,     0,     0,     0,    16,     0,    17,     0,     0,     0,
       0,     0,    18,    62,    63,    64,    65,    66,    59,    60,
       0,   223,     0,    67,    68,     0,     0,    69,    70,    62,
      63,    64,    65,    66,     0,     0,     0,     0,   207,    67,
      68,     0,     0,    69,    70,     0,     0,     0,     0,     0,
      62,    63,    64,    65,    66,     0,     0,     0,     0,     0,
      67,    68,     0,     0,    69,    70,    71,    72,    73,    74,
      75,    76,    77,   108,    78,    79,    80,    81,     0,    82,
      83,    84,    85,    86,     0,    87,     0,   109,   110,    71,
      72,    73,    74,    75,    76,    77,   178,    78,    79,    80,
      81,     0,    82,    83,    84,    85,    86,     0,    87,    71,
      72,    73,    74,    75,    76,    77,   195,    78,    79,    80,
      81,     0,    82,    83,    84,    85,    86,     0,    87,    71,
      72,    73,    74,    75,    76,    77,     0,    78,    79,    80,
      81,     0,    82,    83,    84,    85,    86,     0,    87,   108,
      78,    79,    80,    81,     0,    82,    83,    84,    85,    86,
       0,     0,     0,   109,   110,    78,    79,    80,    81,     0,
      82,    83,    84,    85,    86,     0,     0,     0,   198,   199,
     183,    78,    79,    80,    81,     0,    82,    83,    84,    85,
      86,   208,    78,    79,    80,    81,     0,    82,    83,    84,
      85,    86,    78,    79,    80,    81,     0,    82,    83,    84,
      85,    86
};

static const yytype_int16 yycheck[] =
{
       0,    65,    66,    18,   105,    51,    50,     9,    11,   110,
      56,    62,    63,    12,    13,    11,    16,    10,    11,     0,
       0,    14,    15,    16,    62,    16,    12,    13,    17,    16,
       9,    12,    13,     9,     9,    16,    88,    89,    90,    91,
      42,    93,    44,    95,    17,    44,    45,    46,    47,    48,
      50,    16,    17,    18,    53,    54,    55,    50,    58,    58,
      59,    61,    77,    42,    43,    44,    42,     9,    44,    44,
      70,   115,    58,    59,    18,    63,    69,    65,    71,    72,
      73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
      83,    84,    85,    86,    11,   147,    77,   149,   199,    42,
      63,   153,    44,   155,   104,    63,   106,    65,   101,   109,
      49,    50,   105,    52,    65,   108,    65,   110,   182,    30,
      31,    32,   115,    34,    35,    36,    37,    38,    58,    67,
      27,   146,    19,    58,    20,   116,    58,     3,     4,     5,
       6,     7,     8,   207,    10,    11,    58,    65,    14,    58,
       3,    65,    68,   146,    38,    63,    63,   157,   158,   159,
     160,    65,    63,    58,    63,   146,    32,    63,    34,    63,
       0,    34,    35,    36,    37,    38,    16,   177,   101,    -1,
     180,    -1,    -1,    -1,    -1,   178,    16,    17,    18,    -1,
     183,     0,    58,    -1,    60,    -1,    -1,    -1,   198,    -1,
      66,    -1,   195,    -1,    -1,    -1,   199,    16,    17,    18,
     210,   211,    -1,    -1,   214,   208,    -1,    -1,   218,    49,
      50,    51,    52,    -1,    -1,    -1,    56,    -1,    58,    -1,
      -1,    -1,    62,    63,    64,    65,    -1,    -1,    68,    -1,
      49,    50,    51,    52,    -1,    -1,    -1,    56,    -1,    58,
      -1,    -1,    -1,    62,    63,    64,    65,    -1,    -1,    68,
       3,     4,     5,     6,     7,     8,    -1,    10,    11,    -1,
      -1,    14,     3,     4,     5,     6,     7,     8,    -1,    10,
      11,    -1,    -1,    14,    -1,    -1,    -1,    30,    31,    32,
      -1,    34,    -1,    -1,    -1,    -1,    39,    40,    -1,    30,
      31,    32,    -1,    34,    49,    50,    51,    52,    39,    40,
      -1,    56,    -1,    58,    -1,    58,    -1,    60,    -1,    64,
      63,    -1,    65,    66,    -1,    -1,    -1,    58,    -1,    60,
      61,    -1,    -1,    -1,    -1,    66,     3,     4,     5,     6,
       7,     8,    -1,    10,    11,    -1,    -1,    14,    -1,    -1,
      -1,    -1,    19,    29,    30,    31,    32,    -1,    34,    35,
      36,    37,    38,    30,    31,    32,    -1,    34,    -1,    -1,
      -1,    -1,    39,    40,    -1,     3,     4,     5,     6,     7,
       8,    -1,    10,    11,    -1,    -1,    14,    63,    -1,    -1,
      -1,    58,    20,    60,    -1,    -1,    -1,    -1,    -1,    66,
      -1,    -1,    30,    31,    32,    -1,    34,    -1,    -1,    -1,
      -1,    39,    40,     3,     4,     5,     6,     7,     8,    -1,
      10,    11,    -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,
      58,    -1,    60,    -1,    -1,    -1,    -1,    -1,    66,    -1,
      30,    31,    32,    -1,    34,    -1,    -1,    -1,    -1,    39,
      40,     3,     4,     5,     6,     7,     8,    -1,    10,    11,
      -1,    -1,    14,    -1,    -1,    -1,    -1,    -1,    58,    -1,
      60,    -1,    -1,    -1,    -1,    -1,    66,    -1,    30,    31,
      32,    -1,    34,    -1,    -1,    -1,    -1,    39,    40,     3,
       4,     5,     6,     7,     8,    -1,    10,    11,    -1,    -1,
      14,    -1,    -1,    -1,    -1,    -1,    58,    -1,    60,    -1,
      -1,    -1,    -1,    -1,    66,    -1,    30,    31,    32,    -1,
      34,    12,    13,    -1,    15,    39,    40,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    12,    13,    -1,
      -1,    -1,    -1,    -1,    58,    -1,    60,    -1,    -1,    -1,
      -1,    -1,    66,    44,    45,    46,    47,    48,    12,    13,
      -1,    63,    -1,    54,    55,    -1,    -1,    58,    59,    44,
      45,    46,    47,    48,    -1,    -1,    -1,    -1,    53,    54,
      55,    -1,    -1,    58,    59,    -1,    -1,    -1,    -1,    -1,
      44,    45,    46,    47,    48,    -1,    -1,    -1,    -1,    -1,
      54,    55,    -1,    -1,    58,    59,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    -1,    34,
      35,    36,    37,    38,    -1,    40,    -1,    42,    43,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    -1,    40,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    -1,    40,    21,
      22,    23,    24,    25,    26,    27,    -1,    29,    30,    31,
      32,    -1,    34,    35,    36,    37,    38,    -1,    40,    28,
      29,    30,    31,    32,    -1,    34,    35,    36,    37,    38,
      -1,    -1,    -1,    42,    43,    29,    30,    31,    32,    -1,
      34,    35,    36,    37,    38,    -1,    -1,    -1,    42,    43,
      28,    29,    30,    31,    32,    -1,    34,    35,    36,    37,
      38,    28,    29,    30,    31,    32,    -1,    34,    35,    36,
      37,    38,    29,    30,    31,    32,    -1,    34,    35,    36,
      37,    38
};

  /* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
     symbol of state STATE-NUM.  */
static const yytype_int8 yystos[] =
{
       0,     3,     4,     5,     6,     7,     8,    10,    11,    14,
      30,    31,    32,    34,    39,    40,    58,    60,    66,    70,
      71,    72,    75,    77,    78,    79,    80,    81,    82,    83,
      85,    86,    87,    88,    89,     9,    44,    72,    75,    75,
      72,    72,    75,    75,    11,    61,    71,    75,    78,    90,
      91,    92,    93,    94,    95,    11,    87,     0,    62,    12,
      13,    15,    44,    45,    46,    47,    48,    54,    55,    58,
      59,    21,    22,    23,    24,    25,    26,    27,    29,    30,
      31,    32,    34,    35,    36,    37,    38,    40,    16,    17,
      18,    16,    16,    17,    17,    18,    51,    56,    49,    50,
      52,    58,    64,    11,    42,    43,    42,    63,    28,    42,
      43,    65,    63,    78,    92,    65,    67,    78,    78,    58,
      84,    84,    84,    11,    75,    76,    78,    78,    75,    75,
      75,    75,    75,    75,    72,    75,    87,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    27,    19,    77,    20,
      77,    77,    77,    19,    77,    20,    77,    58,    58,    58,
      58,    75,    76,    74,     9,    44,    78,    75,    95,    78,
      75,    78,    95,    65,    92,    72,     3,    42,    28,    63,
      65,    68,    53,    28,    72,    75,    87,    77,    77,    77,
      77,    78,    78,    78,    78,    28,    63,    73,    42,    43,
      63,    78,    75,    11,    78,    84,    75,    53,    28,    63,
      65,    65,    63,    75,    58,    78,    95,    63,    42,    84,
      75,    78,    78,    63,    78,    78,    63,    63,    63
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    69,    70,    71,    71,    71,    72,    72,    72,    72,
      73,    72,    74,    72,    72,    72,    72,    72,    72,    75,
      75,    75,    75,    75,    76,    76,    76,    76,    76,    72,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    77,
      77,    77,    77,    77,    77,    77,    77,    77,    77,    77,
      77,    77,    78,    78,    78,    78,    78,    78,    79,    79,
      80,    80,    81,    81,    82,    82,    83,    83,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
      84,    84,    85,    86,    86,    72,    72,    72,    72,    72,
      72,    72,    72,    87,    87,    87,    87,    88,    88,    89,
      90,    90,    90,    91,    91,    92,    92,    92,    93,    94,
      95,    95,    95,    95,    95,    95,    72,    72,    72
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     2,     2,     4,     4,
       0,     7,     0,     6,     6,     3,     1,     1,     1,     1,
       2,     2,     2,     2,     0,     1,     3,     3,     5,     4,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     1,
       3,     3,     3,     5,     5,     3,     6,     6,     4,     3,
       3,     3,     1,     1,     1,     1,     1,     1,     3,     3,
       4,     4,     3,     3,     4,     4,     3,     3,     2,     2,
       3,     3,     3,     2,     2,     7,     7,     5,     5,     2,
       0,     3,     1,     1,     0,     1,     1,     1,     1,     1,
       1,     1,     2,     1,     2,     2,     3,     2,     3,     3,
       1,     2,     2,     2,     3,     1,     1,     3,     3,     1,
       3,     3,     5,     3,     3,     5,     2,     2,     4
};


#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)
#define YYEMPTY         (-2)
#define YYEOF           0

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Error token number */
#define YYTERROR        1
#define YYERRCODE       256



/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)

/* This macro is provided for backward compatibility. */
#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


# define YY_SYMBOL_PRINT(Title, Type, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Type, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YYUSE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yytype], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo, int yytype, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yytype < YYNTOKENS ? "token" : "nterm", yytname[yytype]);

  yy_symbol_value_print (yyo, yytype, yyvaluep);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, int yyrule)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       yystos[+yyssp[yyi + 1 - yynrhs]],
                       &yyvsp[(yyi + 1) - (yynrhs)]
                                              );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, Rule); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
#  else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                yy_state_t *yyssp, int yytoken)
{
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTRPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Actual size of YYARG. */
  int yycount = 0;
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[+*yyssp];
      YYPTRDIFF_T yysize0 = yytnamerr (YY_NULLPTRPTR, yytname[yytoken]);
      yysize = yysize0;
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                {
                  YYPTRDIFF_T yysize1
                    = yysize + yytnamerr (YY_NULLPTRPTR, yytname[yyx]);
                  if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
                    yysize = yysize1;
                  else
                    return 2;
                }
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  {
    /* Don't count the "%s"s in the final size, but reserve room for
       the terminator.  */
    YYPTRDIFF_T yysize1 = yysize + (yystrlen (yyformat) - 2 * yycount) + 1;
    if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
      yysize = yysize1;
    else
      return 2;
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
{
  YYUSE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YYUSE (yytype);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}




/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;
/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

int
yyparse (void)
{
    yy_state_fast_t yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       'yyss': related to states.
       'yyvs': related to semantic values.

       Refer to the stacks through separate pointers, to allow yyoverflow
       to xreallocate them elsewhere.  */

    /* The state stack.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss;
    yy_state_t *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYPTRDIFF_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yyssp = yyss = yyssa;
  yyvsp = yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    goto yyexhaustedlab;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to xreallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          goto yyexhaustedlab;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
# undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 4:
#line 479 "ada-exp.y"
                        { ada_wrap2<comma_operation> (BINOP_COMMA); }
#line 2043 "ada-exp.c.tmp"
    break;

  case 5:
#line 481 "ada-exp.y"
                        {
			  operation_up rhs = pstate->pop ();
			  operation_up lhs = ada_pop ();
			  value *lhs_val
			    = lhs->evaluate (nullptr, pstate->expout.get (),
					     EVAL_AVOID_SIDE_EFFECTS);
			  rhs = resolve (std::move (rhs), true,
					 value_type (lhs_val));
			  pstate->push_new<ada_assign_operation>
			    (std::move (lhs), std::move (rhs));
			}
#line 2059 "ada-exp.c.tmp"
    break;

  case 6:
#line 496 "ada-exp.y"
                        { ada_wrap<ada_unop_ind_operation> (); }
#line 2065 "ada-exp.c.tmp"
    break;

  case 7:
#line 500 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_structop_operation>
			    (std::move (arg), copy_name ((yyvsp[0].sval)));
			}
#line 2075 "ada-exp.c.tmp"
    break;

  case 8:
#line 508 "ada-exp.y"
                        { ada_funcall ((yyvsp[-1].lval)); }
#line 2081 "ada-exp.c.tmp"
    break;

  case 9:
#line 510 "ada-exp.y"
                        {
			  if ((yyvsp[-3].tval) != NULL)
			    {
			      if ((yyvsp[-1].lval) != 1)
				error (_("Invalid conversion"));
			      operation_up arg = ada_pop ();
			      pstate->push_new<unop_cast_operation>
				(std::move (arg), (yyvsp[-3].tval));
			    }
			  else
			    ada_funcall ((yyvsp[-1].lval));
			}
#line 2098 "ada-exp.c.tmp"
    break;

  case 10:
#line 524 "ada-exp.y"
                                                { type_qualifier = (yyvsp[-2].tval); }
#line 2104 "ada-exp.c.tmp"
    break;

  case 11:
#line 526 "ada-exp.y"
                        {
			  if ((yyvsp[-6].tval) == NULL)
			    error (_("Type required for qualification"));
			  operation_up arg = ada_pop (true,
						      check_typedef ((yyvsp[-6].tval)));
			  pstate->push_new<ada_qual_operation>
			    (std::move (arg), (yyvsp[-6].tval));
			  type_qualifier = (yyvsp[-4].tval);
			}
#line 2118 "ada-exp.c.tmp"
    break;

  case 12:
#line 537 "ada-exp.y"
                        { (yyval.tval) = type_qualifier; }
#line 2124 "ada-exp.c.tmp"
    break;

  case 13:
#line 542 "ada-exp.y"
                        { ada_wrap3<ada_ternop_slice_operation> (); }
#line 2130 "ada-exp.c.tmp"
    break;

  case 14:
#line 544 "ada-exp.y"
                        { if ((yyvsp[-5].tval) == NULL) 
			    ada_wrap3<ada_ternop_slice_operation> ();
			  else
			    error (_("Cannot slice a type"));
			}
#line 2140 "ada-exp.c.tmp"
    break;

  case 15:
#line 551 "ada-exp.y"
                                { }
#line 2146 "ada-exp.c.tmp"
    break;

  case 16:
#line 563 "ada-exp.y"
                        { if ((yyvsp[0].tval) != NULL)
			    pstate->push_new<type_operation> ((yyvsp[0].tval));
			}
#line 2154 "ada-exp.c.tmp"
    break;

  case 17:
#line 569 "ada-exp.y"
                        { pstate->push_dollar ((yyvsp[0].sval)); }
#line 2160 "ada-exp.c.tmp"
    break;

  case 18:
#line 573 "ada-exp.y"
                        {
			  pstate->push_new<ada_aggregate_operation>
			    (pop_component ());
			}
#line 2169 "ada-exp.c.tmp"
    break;

  case 20:
#line 583 "ada-exp.y"
                        { ada_wrap_overload<ada_neg_operation> (UNOP_NEG); }
#line 2175 "ada-exp.c.tmp"
    break;

  case 21:
#line 587 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  operation_up empty;

			  /* If an overloaded operator was found, use
			     it.  Otherwise, unary + has no effect and
			     the argument can be pushed instead.  */
			  operation_up call = maybe_overload (UNOP_PLUS, arg,
							      empty);
			  if (call != nullptr)
			    arg = std::move (call);
			  pstate->push (std::move (arg));
			}
#line 2193 "ada-exp.c.tmp"
    break;

  case 22:
#line 603 "ada-exp.y"
                        {
			  ada_wrap_overload<unary_logical_not_operation>
			    (UNOP_LOGICAL_NOT);
			}
#line 2202 "ada-exp.c.tmp"
    break;

  case 23:
#line 610 "ada-exp.y"
                        { ada_wrap_overload<ada_abs_operation> (UNOP_ABS); }
#line 2208 "ada-exp.c.tmp"
    break;

  case 24:
#line 613 "ada-exp.y"
                        { (yyval.lval) = 0; }
#line 2214 "ada-exp.c.tmp"
    break;

  case 25:
#line 617 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2220 "ada-exp.c.tmp"
    break;

  case 26:
#line 619 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2226 "ada-exp.c.tmp"
    break;

  case 27:
#line 621 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-2].lval) + 1; }
#line 2232 "ada-exp.c.tmp"
    break;

  case 28:
#line 623 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-4].lval) + 1; }
#line 2238 "ada-exp.c.tmp"
    break;

  case 29:
#line 628 "ada-exp.y"
                        { 
			  if ((yyvsp[-2].tval) == NULL)
			    error (_("Type required within braces in coercion"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<unop_memval_operation>
			    (std::move (arg), (yyvsp[-2].tval));
			}
#line 2250 "ada-exp.c.tmp"
    break;

  case 30:
#line 640 "ada-exp.y"
                        { ada_wrap2<ada_binop_exp_operation> (BINOP_EXP); }
#line 2256 "ada-exp.c.tmp"
    break;

  case 31:
#line 644 "ada-exp.y"
                        { ada_wrap2<ada_binop_mul_operation> (BINOP_MUL); }
#line 2262 "ada-exp.c.tmp"
    break;

  case 32:
#line 648 "ada-exp.y"
                        { ada_wrap2<ada_binop_div_operation> (BINOP_DIV); }
#line 2268 "ada-exp.c.tmp"
    break;

  case 33:
#line 652 "ada-exp.y"
                        { ada_wrap2<ada_binop_rem_operation> (BINOP_REM); }
#line 2274 "ada-exp.c.tmp"
    break;

  case 34:
#line 656 "ada-exp.y"
                        { ada_wrap2<ada_binop_mod_operation> (BINOP_MOD); }
#line 2280 "ada-exp.c.tmp"
    break;

  case 35:
#line 660 "ada-exp.y"
                        { ada_wrap2<repeat_operation> (BINOP_REPEAT); }
#line 2286 "ada-exp.c.tmp"
    break;

  case 36:
#line 664 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_ADD); }
#line 2292 "ada-exp.c.tmp"
    break;

  case 37:
#line 668 "ada-exp.y"
                        { ada_wrap2<concat_operation> (BINOP_CONCAT); }
#line 2298 "ada-exp.c.tmp"
    break;

  case 38:
#line 672 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_SUB); }
#line 2304 "ada-exp.c.tmp"
    break;

  case 40:
#line 679 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_EQUAL); }
#line 2310 "ada-exp.c.tmp"
    break;

  case 41:
#line 683 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_NOTEQUAL); }
#line 2316 "ada-exp.c.tmp"
    break;

  case 42:
#line 687 "ada-exp.y"
                        { ada_un_wrap2<leq_operation> (BINOP_LEQ); }
#line 2322 "ada-exp.c.tmp"
    break;

  case 43:
#line 691 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> (); }
#line 2328 "ada-exp.c.tmp"
    break;

  case 44:
#line 693 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			}
#line 2339 "ada-exp.c.tmp"
    break;

  case 45:
#line 700 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			}
#line 2351 "ada-exp.c.tmp"
    break;

  case 46:
#line 708 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> ();
			  ada_wrap<unary_logical_not_operation> (); }
#line 2358 "ada-exp.c.tmp"
    break;

  case 47:
#line 711 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2370 "ada-exp.c.tmp"
    break;

  case 48:
#line 719 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2383 "ada-exp.c.tmp"
    break;

  case 49:
#line 730 "ada-exp.y"
                        { ada_un_wrap2<geq_operation> (BINOP_GEQ); }
#line 2389 "ada-exp.c.tmp"
    break;

  case 50:
#line 734 "ada-exp.y"
                        { ada_un_wrap2<less_operation> (BINOP_LESS); }
#line 2395 "ada-exp.c.tmp"
    break;

  case 51:
#line 738 "ada-exp.y"
                        { ada_un_wrap2<gtr_operation> (BINOP_GTR); }
#line 2401 "ada-exp.c.tmp"
    break;

  case 58:
#line 751 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2408 "ada-exp.c.tmp"
    break;

  case 59:
#line 754 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2415 "ada-exp.c.tmp"
    break;

  case 60:
#line 760 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2422 "ada-exp.c.tmp"
    break;

  case 61:
#line 763 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2429 "ada-exp.c.tmp"
    break;

  case 62:
#line 769 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2436 "ada-exp.c.tmp"
    break;

  case 63:
#line 772 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2443 "ada-exp.c.tmp"
    break;

  case 64:
#line 778 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2449 "ada-exp.c.tmp"
    break;

  case 65:
#line 780 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2455 "ada-exp.c.tmp"
    break;

  case 66:
#line 784 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2462 "ada-exp.c.tmp"
    break;

  case 67:
#line 787 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2469 "ada-exp.c.tmp"
    break;

  case 68:
#line 800 "ada-exp.y"
                        { ada_addrof (); }
#line 2475 "ada-exp.c.tmp"
    break;

  case 69:
#line 802 "ada-exp.y"
                        { ada_addrof (type_system_address (pstate)); }
#line 2481 "ada-exp.c.tmp"
    break;

  case 70:
#line 804 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_FIRST, (yyvsp[0].lval));
			}
#line 2491 "ada-exp.c.tmp"
    break;

  case 71:
#line 810 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LAST, (yyvsp[0].lval));
			}
#line 2501 "ada-exp.c.tmp"
    break;

  case 72:
#line 816 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LENGTH, (yyvsp[0].lval));
			}
#line 2511 "ada-exp.c.tmp"
    break;

  case 73:
#line 822 "ada-exp.y"
                        { ada_wrap<ada_atr_size_operation> (); }
#line 2517 "ada-exp.c.tmp"
    break;

  case 74:
#line 824 "ada-exp.y"
                        { ada_wrap<ada_atr_tag_operation> (); }
#line 2523 "ada-exp.c.tmp"
    break;

  case 75:
#line 826 "ada-exp.y"
                        { ada_wrap2<ada_binop_min_operation> (BINOP_MIN); }
#line 2529 "ada-exp.c.tmp"
    break;

  case 76:
#line 828 "ada-exp.y"
                        { ada_wrap2<ada_binop_max_operation> (BINOP_MAX); }
#line 2535 "ada-exp.c.tmp"
    break;

  case 77:
#line 830 "ada-exp.y"
                        { ada_wrap<ada_pos_operation> (); }
#line 2541 "ada-exp.c.tmp"
    break;

  case 78:
#line 832 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_atr_val_operation>
			    ((yyvsp[-4].tval), std::move (arg));
			}
#line 2551 "ada-exp.c.tmp"
    break;

  case 79:
#line 838 "ada-exp.y"
                        {
			  struct type *type_arg = check_typedef ((yyvsp[-1].tval));
			  if (!ada_is_modular_type (type_arg))
			    error (_("'modulus must be applied to modular type"));
			  write_int (pstate, ada_modulus (type_arg),
				     TYPE_TARGET_TYPE (type_arg));
			}
#line 2563 "ada-exp.c.tmp"
    break;

  case 80:
#line 848 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2569 "ada-exp.c.tmp"
    break;

  case 81:
#line 850 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-1].typed_val).val; }
#line 2575 "ada-exp.c.tmp"
    break;

  case 82:
#line 855 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Prefix must be type"));
			  (yyval.tval) = (yyvsp[0].tval);
			}
#line 2585 "ada-exp.c.tmp"
    break;

  case 83:
#line 864 "ada-exp.y"
                        { (yyval.tval) = (yyvsp[0].tval); }
#line 2591 "ada-exp.c.tmp"
    break;

  case 84:
#line 866 "ada-exp.y"
                        { (yyval.tval) = parse_type (pstate)->builtin_void; }
#line 2597 "ada-exp.c.tmp"
    break;

  case 85:
#line 871 "ada-exp.y"
                        { write_int (pstate, (LONGEST) (yyvsp[0].typed_val).val, (yyvsp[0].typed_val).type); }
#line 2603 "ada-exp.c.tmp"
    break;

  case 86:
#line 875 "ada-exp.y"
                  { write_int (pstate,
			       convert_char_literal (type_qualifier, (yyvsp[0].typed_val).val),
			       (type_qualifier == NULL) 
			       ? (yyvsp[0].typed_val).type : type_qualifier);
		  }
#line 2613 "ada-exp.c.tmp"
    break;

  case 87:
#line 883 "ada-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation>
			    ((yyvsp[0].typed_val_float).type, data);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2626 "ada-exp.c.tmp"
    break;

  case 88:
#line 894 "ada-exp.y"
                        {
			  struct type *null_ptr_type
			    = lookup_pointer_type (parse_type (pstate)->builtin_int0);
			  write_int (pstate, 0, null_ptr_type);
			}
#line 2636 "ada-exp.c.tmp"
    break;

  case 89:
#line 902 "ada-exp.y"
                        { 
			  pstate->push_new<ada_string_operation>
			    (copy_name ((yyvsp[0].sval)));
			}
#line 2645 "ada-exp.c.tmp"
    break;

  case 90:
#line 909 "ada-exp.y"
                        { write_int (pstate, 1, type_boolean (pstate)); }
#line 2651 "ada-exp.c.tmp"
    break;

  case 91:
#line 911 "ada-exp.y"
                        { write_int (pstate, 0, type_boolean (pstate)); }
#line 2657 "ada-exp.c.tmp"
    break;

  case 92:
#line 915 "ada-exp.y"
                        { error (_("NEW not implemented.")); }
#line 2663 "ada-exp.c.tmp"
    break;

  case 93:
#line 919 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[0].sval)); }
#line 2669 "ada-exp.c.tmp"
    break;

  case 94:
#line 921 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, (yyvsp[-1].bval), (yyvsp[0].sval)); }
#line 2675 "ada-exp.c.tmp"
    break;

  case 95:
#line 923 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2687 "ada-exp.c.tmp"
    break;

  case 96:
#line 931 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, (yyvsp[-2].bval), (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2699 "ada-exp.c.tmp"
    break;

  case 97:
#line 942 "ada-exp.y"
                        { (yyval.bval) = block_lookup (NULL, (yyvsp[-1].sval).ptr); }
#line 2705 "ada-exp.c.tmp"
    break;

  case 98:
#line 944 "ada-exp.y"
                        { (yyval.bval) = block_lookup ((yyvsp[-2].bval), (yyvsp[-1].sval).ptr); }
#line 2711 "ada-exp.c.tmp"
    break;

  case 99:
#line 949 "ada-exp.y"
                        {
			  std::vector<ada_component_up> components
			    = pop_components ((yyvsp[-1].lval));

			  push_component<ada_aggregate_component>
			    (std::move (components));
			}
#line 2723 "ada-exp.c.tmp"
    break;

  case 100:
#line 959 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval); }
#line 2729 "ada-exp.c.tmp"
    break;

  case 101:
#line 961 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-1].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-1].lval) + 1;
			}
#line 2739 "ada-exp.c.tmp"
    break;

  case 102:
#line 967 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[-1].lval) + (yyvsp[0].lval); }
#line 2745 "ada-exp.c.tmp"
    break;

  case 103:
#line 972 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    (0, ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2755 "ada-exp.c.tmp"
    break;

  case 104:
#line 978 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-2].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-2].lval) + 1; 
			}
#line 2765 "ada-exp.c.tmp"
    break;

  case 105:
#line 986 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2771 "ada-exp.c.tmp"
    break;

  case 106:
#line 987 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2777 "ada-exp.c.tmp"
    break;

  case 107:
#line 989 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2783 "ada-exp.c.tmp"
    break;

  case 108:
#line 993 "ada-exp.y"
                        {
			  push_component<ada_others_component> (ada_pop ());
			}
#line 2791 "ada-exp.c.tmp"
    break;

  case 109:
#line 1000 "ada-exp.y"
                        {
			  ada_choices_component *choices = choice_component ();
			  choices->set_associations (pop_associations ((yyvsp[0].lval)));
			}
#line 2800 "ada-exp.c.tmp"
    break;

  case 110:
#line 1013 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = 1;
			}
#line 2810 "ada-exp.c.tmp"
    break;

  case 111:
#line 1019 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2820 "ada-exp.c.tmp"
    break;

  case 112:
#line 1025 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = 1;
			}
#line 2833 "ada-exp.c.tmp"
    break;

  case 113:
#line 1034 "ada-exp.y"
                        {
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2842 "ada-exp.c.tmp"
    break;

  case 114:
#line 1039 "ada-exp.y"
                        {
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2851 "ada-exp.c.tmp"
    break;

  case 115:
#line 1045 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2863 "ada-exp.c.tmp"
    break;

  case 116:
#line 1058 "ada-exp.y"
                        { ada_wrap<ada_unop_ind_operation> (); }
#line 2869 "ada-exp.c.tmp"
    break;

  case 117:
#line 1060 "ada-exp.y"
                        { ada_addrof (); }
#line 2875 "ada-exp.c.tmp"
    break;

  case 118:
#line 1062 "ada-exp.y"
                        {
			  ada_wrap2<subscript_operation> (BINOP_SUBSCRIPT);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2884 "ada-exp.c.tmp"
    break;


#line 2888 "ada-exp.c.tmp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *, YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYTERROR;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;


      yydestruct ("Error: popping",
                  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;


#if !defined yyoverflow || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif


/*-----------------------------------------------------.
| yyreturn -- parsing is finished, return the result.  |
`-----------------------------------------------------*/
yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  yystos[+*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  return yyresult;
}
#line 1068 "ada-exp.y"


/* yylex defined in ada-lex.c: Reads one token, getting characters */
/* through lexptr.  */

/* Remap normal flex interface names (yylex) as well as gratuitiously */
/* global symbol names, so we can have multiple flex-generated parsers */
/* in gdb.  */

/* (See note above on previous definitions for YACC.) */

#define yy_create_buffer ada_yy_create_buffer
#define yy_delete_buffer ada_yy_delete_buffer
#define yy_init_buffer ada_yy_init_buffer
#define yy_load_buffer_state ada_yy_load_buffer_state
#define yy_switch_to_buffer ada_yy_switch_to_buffer
#define yyrestart ada_yyrestart
#define yytext ada_yytext

static struct obstack temp_parse_space;

/* The following kludge was found necessary to prevent conflicts between */
/* defs.h and non-standard stdlib.h files.  */
#define qsort __qsort__dummy
#include "ada-lex.c"

int
ada_parse (struct parser_state *par_state)
{
  /* Setting up the parser state.  */
  scoped_restore pstate_restore = make_scoped_restore (&pstate);
  gdb_assert (par_state != NULL);
  pstate = par_state;

  lexer_init (yyin);		/* (Re-)initialize lexer.  */
  type_qualifier = NULL;
  obstack_free (&temp_parse_space, NULL);
  obstack_init (&temp_parse_space);
  components.clear ();
  associations.clear ();

  int result = yyparse ();
  if (!result)
    {
      struct type *context_type = nullptr;
      if (par_state->void_context_p)
	context_type = parse_type (par_state)->builtin_void;
      pstate->set_operation (ada_pop (true, context_type));
    }
  return result;
}

static void
yyerror (const char *msg)
{
  error (_("Error in expression, near `%s'."), pstate->lexptr);
}

/* Emit expression to access an instance of SYM, in block BLOCK (if
   non-NULL).  */

static void
write_var_from_sym (struct parser_state *par_state, block_symbol sym)
{
  if (symbol_read_needs_frame (sym.symbol))
    par_state->block_tracker->update (sym.block, INNERMOST_BLOCK_FOR_SYMBOLS);

  par_state->push_new<ada_var_value_operation> (sym);
}

/* Write integer or boolean constant ARG of type TYPE.  */

static void
write_int (struct parser_state *par_state, LONGEST arg, struct type *type)
{
  pstate->push_new<long_const_operation> (type, arg);
  ada_wrap<ada_wrapped_operation> ();
}

/* Emit expression corresponding to the renamed object named 
 * designated by RENAMED_ENTITY[0 .. RENAMED_ENTITY_LEN-1] in the
 * context of ORIG_LEFT_CONTEXT, to which is applied the operations
 * encoded by RENAMING_EXPR.  MAX_DEPTH is the maximum number of
 * cascaded renamings to allow.  If ORIG_LEFT_CONTEXT is null, it
 * defaults to the currently selected block. ORIG_SYMBOL is the 
 * symbol that originally encoded the renaming.  It is needed only
 * because its prefix also qualifies any index variables used to index
 * or slice an array.  It should not be necessary once we go to the
 * new encoding entirely (FIXME pnh 7/20/2007).  */

static void
write_object_renaming (struct parser_state *par_state,
		       const struct block *orig_left_context,
		       const char *renamed_entity, int renamed_entity_len,
		       const char *renaming_expr, int max_depth)
{
  char *name;
  enum { SIMPLE_INDEX, LOWER_BOUND, UPPER_BOUND } slice_state;
  struct block_symbol sym_info;

  if (max_depth <= 0)
    error (_("Could not find renamed symbol"));

  if (orig_left_context == NULL)
    orig_left_context = get_selected_block (NULL);

  name = obstack_strndup (&temp_parse_space, renamed_entity,
			  renamed_entity_len);
  ada_lookup_encoded_symbol (name, orig_left_context, VAR_DOMAIN, &sym_info);
  if (sym_info.symbol == NULL)
    error (_("Could not find renamed variable: %s"), ada_decode (name).c_str ());
  else if (SYMBOL_CLASS (sym_info.symbol) == LOC_TYPEDEF)
    /* We have a renaming of an old-style renaming symbol.  Don't
       trust the block information.  */
    sym_info.block = orig_left_context;

  {
    const char *inner_renamed_entity;
    int inner_renamed_entity_len;
    const char *inner_renaming_expr;

    switch (ada_parse_renaming (sym_info.symbol, &inner_renamed_entity,
				&inner_renamed_entity_len,
				&inner_renaming_expr))
      {
      case ADA_NOT_RENAMING:
	write_var_from_sym (par_state, sym_info);
	break;
      case ADA_OBJECT_RENAMING:
	write_object_renaming (par_state, sym_info.block,
			       inner_renamed_entity, inner_renamed_entity_len,
			       inner_renaming_expr, max_depth - 1);
	break;
      default:
	goto BadEncoding;
      }
  }

  slice_state = SIMPLE_INDEX;
  while (*renaming_expr == 'X')
    {
      renaming_expr += 1;

      switch (*renaming_expr) {
      case 'A':
	renaming_expr += 1;
	ada_wrap<ada_unop_ind_operation> ();
	break;
      case 'L':
	slice_state = LOWER_BOUND;
	/* FALLTHROUGH */
      case 'S':
	renaming_expr += 1;
	if (isdigit (*renaming_expr))
	  {
	    char *next;
	    long val = strtol (renaming_expr, &next, 10);
	    if (next == renaming_expr)
	      goto BadEncoding;
	    renaming_expr = next;
	    write_int (par_state, val, type_int (par_state));
	  }
	else
	  {
	    const char *end;
	    char *index_name;
	    struct block_symbol index_sym_info;

	    end = strchr (renaming_expr, 'X');
	    if (end == NULL)
	      end = renaming_expr + strlen (renaming_expr);

	    index_name = obstack_strndup (&temp_parse_space, renaming_expr,
					  end - renaming_expr);
	    renaming_expr = end;

	    ada_lookup_encoded_symbol (index_name, orig_left_context,
				       VAR_DOMAIN, &index_sym_info);
	    if (index_sym_info.symbol == NULL)
	      error (_("Could not find %s"), index_name);
	    else if (SYMBOL_CLASS (index_sym_info.symbol) == LOC_TYPEDEF)
	      /* Index is an old-style renaming symbol.  */
	      index_sym_info.block = orig_left_context;
	    write_var_from_sym (par_state, index_sym_info);
	  }
	if (slice_state == SIMPLE_INDEX)
	  ada_funcall (1);
	else if (slice_state == LOWER_BOUND)
	  slice_state = UPPER_BOUND;
	else if (slice_state == UPPER_BOUND)
	  {
	    ada_wrap3<ada_ternop_slice_operation> ();
	    slice_state = SIMPLE_INDEX;
	  }
	break;

      case 'R':
	{
	  const char *end;

	  renaming_expr += 1;

	  if (slice_state != SIMPLE_INDEX)
	    goto BadEncoding;
	  end = strchr (renaming_expr, 'X');
	  if (end == NULL)
	    end = renaming_expr + strlen (renaming_expr);

	  operation_up arg = ada_pop ();
	  pstate->push_new<ada_structop_operation>
	    (std::move (arg), std::string (renaming_expr,
					   end - renaming_expr));
	  renaming_expr = end;
	  break;
	}

      default:
	goto BadEncoding;
      }
    }
  if (slice_state == SIMPLE_INDEX)
    return;

 BadEncoding:
  error (_("Internal error in encoding of renaming declaration"));
}

static const struct block*
block_lookup (const struct block *context, const char *raw_name)
{
  const char *name;
  struct symtab *symtab;
  const struct block *result = NULL;

  std::string name_storage;
  if (raw_name[0] == '\'')
    {
      raw_name += 1;
      name = raw_name;
    }
  else
    {
      name_storage = ada_encode (raw_name);
      name = name_storage.c_str ();
    }

  std::vector<struct block_symbol> syms
    = ada_lookup_symbol_list (name, context, VAR_DOMAIN);

  if (context == NULL
      && (syms.empty () || SYMBOL_CLASS (syms[0].symbol) != LOC_BLOCK))
    symtab = lookup_symtab (name);
  else
    symtab = NULL;

  if (symtab != NULL)
    result = BLOCKVECTOR_BLOCK (SYMTAB_BLOCKVECTOR (symtab), STATIC_BLOCK);
  else if (syms.empty () || SYMBOL_CLASS (syms[0].symbol) != LOC_BLOCK)
    {
      if (context == NULL)
	error (_("No file or function \"%s\"."), raw_name);
      else
	error (_("No function \"%s\" in specified context."), raw_name);
    }
  else
    {
      if (syms.size () > 1)
	warning (_("Function name \"%s\" ambiguous here"), raw_name);
      result = SYMBOL_BLOCK_VALUE (syms[0].symbol);
    }

  return result;
}

static struct symbol*
select_possible_type_sym (const std::vector<struct block_symbol> &syms)
{
  int i;
  int preferred_index;
  struct type *preferred_type;
	  
  preferred_index = -1; preferred_type = NULL;
  for (i = 0; i < syms.size (); i += 1)
    switch (SYMBOL_CLASS (syms[i].symbol))
      {
      case LOC_TYPEDEF:
	if (ada_prefer_type (SYMBOL_TYPE (syms[i].symbol), preferred_type))
	  {
	    preferred_index = i;
	    preferred_type = SYMBOL_TYPE (syms[i].symbol);
	  }
	break;
      case LOC_REGISTER:
      case LOC_ARG:
      case LOC_REF_ARG:
      case LOC_REGPARM_ADDR:
      case LOC_LOCAL:
      case LOC_COMPUTED:
	return NULL;
      default:
	break;
      }
  if (preferred_type == NULL)
    return NULL;
  return syms[preferred_index].symbol;
}

static struct type*
find_primitive_type (struct parser_state *par_state, const char *name)
{
  struct type *type;
  type = language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 name);
  if (type == NULL && strcmp ("system__address", name) == 0)
    type = type_system_address (par_state);

  if (type != NULL)
    {
      /* Check to see if we have a regular definition of this
	 type that just didn't happen to have been read yet.  */
      struct symbol *sym;
      char *expanded_name = 
	(char *) alloca (strlen (name) + sizeof ("standard__"));
      strcpy (expanded_name, "standard__");
      strcat (expanded_name, name);
      sym = ada_lookup_symbol (expanded_name, NULL, VAR_DOMAIN).symbol;
      if (sym != NULL && SYMBOL_CLASS (sym) == LOC_TYPEDEF)
	type = SYMBOL_TYPE (sym);
    }

  return type;
}

static int
chop_selector (const char *name, int end)
{
  int i;
  for (i = end - 1; i > 0; i -= 1)
    if (name[i] == '.' || (name[i] == '_' && name[i+1] == '_'))
      return i;
  return -1;
}

/* If NAME is a string beginning with a separator (either '__', or
   '.'), chop this separator and return the result; else, return
   NAME.  */

static const char *
chop_separator (const char *name)
{
  if (*name == '.')
   return name + 1;

  if (name[0] == '_' && name[1] == '_')
    return name + 2;

  return name;
}

/* Given that SELS is a string of the form (<sep><identifier>)*, where
   <sep> is '__' or '.', write the indicated sequence of
   STRUCTOP_STRUCT expression operators. */
static void
write_selectors (struct parser_state *par_state, const char *sels)
{
  while (*sels != '\0')
    {
      const char *p = chop_separator (sels);
      sels = p;
      while (*sels != '\0' && *sels != '.' 
	     && (sels[0] != '_' || sels[1] != '_'))
	sels += 1;
      operation_up arg = ada_pop ();
      pstate->push_new<ada_structop_operation>
	(std::move (arg), std::string (p, sels - p));
    }
}

/* Write a variable access (OP_VAR_VALUE) to ambiguous encoded name
   NAME[0..LEN-1], in block context BLOCK, to be resolved later.  Writes
   a temporary symbol that is valid until the next call to ada_parse.
   */
static void
write_ambiguous_var (struct parser_state *par_state,
		     const struct block *block, const char *name, int len)
{
  struct symbol *sym = new (&temp_parse_space) symbol ();

  SYMBOL_DOMAIN (sym) = UNDEF_DOMAIN;
  sym->set_linkage_name (obstack_strndup (&temp_parse_space, name, len));
  sym->set_language (language_ada, nullptr);

  block_symbol bsym { sym, block };
  par_state->push_new<ada_var_value_operation> (bsym);
}

/* A convenient wrapper around ada_get_field_index that takes
   a non NUL-terminated FIELD_NAME0 and a FIELD_NAME_LEN instead
   of a NUL-terminated field name.  */

static int
ada_nget_field_index (const struct type *type, const char *field_name0,
		      int field_name_len, int maybe_missing)
{
  char *field_name = (char *) alloca ((field_name_len + 1) * sizeof (char));

  strncpy (field_name, field_name0, field_name_len);
  field_name[field_name_len] = '\0';
  return ada_get_field_index (type, field_name, maybe_missing);
}

/* If encoded_field_name is the name of a field inside symbol SYM,
   then return the type of that field.  Otherwise, return NULL.

   This function is actually recursive, so if ENCODED_FIELD_NAME
   doesn't match one of the fields of our symbol, then try to see
   if ENCODED_FIELD_NAME could not be a succession of field names
   (in other words, the user entered an expression of the form
   TYPE_NAME.FIELD1.FIELD2.FIELD3), in which case we evaluate
   each field name sequentially to obtain the desired field type.
   In case of failure, we return NULL.  */

static struct type *
get_symbol_field_type (struct symbol *sym, const char *encoded_field_name)
{
  const char *field_name = encoded_field_name;
  const char *subfield_name;
  struct type *type = SYMBOL_TYPE (sym);
  int fieldno;

  if (type == NULL || field_name == NULL)
    return NULL;
  type = check_typedef (type);

  while (field_name[0] != '\0')
    {
      field_name = chop_separator (field_name);

      fieldno = ada_get_field_index (type, field_name, 1);
      if (fieldno >= 0)
	return type->field (fieldno).type ();

      subfield_name = field_name;
      while (*subfield_name != '\0' && *subfield_name != '.' 
	     && (subfield_name[0] != '_' || subfield_name[1] != '_'))
	subfield_name += 1;

      if (subfield_name[0] == '\0')
	return NULL;

      fieldno = ada_nget_field_index (type, field_name,
				      subfield_name - field_name, 1);
      if (fieldno < 0)
	return NULL;

      type = type->field (fieldno).type ();
      field_name = subfield_name;
    }

  return NULL;
}

/* Look up NAME0 (an unencoded identifier or dotted name) in BLOCK (or 
   expression_block_context if NULL).  If it denotes a type, return
   that type.  Otherwise, write expression code to evaluate it as an
   object and return NULL. In this second case, NAME0 will, in general,
   have the form <name>(.<selector_name>)*, where <name> is an object
   or renaming encoded in the debugging data.  Calls error if no
   prefix <name> matches a name in the debugging data (i.e., matches
   either a complete name or, as a wild-card match, the final 
   identifier).  */

static struct type*
write_var_or_type (struct parser_state *par_state,
		   const struct block *block, struct stoken name0)
{
  int depth;
  char *encoded_name;
  int name_len;

  if (block == NULL)
    block = par_state->expression_context_block;

  std::string name_storage = ada_encode (name0.ptr);
  name_len = name_storage.size ();
  encoded_name = obstack_strndup (&temp_parse_space, name_storage.c_str (),
				  name_len);
  for (depth = 0; depth < MAX_RENAMING_CHAIN_LENGTH; depth += 1)
    {
      int tail_index;
      
      tail_index = name_len;
      while (tail_index > 0)
	{
	  struct symbol *type_sym;
	  struct symbol *renaming_sym;
	  const char* renaming;
	  int renaming_len;
	  const char* renaming_expr;
	  int terminator = encoded_name[tail_index];

	  encoded_name[tail_index] = '\0';
	  std::vector<struct block_symbol> syms
	    = ada_lookup_symbol_list (encoded_name, block, VAR_DOMAIN);
	  encoded_name[tail_index] = terminator;

	  type_sym = select_possible_type_sym (syms);

	  if (type_sym != NULL)
	    renaming_sym = type_sym;
	  else if (syms.size () == 1)
	    renaming_sym = syms[0].symbol;
	  else 
	    renaming_sym = NULL;

	  switch (ada_parse_renaming (renaming_sym, &renaming,
				      &renaming_len, &renaming_expr))
	    {
	    case ADA_NOT_RENAMING:
	      break;
	    case ADA_PACKAGE_RENAMING:
	    case ADA_EXCEPTION_RENAMING:
	    case ADA_SUBPROGRAM_RENAMING:
	      {
		int alloc_len = renaming_len + name_len - tail_index + 1;
		char *new_name
		  = (char *) obstack_alloc (&temp_parse_space, alloc_len);
		strncpy (new_name, renaming, renaming_len);
		strcpy (new_name + renaming_len, encoded_name + tail_index);
		encoded_name = new_name;
		name_len = renaming_len + name_len - tail_index;
		goto TryAfterRenaming;
	      }	
	    case ADA_OBJECT_RENAMING:
	      write_object_renaming (par_state, block, renaming, renaming_len,
				     renaming_expr, MAX_RENAMING_CHAIN_LENGTH);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    default:
	      internal_error (__FILE__, __LINE__,
			      _("impossible value from ada_parse_renaming"));
	    }

	  if (type_sym != NULL)
	    {
	      struct type *field_type;
	      
	      if (tail_index == name_len)
		return SYMBOL_TYPE (type_sym);

	      /* We have some extraneous characters after the type name.
		 If this is an expression "TYPE_NAME.FIELD0.[...].FIELDN",
		 then try to get the type of FIELDN.  */
	      field_type
		= get_symbol_field_type (type_sym, encoded_name + tail_index);
	      if (field_type != NULL)
		return field_type;
	      else 
		error (_("Invalid attempt to select from type: \"%s\"."),
		       name0.ptr);
	    }
	  else if (tail_index == name_len && syms.empty ())
	    {
	      struct type *type = find_primitive_type (par_state,
						       encoded_name);

	      if (type != NULL)
		return type;
	    }

	  if (syms.size () == 1)
	    {
	      write_var_from_sym (par_state, syms[0]);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	  else if (syms.empty ())
	    {
	      struct bound_minimal_symbol msym
		= ada_lookup_simple_minsym (encoded_name);
	      if (msym.minsym != NULL)
		{
		  par_state->push_new<ada_var_msym_value_operation> (msym);
		  /* Maybe cause error here rather than later? FIXME? */
		  write_selectors (par_state, encoded_name + tail_index);
		  return NULL;
		}

	      if (tail_index == name_len
		  && strncmp (encoded_name, "standard__", 
			      sizeof ("standard__") - 1) == 0)
		error (_("No definition of \"%s\" found."), name0.ptr);

	      tail_index = chop_selector (encoded_name, tail_index);
	    } 
	  else
	    {
	      write_ambiguous_var (par_state, block, encoded_name,
				   tail_index);
	      write_selectors (par_state, encoded_name + tail_index);
	      return NULL;
	    }
	}

      if (!have_full_symbols () && !have_partial_symbols () && block == NULL)
	error (_("No symbol table is loaded.  Use the \"file\" command."));
      if (block == par_state->expression_context_block)
	error (_("No definition of \"%s\" in current context."), name0.ptr);
      else
	error (_("No definition of \"%s\" in specified context."), name0.ptr);
      
    TryAfterRenaming: ;
    }

  error (_("Could not find renamed symbol \"%s\""), name0.ptr);

}

/* Write a left side of a component association (e.g., NAME in NAME =>
   exp).  If NAME has the form of a selected component, write it as an
   ordinary expression.  If it is a simple variable that unambiguously
   corresponds to exactly one symbol that does not denote a type or an
   object renaming, also write it normally as an OP_VAR_VALUE.
   Otherwise, write it as an OP_NAME.

   Unfortunately, we don't know at this point whether NAME is supposed
   to denote a record component name or the value of an array index.
   Therefore, it is not appropriate to disambiguate an ambiguous name
   as we normally would, nor to replace a renaming with its referent.
   As a result, in the (one hopes) rare case that one writes an
   aggregate such as (R => 42) where R renames an object or is an
   ambiguous name, one must write instead ((R) => 42). */
   
static void
write_name_assoc (struct parser_state *par_state, struct stoken name)
{
  if (strchr (name.ptr, '.') == NULL)
    {
      std::vector<struct block_symbol> syms
	= ada_lookup_symbol_list (name.ptr,
				  par_state->expression_context_block,
				  VAR_DOMAIN);

      if (syms.size () != 1 || SYMBOL_CLASS (syms[0].symbol) == LOC_TYPEDEF)
	pstate->push_new<ada_string_operation> (copy_name (name));
      else
	write_var_from_sym (par_state, syms[0]);
    }
  else
    if (write_var_or_type (par_state, NULL, name) != NULL)
      error (_("Invalid use of type."));

  push_association<ada_name_association> (ada_pop ());
}

/* Convert the character literal whose ASCII value would be VAL to the
   appropriate value of type TYPE, if there is a translation.
   Otherwise return VAL.  Hence, in an enumeration type ('A', 'B'),
   the literal 'A' (VAL == 65), returns 0.  */

static LONGEST
convert_char_literal (struct type *type, LONGEST val)
{
  char name[7];
  int f;

  if (type == NULL)
    return val;
  type = check_typedef (type);
  if (type->code () != TYPE_CODE_ENUM)
    return val;

  if ((val >= 'a' && val <= 'z') || (val >= '0' && val <= '9'))
    xsnprintf (name, sizeof (name), "Q%c", (int) val);
  else
    xsnprintf (name, sizeof (name), "QU%02x", (int) val);
  size_t len = strlen (name);
  for (f = 0; f < type->num_fields (); f += 1)
    {
      /* Check the suffix because an enum constant in a package will
	 have a name like "pkg__QUxx".  This is safe enough because we
	 already have the correct type, and because mangling means
	 there can't be clashes.  */
      const char *ename = TYPE_FIELD_NAME (type, f);
      size_t elen = strlen (ename);

      if (elen >= len && strcmp (name, ename + elen - len) == 0)
	return TYPE_FIELD_ENUMVAL (type, f);
    }
  return val;
}

static struct type *
type_int (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_int;
}

static struct type *
type_long (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long;
}

static struct type *
type_long_long (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long_long;
}

static struct type *
type_long_double (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_long_double;
}

static struct type *
type_char (struct parser_state *par_state)
{
  return language_string_char_type (par_state->language (),
				    par_state->gdbarch ());
}

static struct type *
type_boolean (struct parser_state *par_state)
{
  return parse_type (par_state)->builtin_bool;
}

static struct type *
type_system_address (struct parser_state *par_state)
{
  struct type *type 
    = language_lookup_primitive_type (par_state->language (),
				      par_state->gdbarch (),
				      "system__address");
  return  type != NULL ? type : parse_type (par_state)->builtin_data_ptr;
}

void _initialize_ada_exp ();
void
_initialize_ada_exp ()
{
  obstack_init (&temp_parse_space);
}
