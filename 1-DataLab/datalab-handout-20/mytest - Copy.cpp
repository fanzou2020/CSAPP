#include <iostream>
using namespace std;

int main() {
    unsigned uf = 0x800000;
    int exp = (uf >> 23) & 0xFF;
    cout << exp << endl;
    int frac = uf & 0x7FFFFF;
    int sign = uf & (1 << 31);
    cout << frac << "\n" << sign << endl;
}