- Austin Van Braeckel - 1085829 - avanbrae@uoguelph.ca
- 2021-02-26
- CIS*3110 - Operating Systems - Assignment 2
- Submitted to: Professor Deborah Stacey

# CPU Scheduling Simulator

## Usage
1. Type *make* in the terminal to create the executable, "*simcpu*"
2. Then, type "*./simcpu < input_file*" to run the program with the given input file, of which the file name will take the place of "*input_file*"
3. It can also be run using the following flags, before the "*< input_file*" part of the line:
    - "**-d**" flag: **detailed** mode, giving summary information for each thread
    - "**-v**" flag: **verbose** mode, giving information for the state changes during the simulation, as well as the summary information after a Thread terminates
    - "**-r *quantum***" flag, where *quantum* is a positive integer: **Round Robin** mode, making the simulation use Round Robin scheduling with the given quantum, rather than the default First-Come-First-Served Scheduling 
- **Example**: "*./simcpu -d -v -r 50 < test_file_1.txt*"
    - will run a simulation with Round Robin scheduling (with a quantum of 50 units), with both detailed mode and verbose mode enabled, using the data from the file called "test_file_1.txt"

## Assumptions
- The given input file is formatted correctly, being the same format as the example input file in the assignment description and/or given to the class
- For the priority queue, the ordering is specified by the arrival time, but if the times are the same between two elements, it is then ordered by the process number
- The flags are only accepted as separate arguments, and are sensitive to capitals (eg. not "-dv" but only "-d -v")