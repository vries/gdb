#!/usr/bin/env python3

# Copyright (C) 2017-2025 Free Software Foundation, Inc.
#
# Checks some of the GNU style formatting rules in a set of patches.
# The script is a rewritten of the same bash script and should eventually
# replace the former script.
#
# This file is part of GCC.
#
# GCC is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3, or (at your option) any later
# version.
#
# GCC is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.
#
# The script requires python packages, which can be installed via pip3
# like this:
# $ pip3 install unidiff termcolor

import sys
import re
import unittest

def import_pip3(*args):
    missing=[]
    for (module, names) in args:
        try:
            lib = __import__(module)
        except ImportError:
            missing.append(module)
            continue
        if not isinstance(names, list):
            names=[names]
        for name in names:
            globals()[name]=getattr(lib, name)
    if len(missing) > 0:
        missing_and_sep = ' and '.join(missing)
        missing_space_sep = ' '.join(missing)
        print('%s %s missing (run: pip3 install %s)'
              % (missing_and_sep,
                 ("module is" if len(missing) == 1 else "modules are"),
                 missing_space_sep))
        exit(3)

import_pip3(('termcolor', 'colored'),
            ('unidiff', 'PatchSet'))

from itertools import *

ws_char = '█'
ts = 8

def error_string(s):
    return colored(s, 'red', attrs = ['bold'])

class CheckError:
    def __init__(self, filename, lineno, console_error, error_message,
        column = -1):
        self.filename = filename
        self.lineno = lineno
        self.console_error = console_error
        self.error_message = error_message
        self.column = column

    def error_location(self):
        return '%s:%d:%d:' % (self.filename, self.lineno,
            self.column if self.column != -1 else -1)

class LineLengthCheck:
    def __init__(self):
        self.limit = 80

    def check(self, filename, lineno, line):
        line_expanded = line.expandtabs(ts)
        if len(line_expanded) > self.limit:
            return CheckError(filename, lineno,
                line_expanded[:self.limit]
                    + error_string(line_expanded[self.limit:]),
                'lines should not exceed 80 characters', self.limit)

        return None

class SpacesCheck:
    def __init__(self):
        self.expanded_tab = ' ' * ts
        self.re = re.compile(r'^\t*(' + self.expanded_tab + ')')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if prefix != "":
            return
        m = self.re.search(line)
        if m is None:
            return
        start = m.start(1)
        end = m.end(1)
        error_line = (line[0:start] + error_string(ws_char * ts) + line[end:]
                      + postfix)
        return CheckError(filename, lineno, error_line,
                          'blocks of 8 spaces should be replaced with tabs',
                          start)

class SpacesAndTabsMixedCheck:
    def __init__(self):
        self.re = re.compile(r'^\t*( +)\t')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if prefix != "":
            return
        m = self.re.search(line)
        if m is None:
            return
        start = m.start(1)
        end = m.end(1)
        error_line = (line[0:start] + error_string(m.group(1) + ws_char * ts)
                      + line[end+1:] + postfix)
        return CheckError(filename, lineno, error_line,
                'a space should not precede a tab', 0)

class TrailingWhitespaceCheck:
    def __init__(self):
        self.re = re.compile(r'(\s+)$')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if postfix != "":
            return
        assert(len(line) == 0 or line[-1] != '\n')
        if line == '':
            return
        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(1)] + error_string(ws_char * len(m.group(1)))
                + line[m.end(1):],
                'trailing whitespace', len(prefix) + m.start(1))

class SentenceSeparatorCheck:
    def __init__(self):
        self.re = re.compile(r'(?:((?:\w[.])+)|(\w+[.]))' + r'(\s|\s{3,})'
                             + r'\w')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        m = self.re.search(line)
        if m == None:
            return
        abbrev = m.group(1)
        if abbrev is not None:
            return
        word = m.group(2)
        if word.lower() in ['ie.', 'eg.', 'etc.', 'dr.', 'aka.', 'vs.', 'Nov.']:
            return
        start = m.start(3)
        end = m.end(3)
        n_spaces = len(m.group(3))
        error_line = (prefix + line[:start] + error_string(ws_char * n_spaces)
                      + line[end:] + postfix)
        return CheckError(filename, lineno, error_line,
                          'dot, space, space, new sentence', len(prefix) + start)


class SentenceEndOfCommentCheck:
    def __init__(self):
        self.re = re.compile(r'\w\.(\s{0,1}|\s{3,})$')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if not postfix.startswith("*/"):
            return
        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(1)] + error_string(ws_char * len(m.group(1)))
                + line[m.end(1):] + postfix,
                'dot, space, space, end of comment', len(prefix) + m.start(1))

class SentenceDotEndCheck:
    def __init__(self):
        self.re = re.compile(r'\w \w+(\s*)$')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if not postfix.startswith("*/"):
            return
        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(1)] + error_string(m.group(1) + "*/")
                + postfix[2:],
                'dot, space, space, end of comment', m.start(1))

class FunctionParenthesisCheck:
    # TODO: filter out GTY stuff
    def __init__(self):
        self.re = re.compile(r'\w+(\s{2,})?(\()')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if '#define' in line:
            return None

        m = self.re.search(line)
        if m != None:
            if m.group() == '_(' or m.group() == 'operator(':
                return None
            return CheckError(filename, lineno,
                prefix + line[:m.start(2)] + error_string(m.group(2))
                + line[m.end(2):] + postfix,
                'there should be exactly one space between function name ' \
                'and parenthesis', len(prefix) + m.start(2))

class SquareBracketCheck:
    def __init__(self):
        self.re = re.compile(r'\w\s+(\[)')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        if filename.endswith('.md'):
            return None

        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(1)] + error_string(m.group(1))
                + line[m.end(1):] + postfix,
                'there should be no space before a left square bracket',
                len(prefix) + m.start(1))

class ClosingParenthesisCheck:
    def __init__(self):
        self.re = re.compile(r'\S\s+(\))')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(1)] + error_string(m.group(1))
                + line[m.end(1):] + postfix,
                'there should be no space before closing parenthesis',
                m.start(1))

class BracesOnSeparateLineCheck:
    # This will give false positives for C99 compound literals.

    def __init__(self):
        self.re = re.compile(r'(\)|else)\s*({)')

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(2)] + error_string(m.group(2))
                + line[m.end(2):] + postfix,
                'braces should be on a separate line', m.start(2))

class TrailingOperatorCheck:
    def __init__(self):
        regex = r'^\s.*(([^a-zA-Z_]\*)|([-%<=&|^?])|([^*]/)|([^:][+]))$'
        self.re = re.compile(regex)

    def check(self, filename, lineno, line):
        return self.check_(filename, lineno, "", line, "")

    def check_(self, filename, lineno, prefix, line, postfix):
        m = self.re.search(line)
        if m != None:
            return CheckError(filename, lineno,
                prefix + line[:m.start(1)] + error_string(m.group(1))
                + line[m.end(1):] + postfix,
                'trailing operator', m.start(1))

class UnitTest(unittest.TestCase):
    def setUp(self):
        self.check = None

    def check_match(self, line, column = -1, console_error = None):
        r = self.check.check('foo', 123, line)
        self.assertIsNotNone(r)
        self.assertEqual('foo', r.filename)
        self.assertEqual(123, r.lineno)
        if column != -1:
            self.assertEqual(column, r.column)
        self.assertIsNotNone(r.console_error)
        if console_error != None:
            self.assertEqual(console_error, r.console_error)
        return r

    def check_no_match(self, line):
        r = self.check.check('foo', 123, line)
        self.assertIsNone(r)

class LineLengthTest(UnitTest):
    def setUp(self):
        self.check = LineLengthCheck()

    def test_line_length_check_basic(self):
        limit_str = self.check.limit * 'a'
        self.check_no_match(limit_str)
        self.check_match(limit_str + 'a', self.check.limit)
        self.check_match(limit_str + ' = 123;', self.check.limit,
                         limit_str + error_string(' = 123;'))
        self.check_no_match('a' + '\t' + (self.check.limit - ts) * 'a')

class SpacesTest(UnitTest):
    def setUp(self):
        self.check = SpacesCheck()

    def test_spaces_check_basic(self):
        code_str = 'foo ()'
        self.check_match((ts * ' ') + code_str, 0,
                         error_string (ts * ws_char) + code_str)
        # Todo: Fix column, should be 8.
        self.check_match('\t' + (ts * ' ') + code_str, 1)
        self.check_no_match((ts * 'a') + (ts * ' ') + 'foo ()')

class TrailingWhitespaceTest(UnitTest):
    def setUp(self):
        self.check = TrailingWhitespaceCheck()

    def test_trailing_whitespace_check_basic(self):
        no_tws_str = 'a = 123;'
        self.check_no_match(no_tws_str)
        self.check_match(no_tws_str + ' ', len(no_tws_str),
                         no_tws_str + error_string(ws_char))
        self.check_match(no_tws_str + '\t', len(no_tws_str))
        self.check_no_match('')

class SpacesAndTabsMixedTest(UnitTest):
    def setUp(self):
        self.check = SpacesAndTabsMixedCheck()

    def test_spaces_and_tabs_mixed_check_basic(self):
        code_str = 'a = 123;'
        self.check_match('   \t' + code_str, 0,
                         error_string('   ' + ts * ws_char) + code_str)
        self.check_match('   \t  ' + code_str, 0)
        self.check_no_match('\t  ' + code_str)
        self.check_no_match('foo (); \tfoo ();')

class SentenceSeparatorTest(UnitTest):
    def setUp(self):
        self.check = SentenceSeparatorCheck()

    def test_sentence_separator_check_basic(self):
        self.check_no_match('foo.  Bar')
        self.check_match('foo. Bar', 4,
                         'foo.' + error_string(ws_char) + 'Bar')
        self.check_match('foo.   Bar', 4)
        self.check_no_match('i.e. one that is preceded by a prefix')
        self.check_no_match('ie. one that is preceded by a prefix')

class SentenceEndOfCommentTest(UnitTest):
    def setUp(self):
        self.check = SentenceEndOfCommentCheck()

    def test_sentence_end_of_comment_check_basic(self):
        self.check_match("foo.*/", 4)
        self.check_match("foo. */", 4)
        self.check_no_match("foo.  */")
        self.check_match("foo.   */", 4,
                         "foo." + error_string (3 * ws_char) + "*/")

class SentenceDotEndTest(UnitTest):
    def setUp(self):
        self.check = SentenceDotEndCheck()

    def test_sentence_dot_end_check_basic(self):
        self.check_no_match(' A sentence.  */')
        self.check_match(' A sentence */', 11)
        self.check_match(' A sentence  */', 11,
                         ' A sentence' + error_string ('  */'))
        self.check_no_match(' Word */')

class FunctionParenthesisTest(UnitTest):
    def setUp(self):
        self.check = FunctionParenthesisCheck()

    def test_function_parenthesis_check_basic(self):
        self.check_no_match('foo ()')
        self.check_no_match('#define foo(a)')
        self.check_no_match('_("foo")')
        self.check_no_match('operator()')
        self.check_match('foo()')

class SquareBracketTest(UnitTest):
    def setUp(self):
        self.check = SquareBracketCheck()

    def test_square_bracket_check_basic(self):
        self.check_no_match('a[i] = 1;')
        self.check_match('a [i] = 1;', 2,
                         'a ' + error_string('[') + 'i] = 1;')

class ClosingParenthesisTest(UnitTest):
    def setUp(self):
        self.check = ClosingParenthesisCheck()

    def test_closing_parenthesis_check_basic(self):
        self.check_no_match('foo (1, 2);')
        self.check_match('foo (1, 2 );', 10,
                         'foo (1, 2 ' + error_string (')') + ';')

class BracesOnSeparateLineTest(UnitTest):
    def setUp(self):
        self.check = BracesOnSeparateLineCheck()

    def test_braces_on_separate_line_check_basic(self):
        self.check_no_match('    {')
        self.check_no_match('    }')
        self.check_match('if (c) {', 7,
                         'if (c) ' + error_string('{'))
        self.check_match('else {', 5)

class TrailingOperatorTest(UnitTest):
    def setUp(self):
        self.check = TrailingOperatorCheck()

    def test_trailing_operator_check_basic(self):
        self.check_no_match('  a = 1')
        self.check_match('  a =', 4,
                         '  a ' + error_string('='))

# Checks for text.
line_checks = [LineLengthCheck()]

# Checks for both comments and code.
common_checks = [SpacesCheck(),
                 TrailingWhitespaceCheck(),
                 SpacesAndTabsMixedCheck()]

# Checks for comments.
comment_checks = common_checks + [SentenceSeparatorCheck(),
                                  SentenceEndOfCommentCheck(),
                                  SentenceDotEndCheck()]

# Checks for code.
code_checks = common_checks + [FunctionParenthesisCheck(),
                               SquareBracketCheck(),
                               ClosingParenthesisCheck(),
                               BracesOnSeparateLineCheck(),
                               TrailingOperatorCheck()]

# Regexp constants.
re_start = re.compile(r'[/][*/]|R["][(]|["]|[\']')
re_end_of_comment = re.compile(r"(([^*]|[*][^/])*)\*\/")
re_end_of_string = re.compile(r'([^"\\]*)(["\\])')
re_end_of_raw_string = re.compile(r'(.*)([)]["])')
re_end_of_char = re.compile(r"([^'\\]*)(['\\])")


def handle_comment(errors, filename, linenr, prefix, str, postfix):
    for check in comment_checks:
        e = check.check_(filename, linenr, prefix, str, postfix)
        if e is None:
            continue
        errors.append(e)


def handle_code(errors, filename, linenr, prefix, str, postfix):
    for check in code_checks:
        e = check.check_(filename, linenr, prefix, str, postfix)
        if e is None:
            continue
        errors.append(e)


def handle_line(errors, filename, linenr, line, do_checks):
    global in_comment
    global in_string
    global in_raw_string
    global in_char

    head = ""
    tail = line

    while True:

        if in_comment:
            m = re_end_of_comment.search(tail)
            if m is None:
                if do_checks:
                    handle_comment(errors, filename, linenr, head, tail, "")
                return

            # Found end of comment.
            in_comment = 0
            prefix = head + tail[0 : m.start(0)]
            str = m.group(0)[0:-2]
            token = "*/"
            postfix = tail[m.start(0) + len(m.group(0)) :]
            if do_checks:
                handle_comment(errors, filename, linenr, prefix, str, token + postfix)

            head = prefix + str + token
            tail = postfix
            continue

        if in_string:
            m = re_end_of_string.search(tail)
            if m is None:
                return

            if m.group(2) == '"':
                # Found end of string.
                in_string = False
                head = head + tail[0 : m.start(1)] + m.group(1) + '"'
                tail = tail[m.start(0) + len(m.group(0)) :]
            else:
                # Found backslash.
                head = head + tail[0 : m.start(0)] + m.group(0)
                tail = tail[m.start(0) + len(m.group(0)) :]
                if tail != "":
                    # Backslash not at end of line.
                    head = head + tail[0]
                    tail = tail[1:]

            continue

        if in_raw_string:
            m = re_end_of_raw_string.search(tail)
            if m is None:
                return

            # Found end of raw string.
            in_raw_string = False
            head = head + tail[0 : m.start(1)] + m.group(1) + ')"'
            tail = tail[m.start(0) + len(m.group(0)) :]
            continue

        if in_char:
            m = re_end_of_char.search(tail)
            if m is None:
                return

            if m.group(2) == "'":
                # Found end of char.
                in_char = False
                head = head + tail[0 : m.start(1)] + m.group(1) + "'"
                tail = tail[m.start(0) + len(m.group(0)) :]
            else:
                # Found backslash.
                head = head + tail[0 : m.start(1)] + m.group(1)
                tail = tail[m.start(0) + len(m.group(0)) :]
                if tail != "":
                    # Backslash not at end of line.
                    head = head + tail[0]
                    tail = tail[1:]
            continue

        m = re_start.search(tail)
        if m is None:
            if do_checks:
                handle_code(errors, filename, linenr, head, tail, "")
            return

        if do_checks:
            handle_code(errors, filename, linenr, head, tail[0:m.start(0)],
                        tail[m.start(0):])

        # Skip over match.
        after = m.start(0) + len(m.group(0))
        head = head + tail[0:after]
        tail = tail[after:]

        if m.group(0) == "/*":
            # Start of C comment.
            in_comment = True
        elif m.group(0) == "//":
            # Start of C++ comment.
            if do_checks:
                handle_comment(errors, filename, linenr, head, tail, "")
            return
        elif m.group(0) == '"':
            # Start of string literal.
            in_string = True
        elif m.group(0) == "'":
            # Start of char literal.
            in_char = True
        elif m.group(0) == 'R"(':
            # Start of raw string literal.
            in_raw_string = True
        else:
            assert False, "unreachable"

    return head + tail


def handle_patched_file(errors, filename, pfile):
    patch_lines = []
    for hunk in pfile:
        for line in hunk:
            if not line.is_added or line.target_line_no is None:
                continue
            patch_lines.append(line)

    for patch_line in patch_lines:
        patch_line_chomp = patch_line.value.replace('\n', '')
        for check in line_checks:
            e = check.check(filename, patch_line.target_line_no, patch_line_chomp)
            if e is None:
                continue
            errors.append(e)

    with open(filename, "r") as f:
        lines = f.read().split("\n")

    global in_comment
    global in_string
    global in_raw_string
    global in_char
    in_comment = False
    in_string = False
    in_raw_string = False
    in_char = False

    patch_line_index = 0
    linenr = 1
    for line in lines:
        if patch_line_index >= len(patch_lines):
            break

        patch_line = patch_lines[patch_line_index]
        do_checks = linenr == patch_line.target_line_no

        patch_line_str = patch_line.value.replace('\n', '')
        if do_checks and line != patch_line_str:
            print("patch does not match %s at line %s:" % (filename, linenr))
            print("PATCH: '%s'" % patch_line_str)
            print("FILE : '%s'" % line)
            exit(1)

        handle_line(errors, filename, linenr, line, do_checks)
        linenr += 1
        if do_checks:
            patch_line_index += 1


>>>>>>> bdda8a291ce ([contrib] Reduce false positives in comments and strings in check_GNU_style_lib.py)
def check_GNU_style_file(file, format):
    errors = []

    patch = PatchSet(file)

    for pfile in patch.added_files + patch.modified_files:
        t = pfile.target_file
        if t.startswith('b/'):
            t = t[2:]
        # Skip testsuite files
        if ('testsuite' in t or t.endswith('.py')
            or t.startswith('gdbsupport/unordered_dense')
            or t.startswith('gdb/features')
            or t == 'gdb/ada-casefold.h'
            or t == 'gdb/copying.c'
            or t == 'gdb/gdbarch-gen.h'
            or t == 'gdb/gdbarch-gen.c'
            or t == 'gdb/target-delegates-gen.c'):
            continue
        handle_patched_file(errors, t, pfile)

    if format == 'stdio':
        fn = lambda x: x.error_message
        i = 1
        for (k, errors) in groupby(sorted(errors, key = fn), fn):
            errors = list(errors)
            print('=== ERROR type #%d: %s (%d error(s)) ==='
                % (i, k, len(errors)))
            i += 1
            for e in errors:
                print(e.error_location () + e.console_error)
            print()

        exit(0 if len(errors) == 0 else 1)
    elif format == 'quickfix':
        f = 'errors.err'
        with open(f, 'w+') as qf:
            for e in errors:
                qf.write('%s%s\n' % (e.error_location(), e.error_message))
        if len(errors) == 0:
            exit(0)
        else:
            print('%d error(s) written to %s file.' % (len(errors), f))
            exit(1)
    else:
        assert False

if __name__ == '__main__':
    unittest.main()
