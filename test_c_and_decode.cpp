// Quick test to verify 0x9c89 decodes correctly
#include <iostream>
#include <cstdint>
using namespace std;

int main() {
    uint16_t compressed_inst = 0x9c89;
    
    uint8_t op = compressed_inst & 0x3;
    uint8_t funct3 = (compressed_inst >> 13) & 0x7;
    uint8_t funct2 = (compressed_inst >> 10) & 0x3;
    
    cout << "Testing instruction: 0x" << hex << compressed_inst << dec << endl;
    cout << "op = " << (int)op << " (expected: 1)" << endl;
    cout << "funct3 = " << (int)funct3 << " (expected: 4)" << endl;
    cout << "funct2 = " << (int)funct2 << " (expected: 3)" << endl;
    
    if (op == 0x1 && funct3 == 0x4 && funct2 == 0x3) {
        uint8_t bit12 = (compressed_inst >> 12) & 0x1;
        uint8_t bit8 = (compressed_inst >> 8) & 0x1;
        uint8_t bit6 = (compressed_inst >> 6) & 0x1;
        uint8_t rd_prime = 8 + ((compressed_inst >> 7) & 0x7);
        uint8_t rs2_prime = 8 + ((compressed_inst >> 2) & 0x7);
        
        cout << "bit12 = " << (int)bit12 << " (expected: 1)" << endl;
        cout << "bit8 = " << (int)bit8 << " (expected: 0)" << endl;
        cout << "bit6 = " << (int)bit6 << " (expected: 0)" << endl;
        cout << "rd_prime = " << (int)rd_prime << " (expected: 9)" << endl;
        cout << "rs2_prime = " << (int)rs2_prime << " (expected: 10)" << endl;
        
        if (bit12 == 1 && bit6 == 0 && bit8 == 0) {
            cout << "✓ Matches C.AND case!" << endl;
            uint32_t expanded = (0x33) | (rd_prime << 7) | (0x7 << 12) | (rd_prime << 15) | (rs2_prime << 20);
            cout << "Expanded instruction: 0x" << hex << expanded << dec << endl;
            return 0;
        } else {
            cout << "✗ Does NOT match C.AND case" << endl;
            return 1;
        }
    } else {
        cout << "✗ Does NOT enter funct2==3 block" << endl;
        return 1;
    }
}
