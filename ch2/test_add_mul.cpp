#include <iostream>
using namespace std;

/* Determine whether arguments can be added without overflow */ 
int tadd_ok(int x, int y) {
    int sum = x + y;
    int pos_over = x >= 0 && y >= 0 && sum < 0;
    int neg_over = x < 0 && y < 0 && sum >= 0;
    return !neg_over && !pos_over;
}

// Determine whether the arguments can be multiplied without overflow.
int tmult_ok(int x, int y) {
    // Compute product without overflow
    int64_t pll = (int64_t) x*y;
    // See if casting to int preserves value
    return pll == (int) pll;
}

// Returns the value x/16 for integer argument x.
int div16(int x) {
    // Compute bias to be either 0 (x >= 0) or 15 (x < 0), 15 = (1<<4) -1
    int bias = (x >> 31) & 0xF;
    return (x + bias) >> 4;
}

// M and N are Mystery number
int arith(int x, int y) {
    int M = 31;
    int N = 8;
    int result = 0;
    result = x*M + y/N;
    return result;
}

// Translation of assembly code for arith
int optarith(int x, int y) {
    int t = x;
    x <<= 5;
    x -= t;  // 31 * x
    if (y < 0) y += 7; // if y < 0, bias = 7, which means N = 8;
    y >>= 3;   /* Arithmetic shift */ // N = 8
    return x + y;    
}

int main() {
    int a = 2147483645;
    int b = 1111;
    cout << tadd_ok(a, b) << endl;    // return 0, overflow occurs.

    cout << tmult_ok(a, b) << endl;   // return 0, overflow occurs.

    // divide vs. right shift, for negative number
    int c = -12340;
    int c1 = c >> 4;
    int c2 = c / 16;
    cout << "-12340 >> 4 = " << c1 << endl;  // -772
    cout << "-12340 / 16 = " << c2 << endl;  // -771

    // divide vs. right shift, for positive number
    int d = 12340;
    int d1 = d >> 4;
    int d2 = d / 16;
    cout << "12340 >> 4  = " << d1 << endl;  // 771
    cout << "12340 / 16 = " << d2 << endl;  // 771

    // use div16() function
    cout << "div16(12340) = " << div16(12340) << endl;    // 771
    cout << "div16(-12340) = " << div16(-12340) << endl;  // -771

    // Test equivalence of arith and optarith
    cout << (arith(5, 5) == optarith(5, 5)) << endl;  // return 1 (true).

    int aa = 0xFFFF;
    printf("%x\n", aa*aa);

    const int x = 1;
    const int y = 2;
    unsigned ux = x;
    unsigned uy = y;
    cout << "x = " << x << endl;
    cout << "y = " << y << endl;
    cout << "ux = " << ux << endl;
    cout << "uy = " << uy << endl;
    if (x*~y + uy*ux == -x) printf("ok\n");
}