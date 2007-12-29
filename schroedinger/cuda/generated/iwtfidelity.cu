
// Autogenerated by WaveGen.py, do not edit! //
#include <algorithm>
#include <cassert>
#include "common.h"
#include "wavelet_common.h"

/// Boundaries (depends on wavelet)
/// This much is reserved at the sides of the signal
/// Must be even!
#define BLEFT 4
#define BRIGHT 4

/// Initial shift (to keep precision in integer wavelets)
#define INITIAL_SHIFT 0
#define INITIAL_OFFSET 0

//  static const int16_t stage1_weights[] = { -8, 21, -46, 161, 161, -46, 21, -8 };
//  static const int16_t stage2_weights[] = { 2, -10, 25, -81, -81, 25, -10, 2 };

#define STAGE1_OFFSET 128
#define STAGE1_SHIFT 8
#define STAGE1_COEFF0 (-8)
#define STAGE1_COEFF1 (21)
#define STAGE1_COEFF2 (-46)
#define STAGE1_COEFF3 (161)
#define STAGE1_COEFF4 (161)
#define STAGE1_COEFF5 (-46)
#define STAGE1_COEFF6 (21)
#define STAGE1_COEFF7 (-8)

#define STAGE2_OFFSET 127
#define STAGE2_SHIFT 8
#define STAGE2_COEFF0 (2)
#define STAGE2_COEFF1 (-10)
#define STAGE2_COEFF2 (25)
#define STAGE2_COEFF3 (-81)
#define STAGE2_COEFF4 (-81)
#define STAGE2_COEFF5 (25)
#define STAGE2_COEFF6 (-10)
#define STAGE2_COEFF7 (2)

/// Vertical pass row management
#define RLEFT 6
#define RRIGHT 6
#define COPYROWS 8

static __global__ void a_transform_h( DATATYPE* data, int width, int stride )
{
    extern __shared__ DATATYPE shared[];  

    const int bid = blockIdx.x;    // row
    const int tid = threadIdx.x;   // thread id within row
    const int tidu16 = ((tid&16)>>4)|((tid&15)<<1)|(tid&~31);

    data += __mul24(bid, stride);

    // Load entire line into shared memory
    // Deinterleave right here
    int half = BLEFT+(width>>1)+BRIGHT;

    unsigned int ofs;

    // Shared memory output offset for this thread
    i16_2 *row = (i16_2*)data;
    for(ofs = tid; ofs < (width>>1); ofs += BSH)
    {
        i16_2 val = row[ofs];
        shared[BLEFT + ofs]        = val.a << INITIAL_SHIFT; // even
        shared[BLEFT + ofs + half] = val.b << INITIAL_SHIFT; // odd
    }
    __syncthreads();

    if(tidu16<4)
    {
        /*
          lo[-4] = lo[0];
          lo[-3] = lo[0];
          lo[-2] = lo[0];
          lo[-1] = lo[0];
          lo[n] = lo[n-1];
          lo[n+1] = lo[n-1];
          lo[n+2] = lo[n-1];
        */
        shared[half+BLEFT-4+tidu16] = shared[half+BLEFT];
        shared[half+BLEFT+(width>>1)+tidu16] = shared[half+BLEFT+(width>>1)-1];
    }    
    __syncthreads();
    // Now apply wavelet lifting to entire line at once
    // Process even
    const int end = BLEFT+(width>>1);
    for(ofs = BLEFT+tidu16; ofs < end; ofs += BSH)
    {
        int acc = STAGE1_OFFSET;

        acc += __mul24(STAGE1_COEFF0,shared[half+ofs-4]);
        acc += __mul24(STAGE1_COEFF1,shared[half+ofs-3]);
        acc += __mul24(STAGE1_COEFF2,shared[half+ofs-2]);
        acc += __mul24(STAGE1_COEFF3,shared[half+ofs-1]);
        acc += __mul24(STAGE1_COEFF4,shared[half+ofs+0]);
        acc += __mul24(STAGE1_COEFF5,shared[half+ofs+1]);
        acc += __mul24(STAGE1_COEFF6,shared[half+ofs+2]);
        acc += __mul24(STAGE1_COEFF7,shared[half+ofs+3]);
        
        shared[ofs] += acc >> STAGE1_SHIFT;
    }

    __syncthreads();
    if(tidu16<4)
    {
        /*
          hi[-3] = hi[0];
          hi[-2] = hi[0];
          hi[-1] = hi[0];
          hi[n] = hi[n-1];
          hi[n+1] = hi[n-1];
          hi[n+2] = hi[n-1];
          hi[n+3] = hi[n-1];
        */
        shared[BLEFT-4+tidu16] = shared[BLEFT];
        shared[BLEFT+(width>>1)+tidu16] = shared[BLEFT+(width>>1)-1];
    }
    __syncthreads();

    // Process odd
    for(ofs = BLEFT+tidu16; ofs < end; ofs += BSH)
    {
        int acc = STAGE2_OFFSET;

        acc += __mul24(STAGE2_COEFF0, shared[ofs-3]);
        acc += __mul24(STAGE2_COEFF1, shared[ofs-2]);
        acc += __mul24(STAGE2_COEFF2, shared[ofs-1]);
        acc += __mul24(STAGE2_COEFF3, shared[ofs-0]);
        acc += __mul24(STAGE2_COEFF4, shared[ofs+1]);
        acc += __mul24(STAGE2_COEFF5, shared[ofs+2]);
        acc += __mul24(STAGE2_COEFF6, shared[ofs+3]);
        acc += __mul24(STAGE2_COEFF7, shared[ofs+4]);
        
        shared[ofs + half] += acc >> STAGE2_SHIFT;
    }

    __syncthreads();


    /// Write line back to global memory, don't interleave again
    /// Mind the gap between BLEFT+width/2 and half
    if(width&3) // If width is not a multiple of 4, we need to use the slower method
    {
        /// Left part (even coefficients)
        for(ofs = tid; ofs < (width>>1); ofs += BSH)
            data[ofs] = shared[BLEFT+ofs];

        /// Right part (odd coefficients)
        for(ofs = tid; ofs < (width>>1); ofs += BSH)
            data[(width>>1)+ofs] = shared[half+BLEFT+ofs];   
    } 
    else
    {
        /// Left part (even coefficients)
        for(ofs = tid; ofs < (width>>2); ofs += BSH)
            row[ofs] = *((i16_2*)&shared[BLEFT+(ofs<<1)]);
        row += (width>>2);
        /// Right part (odd coefficients)
        for(ofs = tid; ofs < (width>>2); ofs += BSH)
            row[ofs] = *((i16_2*)&shared[half+BLEFT+(ofs<<1)]);
    }
}

#define BROWS (2*BSVY+COPYROWS) /* Rows to process at once */
#define SKIPTOP COPYROWS

#define PAD_ROWS (WRITEBACK-SKIPTOP+RRIGHT+COPYROWS) /* Rows below which to use s_transform_v_pad */
/// tid is BCOLSxBROWS matrix
/// RLEFT+BROWS+RRIGHT rows
#define TOTALROWS (RLEFT+BROWS+RRIGHT)
#define OVERLAP (RLEFT+RRIGHT+COPYROWS)
#define OVERLAP_OFFSET (TOTALROWS-OVERLAP)
#define WRITEBACK (2*BSVY)

__device__ void doTransform(int xofs)
{
    const int tidx = (threadIdx.x<<1)+xofs;   // column
    const int tidy = threadIdx.y;   // row

    extern __shared__ DATATYPE shared[];
    int ofs;

    ofs = ((RLEFT+(tidy<<1)+8)<<BCOLS_SHIFT) + tidx;

    /* Phase 1 (even) at +8*BCOLS */
    {
        int acc = STAGE1_OFFSET;

        acc += __mul24(STAGE1_COEFF0, shared[ofs-7*BCOLS]);
        acc += __mul24(STAGE1_COEFF1, shared[ofs-5*BCOLS]);        
        acc += __mul24(STAGE1_COEFF2, shared[ofs-3*BCOLS]);
        acc += __mul24(STAGE1_COEFF3, shared[ofs-1*BCOLS]);
        acc += __mul24(STAGE1_COEFF4, shared[ofs+1*BCOLS]);
        acc += __mul24(STAGE1_COEFF5, shared[ofs+3*BCOLS]);
        acc += __mul24(STAGE1_COEFF6, shared[ofs+5*BCOLS]);
        acc += __mul24(STAGE1_COEFF7, shared[ofs+7*BCOLS]);

        shared[ofs] += acc >> STAGE1_SHIFT;
    }
    
    __syncthreads();

    /* Phase 2 (odd) at +1*BCOLS */    
    ofs -= 7*BCOLS;
    {
        int acc = STAGE2_OFFSET;

        acc += __mul24(STAGE2_COEFF0, shared[ofs-7*BCOLS]);
        acc += __mul24(STAGE2_COEFF1, shared[ofs-5*BCOLS]);
        acc += __mul24(STAGE2_COEFF2, shared[ofs-3*BCOLS]);
        acc += __mul24(STAGE2_COEFF3, shared[ofs-1*BCOLS]);
        acc += __mul24(STAGE2_COEFF4, shared[ofs+1*BCOLS]);
        acc += __mul24(STAGE2_COEFF5, shared[ofs+3*BCOLS]);
        acc += __mul24(STAGE2_COEFF6, shared[ofs+5*BCOLS]);
        acc += __mul24(STAGE2_COEFF7, shared[ofs+7*BCOLS]);
        
        shared[ofs] += acc >> STAGE2_SHIFT;
    }

}

// Process 16 lines
__device__ void doTransformTB(int xofs, unsigned int leftover)
{
    const int tidx = (threadIdx.x<<1)+xofs;   // column
    const int tidy = threadIdx.y;   // row
    const int minn = (RLEFT<<BCOLS_SHIFT) + tidx;
    const int maxx = leftover-(2<<BCOLS_SHIFT) + tidx;
    
    extern __shared__ DATATYPE shared[];
    int ofs, ofs_t;

    /* Phase 1 (even) */
    ofs = ((RLEFT+(tidy<<1))<<BCOLS_SHIFT) + tidx;
    
    for(ofs_t=ofs+0*BCOLS; ofs_t<leftover; ofs_t += BSVY*2*BCOLS)
    {
        int acc = STAGE1_OFFSET;

        acc += __mul24(STAGE1_COEFF0, shared[max(ofs_t-7*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF1, shared[max(ofs_t-5*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF2, shared[max(ofs_t-3*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF3, shared[max(ofs_t-1*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF4, shared[ofs_t+1*BCOLS]);
        acc += __mul24(STAGE1_COEFF5, shared[min(ofs_t+3*BCOLS, maxx+BCOLS)]);
        acc += __mul24(STAGE1_COEFF6, shared[min(ofs_t+5*BCOLS, maxx+BCOLS)]);
        acc += __mul24(STAGE1_COEFF7, shared[min(ofs_t+7*BCOLS, maxx+BCOLS)]);

        shared[ofs_t] += acc >> STAGE1_SHIFT;
    }
    
    __syncthreads();
    
    /* Phase 2 (odd) */
    for(ofs_t=ofs+1*BCOLS; ofs_t<leftover; ofs_t += BSVY*2*BCOLS)
    {
        int acc = STAGE2_OFFSET;

        acc += __mul24(STAGE2_COEFF0, shared[max(ofs_t-7*BCOLS, minn)]);
        acc += __mul24(STAGE2_COEFF1, shared[max(ofs_t-5*BCOLS, minn)]);
        acc += __mul24(STAGE2_COEFF2, shared[max(ofs_t-3*BCOLS, minn)]);
        acc += __mul24(STAGE2_COEFF3, shared[ofs_t-1*BCOLS]);
        acc += __mul24(STAGE2_COEFF4, shared[min(ofs_t+1*BCOLS, maxx)]);
        acc += __mul24(STAGE2_COEFF5, shared[min(ofs_t+3*BCOLS, maxx)]);
        acc += __mul24(STAGE2_COEFF6, shared[min(ofs_t+5*BCOLS, maxx)]);
        acc += __mul24(STAGE2_COEFF7, shared[min(ofs_t+7*BCOLS, maxx)]);

        shared[ofs_t] += acc >> STAGE2_SHIFT;
    }
    
}

// Process BROWS-2 lines
__device__ void doTransformT(int xofs)
{
    const int tidx = (threadIdx.x<<1)+xofs;   // column
    const int tidy = threadIdx.y;   // row
    const int minn = ((RLEFT+SKIPTOP)<<BCOLS_SHIFT) + tidx;
    
    extern __shared__ DATATYPE shared[];
    int ofs;

    /* Phase 1 (even), offset +0 */
    ofs = ((SKIPTOP+RLEFT+(tidy<<1))<<BCOLS_SHIFT) + tidx;
    {
        int acc = STAGE1_OFFSET;

        acc += __mul24(STAGE1_COEFF0, shared[max(ofs-7*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF1, shared[max(ofs-5*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF2, shared[max(ofs-3*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF3, shared[max(ofs-1*BCOLS, minn+BCOLS)]);
        acc += __mul24(STAGE1_COEFF4, shared[ofs+1*BCOLS]);
        acc += __mul24(STAGE1_COEFF5, shared[ofs+3*BCOLS]);
        acc += __mul24(STAGE1_COEFF6, shared[ofs+5*BCOLS]);
        acc += __mul24(STAGE1_COEFF7, shared[ofs+7*BCOLS]);
        
        shared[ofs] += acc >> STAGE1_SHIFT;
    }
    
    __syncthreads();

    /* Phase 2 (odd), offset +1, stop at -8 */
    ofs += BCOLS;
    if(tidy<(BSVY-4))
    {
        int acc = STAGE2_OFFSET;

        acc += __mul24(STAGE2_COEFF0, shared[max(ofs-7*BCOLS, minn)]);
        acc += __mul24(STAGE2_COEFF1, shared[max(ofs-5*BCOLS, minn)]);
        acc += __mul24(STAGE2_COEFF2, shared[max(ofs-3*BCOLS, minn)]);
        acc += __mul24(STAGE2_COEFF3, shared[ofs-1*BCOLS]);
        acc += __mul24(STAGE2_COEFF4, shared[ofs+1*BCOLS]);
        acc += __mul24(STAGE2_COEFF5, shared[ofs+3*BCOLS]);
        acc += __mul24(STAGE2_COEFF6, shared[ofs+5*BCOLS]);
        acc += __mul24(STAGE2_COEFF7, shared[ofs+7*BCOLS]);
        
        shared[ofs] += acc >> STAGE2_SHIFT;
    }
    
}

// Finish off leftover
__device__ void doTransformB(int xofs, unsigned int leftover)
{
    const int tidx = (threadIdx.x<<1)+xofs;   // column
    const int tidy = threadIdx.y;   // row
    const int maxx = leftover-(2<<BCOLS_SHIFT) + tidx;
    
    extern __shared__ DATATYPE shared[];
    int ofs, ofs_t;

    ofs = ((RLEFT+(tidy<<1))<<BCOLS_SHIFT) + tidx;

    for(ofs_t=ofs+8*BCOLS; ofs_t<leftover; ofs_t += BSVY*2*BCOLS)
    {
        int acc = STAGE1_OFFSET;

        acc += __mul24(STAGE1_COEFF0, shared[ofs_t-7*BCOLS]);
        acc += __mul24(STAGE1_COEFF1, shared[ofs_t-5*BCOLS]);
        acc += __mul24(STAGE1_COEFF2, shared[ofs_t-3*BCOLS]);
        acc += __mul24(STAGE1_COEFF3, shared[ofs_t-1*BCOLS]);
        acc += __mul24(STAGE1_COEFF4, shared[ofs_t+1*BCOLS]);
        acc += __mul24(STAGE1_COEFF5, shared[min(ofs_t+3*BCOLS, maxx+BCOLS)]);
        acc += __mul24(STAGE1_COEFF6, shared[min(ofs_t+5*BCOLS, maxx+BCOLS)]);
        acc += __mul24(STAGE1_COEFF7, shared[min(ofs_t+7*BCOLS, maxx+BCOLS)]);        

        shared[ofs_t] += acc >> STAGE1_SHIFT;
    }
    
    __syncthreads();
    
    for(ofs_t=ofs+1*BCOLS; ofs_t<leftover; ofs_t += BSVY*2*BCOLS)
    {
        int acc = STAGE2_OFFSET;

        acc += __mul24(STAGE2_COEFF0, shared[ofs_t-7*BCOLS]);
        acc += __mul24(STAGE2_COEFF1, shared[ofs_t-5*BCOLS]);
        acc += __mul24(STAGE2_COEFF2, shared[ofs_t-3*BCOLS]);
        acc += __mul24(STAGE2_COEFF3, shared[ofs_t-1*BCOLS]);
        acc += __mul24(STAGE2_COEFF4, shared[min(ofs_t+1*BCOLS, maxx)]);
        acc += __mul24(STAGE2_COEFF5, shared[min(ofs_t+3*BCOLS, maxx)]);
        acc += __mul24(STAGE2_COEFF6, shared[min(ofs_t+5*BCOLS, maxx)]);
        acc += __mul24(STAGE2_COEFF7, shared[min(ofs_t+7*BCOLS, maxx)]);        

        shared[ofs_t] += acc >> STAGE2_SHIFT;
    }
    
}



static __global__ void a_transform_v( DATATYPE* data, int width, int height, int stride )
{
    extern __shared__ DATATYPE shared[];  

    const unsigned int bid = blockIdx.x;    // slab (BCOLS columns)
    const unsigned int tidx = threadIdx.x<<1;   // column
    const unsigned int tidy = threadIdx.y;   // row    
    const unsigned int swidth = min(width-(bid<<BCOLS_SHIFT), BCOLS); // Width of this slab, usually BCOLS but can be less

    // Element offset in global memory
    //int idata = tidx + (bid<<BCOLS_SHIFT) + __mul24(tidy, stride);
    data += tidx + (bid<<BCOLS_SHIFT) + __mul24(tidy, stride);
    
    const unsigned int istride = __mul24(BSVY, stride);
    const unsigned int sdata = tidx + (tidy<<BCOLS_SHIFT);
    // First read BROWS+RRIGHT
    // After that BROWS
    unsigned int ref = height-(WRITEBACK-SKIPTOP+RRIGHT+COPYROWS);
    unsigned int blocks = ref/WRITEBACK;
    unsigned int leftover = (RLEFT+COPYROWS+RRIGHT+(ref%WRITEBACK))<<BCOLS_SHIFT;
    
    unsigned int gofs,sofs;

    /// More than one block
    /// Read first block of BROWS+RRIGHT rows
    /// Upper RLEFT rows are left unitialized for now, later they should be copied from top
    if(tidx < swidth)
    {
        gofs = 0;
        sofs = sdata + ((RLEFT+SKIPTOP)<<BCOLS_SHIFT);
        for(; sofs < (TOTALROWS<<BCOLS_SHIFT); sofs += (BCOLS*BSVY), gofs += istride)
            *((uint32_t*)&shared[sofs]) = *((uint32_t*)&data[gofs]);
    }
// idata_read = idata_write + __mul24(TOTALROWS-RLEFT, stride)

    __syncthreads();
    
    doTransformT(0);
    doTransformT(1);
    
    __syncthreads();

    /// Write back WRITEBACK rows
    if(tidx < swidth)
    {
        gofs = 0;
        sofs = sdata + ((RLEFT+SKIPTOP)<<BCOLS_SHIFT);
        for(; sofs < ((WRITEBACK+RLEFT)<<BCOLS_SHIFT); sofs += (BCOLS*BSVY), gofs += istride)
            *((uint32_t*)&data[gofs]) = *((uint32_t*)&shared[sofs]);
    }
// idata_read = idata_write + __mul24((BROWS+RRIGHT)-WRITEBACK, stride)

// Difference between global mem read and write pointer
#define DATA_READ_DIFF __mul24((BROWS+RRIGHT)-WRITEBACK, stride)
// Advance pointer with this amount after each block
#define DATA_INC __mul24(WRITEBACK, stride)

    data += __mul24(WRITEBACK-SKIPTOP, stride);
    for(unsigned int block=0; block<blocks; ++block)
    {
        __syncthreads();
        /// Move lower rows to top rows
#if OVERLAP <= BSVY 
        if(tidy < OVERLAP)
        {
            unsigned int l = (tidy<<BCOLS_SHIFT)+tidx;
            *((uint32_t*)&shared[l]) = *((uint32_t*)&shared[(WRITEBACK<<BCOLS_SHIFT)+l]);
        }
#else
        for(sofs = (tidy<<BCOLS_SHIFT)+tidx; sofs < (OVERLAP<<BCOLS_SHIFT); sofs += (BSVY<<BCOLS_SHIFT))
            *((uint32_t*)&shared[sofs]) = *((uint32_t*)&shared[(WRITEBACK<<BCOLS_SHIFT)+sofs]);
#endif                
        
        /// Fill shared memory -- read next block of BROWS rows
        /// We can skip RRIGHT rows as we've already copied them for the previous block
        /// and moved them to the top
        if(tidx < swidth)
        {
            gofs = DATA_READ_DIFF;
            sofs = sdata + (OVERLAP<<BCOLS_SHIFT);
            for(; sofs < (TOTALROWS<<BCOLS_SHIFT); sofs += (BCOLS*BSVY), gofs += istride)
                *((uint32_t*)&shared[sofs]) = *((uint32_t*)&data[gofs]);
        }

        __syncthreads();

        doTransform(0);
        doTransform(1);

        __syncthreads();

        /// Write back BROWS rows
        if(tidx < swidth)
        {
            gofs = 0;
            sofs = sdata + (RLEFT<<BCOLS_SHIFT);
            for(; sofs < ((WRITEBACK+RLEFT)<<BCOLS_SHIFT); sofs += (BCOLS*BSVY), gofs += istride)
                *((uint32_t*)&data[gofs]) = *((uint32_t*)&shared[sofs]);
        }
        
        data += DATA_INC;
    }
    __syncthreads();

    ///
    /// Handle partial last block
    /// Move lower rows to top rows
#if OVERLAP <= BSVY 
        if(tidy < OVERLAP)
        {
            unsigned int l = (tidy<<BCOLS_SHIFT)+tidx;
            *((uint32_t*)&shared[l]) = *((uint32_t*)&shared[(WRITEBACK<<BCOLS_SHIFT)+l]);
        }
#else
        for(sofs = (tidy<<BCOLS_SHIFT)+tidx; sofs < (OVERLAP<<BCOLS_SHIFT); sofs += (BSVY<<BCOLS_SHIFT))
            *((uint32_t*)&shared[sofs]) = *((uint32_t*)&shared[(WRITEBACK<<BCOLS_SHIFT)+sofs]);
#endif                
    
    /// Fill shared memory -- read next block of BROWS rows
    /// We can skip RRIGHT rows as we've already copied them for the previous block
    /// and moved them to the top
    if(tidx < swidth)
    {
        gofs = DATA_READ_DIFF;
        sofs = sdata + (OVERLAP<<BCOLS_SHIFT);
        for(; sofs < leftover; sofs += (BCOLS*BSVY), gofs += istride)
            *((uint32_t*)&shared[sofs]) = *((uint32_t*)&data[gofs]);
    }

    __syncthreads();

    doTransformB(0, leftover);
    doTransformB(1, leftover);
    
    __syncthreads();
    
    /// Write back leftover
    if(tidx < swidth)
    {
        gofs = 0;
        sofs = sdata + (RLEFT<<BCOLS_SHIFT);
        for(; sofs < leftover; sofs += (BCOLS*BSVY), gofs += istride)
            *((uint32_t*)&data[gofs]) = *((uint32_t*)&shared[sofs]);
    }

}

/// Use this if the image is lower than PAD_ROWS
static __global__ void a_transform_v_pad( DATATYPE* data, int width, int height, int stride )
{
    extern __shared__ DATATYPE shared[];  

    const unsigned int bid = blockIdx.x;    // slab (BCOLS columns)
    const unsigned int tidx = threadIdx.x<<1;   // column
    const unsigned int tidy = threadIdx.y;   // row
    const unsigned int swidth = min(width-(bid<<BCOLS_SHIFT), BCOLS); // Width of this slab, usually BCOLS but can be less

    // Element offset in global memory
    //int idata = tidx + (bid<<BCOLS_SHIFT) + __mul24(tidy, stride);
    data +=  tidx + (bid<<BCOLS_SHIFT) + __mul24(tidy, stride);
    const unsigned int istride = __mul24(BSVY, stride);
    const unsigned int sdata = tidx + ((tidy+RLEFT)<<BCOLS_SHIFT); // Does this get converted into a shift?
    // First read BROWS+RRIGHT
    // After that BROWS
    unsigned int leftover = (RLEFT+height) << BCOLS_SHIFT; /// How far to fill buffer on last read
    //unsigned int blocks = (height-RRIGHT)/BROWS;
    
    unsigned int gofs, sofs;
    
    /// Fill shared memory -- read next block of BROWS rows
    /// We can skip RRIGHT rows as we've already copied them for the previous block
    /// and moved them to the top
    if(tidx < swidth)
    {
        gofs = 0; // Read from row (cur+RRIGHT)
        sofs = sdata;
        for(; sofs < leftover; sofs += (BCOLS*BSVY), gofs += istride)
            *((uint32_t*)&shared[sofs]) = *((uint32_t*)&data[gofs]);
    }

    __syncthreads();
    
    doTransformTB(0, leftover);
    doTransformTB(1, leftover);
    
    __syncthreads();
    
    /// Write back leftover
    if(tidx < swidth)
    {
        gofs = 0;
        sofs = sdata;
        for(; sofs < leftover; sofs += (BCOLS*BSVY), gofs += istride)
            *((uint32_t*)&data[gofs]) = *((uint32_t*)&shared[sofs]);
    }
}

void cuda_iwt_fidelity(int16_t *d_data, int lwidth, int lheight, int stride)
{
    /** Invoke kernel */
    dim3 block_size;
    dim3 grid_size;
    int shared_size;

#ifdef HORIZONTAL
    block_size.x = BSH;
    block_size.y = 1;
    block_size.z = 1;
    grid_size.x = lheight;
    grid_size.y = 1;
    grid_size.z = 1;
    shared_size = (lwidth+BLEFT*2+BRIGHT*2) * sizeof(DATATYPE); 
    a_transform_h<<<grid_size, block_size, shared_size>>>(d_data, lwidth, stride);
#endif
#ifdef VERTICAL
    block_size.x = BSVX;
    block_size.y = BSVY;
    block_size.z = 1;
    grid_size.x = (lwidth+BCOLS-1)/BCOLS;
    grid_size.y = 1;
    grid_size.z = 1;
    shared_size = BCOLS*(BROWS+RLEFT+RRIGHT)*2; 
	
    if(lheight < PAD_ROWS)
        a_transform_v_pad<<<grid_size, block_size, shared_size>>>(d_data, lwidth, lheight, stride);
    else
        a_transform_v<<<grid_size, block_size, shared_size>>>(d_data, lwidth, lheight, stride);
#endif
}
