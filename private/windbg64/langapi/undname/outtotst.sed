# Script for converting a failure log from testundn into proper input.
# This is primarily useful for generating baselines.
s/FAILED: {/{ /
s/PASSED: {/{ /
s/"[^"]*"} gave \(.*\)/\1}/
s/}$/ },/
