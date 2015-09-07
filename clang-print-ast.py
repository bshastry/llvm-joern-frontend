#!/usr/bin/env python

import sys
import os
from neo4jrestclient.client import GraphDatabase
from py2neo import Graph, Path
import argparse
import traceback
import asciitree
import json

# Ugly hack for clang binding version mismatch on host
if os.getenv('CLANG_PY'):
    sys.path.insert(0, os.environ['CLANG_PY'])
import clang.cindex

global comdblist


def printchildren(cursor):
    for i in cursor.get_children():
        print i.spelling + "\t" + str(i.kind).split(".")[1]
        if i.get_children():
            printchildren(i)


def initgraphdb():
    gdb = GraphDatabase("http://localhost:7474/db/data/")


def visitTU(cursor):
    # DF visit TU serializing AST into neo4j representation
    print cursor


def init():
    if os.getenv('LLVM_LIB') is None or not os.getenv('LLVM_LIB'):
        print "Please set LLVM_LIB to path where libclang.so can be found"
        return

    clang.cindex.Config.set_library_path(os.environ['LLVM_LIB'])


'''
    FIXME: file might encode a relative path while compdb contains full path
    Doing a clean match needs thought.
    Also, a file might have multiple compile commands associated with it.
    We need to assess if we need to parse an AST each time.
'''


def findCmdForFile(file):
    for i in range(len(comdblist)):
        if comdblist[i]["file"].contains(file):
            return comdblist[i]["command"][0]


# FIXME: Stub
def argumentAdjuster(arg):
    adjarg = arg.strip()
    return adjarg


def main():
    global comdblist
    try:
        # Parse args
        parser = argparse.ArgumentParser(description='pyclang argparser')
        parser.add_argument('infiles', nargs='+', help="Source file(s)")
        parser.add_argument('outfile', nargs='?', type=argparse.FileType('w'), default=sys.stdout)
        parser.add_argument('compdb', nargs='?', type=argparse.FileType('r'), help="Path to compile_commands.json file")

        toolargs = parser.parse_args()
        outfile = toolargs.outfile
        sys.stdout = outfile
        if toolargs.compdb:
            with open(toolargs.compdb) as data_file:
                comdblist = json.load(data_file)

        init()
        index = clang.cindex.Index(clang.cindex.conf.lib.clang_createIndex(False, True))

        for infile in toolargs.infiles:
            args = ['-x']
            if infile.endswith(".c"):
                args.append('c')
            elif infile.endswith(".cpp") or infile.endswith(".cc"):
                args.append('c++')

            if toolargs.compdb:
                args.append(argumentAdjuster(findCmdForFile(infile)))
            try:
                # index.parse does not pass through errors from libclang. Errors are
                # emitted to stderr
                translation_unit = index.parse(infile, args)
            except Exception, TranslationUnitLoadError:
                print traceback.format_exc()
                return
            print asciitree.draw_tree(translation_unit.cursor, lambda n: list(n.get_children()),
                                      lambda n: "%s (%s)" % (n.spelling or n.displayname, str(n.kind).split(".")[1]))

    except Exception, err:
        print traceback.format_exc()
        return


if __name__ == "__main__":
    main()
