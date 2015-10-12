/* 
 * CS:APP Data Lab 
 * 
 * <Please put your name and userid here>
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
    /*
     * By doing "&" between x and y, we can locate positions where bits
     * in x and y are both 1 (positions in the result that are 1s);
     * By doing "&" between ~x and ~y, we can locate positions where 
     * bits are both 0;
     * In the final result, we want these positions to be 0, so we can
     * first reverse the bits and do "&" op between the partial results
     */
    return ~(x & y) & ~(~x & ~y);
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
    /*
     * The minimum integer would be 10000...00(1 followed by 31 0s). So
     * this can be done by left-shifting 1 31 bits.
     */
    return 1 << 31;
}
//2
/*
 * isTmax - returns 1 if x is the maximum, two's complement number,
 *     and 0 otherwise 
 *   Legal ops: ! ~ & ^ | +
 *   Max ops: 10
 *   Rating: 2
 */
int isTmax(int x) {
    /*
     * The maximum integer would be 01111...11(0 followed by 31 1s). If 
     * we double this integer and plus 2, we can get 0. This happens on 
     * -1 too, so we have to distinguish it from -1. 
     * If we reverse -1, we can get 0, but for 01111...11, it's not 0. 
     * This can be used to differentiate -1 and 01111...11.
     */
    return !((x + x + 2) | !~x);
}
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
    /*
     * If all odd-numbered bits in x are 1, then x & 0xAAAAAAAA would be
     * 0xAAAAAAAA. We can first create an integer whose value is 0xAA...A,
     * and then do "&" between it and x.
     */
    int temp1;
    int temp2;
    temp1 = (0xAA << 8) + 0xAA;
    temp2 = (temp1 << 16) + temp1;
    return !((x & temp2) ^ temp2); 
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
    /*
     * We can do this by first reversing bits in x and add it by 1.
     */
    return ~x + 1;
}
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
    /*
     * If x is between 0x30 and 0x39, then right-shifting x 4 bits will get
     * 0x3. What's more, we need to decide whether the last 4 bits of x
     * is between 0 and 0x9. To do this, we can subtract the 4 bits from
     * 9 and see whether the result is greater than or equal to 0.
     */
    
    //return !((x >> 4 ^ 3) | ((10 + ~(x & 0xf)) >> 4));
    return !(((x ^ 0x30) | (10 + ~(x & 0xf))) >> 4);
}
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
    /*
     * If x is not 0, then !x would be 0, we can add it with -1 to get
     * a sequence of 1 (0xFFFFFFFF). By doing "&" between it and y, the 
     * result would be y.
     * If x is 0, then !x would be 1, and !x - 1 would be 0. So by doing
     * "&" between ~(!x - 1) and z, the result would be z.
     */
    int X;
    int reverseX;
    X = !x + ~0;
    reverseX = ~X;
    return (X & y) | (reverseX & z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
    /*
     * If x and y have the same sign (both positive or both negative), we
     * can subtract x from y and see whether the result is larger or 
     * equal to 0.
     * If x and y have different signs, then subtraction will cause over-
     * flow. Therefore we need to consider this situation separately.
     */
    int diffSign = x & ~y;
	int sameSign = ~((x ^ y) | (y + ~x + 1));
    return ((diffSign | sameSign) >> 31) & 1;
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
    /*
     * If x is 0, then -x will also be 0. If x is not 0, then x and -x
     * will have different signs. We can do "|" between x and -x to see
     * whether the most significant bit is 1 or 0.
     */
    int complement;
    complement = ~x + 1;
    return ((~(x | complement)) >> 31 & 1);
}
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
    /*
     * If x is positive, then the number of bits would be the position of
     * the most significant 1 in x plus 1 (for example, the most significant 1
     * of 5(0101) is in the 3rd position from the right, so the result is 4).
     * If x is negative, then the number of bits would be the position of 
     * most significant 1 in ~x, plus 1.
     * To calculate the position of the most significant 1, we need to 
     * find out how many bits the number needs to be right-shifted to become
     * 0. Then we can add 1 to get result.
     */
    int sign;
    int positive;
    int shift;
	int shift1;
	int shift2;
	int shift3;
	int shift4;
	int shift5;
    int bits;
    
    sign = x >> 31;
    positive = x ^ sign;
    shift = 0;
	
	shift1 = positive >> 16;
    shift = shift + (!!shift1 << 4);
	shift2 = positive >> (8 + shift);
    shift = shift + (!!shift2 << 3);
	shift3 = positive >> (4 + shift);
    shift = shift + (!!shift3 << 2);	
	shift4 = positive >> (2 + shift);
    shift = shift + (!!shift4 << 1);	
	shift5 = positive >> (1 + shift);
    shift = shift + (!!shift5);
    
    bits = shift + !!(positive) + 1; 
    return bits;
}
//float
/* 
 * float_twice - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_twice(unsigned uf) {
    /*
     * If the exponential bits are all one, uf is infinite or NaN, so just 
     * return uf. 
     * If the exponential bits are all zero, uf is a number close to 0.
     * We need to left-shift fraction part for 1 bit.
     * For other situations, we can add the exponential part by 1.
     */
    unsigned orderBits;
    unsigned result;
    
    orderBits = uf & 0x7F800000;
    if (orderBits == 0x7F800000) {
        return uf;
    }
    if (orderBits == 0x0) {
        return (uf << 1) | (uf & 0x80000000);
    }
    
    result = uf + 0x00800000;
    if ((result & 0x7F800000) == 0x7F800000) {
        result &= 0xFF800000;
    }
    return result;
}
/* 
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point values.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_i2f(int x) {
    /*
     * If x is 0, just return x.
     * For other situations, we need to first turn x into positive number
     * and get its sign bit. Then we need to calculate the exponential
     * bits. We can get its value by calculating the position of the most
     * significant 1.
     * After that, we need to shift bits to the fraction region. If some
     * bits exceed the region, we will have to do rounding. If the number
     * is halfway between lower and upper bound, we will use round-to-even.
     * Finally, we can add sign, exponential bits, and fraction bits 
     * together to get the result.
     */
    int copy = x;
    unsigned sign = 0;
    unsigned order = 158;
    unsigned fraction;
    unsigned lastEight;
    unsigned result;
    
    if (x == 0) return x;
    
    if (copy < 0) {
        copy = -copy;
        sign = 0x80000000;
    }
    
    while (!(copy & 0x80000000)) {
        copy <<= 1;
        --order;
    }
    
    fraction = (copy >> 8);
    lastEight = copy & 0xFF;
    if (lastEight > 0x80 - (fraction & 1)) {
        fraction += 1;
        if (fraction == 0)
            ++order;
    }
    result = (fraction & 0x7FFFFF) | (order << 23) | sign;
    return result;
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
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
int float_f2i(unsigned uf) {
    /*
     * Given a floating point number, we can get the exponential bits by
     * right-shifting the number 23 bits and subtract 127. Besides, we can 
     * get the fraction bits by extracting the last 23 bits.
     * Then, according to the exponent, we can know how to shift the fraction
     * bits to change the number to an integer. If the sign bit is 1, we 
     * need to change the result to negative before return it.
     */
    int order;
    int fraction;
    int result;
    
    order = ((uf & 0x7F800000) >> 23) - 127;
    fraction = uf & 0x7FFFFF;
    
    if (order > 30) {
        return 0x80000000u;
    }
    if (order < 0) {
        return 0;
    }
    
    if (order > 23) {
        result = fraction << (order - 23);
    } 
    else {
        result = fraction >> (23 - order);
    } 
    
    result |= (1 << order);
    if (uf >> 31) {
        result = -result;
    }
    return result;
    
}
