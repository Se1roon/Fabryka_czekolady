#!/bin/bash


echo -n "A: " && cat simulation.log | grep -E ".*Delivered.*A.*" | wc -l
echo -n "B: " && cat simulation.log | grep -E ".*Delivered.*B.*" | wc -l
echo -n "C: " && cat simulation.log | grep -E ".*Delivered.*C.*" | wc -l
echo -n "D: " && cat simulation.log | grep -E ".*Delivered.*D.*" | wc -l
