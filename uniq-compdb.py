#!/usr/bin/env python

import json
import argparse
import sys


def main():
    parser = argparse.ArgumentParser(description='Dedup compdb entries for AST export')
    parser.add_argument('infile', nargs='+', type=argparse.FileType('r'), help="Compile database json file")
    parser.add_argument('outfilename', nargs='+', help="Output json file name")
    args = parser.parse_args()
    infile = args.infile[0].read()
    outfilename = args.outfilename[0]
    args.infile[0].close()

    ds = json.loads(infile)
    all_ids = [each['file'] for each in ds]
    unique_stuff = [ds[all_ids.index(id)] for id in set(all_ids)]

    with open(outfilename, 'w') as outfile:
        json.dump(unique_stuff, outfile, indent=4)
        outfile.close()


if __name__ == "__main__":
    main()
