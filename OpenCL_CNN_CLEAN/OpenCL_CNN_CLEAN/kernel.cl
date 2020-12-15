__kernel void Conv2D( __global float * image_in,  //image input
                                int image_ch,
                      __global float * filter_in, //filter input
                                int K,            //filter kernel size
                                int active_fn,
                      __global float * image_out) //feature map output
{
    int W;       //work group global size
    int H;
    int out_dim;
    int im_ch;
    int Wn;      //padded image width
    int Hn;
    int x;         //global id x
    int y;         //global id y
    int ki, kj;     //filter coordinate,(kj, ki)

    float sum = 0; //multiply and sum of filter and data

    W = get_global_size(0);
    H = get_global_size(1);
    //pixel-x
    x = get_global_id(0);
    //pixel-y
    y = get_global_id(1);
    //color-plane
    out_dim = get_global_id(2);
    
    Wn = W + (K - 1);
    Hn = H + (K - 1);
    //image_depth
    for(im_ch=0;im_ch<image_ch;++im_ch){
        //filter
        for(ki=0; ki<K; ++ki){
            for(kj=0; kj<K; ++kj)
                {
                    sum  = sum + filter_in[out_dim*K*K+ki*K + kj] * image_in[im_ch*Wn*Hn+Wn*(y+ki) + x + kj];
                }
            }
    }
    if(active_fn==0){
        image_out[out_dim*W*H+y*W + x] = sum;
    }
    else if(active_fn==1){
        image_out[out_dim*W*H+y*W + x] = (sum<0)?0:sum;
    }
    
}

__kernel void Max_pool_2D( __global float * image_in,  //image input
                          int K,
                          __global float * image_out,
                          int ori_x,
                          int ori_y) //feature map output
{
    int W;       //work group global size
    int H;
    int Wn;      //padded image width
    int Hn;
    int x;         //global id x
    int y;         //global id y
    int ki, kj;     //filter coordinate,(kj, ki)
    float max; //max in windows
    int out_dim;
    W = get_global_size(0);
    H = get_global_size(1);
    
    x = get_global_id(0);
    //pixel-x
    y = get_global_id(1);
    //pixel-y
    out_dim = get_global_id(2);
    //color-plane
    Wn = ori_x;
    Hn = ori_y;
    
    //first_element
    max = image_in[out_dim*Wn*Hn +Wn*(K*y) + K*x];
        //filter
    //printf("output[%d]:",out_dim*W*H +W*(y) + x);
    
    for(ki=0; ki<K; ++ki){
        for(kj=0; kj<K; ++kj)
        {
            if((x*K + kj<Wn) && (y*K+ki<Hn)){
                if(image_in[out_dim*Wn*Hn +Wn*(K*y+ki) + K*x + kj]>max){
                    max = image_in[out_dim*Wn*Hn +Wn*(K*y+ki) + K*x + kj];
                }
                //printf("[%d][%d]:%f\t",(K*y+ki),K*x + kj,image_in[out_dim*Wn*Hn +Wn*(K*y+ki) + K*x + kj]);
            }
        }
    }
    //printf(" and max is %f\n",max);
    
    image_out[out_dim*W*H +y*W + x] = max;

}
__kernel void Global_max_pool( __global float * image_in,  //image input
                              int H,
                              int W,
                              __global float * image_out
                              ) //feature map output
{

    int x;         //global id x
    int y;         //global id y
    int out_dim;
    float max; //max in windows
    out_dim = get_global_id(2);
    max = image_in[out_dim*W*H];
    for(y=0;y<H;++y){
        for(x=0;x<W;++x){
            if(image_in[out_dim*W*H + y*W + x]>max){
                max =image_in[out_dim*W*H + y*W + x];
            }
        }
    }
    
    image_out[out_dim] = max;

}
__kernel void Global_avg_pool( __global float * image_in,  //image input
                              int H,
                              int W,
                              __global float * image_out
                              ) //feature map output
{
    int x;         //global id x
    int y;         //global id y
    int out_dim;
    float sum; //max in windows
    out_dim = get_global_id(2);
    sum =0;
    for(y=0;y<H;++y){
        for(x=0;x<W;++x){

            sum +=image_in[out_dim*W*H + y*W + x];

        }
    }
    printf("avg_global_2d sum is:%f\n",sum);
    image_out[out_dim] = sum/(W*H);

}
__kernel void Avg_pool_2D( __global float * image_in,  //image input
                       int K,
                      __global float * image_out,
                          int ori_x,
                          int ori_y) //feature map output
{
    int W;       //work group global size
    int H;
    int Wn;      //padded image width
    int Hn;
    int x;         //global id x
    int y;         //global id y
    int ki, kj;     //filter coordinate,(kj, ki)
    int out_dim;
    float sum; //max in windows
    W = get_global_size(0);
    H = get_global_size(1);
    
    x = get_global_id(0);
    //pixel-x
    y = get_global_id(1);
    //pixel-y
    out_dim = get_global_id(2);
    //color-plane
    Wn = ori_x;
    Hn = ori_y;
    
    //sum initialized
    sum = 0;
    //total point
    int counter =0;
        //filter
    //printf("output[%d]:",out_dim*W*H +W*(y) + x);
    for(ki=0; ki<K; ++ki){
        for(kj=0; kj<K; ++kj)
        {
            if((x*K + kj<Wn) && (y*K+ki<Hn)){
                sum += image_in[out_dim*Wn*Hn +Wn*(K*y+ki) + K*x + kj];
                //printf("[%d][%d]:%f\t",(K*y+ki),K*x + kj,image_in[out_dim*Wn*Hn +Wn*(K*y+ki) + K*x + kj]);
                counter+=1;
            }
        }
    }
    //printf(" and sum is %f\n",sum);
    
    image_out[out_dim*W*H +y*W + x] = sum/counter;

}
