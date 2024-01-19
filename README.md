- Austin Van Braeckel
- 2021-02-26
- Operating Systems course Assignment
- Submitted to: Professor Deborah Stacey

# CPU Scheduling Simulator

## Usage
1. Type *make* in the terminal to create the executable, "*simcpu*"
2. Then, type "*./simcpu < input_file*" to run the program with the given input file, of which the file name will take the place of "*input_file*"
3. It can also be run using the following flags, before the "*< input_file*" part of the line:
    - "**-d**" flag: **detailed** mode, giving summary information for each thread
    - "**-v**" flag: **verbose** mode, giving information for the state changes during the simulation, as well as the summary information after a Thread terminates (detailed mode)
    - "**-r *quantum***" flag, where *quantum* is a <u>positive</U> integer: **Round Robin** mode, making the simulation use Round Robin scheduling with the given quantum, rather than the default First-Come-First-Served Scheduling 
- **Example**: "*./simcpu -v -r 50 < test_file_1.txt*"
    - will run a simulation with Round Robin scheduling (with a quantum of 50 units), with verbose mode enabled, using the data from the file called "test_file_1.txt"

## Assumptions
- The given input file is formatted correctly, being the same format as the example input file in the assignment description and/or given to the class
- For the priority queue, the ordering is specified by the arrival time, but if the times are the same between two elements, it is then ordered by the process number
- The flags are only accepted as separate arguments, and are sensitive to capitals (eg. not "-dv" but only "-d -v")

## Functionality
- All functionality specified in the assignment description is present in the program
<br></br>
#### SOURCES:
- Code for Heap inspired by the implementation at https://gist.github.com/sudhanshuptl/d86da25da46aa3d060e7be876bbdb343
- CIS*3110 class material and examples from CourseLink
<br></br><br></br>
*All code was written by me, and all sources used are listed above*
