# doublechoco-solver

A solver for [Double Choco](https://www.nikoli.co.jp/en/puzzles/double_choco/) based on SAT solver.

## How to build

```
$ git clone https://github.com/semiexp/doublechoco-solver.git
$ cd doublechoco-solver
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release .. && make
```

## Usage

```
$ ./doublechoco-solver <puzz.link URL>
```
