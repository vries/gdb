#!/usr/bin/env python3

import argparse
import re

# Constants.
repl_comment = "C"
repl_string = "s"
repl_char = "c"

# Regexp constants.
re_start = re.compile(r'[/][*/]|R["][(]|["]|[\']')
re_end_of_comment = re.compile(r"(([^*]|[*][^/])*)\*\/")
re_end_of_string = re.compile(r'([^"\\]*)(["\\])')
re_end_of_raw_string = re.compile(r'(.*)([)]["])')
re_end_of_char = re.compile(r"([^'\\]*)(['\\])")


def handle_comment(str):
    return re.sub(r"[^ \t.]", repl_comment, str)


def handle_string(str):
    return re.sub(r".", repl_string, str)


def handle_char(str):
    return re.sub(r".", repl_char, str)


def handle_line(line):
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
                return head + handle_comment(tail)

            # Found end of comment.
            in_comment = 0
            head = head + tail[0 : m.start(0)] + handle_comment(m.group(0)[0:-2]) + "*/"
            tail = tail[m.start(0) + len(m.group(0)) :]
            continue

        if in_string:
            m = re_end_of_string.search(tail)
            if m is None:
                return head + handle_string(tail)

            if m.group(2) == '"':
                # Found end of string.
                in_string = False
                head = head + tail[0 : m.start(1)] + handle_string(m.group(1)) + '"'
                tail = tail[m.start(0) + len(m.group(0)) :]
            else:
                # Found backslash.
                head = head + tail[0 : m.start(0)] + handle_string(m.group(0))
                tail = tail[m.start(0) + len(m.group(0)) :]
                if tail != "":
                    # Backslash not at end of line.
                    head = head + handle_string(tail[0])
                    tail = tail[1:]

            continue

        if in_raw_string:
            m = re_end_of_raw_string.search(tail)
            if m is None:
                return head + handle_string(tail)

            # Found end of raw string.
            in_raw_string = False
            head = head + tail[0 : m.start(1)] + handle_comment(m.group(1)) + ')"'
            tail = tail[m.start(0) + len(m.group(0)) :]
            continue

        if in_char:
            m = re_end_of_char.search(tail)
            if m is None:
                return head + handle_char(tail)

            if m.group(2) == "'":
                # Found end of char.
                in_char = False
                head = head + tail[0 : m.start(1)] + handle_char(m.group(1)) + "'"
                tail = tail[m.start(0) + len(m.group(0)) :]
            else:
                # Found backslash.
                head = head + tail[0 : m.start(1)] + handle_char(m.group(1))
                tail = tail[m.start(0) + len(m.group(0)) :]
                if tail != "":
                    # Backslash not at end of line.
                    head = head + handle_char(tail[0])
                    tail = tail[1:]
            continue

        m = re_start.search(tail)
        if m is None:
            return head + tail

        # Skip over match.
        after = m.start(0) + len(m.group(0))
        head = head + tail[0:after]
        tail = tail[after:]

        if m.group(0) == "/*":
            # Start of C comment.
            in_comment = True
        elif m.group(0) == "//":
            # Start of C++ comment.
            return head + handle_comment(tail)
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


def handle_file(filename):
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

    for line in lines:
        print(handle_line(line))

    if in_comment:
        assert False, "unterminated comment"

    if in_string or in_raw_string:
        assert False, "unterminated string"

    if in_char:
        assert False, "unterminated char"


def main():
    parser = argparse.ArgumentParser(
        description="Filter comments, strings and chars from C/C++ files."
    )
    parser.add_argument("file", help="C/C++ file")
    args = parser.parse_args()
    handle_file(args.file)


main()
