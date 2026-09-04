#!/bin/bash

set -e

OUTPUT="Assignment-1.tar.gz"

git archive --format=tar.gz --output="$OUTPUT" HEAD .

echo "Created $OUTPUT"
tar -tzf "$OUTPUT"