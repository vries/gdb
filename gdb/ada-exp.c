/* A Bison parser, made by GNU Bison 3.7.6.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
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
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30706

/* Bison version string.  */
#define YYBISON_VERSION "3.7.6"

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

static void write_ambiguous_var (struct parser_state *,
				 const struct block *, const char *, int);

static struct type *type_int (struct parser_state *);

static struct type *type_long (struct parser_state *);

static struct type *type_long_long (struct parser_state *);

static struct type *type_long_double (struct parser_state *);

static struct type *type_for_char (struct parser_state *, ULONGEST);

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
  if (res != nullptr)
    return res->replace (std::move (result),
			 pstate->expout.get (),
			 deprocedure_p,
			 pstate->parse_completion,
			 pstate->block_tracker,
			 context_type);
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
      || vvo->get_symbol ()->domain () != UNDEF_DOMAIN)
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


#line 433 "ada-exp.c.tmp"

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


/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    INT = 258,                     /* INT  */
    NULL_PTR = 259,                /* NULL_PTR  */
    CHARLIT = 260,                 /* CHARLIT  */
    FLOAT = 261,                   /* FLOAT  */
    TRUEKEYWORD = 262,             /* TRUEKEYWORD  */
    FALSEKEYWORD = 263,            /* FALSEKEYWORD  */
    COLONCOLON = 264,              /* COLONCOLON  */
    STRING = 265,                  /* STRING  */
    NAME = 266,                    /* NAME  */
    DOT_ID = 267,                  /* DOT_ID  */
    DOT_ALL = 268,                 /* DOT_ALL  */
    DOLLAR_VARIABLE = 269,         /* DOLLAR_VARIABLE  */
    ASSIGN = 270,                  /* ASSIGN  */
    _AND_ = 271,                   /* _AND_  */
    OR = 272,                      /* OR  */
    XOR = 273,                     /* XOR  */
    THEN = 274,                    /* THEN  */
    ELSE = 275,                    /* ELSE  */
    NOTEQUAL = 276,                /* NOTEQUAL  */
    LEQ = 277,                     /* LEQ  */
    GEQ = 278,                     /* GEQ  */
    IN = 279,                      /* IN  */
    DOTDOT = 280,                  /* DOTDOT  */
    UNARY = 281,                   /* UNARY  */
    MOD = 282,                     /* MOD  */
    REM = 283,                     /* REM  */
    STARSTAR = 284,                /* STARSTAR  */
    ABS = 285,                     /* ABS  */
    NOT = 286,                     /* NOT  */
    VAR = 287,                     /* VAR  */
    ARROW = 288,                   /* ARROW  */
    TICK_ACCESS = 289,             /* TICK_ACCESS  */
    TICK_ADDRESS = 290,            /* TICK_ADDRESS  */
    TICK_FIRST = 291,              /* TICK_FIRST  */
    TICK_LAST = 292,               /* TICK_LAST  */
    TICK_LENGTH = 293,             /* TICK_LENGTH  */
    TICK_MAX = 294,                /* TICK_MAX  */
    TICK_MIN = 295,                /* TICK_MIN  */
    TICK_MODULUS = 296,            /* TICK_MODULUS  */
    TICK_POS = 297,                /* TICK_POS  */
    TICK_RANGE = 298,              /* TICK_RANGE  */
    TICK_SIZE = 299,               /* TICK_SIZE  */
    TICK_TAG = 300,                /* TICK_TAG  */
    TICK_VAL = 301,                /* TICK_VAL  */
    NEW = 302,                     /* NEW  */
    OTHERS = 303                   /* OTHERS  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
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
#line 399 "ada-exp.y"

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
  

#line 596 "ada-exp.c.tmp"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);


/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_INT = 3,                        /* INT  */
  YYSYMBOL_NULL_PTR = 4,                   /* NULL_PTR  */
  YYSYMBOL_CHARLIT = 5,                    /* CHARLIT  */
  YYSYMBOL_FLOAT = 6,                      /* FLOAT  */
  YYSYMBOL_TRUEKEYWORD = 7,                /* TRUEKEYWORD  */
  YYSYMBOL_FALSEKEYWORD = 8,               /* FALSEKEYWORD  */
  YYSYMBOL_COLONCOLON = 9,                 /* COLONCOLON  */
  YYSYMBOL_STRING = 10,                    /* STRING  */
  YYSYMBOL_NAME = 11,                      /* NAME  */
  YYSYMBOL_DOT_ID = 12,                    /* DOT_ID  */
  YYSYMBOL_DOT_ALL = 13,                   /* DOT_ALL  */
  YYSYMBOL_DOLLAR_VARIABLE = 14,           /* DOLLAR_VARIABLE  */
  YYSYMBOL_ASSIGN = 15,                    /* ASSIGN  */
  YYSYMBOL__AND_ = 16,                     /* _AND_  */
  YYSYMBOL_OR = 17,                        /* OR  */
  YYSYMBOL_XOR = 18,                       /* XOR  */
  YYSYMBOL_THEN = 19,                      /* THEN  */
  YYSYMBOL_ELSE = 20,                      /* ELSE  */
  YYSYMBOL_21_ = 21,                       /* '='  */
  YYSYMBOL_NOTEQUAL = 22,                  /* NOTEQUAL  */
  YYSYMBOL_23_ = 23,                       /* '<'  */
  YYSYMBOL_24_ = 24,                       /* '>'  */
  YYSYMBOL_LEQ = 25,                       /* LEQ  */
  YYSYMBOL_GEQ = 26,                       /* GEQ  */
  YYSYMBOL_IN = 27,                        /* IN  */
  YYSYMBOL_DOTDOT = 28,                    /* DOTDOT  */
  YYSYMBOL_29_ = 29,                       /* '@'  */
  YYSYMBOL_30_ = 30,                       /* '+'  */
  YYSYMBOL_31_ = 31,                       /* '-'  */
  YYSYMBOL_32_ = 32,                       /* '&'  */
  YYSYMBOL_UNARY = 33,                     /* UNARY  */
  YYSYMBOL_34_ = 34,                       /* '*'  */
  YYSYMBOL_35_ = 35,                       /* '/'  */
  YYSYMBOL_MOD = 36,                       /* MOD  */
  YYSYMBOL_REM = 37,                       /* REM  */
  YYSYMBOL_STARSTAR = 38,                  /* STARSTAR  */
  YYSYMBOL_ABS = 39,                       /* ABS  */
  YYSYMBOL_NOT = 40,                       /* NOT  */
  YYSYMBOL_VAR = 41,                       /* VAR  */
  YYSYMBOL_ARROW = 42,                     /* ARROW  */
  YYSYMBOL_43_ = 43,                       /* '|'  */
  YYSYMBOL_TICK_ACCESS = 44,               /* TICK_ACCESS  */
  YYSYMBOL_TICK_ADDRESS = 45,              /* TICK_ADDRESS  */
  YYSYMBOL_TICK_FIRST = 46,                /* TICK_FIRST  */
  YYSYMBOL_TICK_LAST = 47,                 /* TICK_LAST  */
  YYSYMBOL_TICK_LENGTH = 48,               /* TICK_LENGTH  */
  YYSYMBOL_TICK_MAX = 49,                  /* TICK_MAX  */
  YYSYMBOL_TICK_MIN = 50,                  /* TICK_MIN  */
  YYSYMBOL_TICK_MODULUS = 51,              /* TICK_MODULUS  */
  YYSYMBOL_TICK_POS = 52,                  /* TICK_POS  */
  YYSYMBOL_TICK_RANGE = 53,                /* TICK_RANGE  */
  YYSYMBOL_TICK_SIZE = 54,                 /* TICK_SIZE  */
  YYSYMBOL_TICK_TAG = 55,                  /* TICK_TAG  */
  YYSYMBOL_TICK_VAL = 56,                  /* TICK_VAL  */
  YYSYMBOL_57_ = 57,                       /* '.'  */
  YYSYMBOL_58_ = 58,                       /* '('  */
  YYSYMBOL_59_ = 59,                       /* '['  */
  YYSYMBOL_NEW = 60,                       /* NEW  */
  YYSYMBOL_OTHERS = 61,                    /* OTHERS  */
  YYSYMBOL_62_ = 62,                       /* ';'  */
  YYSYMBOL_63_ = 63,                       /* ')'  */
  YYSYMBOL_64_ = 64,                       /* '\''  */
  YYSYMBOL_65_ = 65,                       /* ','  */
  YYSYMBOL_66_ = 66,                       /* '{'  */
  YYSYMBOL_67_ = 67,                       /* '}'  */
  YYSYMBOL_68_ = 68,                       /* ']'  */
  YYSYMBOL_YYACCEPT = 69,                  /* $accept  */
  YYSYMBOL_start = 70,                     /* start  */
  YYSYMBOL_exp1 = 71,                      /* exp1  */
  YYSYMBOL_primary = 72,                   /* primary  */
  YYSYMBOL_simple_exp = 73,                /* simple_exp  */
  YYSYMBOL_arglist = 74,                   /* arglist  */
  YYSYMBOL_relation = 75,                  /* relation  */
  YYSYMBOL_exp = 76,                       /* exp  */
  YYSYMBOL_and_exp = 77,                   /* and_exp  */
  YYSYMBOL_and_then_exp = 78,              /* and_then_exp  */
  YYSYMBOL_or_exp = 79,                    /* or_exp  */
  YYSYMBOL_or_else_exp = 80,               /* or_else_exp  */
  YYSYMBOL_xor_exp = 81,                   /* xor_exp  */
  YYSYMBOL_tick_arglist = 82,              /* tick_arglist  */
  YYSYMBOL_type_prefix = 83,               /* type_prefix  */
  YYSYMBOL_opt_type_prefix = 84,           /* opt_type_prefix  */
  YYSYMBOL_var_or_type = 85,               /* var_or_type  */
  YYSYMBOL_block = 86,                     /* block  */
  YYSYMBOL_aggregate = 87,                 /* aggregate  */
  YYSYMBOL_aggregate_component_list = 88,  /* aggregate_component_list  */
  YYSYMBOL_positional_list = 89,           /* positional_list  */
  YYSYMBOL_component_groups = 90,          /* component_groups  */
  YYSYMBOL_others = 91,                    /* others  */
  YYSYMBOL_component_group = 92,           /* component_group  */
  YYSYMBOL_component_associations = 93     /* component_associations  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




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

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
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
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
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

#if !defined yyoverflow

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
#endif /* !defined yyoverflow */

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
#define YYNNTS  25
/* YYNRULES -- Number of rules.  */
#define YYNRULES  116
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  227

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   303


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

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
       0,   462,   462,   466,   467,   469,   484,   488,   496,   498,
     513,   525,   527,   535,   546,   552,   556,   563,   566,   570,
     586,   593,   597,   600,   602,   604,   606,   610,   623,   627,
     631,   635,   639,   643,   647,   651,   655,   659,   662,   666,
     670,   674,   676,   683,   691,   694,   702,   713,   717,   721,
     725,   726,   727,   728,   729,   730,   734,   737,   743,   746,
     752,   755,   761,   763,   767,   770,   783,   785,   787,   793,
     799,   805,   807,   809,   811,   813,   815,   821,   831,   833,
     838,   847,   850,   854,   858,   864,   875,   883,   890,   892,
     896,   900,   902,   904,   912,   923,   925,   930,   941,   942,
     948,   953,   959,   968,   969,   970,   974,   981,   994,  1000,
    1006,  1015,  1020,  1025,  1039,  1041,  1043
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if YYDEBUG || 0
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "INT", "NULL_PTR",
  "CHARLIT", "FLOAT", "TRUEKEYWORD", "FALSEKEYWORD", "COLONCOLON",
  "STRING", "NAME", "DOT_ID", "DOT_ALL", "DOLLAR_VARIABLE", "ASSIGN",
  "_AND_", "OR", "XOR", "THEN", "ELSE", "'='", "NOTEQUAL", "'<'", "'>'",
  "LEQ", "GEQ", "IN", "DOTDOT", "'@'", "'+'", "'-'", "'&'", "UNARY", "'*'",
  "'/'", "MOD", "REM", "STARSTAR", "ABS", "NOT", "VAR", "ARROW", "'|'",
  "TICK_ACCESS", "TICK_ADDRESS", "TICK_FIRST", "TICK_LAST", "TICK_LENGTH",
  "TICK_MAX", "TICK_MIN", "TICK_MODULUS", "TICK_POS", "TICK_RANGE",
  "TICK_SIZE", "TICK_TAG", "TICK_VAL", "'.'", "'('", "'['", "NEW",
  "OTHERS", "';'", "')'", "'\\''", "','", "'{'", "'}'", "']'", "$accept",
  "start", "exp1", "primary", "simple_exp", "arglist", "relation", "exp",
  "and_exp", "and_then_exp", "or_exp", "or_else_exp", "xor_exp",
  "tick_arglist", "type_prefix", "opt_type_prefix", "var_or_type", "block",
  "aggregate", "aggregate_component_list", "positional_list",
  "component_groups", "others", "component_group",
  "component_associations", YY_NULLPTRPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#ifdef YYPRINT
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
#endif

#define YYPACT_NINF (-102)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-81)

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
      91,   257,    92,    58,   410,   448,   410,  -102,   410,   410,
     448,  -102,  -102,    82,  -102,   269,   134,  -102,  -102,   148,
    -102,  -102,  -102,    -2,   608,    12,  -102,    84,   713,   713,
     713,   713,   713,   713,     1,   692,   170,    89,   137,   137,
     137,   116,   116,   116,   116,   116,   410,   410,  -102,   410,
    -102,  -102,  -102,   410,  -102,   410,  -102,   410,   410,   410,
     410,   628,    42,   410,  -102,  -102,  -102,   661,  -102,  -102,
     676,  -102,  -102,  -102,  -102,    14,    93,   410,   410,  -102,
     486,  -102,    70,   410,   525,   703,   191,  -102,  -102,  -102,
    -102,    98,    90,    97,   101,   410,  -102,   104,   410,   448,
    -102,  -102,   324,    24,  -102,  -102,   713,    70,   410,  -102,
     410,   410,  -102,   498,  -102,  -102,  -102,  -102,   410,  -102,
     713,   106,   115,  -102,  -102,  -102,  -102
};

  /* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
     Performed when YYTABLE does not specify something else to do.  Zero
     means the default is an error.  */
static const yytype_int8 yydefact[] =
{
      82,    83,    86,    84,    85,    88,    89,    87,    91,    15,
      82,    82,    82,    82,    82,    82,    82,     0,     0,     0,
       2,    17,    37,    50,     3,    51,    52,    53,    54,    55,
      81,     0,    14,     0,    16,    95,    93,    17,    19,    18,
     115,   114,    21,    20,    91,     0,     0,    37,     3,     0,
      82,    98,   103,   104,   107,    90,     0,     1,    82,     7,
       6,    82,    66,    67,    78,    78,    78,    71,    72,    82,
      82,    82,    82,    82,    82,    82,    82,    82,    82,    82,
      82,    82,    82,    82,    82,    82,    82,     0,    82,    82,
      82,    82,     0,    82,     0,    82,    77,     0,     0,     0,
       0,    82,     0,    92,    82,    82,    82,    13,    82,    82,
      82,   101,    97,    99,   100,    82,    82,     4,     5,     0,
      68,    69,    70,    91,    37,     0,    23,     0,    38,    39,
      48,    49,    40,    47,    17,     0,    14,    33,    34,    36,
      35,    29,    30,    32,    31,    28,    82,    82,    56,    82,
      60,    64,    57,    82,    61,    82,    65,    82,    82,    82,
      82,    37,     0,    82,    96,    94,   108,     0,   111,   106,
       0,   109,   112,   102,   105,    27,     0,    82,    82,     8,
      82,   116,    78,    82,    17,     0,    14,    58,    62,    59,
      63,     0,     0,     0,     0,    82,     9,     0,    82,    82,
      79,    24,     0,    91,    25,    42,    41,    78,    82,    76,
      82,    82,    75,     0,    10,   110,   113,    11,    82,    45,
      44,     0,     0,    12,    26,    74,    73
};

  /* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -102,  -102,   160,    19,     7,    80,   -52,     0,  -102,  -102,
    -102,  -102,  -102,   -64,  -102,  -102,   -15,  -102,  -102,  -102,
    -102,   -44,  -102,  -102,  -101
};

  /* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int8 yydefgoto[] =
{
       0,    19,    20,    37,    22,   125,    23,   126,    25,    26,
      27,    28,    29,   120,    30,    31,    32,    33,    34,    49,
      50,    51,    52,    53,    54
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
     163,   176,   181,   185,    86,   210,   200,   191,   192,   193,
     194,   209,   211,   197,   212,   184,    12,   214,    13,   225,
     -43,    82,    83,    84,    85,    86,    46,   201,   226,     0,
     204,   162,     0,     0,     0,   202,   -43,   -43,   -43,     0,
     206,   -46,    16,     0,    17,     0,     0,     0,   215,     0,
      18,     0,   213,     0,     0,     0,   167,   -46,   -46,   -46,
     221,   222,     0,     0,     0,   220,     0,     0,   224,   -80,
     -80,   -80,   -80,     0,     0,     0,   -80,     0,   101,     0,
       0,     0,   -43,   -43,   102,   -43,     0,     0,   -43,     0,
     -80,   -80,   -80,   -80,     0,     0,     0,   -80,     0,   101,
       0,     0,     0,   -46,   -46,   102,   -46,     0,     0,   -46,
       1,     2,     3,     4,     5,     6,     0,     7,   123,     0,
       0,     9,     1,     2,     3,     4,     5,     6,     0,     7,
      44,     0,     0,     9,     0,     0,     0,    10,    11,    12,
       0,    13,     0,     0,     0,     0,    14,    15,     0,    10,
      11,    12,     0,    13,   -80,   -80,   -80,   -80,    14,    15,
       0,   -80,     0,   101,     0,    16,     0,    17,     0,   102,
     -22,     0,   -22,    18,     0,     0,     0,    16,     0,    17,
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
      58,     3,    68,   146,    38,    65,    63,   157,   158,   159,
     160,    63,    65,   163,    63,   146,    32,    63,    34,    63,
       0,    34,    35,    36,    37,    38,    16,   177,    63,    -1,
     180,   101,    -1,    -1,    -1,   178,    16,    17,    18,    -1,
     183,     0,    58,    -1,    60,    -1,    -1,    -1,   198,    -1,
      66,    -1,   195,    -1,    -1,    -1,   199,    16,    17,    18,
     210,   211,    -1,    -1,    -1,   208,    -1,    -1,   218,    49,
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
      71,    72,    73,    75,    76,    77,    78,    79,    80,    81,
      83,    84,    85,    86,    87,     9,    44,    72,    73,    73,
      72,    72,    73,    73,    11,    61,    71,    73,    76,    88,
      89,    90,    91,    92,    93,    11,    85,     0,    62,    12,
      13,    15,    44,    45,    46,    47,    48,    54,    55,    58,
      59,    21,    22,    23,    24,    25,    26,    27,    29,    30,
      31,    32,    34,    35,    36,    37,    38,    40,    16,    17,
      18,    16,    16,    17,    17,    18,    51,    56,    49,    50,
      52,    58,    64,    11,    42,    43,    42,    63,    28,    42,
      43,    65,    63,    76,    90,    65,    67,    76,    76,    58,
      82,    82,    82,    11,    73,    74,    76,    76,    73,    73,
      73,    73,    73,    73,    72,    73,    85,    73,    73,    73,
      73,    73,    73,    73,    73,    73,    27,    19,    75,    20,
      75,    75,    75,    19,    75,    20,    75,    58,    58,    58,
      58,    73,    74,    58,     9,    44,    76,    73,    93,    76,
      73,    76,    93,    65,    90,    72,     3,    42,    28,    63,
      65,    68,    53,    28,    72,    73,    85,    75,    75,    75,
      75,    76,    76,    76,    76,    28,    63,    76,    42,    43,
      63,    76,    73,    11,    76,    82,    73,    53,    28,    63,
      65,    65,    63,    73,    63,    76,    93,    63,    42,    82,
      73,    76,    76,    63,    76,    63,    63
};

  /* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_int8 yyr1[] =
{
       0,    69,    70,    71,    71,    71,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    73,    73,    73,
      73,    73,    74,    74,    74,    74,    74,    72,    73,    73,
      73,    73,    73,    73,    73,    73,    73,    75,    75,    75,
      75,    75,    75,    75,    75,    75,    75,    75,    75,    75,
      76,    76,    76,    76,    76,    76,    77,    77,    78,    78,
      79,    79,    80,    80,    81,    81,    72,    72,    72,    72,
      72,    72,    72,    72,    72,    72,    72,    72,    82,    82,
      83,    84,    84,    72,    72,    72,    72,    72,    72,    72,
      72,    85,    85,    85,    85,    86,    86,    87,    88,    88,
      88,    89,    89,    90,    90,    90,    91,    92,    93,    93,
      93,    93,    93,    93,    72,    72,    72
};

  /* YYR2[YYN] -- Number of symbols on the right hand side of rule YYN.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     1,     1,     3,     3,     2,     2,     4,     4,
       5,     6,     6,     3,     1,     1,     1,     1,     2,     2,
       2,     2,     0,     1,     3,     3,     5,     4,     3,     3,
       3,     3,     3,     3,     3,     3,     3,     1,     3,     3,
       3,     5,     5,     3,     6,     6,     4,     3,     3,     3,
       1,     1,     1,     1,     1,     1,     3,     3,     4,     4,
       3,     3,     4,     4,     3,     3,     2,     2,     3,     3,
       3,     2,     2,     7,     7,     5,     5,     2,     0,     3,
       1,     1,     0,     1,     1,     1,     1,     1,     1,     1,
       2,     1,     2,     2,     3,     2,     3,     3,     1,     2,
       2,     2,     3,     1,     1,     3,     3,     1,     3,     3,
       5,     3,     3,     5,     2,     2,     4
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

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

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF


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
# ifndef YY_LOCATION_PRINT
#  define YY_LOCATION_PRINT(File, Loc) ((void) 0)
# endif


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yykind < YYNTOKENS)
    YYPRINT (yyo, yytoknum[yykind], *yyvaluep);
# endif
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  yy_symbol_value_print (yyo, yykind, yyvaluep);
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
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp,
                 int yyrule)
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
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)]);
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
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
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






/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep)
{
  YY_USE (yyvaluep);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/* Lookahead token kind.  */
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
    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to xreallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;



#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

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
  YY_STACK_PRINT (yyss, yyssp);

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
#  undef YYSTACK_RELOCATE
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

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex ();
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      goto yyerrlab1;
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
  case 4: /* exp1: exp1 ';' exp  */
#line 468 "ada-exp.y"
                        { ada_wrap2<comma_operation> (BINOP_COMMA); }
#line 1937 "ada-exp.c.tmp"
    break;

  case 5: /* exp1: primary ASSIGN exp  */
#line 470 "ada-exp.y"
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
#line 1953 "ada-exp.c.tmp"
    break;

  case 6: /* primary: primary DOT_ALL  */
#line 485 "ada-exp.y"
                        { ada_wrap<ada_unop_ind_operation> (); }
#line 1959 "ada-exp.c.tmp"
    break;

  case 7: /* primary: primary DOT_ID  */
#line 489 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_structop_operation>
			    (std::move (arg), copy_name ((yyvsp[0].sval)));
			}
#line 1969 "ada-exp.c.tmp"
    break;

  case 8: /* primary: primary '(' arglist ')'  */
#line 497 "ada-exp.y"
                        { ada_funcall ((yyvsp[-1].lval)); }
#line 1975 "ada-exp.c.tmp"
    break;

  case 9: /* primary: var_or_type '(' arglist ')'  */
#line 499 "ada-exp.y"
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
#line 1992 "ada-exp.c.tmp"
    break;

  case 10: /* primary: var_or_type '\'' '(' exp ')'  */
#line 514 "ada-exp.y"
                        {
			  if ((yyvsp[-4].tval) == NULL)
			    error (_("Type required for qualification"));
			  operation_up arg = ada_pop (true,
						      check_typedef ((yyvsp[-4].tval)));
			  pstate->push_new<ada_qual_operation>
			    (std::move (arg), (yyvsp[-4].tval));
			}
#line 2005 "ada-exp.c.tmp"
    break;

  case 11: /* primary: primary '(' simple_exp DOTDOT simple_exp ')'  */
#line 526 "ada-exp.y"
                        { ada_wrap3<ada_ternop_slice_operation> (); }
#line 2011 "ada-exp.c.tmp"
    break;

  case 12: /* primary: var_or_type '(' simple_exp DOTDOT simple_exp ')'  */
#line 528 "ada-exp.y"
                        { if ((yyvsp[-5].tval) == NULL) 
			    ada_wrap3<ada_ternop_slice_operation> ();
			  else
			    error (_("Cannot slice a type"));
			}
#line 2021 "ada-exp.c.tmp"
    break;

  case 13: /* primary: '(' exp1 ')'  */
#line 535 "ada-exp.y"
                                { }
#line 2027 "ada-exp.c.tmp"
    break;

  case 14: /* primary: var_or_type  */
#line 547 "ada-exp.y"
                        { if ((yyvsp[0].tval) != NULL)
			    pstate->push_new<type_operation> ((yyvsp[0].tval));
			}
#line 2035 "ada-exp.c.tmp"
    break;

  case 15: /* primary: DOLLAR_VARIABLE  */
#line 553 "ada-exp.y"
                        { pstate->push_dollar ((yyvsp[0].sval)); }
#line 2041 "ada-exp.c.tmp"
    break;

  case 16: /* primary: aggregate  */
#line 557 "ada-exp.y"
                        {
			  pstate->push_new<ada_aggregate_operation>
			    (pop_component ());
			}
#line 2050 "ada-exp.c.tmp"
    break;

  case 18: /* simple_exp: '-' simple_exp  */
#line 567 "ada-exp.y"
                        { ada_wrap_overload<ada_neg_operation> (UNOP_NEG); }
#line 2056 "ada-exp.c.tmp"
    break;

  case 19: /* simple_exp: '+' simple_exp  */
#line 571 "ada-exp.y"
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
#line 2074 "ada-exp.c.tmp"
    break;

  case 20: /* simple_exp: NOT simple_exp  */
#line 587 "ada-exp.y"
                        {
			  ada_wrap_overload<unary_logical_not_operation>
			    (UNOP_LOGICAL_NOT);
			}
#line 2083 "ada-exp.c.tmp"
    break;

  case 21: /* simple_exp: ABS simple_exp  */
#line 594 "ada-exp.y"
                        { ada_wrap_overload<ada_abs_operation> (UNOP_ABS); }
#line 2089 "ada-exp.c.tmp"
    break;

  case 22: /* arglist: %empty  */
#line 597 "ada-exp.y"
                        { (yyval.lval) = 0; }
#line 2095 "ada-exp.c.tmp"
    break;

  case 23: /* arglist: exp  */
#line 601 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2101 "ada-exp.c.tmp"
    break;

  case 24: /* arglist: NAME ARROW exp  */
#line 603 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2107 "ada-exp.c.tmp"
    break;

  case 25: /* arglist: arglist ',' exp  */
#line 605 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-2].lval) + 1; }
#line 2113 "ada-exp.c.tmp"
    break;

  case 26: /* arglist: arglist ',' NAME ARROW exp  */
#line 607 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-4].lval) + 1; }
#line 2119 "ada-exp.c.tmp"
    break;

  case 27: /* primary: '{' var_or_type '}' primary  */
#line 612 "ada-exp.y"
                        { 
			  if ((yyvsp[-2].tval) == NULL)
			    error (_("Type required within braces in coercion"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<unop_memval_operation>
			    (std::move (arg), (yyvsp[-2].tval));
			}
#line 2131 "ada-exp.c.tmp"
    break;

  case 28: /* simple_exp: simple_exp STARSTAR simple_exp  */
#line 624 "ada-exp.y"
                        { ada_wrap2<ada_binop_exp_operation> (BINOP_EXP); }
#line 2137 "ada-exp.c.tmp"
    break;

  case 29: /* simple_exp: simple_exp '*' simple_exp  */
#line 628 "ada-exp.y"
                        { ada_wrap2<ada_binop_mul_operation> (BINOP_MUL); }
#line 2143 "ada-exp.c.tmp"
    break;

  case 30: /* simple_exp: simple_exp '/' simple_exp  */
#line 632 "ada-exp.y"
                        { ada_wrap2<ada_binop_div_operation> (BINOP_DIV); }
#line 2149 "ada-exp.c.tmp"
    break;

  case 31: /* simple_exp: simple_exp REM simple_exp  */
#line 636 "ada-exp.y"
                        { ada_wrap2<ada_binop_rem_operation> (BINOP_REM); }
#line 2155 "ada-exp.c.tmp"
    break;

  case 32: /* simple_exp: simple_exp MOD simple_exp  */
#line 640 "ada-exp.y"
                        { ada_wrap2<ada_binop_mod_operation> (BINOP_MOD); }
#line 2161 "ada-exp.c.tmp"
    break;

  case 33: /* simple_exp: simple_exp '@' simple_exp  */
#line 644 "ada-exp.y"
                        { ada_wrap2<repeat_operation> (BINOP_REPEAT); }
#line 2167 "ada-exp.c.tmp"
    break;

  case 34: /* simple_exp: simple_exp '+' simple_exp  */
#line 648 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_ADD); }
#line 2173 "ada-exp.c.tmp"
    break;

  case 35: /* simple_exp: simple_exp '&' simple_exp  */
#line 652 "ada-exp.y"
                        { ada_wrap2<ada_concat_operation> (BINOP_CONCAT); }
#line 2179 "ada-exp.c.tmp"
    break;

  case 36: /* simple_exp: simple_exp '-' simple_exp  */
#line 656 "ada-exp.y"
                        { ada_wrap_op<ada_binop_addsub_operation> (BINOP_SUB); }
#line 2185 "ada-exp.c.tmp"
    break;

  case 38: /* relation: simple_exp '=' simple_exp  */
#line 663 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_EQUAL); }
#line 2191 "ada-exp.c.tmp"
    break;

  case 39: /* relation: simple_exp NOTEQUAL simple_exp  */
#line 667 "ada-exp.y"
                        { ada_wrap_op<ada_binop_equal_operation> (BINOP_NOTEQUAL); }
#line 2197 "ada-exp.c.tmp"
    break;

  case 40: /* relation: simple_exp LEQ simple_exp  */
#line 671 "ada-exp.y"
                        { ada_un_wrap2<leq_operation> (BINOP_LEQ); }
#line 2203 "ada-exp.c.tmp"
    break;

  case 41: /* relation: simple_exp IN simple_exp DOTDOT simple_exp  */
#line 675 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> (); }
#line 2209 "ada-exp.c.tmp"
    break;

  case 42: /* relation: simple_exp IN primary TICK_RANGE tick_arglist  */
#line 677 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			}
#line 2220 "ada-exp.c.tmp"
    break;

  case 43: /* relation: simple_exp IN var_or_type  */
#line 684 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			}
#line 2232 "ada-exp.c.tmp"
    break;

  case 44: /* relation: simple_exp NOT IN simple_exp DOTDOT simple_exp  */
#line 692 "ada-exp.y"
                        { ada_wrap3<ada_ternop_range_operation> ();
			  ada_wrap<unary_logical_not_operation> (); }
#line 2239 "ada-exp.c.tmp"
    break;

  case 45: /* relation: simple_exp NOT IN primary TICK_RANGE tick_arglist  */
#line 695 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  pstate->push_new<ada_binop_in_bounds_operation>
			    (std::move (lhs), std::move (rhs), (yyvsp[0].lval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2251 "ada-exp.c.tmp"
    break;

  case 46: /* relation: simple_exp NOT IN var_or_type  */
#line 703 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Right operand of 'in' must be type"));
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_range_operation>
			    (std::move (arg), (yyvsp[0].tval));
			  ada_wrap<unary_logical_not_operation> ();
			}
#line 2264 "ada-exp.c.tmp"
    break;

  case 47: /* relation: simple_exp GEQ simple_exp  */
#line 714 "ada-exp.y"
                        { ada_un_wrap2<geq_operation> (BINOP_GEQ); }
#line 2270 "ada-exp.c.tmp"
    break;

  case 48: /* relation: simple_exp '<' simple_exp  */
#line 718 "ada-exp.y"
                        { ada_un_wrap2<less_operation> (BINOP_LESS); }
#line 2276 "ada-exp.c.tmp"
    break;

  case 49: /* relation: simple_exp '>' simple_exp  */
#line 722 "ada-exp.y"
                        { ada_un_wrap2<gtr_operation> (BINOP_GTR); }
#line 2282 "ada-exp.c.tmp"
    break;

  case 56: /* and_exp: relation _AND_ relation  */
#line 735 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2289 "ada-exp.c.tmp"
    break;

  case 57: /* and_exp: and_exp _AND_ relation  */
#line 738 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_and_operation>
			    (BINOP_BITWISE_AND); }
#line 2296 "ada-exp.c.tmp"
    break;

  case 58: /* and_then_exp: relation _AND_ THEN relation  */
#line 744 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2303 "ada-exp.c.tmp"
    break;

  case 59: /* and_then_exp: and_then_exp _AND_ THEN relation  */
#line 747 "ada-exp.y"
                        { ada_wrap2<logical_and_operation>
			    (BINOP_LOGICAL_AND); }
#line 2310 "ada-exp.c.tmp"
    break;

  case 60: /* or_exp: relation OR relation  */
#line 753 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2317 "ada-exp.c.tmp"
    break;

  case 61: /* or_exp: or_exp OR relation  */
#line 756 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_ior_operation>
			    (BINOP_BITWISE_IOR); }
#line 2324 "ada-exp.c.tmp"
    break;

  case 62: /* or_else_exp: relation OR ELSE relation  */
#line 762 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2330 "ada-exp.c.tmp"
    break;

  case 63: /* or_else_exp: or_else_exp OR ELSE relation  */
#line 764 "ada-exp.y"
                        { ada_wrap2<logical_or_operation> (BINOP_LOGICAL_OR); }
#line 2336 "ada-exp.c.tmp"
    break;

  case 64: /* xor_exp: relation XOR relation  */
#line 768 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2343 "ada-exp.c.tmp"
    break;

  case 65: /* xor_exp: xor_exp XOR relation  */
#line 771 "ada-exp.y"
                        { ada_wrap2<ada_bitwise_xor_operation>
			    (BINOP_BITWISE_XOR); }
#line 2350 "ada-exp.c.tmp"
    break;

  case 66: /* primary: primary TICK_ACCESS  */
#line 784 "ada-exp.y"
                        { ada_addrof (); }
#line 2356 "ada-exp.c.tmp"
    break;

  case 67: /* primary: primary TICK_ADDRESS  */
#line 786 "ada-exp.y"
                        { ada_addrof (type_system_address (pstate)); }
#line 2362 "ada-exp.c.tmp"
    break;

  case 68: /* primary: primary TICK_FIRST tick_arglist  */
#line 788 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_FIRST, (yyvsp[0].lval));
			}
#line 2372 "ada-exp.c.tmp"
    break;

  case 69: /* primary: primary TICK_LAST tick_arglist  */
#line 794 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LAST, (yyvsp[0].lval));
			}
#line 2382 "ada-exp.c.tmp"
    break;

  case 70: /* primary: primary TICK_LENGTH tick_arglist  */
#line 800 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_unop_atr_operation>
			    (std::move (arg), OP_ATR_LENGTH, (yyvsp[0].lval));
			}
#line 2392 "ada-exp.c.tmp"
    break;

  case 71: /* primary: primary TICK_SIZE  */
#line 806 "ada-exp.y"
                        { ada_wrap<ada_atr_size_operation> (); }
#line 2398 "ada-exp.c.tmp"
    break;

  case 72: /* primary: primary TICK_TAG  */
#line 808 "ada-exp.y"
                        { ada_wrap<ada_atr_tag_operation> (); }
#line 2404 "ada-exp.c.tmp"
    break;

  case 73: /* primary: opt_type_prefix TICK_MIN '(' exp ',' exp ')'  */
#line 810 "ada-exp.y"
                        { ada_wrap2<ada_binop_min_operation> (BINOP_MIN); }
#line 2410 "ada-exp.c.tmp"
    break;

  case 74: /* primary: opt_type_prefix TICK_MAX '(' exp ',' exp ')'  */
#line 812 "ada-exp.y"
                        { ada_wrap2<ada_binop_max_operation> (BINOP_MAX); }
#line 2416 "ada-exp.c.tmp"
    break;

  case 75: /* primary: opt_type_prefix TICK_POS '(' exp ')'  */
#line 814 "ada-exp.y"
                        { ada_wrap<ada_pos_operation> (); }
#line 2422 "ada-exp.c.tmp"
    break;

  case 76: /* primary: type_prefix TICK_VAL '(' exp ')'  */
#line 816 "ada-exp.y"
                        {
			  operation_up arg = ada_pop ();
			  pstate->push_new<ada_atr_val_operation>
			    ((yyvsp[-4].tval), std::move (arg));
			}
#line 2432 "ada-exp.c.tmp"
    break;

  case 77: /* primary: type_prefix TICK_MODULUS  */
#line 822 "ada-exp.y"
                        {
			  struct type *type_arg = check_typedef ((yyvsp[-1].tval));
			  if (!ada_is_modular_type (type_arg))
			    error (_("'modulus must be applied to modular type"));
			  write_int (pstate, ada_modulus (type_arg),
				     TYPE_TARGET_TYPE (type_arg));
			}
#line 2444 "ada-exp.c.tmp"
    break;

  case 78: /* tick_arglist: %empty  */
#line 832 "ada-exp.y"
                        { (yyval.lval) = 1; }
#line 2450 "ada-exp.c.tmp"
    break;

  case 79: /* tick_arglist: '(' INT ')'  */
#line 834 "ada-exp.y"
                        { (yyval.lval) = (yyvsp[-1].typed_val).val; }
#line 2456 "ada-exp.c.tmp"
    break;

  case 80: /* type_prefix: var_or_type  */
#line 839 "ada-exp.y"
                        { 
			  if ((yyvsp[0].tval) == NULL)
			    error (_("Prefix must be type"));
			  (yyval.tval) = (yyvsp[0].tval);
			}
#line 2466 "ada-exp.c.tmp"
    break;

  case 81: /* opt_type_prefix: type_prefix  */
#line 848 "ada-exp.y"
                        { (yyval.tval) = (yyvsp[0].tval); }
#line 2472 "ada-exp.c.tmp"
    break;

  case 82: /* opt_type_prefix: %empty  */
#line 850 "ada-exp.y"
                        { (yyval.tval) = parse_type (pstate)->builtin_void; }
#line 2478 "ada-exp.c.tmp"
    break;

  case 83: /* primary: INT  */
#line 855 "ada-exp.y"
                        { write_int (pstate, (LONGEST) (yyvsp[0].typed_val).val, (yyvsp[0].typed_val).type); }
#line 2484 "ada-exp.c.tmp"
    break;

  case 84: /* primary: CHARLIT  */
#line 859 "ada-exp.y"
                        {
			  pstate->push_new<ada_char_operation> ((yyvsp[0].typed_val).type, (yyvsp[0].typed_val).val);
			}
#line 2492 "ada-exp.c.tmp"
    break;

  case 85: /* primary: FLOAT  */
#line 865 "ada-exp.y"
                        {
			  float_data data;
			  std::copy (std::begin ((yyvsp[0].typed_val_float).val), std::end ((yyvsp[0].typed_val_float).val),
				     std::begin (data));
			  pstate->push_new<float_const_operation>
			    ((yyvsp[0].typed_val_float).type, data);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2505 "ada-exp.c.tmp"
    break;

  case 86: /* primary: NULL_PTR  */
#line 876 "ada-exp.y"
                        {
			  struct type *null_ptr_type
			    = lookup_pointer_type (parse_type (pstate)->builtin_int0);
			  write_int (pstate, 0, null_ptr_type);
			}
#line 2515 "ada-exp.c.tmp"
    break;

  case 87: /* primary: STRING  */
#line 884 "ada-exp.y"
                        { 
			  pstate->push_new<ada_string_operation>
			    (copy_name ((yyvsp[0].sval)));
			}
#line 2524 "ada-exp.c.tmp"
    break;

  case 88: /* primary: TRUEKEYWORD  */
#line 891 "ada-exp.y"
                        { write_int (pstate, 1, type_boolean (pstate)); }
#line 2530 "ada-exp.c.tmp"
    break;

  case 89: /* primary: FALSEKEYWORD  */
#line 893 "ada-exp.y"
                        { write_int (pstate, 0, type_boolean (pstate)); }
#line 2536 "ada-exp.c.tmp"
    break;

  case 90: /* primary: NEW NAME  */
#line 897 "ada-exp.y"
                        { error (_("NEW not implemented.")); }
#line 2542 "ada-exp.c.tmp"
    break;

  case 91: /* var_or_type: NAME  */
#line 901 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[0].sval)); }
#line 2548 "ada-exp.c.tmp"
    break;

  case 92: /* var_or_type: block NAME  */
#line 903 "ada-exp.y"
                                { (yyval.tval) = write_var_or_type (pstate, (yyvsp[-1].bval), (yyvsp[0].sval)); }
#line 2554 "ada-exp.c.tmp"
    break;

  case 93: /* var_or_type: NAME TICK_ACCESS  */
#line 905 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, NULL, (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2566 "ada-exp.c.tmp"
    break;

  case 94: /* var_or_type: block NAME TICK_ACCESS  */
#line 913 "ada-exp.y"
                        { 
			  (yyval.tval) = write_var_or_type (pstate, (yyvsp[-2].bval), (yyvsp[-1].sval));
			  if ((yyval.tval) == NULL)
			    ada_addrof ();
			  else
			    (yyval.tval) = lookup_pointer_type ((yyval.tval));
			}
#line 2578 "ada-exp.c.tmp"
    break;

  case 95: /* block: NAME COLONCOLON  */
#line 924 "ada-exp.y"
                        { (yyval.bval) = block_lookup (NULL, (yyvsp[-1].sval).ptr); }
#line 2584 "ada-exp.c.tmp"
    break;

  case 96: /* block: block NAME COLONCOLON  */
#line 926 "ada-exp.y"
                        { (yyval.bval) = block_lookup ((yyvsp[-2].bval), (yyvsp[-1].sval).ptr); }
#line 2590 "ada-exp.c.tmp"
    break;

  case 97: /* aggregate: '(' aggregate_component_list ')'  */
#line 931 "ada-exp.y"
                        {
			  std::vector<ada_component_up> components
			    = pop_components ((yyvsp[-1].lval));

			  push_component<ada_aggregate_component>
			    (std::move (components));
			}
#line 2602 "ada-exp.c.tmp"
    break;

  case 98: /* aggregate_component_list: component_groups  */
#line 941 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval); }
#line 2608 "ada-exp.c.tmp"
    break;

  case 99: /* aggregate_component_list: positional_list exp  */
#line 943 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-1].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-1].lval) + 1;
			}
#line 2618 "ada-exp.c.tmp"
    break;

  case 100: /* aggregate_component_list: positional_list component_groups  */
#line 949 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[-1].lval) + (yyvsp[0].lval); }
#line 2624 "ada-exp.c.tmp"
    break;

  case 101: /* positional_list: exp ','  */
#line 954 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    (0, ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2634 "ada-exp.c.tmp"
    break;

  case 102: /* positional_list: positional_list exp ','  */
#line 960 "ada-exp.y"
                        {
			  push_component<ada_positional_component>
			    ((yyvsp[-2].lval), ada_pop ());
			  (yyval.lval) = (yyvsp[-2].lval) + 1; 
			}
#line 2644 "ada-exp.c.tmp"
    break;

  case 103: /* component_groups: others  */
#line 968 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2650 "ada-exp.c.tmp"
    break;

  case 104: /* component_groups: component_group  */
#line 969 "ada-exp.y"
                                         { (yyval.lval) = 1; }
#line 2656 "ada-exp.c.tmp"
    break;

  case 105: /* component_groups: component_group ',' component_groups  */
#line 971 "ada-exp.y"
                                         { (yyval.lval) = (yyvsp[0].lval) + 1; }
#line 2662 "ada-exp.c.tmp"
    break;

  case 106: /* others: OTHERS ARROW exp  */
#line 975 "ada-exp.y"
                        {
			  push_component<ada_others_component> (ada_pop ());
			}
#line 2670 "ada-exp.c.tmp"
    break;

  case 107: /* component_group: component_associations  */
#line 982 "ada-exp.y"
                        {
			  ada_choices_component *choices = choice_component ();
			  choices->set_associations (pop_associations ((yyvsp[0].lval)));
			}
#line 2679 "ada-exp.c.tmp"
    break;

  case 108: /* component_associations: NAME ARROW exp  */
#line 995 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = 1;
			}
#line 2689 "ada-exp.c.tmp"
    break;

  case 109: /* component_associations: simple_exp ARROW exp  */
#line 1001 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = 1;
			}
#line 2699 "ada-exp.c.tmp"
    break;

  case 110: /* component_associations: simple_exp DOTDOT simple_exp ARROW exp  */
#line 1007 "ada-exp.y"
                        {
			  push_component<ada_choices_component> (ada_pop ());
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = 1;
			}
#line 2712 "ada-exp.c.tmp"
    break;

  case 111: /* component_associations: NAME '|' component_associations  */
#line 1016 "ada-exp.y"
                        {
			  write_name_assoc (pstate, (yyvsp[-2].sval));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2721 "ada-exp.c.tmp"
    break;

  case 112: /* component_associations: simple_exp '|' component_associations  */
#line 1021 "ada-exp.y"
                        {
			  push_association<ada_name_association> (ada_pop ());
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2730 "ada-exp.c.tmp"
    break;

  case 113: /* component_associations: simple_exp DOTDOT simple_exp '|' component_associations  */
#line 1027 "ada-exp.y"
                        {
			  operation_up rhs = ada_pop ();
			  operation_up lhs = ada_pop ();
			  push_association<ada_discrete_range_association>
			    (std::move (lhs), std::move (rhs));
			  (yyval.lval) = (yyvsp[0].lval) + 1;
			}
#line 2742 "ada-exp.c.tmp"
    break;

  case 114: /* primary: '*' primary  */
#line 1040 "ada-exp.y"
                        { ada_wrap<ada_unop_ind_operation> (); }
#line 2748 "ada-exp.c.tmp"
    break;

  case 115: /* primary: '&' primary  */
#line 1042 "ada-exp.y"
                        { ada_addrof (); }
#line 2754 "ada-exp.c.tmp"
    break;

  case 116: /* primary: primary '[' exp ']'  */
#line 1044 "ada-exp.y"
                        {
			  ada_wrap2<subscript_operation> (BINOP_SUBSCRIPT);
			  ada_wrap<ada_wrapped_operation> ();
			}
#line 2763 "ada-exp.c.tmp"
    break;


#line 2767 "ada-exp.c.tmp"

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
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

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
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      yyerror (YY_("syntax error"));
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

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
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
                  YY_ACCESSING_SYMBOL (yystate), yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

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


#if !defined yyoverflow
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturn;
#endif


/*-------------------------------------------------------.
| yyreturn -- parsing is finished, clean up and return.  |
`-------------------------------------------------------*/
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
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif

  return yyresult;
}

#line 1050 "ada-exp.y"


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
   designated by RENAMED_ENTITY[0 .. RENAMED_ENTITY_LEN-1] in the
   context of ORIG_LEFT_CONTEXT, to which is applied the operations
   encoded by RENAMING_EXPR.  MAX_DEPTH is the maximum number of
   cascaded renamings to allow.  If ORIG_LEFT_CONTEXT is null, it
   defaults to the currently selected block. ORIG_SYMBOL is the 
   symbol that originally encoded the renaming.  It is needed only
   because its prefix also qualifies any index variables used to index
   or slice an array.  It should not be necessary once we go to the
   new encoding entirely (FIXME pnh 7/20/2007).  */

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
  else if (sym_info.symbol->aclass () == LOC_TYPEDEF)
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
	    else if (index_sym_info.symbol->aclass () == LOC_TYPEDEF)
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
      && (syms.empty () || syms[0].symbol->aclass () != LOC_BLOCK))
    symtab = lookup_symtab (name);
  else
    symtab = NULL;

  if (symtab != NULL)
    result = BLOCKVECTOR_BLOCK (symtab->blockvector (), STATIC_BLOCK);
  else if (syms.empty () || syms[0].symbol->aclass () != LOC_BLOCK)
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
    switch (syms[i].symbol->aclass ())
      {
      case LOC_TYPEDEF:
	if (ada_prefer_type (syms[i].symbol->type (), preferred_type))
	  {
	    preferred_index = i;
	    preferred_type = syms[i].symbol->type ();
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
      if (sym != NULL && sym->aclass () == LOC_TYPEDEF)
	type = sym->type ();
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

  sym->set_domain (UNDEF_DOMAIN);
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
  struct type *type = sym->type ();
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
	  /* In order to avoid double-encoding, we want to only pass
	     the decoded form to lookup functions.  */
	  std::string decoded_name = ada_decode (encoded_name);
	  encoded_name[tail_index] = terminator;

	  std::vector<struct block_symbol> syms
	    = ada_lookup_symbol_list (decoded_name.c_str (), block, VAR_DOMAIN);

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
		return type_sym->type ();

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
		= ada_lookup_simple_minsym (decoded_name.c_str ());
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

      if (syms.size () != 1 || syms[0].symbol->aclass () == LOC_TYPEDEF)
	pstate->push_new<ada_string_operation> (copy_name (name));
      else
	write_var_from_sym (par_state, syms[0]);
    }
  else
    if (write_var_or_type (par_state, NULL, name) != NULL)
      error (_("Invalid use of type."));

  push_association<ada_name_association> (ada_pop ());
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
type_for_char (struct parser_state *par_state, ULONGEST value)
{
  if (value <= 0xff)
    return language_string_char_type (par_state->language (),
				      par_state->gdbarch ());
  else if (value <= 0xffff)
    return language_lookup_primitive_type (par_state->language (),
					   par_state->gdbarch (),
					   "wide_character");
  return language_lookup_primitive_type (par_state->language (),
					 par_state->gdbarch (),
					 "wide_wide_character");
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
