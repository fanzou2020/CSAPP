# CSAPP: Data Lab

### bitXor
```C
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  /* Use ~ and & to calculate bitXor, with 8 ops */
  int p = ~x & y;
  int q = x & ~y;
  int r = ~(~p & ~q);
  return r;
}
```
`p ^ q = (~p & ~q) | (p & ~q)`       
`p | q = ~(~p & ~q)`


### tmin
```C
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  /* tmin = 0x80000000 */
  return 1 << 31;
}
```
`1 << 31`, will get the Minimum integer.

### isTmax
```C
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 1
 */
int isTmax(int x) {
  int tmp = (x << 1) + 1;
  return !(~tmp | (x >> 31));
}
```
`Tmax = 0x7FFFFFFF`, `tmp = (Tmax << 1) + 1 = 0xFFFFFFFF`    
`!(~tmp)` will return `1`, if `x` is `Tmax`.    
We also need to eliminate `0xFFFFFFFF`, which give the same result, 
`!(~(x >> 31))` will return 

### allOddBits
```C
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  /* Construct 0xAAAAAAAA as a mask. */
  int mask = 0xAA + (0xAA << 8) + (0xAA << 16) + (0xAA << 24);  
  return !((x & mask) ^ mask);
}
```
First construct `mask = 0xAAAAAAAA` as a mask,   
then `x & mask` get all the odd bits of x, and even bits are 0.    
`(x & mask) ^ mask` will return `0x0` if all odd bits are `1`s.   
`!((x & mask) ^ mask)` will return the demanded result.


### negate
```C
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  /* (flip + 1) is the negation of an integer */
  return (~x + 1);
}
```

### isAsciiDigit
```C
//3
/* 
 * isAsciiDigit - return 1 if 0x30 <= x <= 0x39 (ASCII codes for characters '0' to '9')
 *   Example: isAsciiDigit(0x35) = 1.
 *            isAsciiDigit(0x3a) = 0.
 *            isAsciiDigit(0x05) = 0.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int isAsciiDigit(int x) {
  int r1 = (0x39 + (~x + 1)) >> 31;  // if true r1 == 0x0, otherwise, r1 = 0xFFFFFFFF
  int r2 = (x + (~0x30 + 1)) >> 31;  // if true r2 == 0x0, otherwise, r2 = 0xFFFFFFFF

  return (!r1) & (!r2);  
}
```
`0x30 <= x <= 0x39` is equivalent to `0x39 - x >= 0` and `x - 0x30 >= 0`.   
How to determine a number is negative or positive?   
`positiveNum >> 31 == 0x0`, `zero >> 31 == 0x0`, `negativeNum >> 31 == 0xFFFFFFFF(-1)`


### conditional
```C
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  int t = ~(!x) + 1;
  return (~t & y) | (t & z);
}
```
After, `notX = !x`, what we want to achieve is: if `notX` is `0x0`, return `y`, if `notX` is `0x1`,  return `z`.   
Let `t = ~notX + 1`, if `x` is true, `t = 0x0`; if `x` is false, `t = 0xFFFFFFFF`.    
Thus `~t & y` will return `y` if `x` is true; return `0x0` if `x` is false.     
`t & z` will return `0x0` if `x` is true; return `z` if `x` is false.   
`(~t & y) | (t & z)` will give us `x ? y : z`.


### isLessOrEqual
```C
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int sx = (x >> 31) & 0x1;  // sign bit of x
  int sy = (y >> 31) & 0x1;  // sign bit of y
  int sdiff = ((y + (~x + 1)) >> 31) & 0x1;  // sign bit of y-x
  return (((sx ^ sy) & sx) | !sdiff & !(sx ^ sy));
}
```
Return `1` if `(y - x) >= 0`, similar to `isAsciiDigit()`.   
Applying `-x = ~x + 1`, we can transform subtraction to addition.   
then, right shifts `31` bits to reveal the sign bit, if result is non-negative, the value after right shifts will be `0x0`. If result is negative, the value after right shifts will be `0xFFFFFFFF`.


### logicalNeg
```C
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  return ((x | (~x + 1)) >> 31) + 1;
}
```
What we want to achieve is: if `x == 0x0`, return `1`, otherwise, return `0`.    
Consider that `(~x + 1) == x` iff `x == 0x0`, which means that they have the same sign bit.   
For any other number, the sign bit are different.   
We have the code `((x >> 31) | (~x + 1) >> 31) + 1`.   
To further simplify, we have `((x | (~x + 1)) >> 31) + 1`.   
This means that, if `x == 0x0`, `(x | (~x + 1)) >> 31` will give us `0x0`.   
If `x != 0x0`, `(x | (~x + 1)) >> 31` will give us `0xFFFFFFFF`. After `+1`, we get the desired result.


### howManyBits
```C
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  int b16, b8, b4, b2, b1, b0;
  int sign = x >> 31;  // signed bit, 0x0 if non-negative, 0xFFFFFFFF if negative
  x = (sign & ~x) | (~sign & x);  // If is negative number, flip it. 

  b16 = !!(x >> 16) << 4;  // whether most significant 16 bits have 1. If yes, b16 =16, if not, b16 = 0
  x = x >> b16;  // If yes, right shift 16 bits; if no, do not right shift.
  b8 = !!(x >> 8) << 3;  // whether the rest most significant 8 bits have 1.
  x = x >> b8;
  b4 = !!(x >> 4) << 2;
  x = x >> b4;
  b2 = !!(x >> 2) << 1;
  x = x >> b2;
  b1 = !!(x >> 1);
  x = x >> b1;
  b0 = x;
  return b16 + b8 + b4 + b2 + b1 + b0 + 1;  // 1 is the sign bit.
}
```
If this number is positive, we want to know the first appearance of `1`; if this number is negative, we need the first appearance of `0`.   
So, after flipping the negative number `x = (sign & ~x) | (~sign & x)`, we only need to find the first appearance of `1`. 

`b16 = !!(x >> 16) << 4;`    
If most significant 16 bits have at least one `1`, `b16 = 16`, otherwise, `b16 = 0`.  
`x = x >> b16;`   
If `b16 = 16`, right shift `x` by `16` bits, focusing on the left most 16 bits.   
Otherwise if `b16 = 0`, the first `1` is on the right 16 bits. No right shifts.

`b8 = !!(x >> 8) << 3;`   
If the rest left 8 bits have at least one `1`, `b8 == 8`, otherwise, `b8 = 0`.   
`x = x >> b8;`

Continue this process until we reach `b0`. Then sum up `b16` to `b0` and the sign bit will give the result.

### floatScale2
```C
//float
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  int exp = (uf >> 23) & 0xFF;  // Get the exponent bits.
  int sign = uf & (1 << 31);  // Most significant sign bit
  if (exp == 0xFF) {  // If uf is NAN or Infinity, return uf
    return uf;
  }

  if (exp == 0x0) { // If uf is denorm number, 
    return (uf << 1) | sign;  // Left shift by 1, and then add sign bit
  } 

  // normal number
  exp = exp + 1;  
  if (exp == 0xFF) {  // if exp+1 is equal to 0xFF, return Infinity
    return 0x7F800000 | sign;
  }
  else {  // exp+1 not equal to 0xFF, no overflow happens.
    int frac = uf & 0x7FFFFF;  // get the fraction bits 
    return (frac | exp << 23) | sign;  // add frac, exp, sign together.
  }
}
```
If `NAN`, return argument; 
if `INF`, return argument;
if `norm`, `E = E + 1`, if `++E` equals to `0xFF`, return `Infinity`.
if `denorm`, `f << 1`.




### floatFloat2Int
```c
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  
  int exp = (uf >> 23) & 0xFF; 
  int frac = uf & 0x7FFFFF;
  int sign = uf & (1 << 31);  // negative 0x80000000, positive 0x0.
  
  if (exp == 0xFF) return 0x80000000u;  // if out of range, return 0x80000000

  if (exp == 0x0) return 0;  // if de-norm number, means its range is (-1, 1)

  int frac1 = frac | 0x800000;  // Add the hidden 1 in front of frac
  int biasedExp = exp - 127; // E = e - Bias, Bias = 127 = 0x7F, -bias = 0xFFFFFF81

  if (biasedExp > 31) return 0x80000000;
  else if (biasedExp < 0) return 0;

  if (biasedExp > 23) frac1 <<= (biasedExp - 23);
  else frac1 >>= (23 - biasedExp);

  if (sign) return ~frac1 + 1;  // if negative number
  else if (frac1 >> 31) return 0x80000000;  // if frac1 overflows, return 0x80000000;
  else return frac1;

/*
  int s_ = uf >> 31;
  int exp_ = ((uf & 0x7f800000) >> 23) - 127;
  int frac_ = (uf & 0x007fffff) | 0x00800000;
  if (!(uf & 0x7fffffff))
    return 0;

  if (exp_ > 31)
    return 0x80000000;
  if (exp_ < 0)
    return 0;

  if (exp_ > 23)
    frac_ <<= (exp_ - 23);
  else
    frac_ >>= (23 - exp_);

  if (!((frac_ >> 31) ^ s_))
    return frac_;
  else if (frac_ >> 31)
    return 0x80000000;
  else
    return ~frac_ + 1;
  */
}
```



### floatPower2
```c
/* 
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 * 
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. Also if, while 
 *   Max ops: 30 
 *   Rating: 4
 */
unsigned floatPower2(int x) {
  int exp = x + 127;
  if (exp <= 0) return 0;
  if (exp >= 255) return 0xff << 23;
  return exp << 23;
}
```