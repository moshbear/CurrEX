CurrEX
======
- Requires c++11 compiler, boost {asio,graph,lexical\_cast,system,tokenizer} and json-c

Compiling:

	make

Running:

	./run-instr-ls | ./run-pruner | ./run-rates | ./run-graph

Output contains space-delimited fields:
	
	PATH LRATE

PATH is a semicolon-delimited field containing the precise walk used:

	X01;...;X01

LRATE is a log-rate of cost.
	
Feed output line to run-eval, followed by any number of lines of one or more values:

	exp(log($1) - LRATE) - $1

is evaluated for each token, yielding revenue and profit numbers.



Currently only works when the service is available from the [Oanda REST sandbox](http://api-sandbox.oanda.com/v1/{instruments,prices})
(it is known to have gone offline once in a while, beware). 

Plans include heuristic stateful initial simplex and expansion explorations.
Experiments show a 95% chance that the final graph is Hamiltonian; a correct longest-path search would require branch-and-cut and other
bounded-subset-based TSP-like algorithms for solution.

Threading is considered a non-issue as pruning and graphing take <10 ms each - under network RTT.


Concept credit: Francesco Gramano

