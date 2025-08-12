// file: cpusim.cpp

#include "CPU.h"
#include "MemoryIf.h"   // NEW
#include "Cache.h"      // NEW

#include <iostream>
#include <bitset>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
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
        instMem[i] = x; // hex char
        i++;
        line2>>x;
        instMem[i] = x; // hex char
        i++;
    }
    int maxPC= i/2; 

    // ==== NEW: Build memory hierarchy for DATA ====
    // Backing store: 64KB RAM (adjust as you like)
    SimpleRAM dram(64 * 1024);

    // Put any initial data here if desired, e.g. dram.poke_bytes(addr, buf, len);

    // 4KB D-cache with 32B lines
    DirectMappedCache dcache(&dram, /*totalSizeBytes=*/4*1024, /*lineSizeBytes=*/32);

    CPU myCPU;
    myCPU.set_data_memory(&dcache);   // << hook cache into CPU

    // Set the maximum PC value
    myCPU.set_max_pc(maxPC);

    // Set up logging if enabled
    if (enable_logging) {
        myCPU.set_logging(true, log_filename);
    }

    if (debug) {
        cout << std::dec;
        cout << "Starting pipeline simulation..." << endl;
        cout << "Max PC: " << maxPC << endl;
        cout << "Instruction memory size: " << i << " bytes" << endl;
    }

    // Pipeline simulation loop
    int cycle = 0;
    const int MAX_CYCLES = 1000; // Prevent infinite loops

    while (cycle < MAX_CYCLES) {
        cycle++;
        myCPU.run_pipeline_cycle(instMem, cycle, debug);

        if (myCPU.is_pipeline_empty() && myCPU.readPC() >= maxPC - 4) {
            if (debug) {
                cout << "Pipeline empty and end of program reached at cycle " << cycle << endl;
            }
            break;
        }

        if (debug && cycle % 100 == 0) {
            cout << "Cycle " << cycle << ": PC=" << myCPU.readPC() << ", maxPC=" << maxPC 
                 << ", pipeline_empty=" << (myCPU.is_pipeline_empty() ? "true" : "false") << endl;
        }
    }

    if (cycle >= MAX_CYCLES) {
        cout << "Warning: Maximum cycles reached. Simulation stopped." << endl;
    }

    int a0 = myCPU.get_register_value(10); // a0
    int a1 = myCPU.get_register_value(11); // a1

    cout << std::dec;
    cout << "\n=== Final Results ===" << endl;
    cout << "Total cycles: " << cycle << endl;
    myCPU.print_all_registers();

    return 0;
}
