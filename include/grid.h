//
// @author raver119@gmail.com
//
//
#ifndef LIBND4J_GRID_H
#define LIBND4J_GRID_H

// number of pointers within single grid row
#define GRID_WIDTH 19

namespace functions {
    namespace meta {

        template<typename T>
        class MetaTransform {
        public:
            template<typename OpTypeA, typename OpTypeB>
            static inline __device__ void transformCuda(
                    Nd4jIndex n,
                    T *dy,
                    int incy,
                    T *paramsA,
                    T *paramsB,
                    T *result,
                    int resultStride) {

                int totalThreads = gridDim.x * blockDim.x;
                int tid = blockIdx.x * blockDim.x + threadIdx.x;

                Nd4jIndex i = tid;
                if (incy == 1 && resultStride == 1) {

                    for (; i < n; i += totalThreads) {
                        result[i] = OpTypeB::op(OpTypeA::op(dy[i], paramsA), paramsB);
                    }
                } else {

                    for (; i < n; i += totalThreads) {
                        result[i * resultStride] = OpTypeB::op(OpTypeA::op(dy[i * incy], paramsA), paramsB);
                    }
                }
            }

            static inline __device__ void processMetaLinear(const int opTypeA, const int opNumA, const int opTypeB, const int opNumB,
                                                            Nd4jIndex n,
                                                            T *dy,
                                                            int incy,
                                                            T *paramsA,
                                                            T *paramsB,
                                                            T *result,
                                                            int resultStride) {
                if (opTypeA == 0) {
                    transformCuda<simdOps::Add <T>, simdOps::Abs <T>> (n, dy, incy, paramsA, paramsB, result, resultStride);
                } else if (opTypeA == 1) {
                    transformCuda<simdOps::Abs <T>, simdOps::Add <T>> (n, dy, incy, paramsB, paramsA, result, resultStride);
                }
            }
        };
    }
}

template <typename T>
__device__ inline void metaStridedGeneric(const int opTypeA, const int opNumA, const int opTypeB, const int opNumB,
                                          Nd4jIndex n,
                                          T dx,
                                          T *dy,
                                          int incy,
                                          T *params,
                                          T *result,
                                          int resultStride) {

    /*
    __shared__ T paramsA[1];
    if (threadIdx.x == 0)
        paramsA[0] = dx;
    __syncthreads();
*/
    functions::meta::MetaTransform<T>::processMetaLinear(opTypeA, opNumA, opTypeB, opNumB, n, dy, incy, &dx, params, result, resultStride);
}


__global__ void metaStridedFloat(const int opTypeA, const int opNumA, const int opTypeB, const int opNumB,
                                 Nd4jIndex n,
                                 float dx,
                                 float *dy,
                                 int incy,
                                 float *paramsB,
                                 float *result,
                                 int resultStride) {


    metaStridedGeneric<float>(opTypeA, opNumA, opTypeB, opNumB, n, dx, dy, incy, paramsB, result, resultStride);
}

#endif //LIBND4J_GRID_H