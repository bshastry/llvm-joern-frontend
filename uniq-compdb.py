#!/usr/bin/env python

import json
import argparse
import sys
import os


def main():
    ## Argparse boilerplat
    parser = argparse.ArgumentParser(description='Dedup compdb entries for AST export. Renames original json file as compile_commands_orig.json')
    parser.add_argument('infilename', nargs='?', help="Path to compile database json file", default='compile_commands.json')
    parser.add_argument('outfilename', nargs='?', help="Output json file name", default='compile_commands.json')
    args = parser.parse_args()

    infilename = args.infilename
    outfilename = args.outfilename

    ## Read compile_commands.json to infile
    if not os.path.isfile(infilename):
        print 'compile_commands.json does not exist in working dir. Need help? Use the -h flag'
        exit()

    with open(infilename) as infile:
        buffer = infile.read()
        ds = json.loads(buffer)
        infile.close()

    ## Rename 'compile_commands.json' to 'compile_commands_orig.json'
    os.rename(infilename, 'compile_commands_orig.json')

    all_ids = [each['file'] for each in ds]
    unique_stuff = [ds[all_ids.index(id)] for id in set(all_ids)]

    with open(outfilename, 'w') as outfile:
        json.dump(unique_stuff, outfile, indent=4)
        outfile.close()


if __name__ == "__main__":
    main()
