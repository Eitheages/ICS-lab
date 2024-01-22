/*
 * CS:APP Data Lab
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

#include "btest.h"
#include <limits.h>

/*
 * Instructions to Students:
 *
 * STEP 1: Fill in the following struct with your identifying info.
 */
team_struct team =
{
   /* Team name: Replace with either:
      Your login ID if working as a one person team
      or, ID1+ID2 where ID1 is the login ID of the first team member
      and ID2 is the login ID of the second team member */
    "521021910788",
   /* Student name 1: Replace with the full name of first team member */
   "Bojun Ren",
   /* Login ID 1: Replace with the login ID of first team member */
   "521021910788",

   /* The following should only be changed if there are two team members */
   /* Student name 2: Full name of the second team member */
   "",
   /* Login ID 2: Login ID of the second team member */
   ""
};

#if 0
/*
 * STEP 2: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

CODING RULES:

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


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function.
     The max operator count is checked by dlc. Note that '=' is not
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.
#endif

/*
 * STEP 3: Modify the following functions according the coding rules.
 *
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the btest test harness to check that your solutions produce
 *      the correct answers. Watch out for corner cases around Tmin and Tmax.
 */
/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */

/*
 * bitCount - returns count of number of 1's in word
 *   Examples: bitCount(5) = 2, bitCount(7) = 3
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int bitCount(int x) {
  /**
   * Divide and conquer.
   * Count the count of 1s in each x bits,
   * and gather the neighbored counts into 2x bits.
   *
   * Note: my solution is a little different from that on the ICS slice.
   * It may need more operators, but it can be more easily extended to
   * the cases for longer integers (64-bit in the context).
   */
  int tmp, m1, m2, m4, m8, m16;
  tmp = 0x55 | 0x55 << 8 ;
  m1 =  tmp  | tmp  << 16;
  tmp = 0x33 | 0x33 << 8 ;
  m2 =  tmp  | tmp  << 16;
  tmp = 0x0f | 0x0f << 8 ;
  m4 =  tmp  | tmp  << 16;
  m8 =  0xff | 0xff << 16;
  m16 = 0xff | 0xff << 8 ;
  x = (x & m1 ) + ((x >>  1) & m1 );
  x = (x & m2 ) + ((x >>  2) & m2 );
  x = (x & m4 ) + ((x >>  4) & m4 );
  x = (x & m8 ) + ((x >>  8) & m8 );
  x = (x & m16) + ((x >> 16) & m16);
  return x;
}
/*
 * copyLSB - set all bits of result to least significant bit of x
 *   Example: copyLSB(5) = 0xFFFFFFFF, copyLSB(6) = 0x00000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int copyLSB(int x) {
  return ~(x & 0x1) + 1;
}
/*
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 2
 */
int evenBits(void) {
  /**
   * 0x55 = 0b0101
   */
  int tmp = 0x55 | 0x55 << 8;
  return tmp | tmp << 16;
}
/*
 * fitsBits - return 1 if x can be represented as an
 *  n-bit, two's complement integer.
 *   1 <= n <= 32
 *   Examples: fitsBits(5,3) = 0, fitsBits(-4,3) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int fitsBits(int x, int n) {
  /**
   * If x can be represented as an n-bit, after setting its (32 - n)
   * most significant bits to 0, its value remains.
   * And we need to simulate minus (-) and equal (==) with bit operator.
   * x - y -> x + 1 - ~y; x == y -> !(x ^ y)
   */
  int l = 33 + ~n;
  return !(x << l >> l ^ x);
}
/*
 * getByte - Extract byte n from word x
 *   Bytes numbered from 0 (LSB) to 3 (MSB)
 *   Examples: getByte(0x12345678,1) = 0x56
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int getByte(int x, int n) {
  /**
   * Trick: (n << 3) == n * 8
   */
  return (x >> (n << 3)) & 0xff;
}
/*
 * isGreater - if x > y  then return 1, else return 0
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
  /**
   * Calculate y - x, and compare it with 0 (check the sign bit).
   * Return 1 iff the sign bit is 1.
   * However, overflow need to be considered.
   * A fact: overflow only happens when sgn(x) != sgn(y).
   *
   * Therefore, return 1 iff:
   *      1. sgn(x) == 1 && sgn(y) == 0
   *      2. the sign bit of (y - x) is 1 and not overflow
   */
  int sgnx = x >> 31;
  int sgny = y >> 31;
  int minus = y + ~x + 1;
  int sgn_minus = minus >> 31;

  int is_overflow = (sgny & ~sgnx & ~sgn_minus) /* (-) - (+) = (+) */
                  | (~sgny & sgnx & sgn_minus) /* (+) - (-) = (-) */;
  return !!(sgn_minus & ~is_overflow) | (!sgnx & sgny);
}
/*
 * isNonNegative - return 1 if x >= 0, return 0 otherwise
 *   Example: isNonNegative(-1) = 0.  isNonNegative(0) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 3
 */
int isNonNegative(int x) {
  return !(x >> 31);
}
/*
 * isNotEqual - return 0 if x == y, and 1 otherwise
 *   Examples: isNotEqual(5,5) = 0, isNotEqual(4,5) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 2
 */
int isNotEqual(int x, int y) {
  return !!(x ^ y);
}
/*
 * leastBitPos - return a mask that marks the position of the
 *               least significant 1 bit. If x == 0, return 0
 *   Example: leastBitPos(96) = 0x20
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 6
 *   Rating: 4
 */
int leastBitPos(int x) {
  // return ~(x + 1 + ~1) & x;
  return (~x + 1) & x;
}
/*
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 1 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int logicalShift(int x, int n) {
  /**
   * Always replace the n most significant bits to 0.
   */
  int mask = ~(~0 << (33 + ~n));
  return x >> n & mask;
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          the maximum value, and when negative overflow occurs,
 *          it returns the minimum value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {
  /**
   * Judge whether overflow has happened, and use bit operators to simulate
   * the branch logic.
   */

  int o_sum = x + y;
  int sgnx = x >> 31;
  int sgny = y >> 31;
  int sgn_o_sum = o_sum >> 31;

  /* Positive overflow: sgn(x) == 1 and sgn(y) == 1 and sgn(sum) == -1 */
  int positive_of = ~sgnx & ~sgny & sgn_o_sum;
  int negative_of = sgnx & sgny & ~sgn_o_sum;
  // assert(!(positive_of & negative_of));
  int is_overflow = positive_of | negative_of;

  int min_int = 0x1 << 31;
  int max_int = ~min_int;

  return (~is_overflow & o_sum)
         | (is_overflow & ((positive_of & max_int) | (negative_of & min_int)));

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
  /**
   * Firstly, convert negative number: 11110... -> 00001...
   * Secondly, find the most significant 1 bit.
   * Divide and conquer: the number is divided into two part for many times.
   */
  int sign, w16, w8, w4, w2, w1;

  sign = x >> 31;
  x = (~sign & x) | (sign & ~x);

  /**
   * Just like binary search. We need to find the most significant 1 bit.
   * If [16, 32) exists 1, drop [0, 16), otherwise drop [16, 32).
   * Assume we dropped [16, 32), then if [8, 16) exists 1, drop [0, 8),
   * otherwise drop [8, 16).
   * ...
   * The key point: we search the 1 from the most significant bit to the least.
   */

  /**
   * If 1 occurs in the 16 most significant bits, the 16 least significant bits
   * are removed. Otherwise, x remains the same.
   */
  w16 = !!(x >> 16) << 4; // 16 or 0
  x >>= w16; // Now x can be represented by at most 16 bits

  w8 = !!(x >> 8) << 3; // 8 or 0
  x >>= w8; // Now x can be represented by at most 8 bits

  w4 = !!(x >> 4) << 2; // 4 or 0
  x >>= w4; // Now x can be represented by at most 4 bits

  w2 = !!(x >> 2) << 1; // 2 or 0
  x >>= w2; // Now x can be represented by at most 2 bits

  w1 = !!(x >> 1); // 1 or 0
  x >>= w1; // Now x is 1 or 0

  return x + w1 + w2 + w4 + w8 + w16 + 1 /* the sign bit */;
}

/*
 * logicalNeg - implement the ! operator, using all of
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4
 */
int logicalNeg(int x) {
  /**
   * If x is positive:
   *     x >> 31 : 0x00000000
   *     ~x + 1 >> 31 : 0xffffffff
   *     or them together: 0xffffffff
   * If x is negative:
   *     x >> 31 : 0xffffffff
   *     ~x + 1 >> 31 : 0x00000000
   *     or them together: 0xffffffff
   * Only when x == 0:
   *     x >> 31: 0
   *     ~x + 1 >> 31: 0
   */
  return (x >> 31 | (~x + 1) >> 31) + 1;
}

/*
 * dividePower2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: dividePower2(15,1) = 7, dividePower2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 3
 */
int dividePower2(int x, int n) {
  /**
   * Pretty obvious for non-negative x.
   * If x is negative, add 2 ^ n - 1 to x.
   */
  x = (x >> 31 & ~0 + (1 << n)) + x;
  return x >> n;
}

/*
 * bang - Convert from two's complement to sign-magnitude
 *   where the MSB is the sign bit
 *   You can assume that x > TMin
 *   Example: bang(-5) = 0x80000005.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 4
 */
int bang(int x) {
  /**
   * Find the abs of x, and then add the sign bit.
   */
  int sign = x >> 31;
  int absx = (~sign & x) | (sign & (~x + 1));
  return absx | sign << 31;
}

