#!/bin/bash
# runs the engine measurement copy for every chain in interleaved, shuffled rounds
# usage: measure.sh <postprocessing_measure-binary> <rounds> <output.csv>
set -e

if [ $# -ne 3 ]; then
	echo "usage: measure.sh <postprocessing_measure-binary> <rounds> <output.csv>" >&2
	exit 1
fi
MEASURE=$1
ROUNDS=$2
OUT=$3

# warm-up and timed frames per run, to be fixed in the pilot
WARMUP=100
FRAMES=500
CHAINS="direct empty full"
ERR=$(mktemp)

echo "round,chain,warmup,frames,mean_ms" > "$OUT"

for round in $(seq 1 "$ROUNDS"); do
	echo "round $round of $ROUNDS" >&2

	# every chain once per round, in a new random order
	for chain in $CHAINS; do
		echo "$chain"
	done | sort -R | while read -r chain; do
		# a failed run or a skipped frame discards the run, the session goes on
		if line=$(VVPP_CHAIN=$chain "$MEASURE" "$WARMUP" "$FRAMES" 2>"$ERR") && ! grep -q "frame skipped" "$ERR"; then
			echo "$round,$line" >> "$OUT"
		else
			echo "round $round: $chain discarded" >&2
			cat "$ERR" >&2
		fi
	done
done

rm "$ERR"
