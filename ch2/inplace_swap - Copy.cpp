#include <iostream>
#include <string>
using namespace std;

void inplace_swap(int *x, int *y) {
    *y = *x ^ *y;
    *x = *x ^ *y;
    *y = *x ^ *y;
}

void reverse_array(int a[], int cnt) {
    int first, last;
    for (first = 0, last = cnt-1; 
    first <= last && first != last; 
    ++first, --last) {
        inplace_swap(&a[first], &a[last]);
    }
}


int main() {
    int a[]{1, 2, 3, 4, 5, 6};
    reverse_array(a, 6);
    for (int i: a) {
        cout << i << endl;
    }
    
    unsigned x = 0x87654321;
    unsigned y = (x & 0xFF) | (~x & 0xFFFFFF00);
    printf("%x\n", y);
    printf("%x\n", x | 0xFF);
}