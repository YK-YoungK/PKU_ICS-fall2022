
/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"
#include "contracts.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. The REQUIRES and ENSURES from 15-122 are included
 *     for your convenience. They can be removed if you like.
 */
/*
 *  32*32: Use 8*8 blocking, copy one line to one column each time.
 *  64*64: Use 8*8 blocking, each block was copied by the following steps:
 *      Use 4*4 blocking again in each block, assume the matrix is
 *          A   B
 *          C   D
 *      and our goal is
 *          A^T C^T
 *          B^T D^T
 *      (1) Copy one line in (A B) to the dest but put B^T in the place where
 *          C^T should lies, after 4 copies we get (A^T B^T)    
 *      (2) Copy two column in C to the place C^T should lies each time, and 
 *          copy two row in C^T to the place where B^T should actually lies,
 *          after 2 copies we get
 *              A^T C^T
 *              B^T 
 *      (3) Copy two column in D to the place D^T each time, after 2 copies we
 *          get
 *              A^T C^T
 *              B^T D^T
 *          which is what we want.
 *  68*60: Use 4*4 blocking, copy two column in one block (8 ints) to the 
 *         destination each time.
*/
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    REQUIRES(M > 0);
    REQUIRES(N > 0);

    if (N==32&&M==32)
    {
        for (int i=0;i<4;i++)
        {
            for (int j=0;j<4;j++)
            {
                for (int k=0;k<8;k++)
                {
                    int a1=A[8*i+k][8*j];
                    int a2=A[8*i+k][8*j+1];
                    int a3=A[8*i+k][8*j+2];
                    int a4=A[8*i+k][8*j+3];
                    int a5=A[8*i+k][8*j+4];
                    int a6=A[8*i+k][8*j+5];
                    int a7=A[8*i+k][8*j+6];
                    int a8=A[8*i+k][8*j+7];
                    B[8*j][8*i+k]=a1;
                    B[8*j+1][8*i+k]=a2;
                    B[8*j+2][8*i+k]=a3;
                    B[8*j+3][8*i+k]=a4;
                    B[8*j+4][8*i+k]=a5;
                    B[8*j+5][8*i+k]=a6;
                    B[8*j+6][8*i+k]=a7;
                    B[8*j+7][8*i+k]=a8;
                }
                
            }
        }
    }
    else if (N==64&&M==64)
    {
        for (int i=0;i<8;i++)
        {
            for (int j=0;j<8;j++)
            {
                for (int k=0;k<4;k++)
                {
                    int a1=A[8*i+k][8*j];
                    int a2=A[8*i+k][8*j+1];
                    int a3=A[8*i+k][8*j+2];
                    int a4=A[8*i+k][8*j+3];
                    int a5=A[8*i+k][8*j+4];
                    int a6=A[8*i+k][8*j+5];
                    int a7=A[8*i+k][8*j+6];
                    int a8=A[8*i+k][8*j+7];
                    B[8*j][8*i+k]=a1;
                    B[8*j+1][8*i+k]=a2;
                    B[8*j+2][8*i+k]=a3;
                    B[8*j+3][8*i+k]=a4;
                    B[8*j][8*i+k+4]=a5;
                    B[8*j+1][8*i+k+4]=a6;
                    B[8*j+2][8*i+k+4]=a7;
                    B[8*j+3][8*i+k+4]=a8;
                }
                for (int k=0;k<2;k++)
                {
                    int a1=A[8*i+4][8*j+2*k];
                    int a2=A[8*i+5][8*j+2*k];
                    int a3=A[8*i+6][8*j+2*k];
                    int a4=A[8*i+7][8*j+2*k];
                    int a5=A[8*i+4][8*j+2*k+1];
                    int a6=A[8*i+5][8*j+2*k+1];
                    int a7=A[8*i+6][8*j+2*k+1];
                    int a8=A[8*i+7][8*j+2*k+1];
                    int a9;
                    a9=B[8*j+2*k][8*i+4];
                    B[8*j+2*k][8*i+4]=a1;
                    a1=a9;
                    a9=B[8*j+2*k][8*i+5];
                    B[8*j+2*k][8*i+5]=a2;
                    a2=a9;
                    a9=B[8*j+2*k][8*i+6];
                    B[8*j+2*k][8*i+6]=a3;
                    a3=a9;
                    a9=B[8*j+2*k][8*i+7];
                    B[8*j+2*k][8*i+7]=a4;
                    a4=a9;

                    a9=B[8*j+2*k+1][8*i+4];
                    B[8*j+2*k+1][8*i+4]=a5;
                    a5=a9;
                    a9=B[8*j+2*k+1][8*i+5];
                    B[8*j+2*k+1][8*i+5]=a6;
                    a6=a9;
                    a9=B[8*j+2*k+1][8*i+6];
                    B[8*j+2*k+1][8*i+6]=a7;
                    a7=a9;
                    a9=B[8*j+2*k+1][8*i+7];
                    B[8*j+2*k+1][8*i+7]=a8;
                    a8=a9;

                    B[8*j+2*k+4][8*i]=a1;
                    B[8*j+2*k+4][8*i+1]=a2;
                    B[8*j+2*k+4][8*i+2]=a3;
                    B[8*j+2*k+4][8*i+3]=a4;
                    B[8*j+2*k+5][8*i]=a5;
                    B[8*j+2*k+5][8*i+1]=a6;
                    B[8*j+2*k+5][8*i+2]=a7;
                    B[8*j+2*k+5][8*i+3]=a8;
                }
                for (int k=0;k<2;k++)
                {
                    int a1=A[8*i+4][8*j+2*k+4];
                    int a2=A[8*i+5][8*j+2*k+4];
                    int a3=A[8*i+6][8*j+2*k+4];
                    int a4=A[8*i+7][8*j+2*k+4];
                    int a5=A[8*i+4][8*j+2*k+5];
                    int a6=A[8*i+5][8*j+2*k+5];
                    int a7=A[8*i+6][8*j+2*k+5];
                    int a8=A[8*i+7][8*j+2*k+5];
                    B[8*j+2*k+4][8*i+4]=a1;
                    B[8*j+2*k+4][8*i+5]=a2;
                    B[8*j+2*k+4][8*i+6]=a3;
                    B[8*j+2*k+4][8*i+7]=a4;
                    B[8*j+2*k+5][8*i+4]=a5;
                    B[8*j+2*k+5][8*i+5]=a6;
                    B[8*j+2*k+5][8*i+6]=a7;
                    B[8*j+2*k+5][8*i+7]=a8;
                }
            }
        }
    }
    else if (N==68&&M==60)
    {
        for (int i=0;i<17;i++)
        {
            for (int j=0;j<15;j++)
            {
                for (int k=0;k<2;k++)
                {
                    int a1=A[4*i][4*j+2*k];
                    int a2=A[4*i+1][4*j+2*k];
                    int a3=A[4*i+2][4*j+2*k];
                    int a4=A[4*i+3][4*j+2*k];
                    int a5=A[4*i][4*j+2*k+1];
                    int a6=A[4*i+1][4*j+2*k+1];
                    int a7=A[4*i+2][4*j+2*k+1];
                    int a8=A[4*i+3][4*j+2*k+1];
                    B[4*j+2*k][4*i]=a1;
                    B[4*j+2*k][4*i+1]=a2;
                    B[4*j+2*k][4*i+2]=a3;
                    B[4*j+2*k][4*i+3]=a4;
                    B[4*j+2*k+1][4*i]=a5;
                    B[4*j+2*k+1][4*i+1]=a6;
                    B[4*j+2*k+1][4*i+2]=a7;
                    B[4*j+2*k+1][4*i+3]=a8;
                }
            }
        }
    }
    else
    {
        for (int i=0;i<N;i++)
        {
            for (int j=0;j<M;j++)
            {
                int tmp=A[i][j];
                B[j][i]=tmp;
            }
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

 /*
  * trans - A simple baseline transpose function, not optimized for the cache.
  */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    REQUIRES(M > 0);
    REQUIRES(N > 0);

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }

    ENSURES(is_transpose(M, N, A, B));
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);

}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

