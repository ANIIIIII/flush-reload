Some test of cache attack and simple implementation of the flush+reload attack

## Environment

macOS Big Sur

parallels Ubuntu 20.04

## File Description

` clflush_test.c ` 
 
 It test the functionality of `clflush` instruction in x86 architecture. 
 It also measure the miss penalty to get the threashold to implement flush+reload attack 
 
` leak_data_test.c `
 
 A simple example where it leak the private data accessed.
 
` leak_instruciton_test.c`
 
 A simple example where it leak the instructions in text segments executed by victim process.
 

## Usage
 
 ` gcc file.c -o file `
