// file: cpusim.cpp

#include "CPU.h"

#include <iostream>
#include <bitset>
#include <stdio.h>
#include<stdlib.h>
#include <string>
#include<fstream>
#include <sstream>
using namespace std;


int main(int argc, char* argv[])
{
	bool debug = false;
	bool enable_logging = false;
	string log_filename = "";

	char instMem[4096];

	if (argc < 2) {
		cout << "Usage: " << argv[0] << " <instruction_file> [--debug] [--log <logfile>]" << endl;
		cout << "No file name entered. Exiting...";
		return -1;
	}

	// Parse command line arguments
	for (int i = 2; i < argc; i++) {
		string arg = argv[i];
		if (arg == "--debug") {
			debug = true;
		} else if (arg == "--log" && i + 1 < argc) {
			enable_logging = true;
			log_filename = argv[i + 1];
			i++; // Skip next argument
		}
	}

	ifstream infile(argv[1]); //open the file
	if (!(infile.is_open() && infile.good())) {
		cout<<"error opening file\n";
		return 0; 
	}
	string line; 
	int i = 0;
	while (infile) {
			infile>>line;
			stringstream line2(line);
			char x; 
			line2>>x;
			instMem[i] = x; // be careful about hex
			i++;
			line2>>x;
			instMem[i] = x; // be careful about hex
			i++;
		}
	int maxPC= i/2; 
	

	CPU myCPU;
	
	// Set the maximum PC value
	myCPU.set_max_pc(maxPC);
	
	// Set up logging if enabled
	if (enable_logging) {
		myCPU.set_logging(true, log_filename);
	}
	
	if (debug) {
		cout << "Starting pipeline simulation..." << endl;
		cout << "Max PC: " << maxPC << endl;
		cout << "Instruction memory size: " << i << " bytes" << endl;
	}

	// Pipeline simulation loop
	int cycle = 0;
	const int MAX_CYCLES = 1000; // Prevent infinite loops
	
	while (cycle < MAX_CYCLES) {
		cycle++;
		
		// Run one pipeline cycle
		myCPU.run_pipeline_cycle(instMem, cycle, debug);
		
		// Check if pipeline is empty and we've reached the end
		if (myCPU.is_pipeline_empty() && myCPU.readPC() >= maxPC - 4) {
			if (debug) {
				cout << "Pipeline empty and end of program reached at cycle " << cycle << endl;
			}
			break;
		}
		
		// Debug: Print pipeline state every 100 cycles (only in debug mode)
		if (debug && cycle % 100 == 0) {
			cout << "Cycle " << cycle << ": PC=" << myCPU.readPC() << ", maxPC=" << maxPC 
				 << ", pipeline_empty=" << (myCPU.is_pipeline_empty() ? "true" : "false") << endl;
		}
		
		// Optional: Add a small delay for visualization
		if (debug) {
			// Uncomment the following line to add a delay between cycles
			// std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
	
	if (cycle >= MAX_CYCLES) {
		cout << "Warning: Maximum cycles reached. Simulation stopped." << endl;
	}
	
	int a0 = myCPU.get_register_value(10);	// a0
	int a1 = myCPU.get_register_value(11);  //a1
	
	// print the results 
	cout << "\n=== Final Results ===" << endl;
	cout << "Total cycles: " << cycle << endl;
	myCPU.print_all_registers();
	// std::cout << "( " << a0 << ", " << a1 << ")" << std::endl;

	return 0;

}