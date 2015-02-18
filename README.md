CurrEX
======
- Requires c++11 compiler, boost {graph,lexical\_cast}, perl, and swig.

Compiling:

	make

Running:

	perl Currex.pl


Currently only works when the service is available from the [Oanda REST sandbox](http://api-sandbox.oanda.com/v1/{instruments,prices})
(it is known to have gone offline once in a while, beware). 

Plans include heuristic stateful initial simplex and expansion explorations.
Experiments show a 95% chance that the final graph is Hamiltonian; a correct longest-path search would require branch-and-cut and other
bounded-subset-based TSP-like algorithms for solution.

Threading is considered a non-issue as pruning and graphing take <10 ms each - under network RTT.


Concept credit: Francesco Gramano

