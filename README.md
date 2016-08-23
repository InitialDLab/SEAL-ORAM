# README
This is a unified testbed for evaluating different Oblivious RAM schemes. Specifically, we have implemented the following ORAM schemes:

- [Basic Square Root](http://dl.acm.org/citation.cfm?id=28416)
- [IBS OS](http://www.cs.utah.edu/~dongx/paper/psp-icde.pdf)
- [Towards Practical Oblivious RAM](http://arxiv.org/pdf/1106.3652.pdf)
- [Binary Tree ORAM](https://eprint.iacr.org/2011/407.pdf)
- [Path ORAM](https://people.csail.mit.edu/devadas/pubs/PathORam.pdf)

**Note:** We also implemented recursive construction for TPORAM, Path ORAM and Binary Tree ORAM.

*More schemes is now being integrating into this testbed....*

## Dependencies
To compile this project, the following libraries and enviroments are required:

- g++ 4.8+
- cryptopp 5.6.3+
- boost 1.56+
- mongo cxx legacy driver 1.10+

**Note:** Please make sure that all libraries are accessible directly by the complier, which typically means they are installed under `/usr` or `/usr/local`.

## Build
Do `cmake .` first, then `make` and enjoy!

## Contacts
Dong Xie: dongx [at] cs [dot] utah [dot] edu  
Zhao Chang: zchang [at] cs [dot] utah [dot] edu
Feifei Li: lifeifei [at] cs [dot] utah [dot] edu
