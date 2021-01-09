/**
 * When length is 0, this will cause an error.
 * 原因是如果
 */


# include <iostream>


// WARNING: This is buggy code
float sum_elements(float a[], size_t length) {
    int i;
    float result = 0;

    for (i = 0; i <= length-1; i++) { // change "i <= length" to "i < length"
        result += a[i];
    }
    return result;
}

// WARNING: This code is buggy
int strlonger(const char* s, const char* t) {
    std::cout << strlen(s) - strlen(t) << std::endl;
    return strlen(s) - strlen(t) > 0;
}


int main() {
    // Test cast between unsigned and signed int.
    unsigned length = 0;
    std::cout << length -1 << std::endl;
    int i = -1;
    std::cout << (unsigned) i << std::endl;


    float a[] {0, 1, 2};
    float sum = sum_elements(a, 0);
    std::cout << sum << std::endl;

    std::cout << sizeof(size_t) << std::endl;

    const char* s = "abc";
    const char* t = "abcd";
    bool result = strlonger(s, t);
    std::cout << result << std::endl;  // return true
    
    int i1 = 1;
    unsigned u2 = -1;  // 4,294,967,295
    std::cout << (u2) << std::endl;

}