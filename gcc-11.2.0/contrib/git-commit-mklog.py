#!/usr/bin/env python3

# Copyright (C) 2020 Free Software Foundation, Inc.
#
# This file is part of GCC.
#
# GCC is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GCC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING.  If not, write to
# the Free Software Foundation, 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
#
# The script is wrapper for git commit-mklog alias where it parses
# -b/--pr-numbers argument and passes it via environment variable
# to mklog.py script.

import argparse
import os
import subprocess

if __name__ == '__main__':
    children_args = []
    myenv = os.environ.copy()

    parser = argparse.ArgumentParser(description='git-commit-mklog wrapped')
    parser.add_argument('-b', '--pr-numbers', action='store',
                        type=lambda arg: arg.split(','), nargs='?',
                        help='Add the specified PRs (comma separated)')
    parser.add_argument('-p', '--fill-up-bug-titles', action='store_true',
                        help='Download title of mentioned PRs')
    args, unknown_args = parser.parse_known_args()

    myenv['GCC_FORCE_MKLOG'] = '1'
    mklog_args = []
    if args.pr_numbers:
        mklog_args.append(f'-b {",".join(args.pr_numbers)}')
    if args.fill_up_bug_titles:
        mklog_args.append('-p')

    if mklog_args:
        myenv['GCC_MKLOG_ARGS'] = ' '.join(mklog_args)

    commit_args = ' '.join(unknown_args)
    subprocess.run(f'git commit {commit_args}', shell=True, env=myenv)
