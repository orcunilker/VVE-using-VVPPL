#!/bin/bash
# runs the engine measurement copy for every chain in interleaved, shuffled rounds
# usage: measure.sh <postprocessing_measure-binary> <rounds> <output.csv>
set -euo pipefail

if [ $# -ne 3 ]; then
	echo "usage: measure.sh <postprocessing_measure-binary> <rounds> <output.csv>" >&2
	exit 1
fi
MEASURE=$1
ROUNDS=$2
OUT=$3
if ! [[ "$ROUNDS" =~ ^[1-9][0-9]*$ ]] || [ ! -x "$MEASURE" ]; then
	echo "expected an executable measurement program and a positive round count" >&2
	exit 1
fi
if [ -e "$OUT" ]; then
	echo "refusing to overwrite $OUT" >&2
	exit 1
fi

# Provisional values; complete the pilot before fixing the main protocol.
WARMUP=500
FRAMES=1500
CHAINS="direct empty full"
ERR=$(mktemp)
trap 'rm -f "$ERR"' EXIT

echo "round,chain,warmup,frames,mean_ms" > "$OUT"

for round in $(seq 1 "$ROUNDS"); do
	echo "round $round of $ROUNDS" >&2

	# every chain once per round, in a new random order
	for chain in $CHAINS; do
		echo "$chain"
	done | sort -R | while read -r chain; do
		# Stop on failure; an incomplete round must not silently enter the analysis.
		if line=$(VVPP_CHAIN=$chain "$MEASURE" "$WARMUP" "$FRAMES" 2>"$ERR") && ! grep -q "frame skipped" "$ERR"; then
			cat "$ERR" >&2
			echo "$round,$line" >> "$OUT"
		else
			echo "round $round: $chain failed; stopping incomplete series" >&2
			cat "$ERR" >&2
			exit 1
		fi
	done
done
