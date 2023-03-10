/* 
 * CS:APP Data Lab 
 * 
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
/* Copyright (C) 1991-2022 Free Software Foundation, Inc.
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
 * bitAnd - x&y using only ~ and | 
 *   Example: bitAnd(6, 5) = 4
 *   Legal ops: ~ |
 *   Max ops: 8
 *   Rating: 1
 */
int bitAnd(int x, int y) {
  return ~((~x)|(~y));
}
/* 
 * bitConditional - x ? y : z for each bit respectively
 *   Example: bitConditional(0b00110011, 0b01010101, 0b00001111) = 0b00011101
 *   Legal ops: & | ^ ~
 *   Max ops: 8
 *   Rating: 1
 */
int bitConditional(int x, int y, int z) {
  return ((x&y)|((~x)&z));
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
  int n1=(x>>(n<<3))&0xff;
  int m1=(x>>(m<<3))&0xff;
  int tmpx=x&(~(0xff<<(n<<3)));
  tmpx=tmpx&(~(0xff<<(m<<3)));//clear
  tmpx=tmpx|(n1<<(m<<3));
  tmpx=tmpx|(m1<<(n<<3));
    return tmpx;
}
/* 
 * logicalShift - shift x to the right by n, using a logical shift
 *   Can assume that 0 <= n <= 31
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3 
 */
int logicalShift(int x, int n) {
  //set the first bit
  /*int tmp=x&(~(1<<31));
  int first=(x>>31)&1;
  int f=(first<<31)>>n;
  int second=(x>>31)<<n;
  tmp=tmp>>n;
  if n==0, reset the highest bit
  return tmp+f;*/
  
  //set the first bit
  int tmp=x&(~(1<<31));
  int first=(x>>31)&1;
  int f=first<<(32+(~n));
  tmp=tmp>>n;
  //reset the highest bit
  return tmp+f;

}
/* 
 * cleanConsecutive1 - change any consecutive 1 to zeros in the binary form of x.
 *   Consecutive 1 means a set of 1 that contains more than one 1.
 *   Examples cleanConsecutive1(0x10) = 0x10
 *            cleanConsecutive1(0xF0) = 0x0
 *            cleanConsecutive1(0xFFFF0001) = 0x1
 *            cleanConsecutive1(0x4F4F4F4F) = 0x40404040
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 4
 */
int cleanConsecutive1(int x){
    int left=x<<1;
    int right=x>>1;
    int del=~(left|right);
    int tmp=x&del;
    int first=((x>>31)&(!((x>>30)&1)))<<31;
    return tmp+first;
}
/* 
 * countTrailingZero - return the number of consecutive 0 from the lowest bit of 
 *   the binary form of x.
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   YOU MAY USE BIG CONST IN THIS PROBLEM, LIKE 0xFFFF0000
 *   Examples countTrailingZero(0x0) = 32, countTrailingZero(0x1) = 0,
 *            countTrailingZero(0xFFFF0000) = 16,
 *            countTrailingZero(0xFFFFFFF0) = 8,
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 40
 *   Rating: 4
 */
int countTrailingZero(int x){
    int total=0;
    //16bits
    {
        int t16=x&0xffff;
        total+=(!t16)<<4;
    }
    //8bits
    {
        int t8=(x>>total)&0xff;
        total+=(!t8)<<3;
    }
    //4bits
    {
        int t4=(x>>total)&0xf;
        total+=(!t4)<<2;
    }
    //2bits
    {
        int t2=(x>>total)&3;
        total+=(!t2)<<1;
    }
    //1bit
    {
        int t1=(x>>total)&1;
        total+=(!t1);
    }
    {
        int t1=(x>>total)&1;
        total+=(!t1);
    }
    return total;
}
/* 
 * divpwr2 - Compute x/(2^n), for 0 <= n <= 30
 *  Round toward zero
 *   Examples: divpwr2(15,1) = 7, divpwr2(-33,4) = -2
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int divpwr2(int x, int n) {
    int judge=(x>>31)&1;
    return (x+(judge<<n)+(~judge)+1)>>n;//judge*(2^n-1)=judge<<n-judge
}
/* 
 * oneMoreThan - return 1 if y is one more than x, and 0 otherwise
 *   Examples oneMoreThan(0, 1) = 1, oneMoreThan(-1, 1) = 0
 *   Legal ops: ~ & ! ^ | + << >>
 *   Max ops: 15
 *   Rating: 2
 */
int oneMoreThan(int x, int y) {
  //x is tmax, return false
  int judgetmax=x+(1<<31)+1;//is tmax,0;else,not 0
  int judge=y+(~x)+(~1)+2;//y=x+1,0;else,not 0
  return (!judge)&(!(!judgetmax));
}
/*
 * satMul3 - multiplies by 3, saturating to Tmin or Tmax if overflow
 *  Examples: satMul3(0x10000000) = 0x30000000
 *            satMul3(0x30000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0x70000000) = 0x7FFFFFFF (Saturate to TMax)
 *            satMul3(0xD0000000) = 0x80000000 (Saturate to TMin)
 *            satMul3(0xA0000000) = 0x80000000 (Saturate to TMin)
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 3s
 */
int satMul3(int x) {
    //reference: csapp page65 practice 2.30
    int tmp1=x<<1;
    int signbit=(x>>31);
    //judge the first bit
    int judge=((tmp1>>31))^signbit;//overflow,1;else,0
    int tmpr=tmp1+x;
    int test=0;
    int minusone=~0;

    //0->~0=-1;1->0: x-1
    int overmax=signbit&1;
    int calmax=overmax+minusone;
    int overmin=!overmax;
    int calmin=overmin+minusone;
    //if x<0,overmax=1,calmax=0,overmin=0,calmin=-1;if x>=0,overmax=0,calmax=-1,overmin=1,calmin=0

    int tmin=1<<31;
    int tmax=~tmin;

    //result=tmpr*(1-judge)+tmin*judge*((x>>31)&1)+tmax*judge*(...)
    int result=0;

    judge=judge|(((tmpr>>31))^signbit);//second overflow
    test=(!judge)+minusone;//0->0;1->-1
    //printf("%x\n",judge);
    //printf("%x\n",test);
    //printf("%d\n",calmin);
    //printf("%d\n",calmax);
    result=tmpr&(~test);
    result+=(tmin&test)&calmin;
    result+=(tmax&test)&calmax;
    return result;
    
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
  //reference: csapp page65 practice 2.30
  //have some problems with tmin
  int f1=(x>>31);
  //printf("%d\n",f1);
  int f=(~y)+1;
  int istmin=(!(y+y))&(y>>31);
  int f2=(f>>31);
  //printf("%d\n",f2);
  int r=x+f;
  int isdiff=(f1^f2);
  //printf("%d\n",r);
  int judge=(!isdiff&(!(f1^((r>>31)))))|isdiff;
  int judge_tmin=f1;
  return (istmin&judge_tmin)|((!istmin)&judge);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  int signx=(x>>31)&1;
  int signy=(y>>31)&1;
  int signdiff=signx^signy;
  int judge_diff=signx;
  int judge_same=!((y+(~x)+1)>>31);
  return (signdiff&judge_diff)|((!signdiff)&judge_same);
}
/*
 * trueThreeFourths - multiplies by 3/4 rounding toward 0,
 *   avoiding errors due to overflow
 *   Examples: trueThreeFourths(11) = 8
 *             trueThreeFourths(-9) = -6
 *             trueThreeFourths(1073741824) = 805306368 (no overflow)
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 4
 */
int trueThreeFourths(int x)
{
  int high=(x>>1)+(x>>2);
  int low=x&3;
  int notzero=!(!low);
  //lowr=low-((x>>1)&1)-notzero*greaterthanzero
  int lowr=low+2+(~((x>>1)&1))+(~(notzero&(!((x>>31)&1))));
  //printf("%d\n",lowr);
  return high+lowr;
}
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
  int exp=(uf>>23)&0xff;
  int sfrac=uf&(~(0xff<<23));
  int sign=(uf>>31)&1;
  //printf("%x\n",exp);
  if (exp==0xff)
    return uf;
  if (!exp)
  {
      //int f1=(sfrac>>22)&1;
      return (sfrac<<1)+(sign<<31);
  }
  if (exp==0xfe)//inf
  {
    //printf("haha\n");
      return (0xff<<23)+(sign<<31);
  }
  return sfrac+((exp+1)<<23);
  //return 2;
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
  int sign=x&0x80000000;
  int const23=0x7fffff;
  
  
  //find first one
  int firstone=31;
  unsigned result=0;
  //int addflag=0;
  int exp=127;

  if (!x)
      return 0;

  if (sign)
      x=(~x)+1;
  while ((!(x>>firstone)))
      firstone--;
  //printf("%d\n",firstone);
  /*if (firstone==-1)
  {
      if (sign)
          return (1<<31)+(158<<23);
      else
          return 0;
  }*/
      //return 0;

  //copy

  if (firstone>23)
  {
      int rest=firstone-23;
      
      //rest
      int reststd=(1<<rest)-1;
      int restcmp=1<<(rest-1);
      int restnum=x&reststd;

      result=(x>>rest)&const23;
      if (restnum>restcmp||(restnum==restcmp&&(result&1)))
      {
          result++;
          //printf("haha");
          
          //addflag=result>>23;
      }
      //result&=const23;
  }
  else
  {
      result=(x<<(23-firstone))&const23;
  }
  exp+=firstone;
  //printf("%x\n",result);
  return result+(exp<<23)+sign;
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
  int sign=(uf>>31)&1;
  int exp=(uf>>23)&0xff;
  int frac=uf&((1<<23)-1);
  int result=0;
  if (exp<=126)
      return 0;
  if (exp>=158)
      return 0x80000000u;
  //exp-=127;
  if (exp>=150)
      result=(frac<<(exp-150))+(1<<(exp-127));
  else
      result=(frac>>(150-exp))+(1<<(exp-127));
  if (sign)
      result=(~result)+1;
  return result;
}
/* 
 * float_pwr2 - Return bit-level equivalent of the expression 2.0^x
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
unsigned float_pwr2(int x) {
    if (x>=128)
        return (0xff<<23);
    if (x<=-150)
        return 0;
    if (x<=-127)
        return (1<<(x+149));
    return (x+127)<<23;
    return 2;
}
