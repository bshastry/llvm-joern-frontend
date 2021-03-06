### Tooling to export Clang internals to Joern/Neo4J

This repository contains software that exports Clang internals (e.g., AST) into a [neo4J][3] database. Ultimately, a tool that generates [Code property graphs][4] using Clang is desired.

### Pre-requisites

- Debian/Ubuntu OS
- LLVM & Clang 3.6 dev libraries. Check out llvm [apt][5] page for info on how to install them

### Usage

- First read [libtooling][2] doc on how to set up libtooling for code under analysis
- Then, do:

```bash
$ cd $WORKING_DIR/clang-joern-frontend
$ mkdir build
$ cd build
$ cmake -DCJ_LLVM_BUILD_ROOT_PATH=<PATH_TO_LLVM-3.6> -DCJ_LLVM_ROOT_PATH=<PATH_TO_LLVM-3.6> ../src/ &> /dev/null
## Clang-joern binary and csv-exporter library will be installed in /usr
$ make && sudo make install
$ cd $ANALYSIS_TARGET
$ clang-joern -help | less
$ clang-joern -p <PATH_TO_PROJECT_BUILD> -ast-export <PATH_TO_SOME_SOURCE_FILE>
```

Clangpy bindings can be used as follows:

```bash
$ cd $WORKING_DIR
$ ./clang-print-ast.py tests/*.c
```

Since libclang bindings are inferior to libtooling in terms of richness of AST, development of bindings-based tool is discontinued.

### Acknowledgements

- clang-ast-print.py uses the `asciitree` print proposed in [Asciitree + clangpy][1]
- clang-joern is based off of [libtooling][2] infrastructure part of the LLVM infrastructure project
- `Code property graphs` has been proposed by Yamaguchi et. al., in their [paper][4] titled `Modeling and Discovering Vulnerabilities with Code Property Graphs` as a means to model and discover vulnerabilities in open source software

[1]: http://blog.glehmann.net/2014/12/29/Playing-with-libclang/
[2]: http://clang.llvm.org/docs/LibTooling.html
[3]: http://neo4j.com/
[4]: https://user.informatik.uni-goettingen.de/~krieck/docs/2014-ieeesp.pdf
[5]: http://llvm.org/apt/
