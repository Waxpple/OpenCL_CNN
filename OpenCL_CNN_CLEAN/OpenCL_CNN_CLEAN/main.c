//
//  main.c
//  conv2d
//
//  Created by 劉獻章 on 2020/11/30.
//
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <OpenCL/opencl.h>
typedef unsigned int int32;
typedef short int16;
typedef unsigned char byte;
#define DATA_OFFSET_OFFSET 0x000A
#define WIDTH_OFFSET 0x0012
#define HEIGHT_OFFSET 0x0016
#define BITS_PER_PIXEL_OFFSET 0x001C
#define HEADER_SIZE 14
#define INFO_HEADER_SIZE 40
#define NO_COMPRESION 0
#define MAX_NUMBER_OF_COLORS 0
#define ALL_COLORS_REQUIRED 0
void ReadImage(const char *fileName,byte **pixels, int32 *width, int32 *height, int32 *bytesPerPixel)
{
        FILE *imageFile = fopen(fileName, "rb");
        int32 dataOffset;
        fseek(imageFile, DATA_OFFSET_OFFSET, SEEK_SET);
        fread(&dataOffset, 4, 1, imageFile);
        fseek(imageFile, WIDTH_OFFSET, SEEK_SET);
        fread(width, 4, 1, imageFile);
        fseek(imageFile, HEIGHT_OFFSET, SEEK_SET);
        fread(height, 4, 1, imageFile);
        int16 bitsPerPixel;
        fseek(imageFile, BITS_PER_PIXEL_OFFSET, SEEK_SET);
        fread(&bitsPerPixel, 2, 1, imageFile);
        *bytesPerPixel = ((int32)bitsPerPixel) / 8;
 
        int paddedRowSize = (int)(4 * ceil((float)(*width*(*bytesPerPixel)) / 4.0f));
        int unpaddedRowSize = (*width)*(*bytesPerPixel);
        int totalSize = unpaddedRowSize*(*height);
        *pixels = (byte*)malloc(totalSize);
        int i = 0;
        byte *currentRowPointer = *pixels+((*height-1)*unpaddedRowSize);
        for (i = 0; i < *height; i++)
        {
            fseek(imageFile, dataOffset+(i*paddedRowSize), SEEK_SET);
            fread(currentRowPointer, 1, unpaddedRowSize, imageFile);
            currentRowPointer -= unpaddedRowSize;
        }
 
        fclose(imageFile);
}
void WriteImage(const char *fileName, byte *pixels, int32 width, int32 height,int32 bytesPerPixel)
{
        FILE *outputFile = fopen(fileName, "wb");
        //*****HEADER************//
        const char *BM = "BM";
        fwrite(&BM[0], 1, 1, outputFile);
        fwrite(&BM[1], 1, 1, outputFile);
        int paddedRowSize = (int)(4 * ceil((float)width*bytesPerPixel/4.0f));
        int32 fileSize = paddedRowSize*height + HEADER_SIZE + INFO_HEADER_SIZE;
        fwrite(&fileSize, 4, 1, outputFile);
        int32 reserved = 0x0000;
        fwrite(&reserved, 4, 1, outputFile);
        int32 dataOffset = HEADER_SIZE+INFO_HEADER_SIZE;
        fwrite(&dataOffset, 4, 1, outputFile);
 
        //*******INFO*HEADER******//
        int32 infoHeaderSize = INFO_HEADER_SIZE;
        fwrite(&infoHeaderSize, 4, 1, outputFile);
        fwrite(&width, 4, 1, outputFile);
        fwrite(&height, 4, 1, outputFile);
        int16 planes = 1; //always 1
        fwrite(&planes, 2, 1, outputFile);
        int16 bitsPerPixel = bytesPerPixel * 8;
        fwrite(&bitsPerPixel, 2, 1, outputFile);
        //write compression
        int32 compression = NO_COMPRESION;
        fwrite(&compression, 4, 1, outputFile);
        //write image size (in bytes)
        int32 imageSize = width*height*bytesPerPixel;
        fwrite(&imageSize, 4, 1, outputFile);
        int32 resolutionX = 11811; //300 dpi
        int32 resolutionY = 11811; //300 dpi
        fwrite(&resolutionX, 4, 1, outputFile);
        fwrite(&resolutionY, 4, 1, outputFile);
        int32 colorsUsed = MAX_NUMBER_OF_COLORS;
        fwrite(&colorsUsed, 4, 1, outputFile);
        int32 importantColors = ALL_COLORS_REQUIRED;
        fwrite(&importantColors, 4, 1, outputFile);
        int i = 0;
        int unpaddedRowSize = width*bytesPerPixel;
        for ( i = 0; i < height; i++)
        {
                int pixelOffset = ((height - i) - 1)*unpaddedRowSize;
                fwrite(&pixels[pixelOffset], 1, paddedRowSize, outputFile);
        }
        fclose(outputFile);
}
void bmp_padding(byte *ori, byte *tar, int32 xsize,int32 ysize,int32 pixel_perbyte,int K){
    for(int y=0;y!=ysize;++y){
        for(int x=0;x!=xsize;++x){
            //R
            *(tar + pixel_perbyte*((y+1)*(xsize+K-1) + (x+1))+2)= *(ori + pixel_perbyte*(y*xsize + x)+2);
            //G
            *(tar + pixel_perbyte*((y+1)*(xsize+K-1) + (x+1))+1)= *(ori + pixel_perbyte*(y*xsize + x)+1);
            //B
            *(tar + pixel_perbyte*((y+1)*(xsize+K-1) + (x+1))+0)= *(ori + pixel_perbyte*(y*xsize + x)+0);
        }
    }
    //填補第一行與最後一行
    for(int x=0;x<xsize+2;++x){
        *(tar + (pixel_perbyte*(x)+2)) = 0;
        *(tar + (pixel_perbyte*(x)+1)) = 0;
        *(tar + (pixel_perbyte*(x)+0)) = 0;
        *(tar + (pixel_perbyte*((ysize+1)*(xsize+K-1)+x)+2)) = 0;
        *(tar + (pixel_perbyte*((ysize+1)*(xsize+K-1)+x)+1)) = 0;
        *(tar + (pixel_perbyte*((ysize+1)*(xsize+K-1)+x)+0)) = 0;
    }
    //補第一列與最後一列
    for(int y=1;y<ysize+1;++y){

        *(tar + (pixel_perbyte*(y*(xsize+K-1))+2)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1))+1)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1))+0)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1)+xsize+1)+2)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1)+xsize+1)+1)) = 0;
        *(tar + (pixel_perbyte*(y*(xsize+K-1)+xsize+1)+0)) = 0;
    }
}
void read_file(char **program_file,size_t *program_size, char *root){
    FILE *program_handle;               // for read source file
    program_handle = fopen(root,"r");
    fseek(program_handle,0,SEEK_END);
    *program_size = ftell(program_handle);
    rewind(program_handle);
    *program_file = (char *)malloc(*program_size +1);
    fread(*program_file,sizeof(char),*program_size,program_handle);
    fclose(program_handle);
}


int main(int argc, const char * argv[]) {
   
    size_t global_item_size[3];                      // global domain size for our calculation
    size_t local_item_size[3];                       // local domain size for our calculation
    cl_uint work_dim;// working dimension
    cl_device_id *device;
    cl_context context;                 // compute context
    cl_command_queue command_queue;          // compute command queue
    //kernel variable section
    cl_kernel kernel;                   // compute kernel
    cl_kernel kernel_2,kernel_3,kernel_4;
    //data buffer section
    cl_mem data_in;
    cl_mem data_out;                      // device memory used for the input array
    cl_mem data_out_2,data_out_3,data_out_4;
    //dim buffer section
    cl_mem dim_buffer_0;                    //dimension
    cl_mem dim_buffer_1;                    //dimension
    //filter buffer section
    cl_mem filter_in_0;                      // device memory used for the output array

    cl_program program_cl; //for program
    cl_uint num_device;
    cl_event event;         //for calculate time
    cl_platform_id *platform;   //for platform
    cl_uint num_platform;   //for multi-platform
    
    
    int err;                            //for error
    char *program_file;
    size_t program_size;
    int const K = 3;            //filter kernel size
    
    
    int const filter_ch = 3;
    float filter_coe_0[K*K*filter_ch] = { -1,0,1,-2,0,2,-1,0,1,-1,0,1,-2,0,2,-1,0,1,-1,0,1,-2,0,2,-1,0,1 }; //sobel filter: horizontal gradient
    
    //relu
    int active_fn = 1;
    
    byte *pixels;
    int32 width;
    int32 height;
    int32 bytesPerPixel;
    
    ReadImage("/Users/ethan/Downloads/Xcode/read_image/read_image/2.bmp", &pixels, &width, &height,&bytesPerPixel);
    int const W =(int) width;            //image width
    int const H =(int) height;            //image height
    int const Wn = (W + K - 1); //padded image width
    int const Hn = (H + K - 1); //padded image height
    int const image_ch = 3;     //image channel
    float *image_input;
    image_input = (float*)malloc(sizeof(float)*Wn*Hn*image_ch);
    
    int const K_maxpool=2;
    int const ori_x = W;
    int const ori_y = H;
    
    float *result;
    result = (float*)malloc(W*H*sizeof(float)* image_ch);
    
    float *result2;
    result2 = (float*)malloc((int)ceil((float)(W)/2) * (int)ceil((float)(H)/2)*filter_ch*sizeof(float));
    
    byte *padded_image;
    padded_image = (byte*)malloc(Wn*Hn*sizeof(byte)*bytesPerPixel);
    
    bmp_padding(pixels, padded_image, W, H, bytesPerPixel, K);
    
    WriteImage("/Users/ethan/Downloads/Xcode/OpenCL_CNN_CLEAN/OpenCL_CNN_CLEAN/padding.bmp", padded_image, Wn, Hn, bytesPerPixel);
    //map bmp into RGB array channel last and R-G-B order
    for(int y=0;y<Hn;y++){
        for(int x=0;x<Wn;x++){
            //R
            *(image_input + Wn*Hn*0 + y*Wn+x) =  (int) *(padded_image + bytesPerPixel*(y*Wn+x)+2);
            //G
            *(image_input + Wn*Hn*1 + y*Wn+x) =  (int) *(padded_image + bytesPerPixel*(y*Wn+x)+1);
            //B
            *(image_input + Wn*Hn*2 + y*Wn+x) =  (int) *(padded_image + bytesPerPixel*(y*Wn+x)+0);
        }
    }
    //read opencl kernel out
    read_file(&program_file, &program_size, "/Users/ethan/Downloads/Xcode/OpenCL_CNN_CLEAN/OpenCL_CNN_CLEAN/kernel.cl");
    
    int gpu = 0;
    //get number of platform
    err = clGetPlatformIDs(0, 0, &num_platform);
    //set fix size array
    platform = (cl_platform_id *)malloc(sizeof(cl_platform_id) * num_platform);
    //get platform ids
    err = clGetPlatformIDs(num_platform, platform, NULL);
    //get device number
    err = clGetDeviceIDs(platform[0], gpu?CL_DEVICE_TYPE_GPU:CL_DEVICE_TYPE_CPU, 0, NULL, &num_device);
    device = (cl_device_id *)malloc(sizeof(cl_device_id) * num_device);
    err = clGetDeviceIDs(platform[0], gpu?CL_DEVICE_TYPE_GPU:CL_DEVICE_TYPE_CPU, num_device, device, NULL);
    printf("Now using %s\n",gpu?"gpu":"cpu");
    
    if (err != CL_SUCCESS)
    {
        printf("Error: Failed to create a device group!\n");
        return EXIT_FAILURE;
    }
    
    // Create a compute context
    //
    context = clCreateContext(0, 1, device, NULL, NULL, &err);
    
    if (!context)
    {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }
    
    // Create a command commands
    //
    command_queue = clCreateCommandQueue(context, device[0], CL_QUEUE_PROFILING_ENABLE, &err);
    if (!command_queue)
    {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }
    program_cl = clCreateProgramWithSource(context, 1, (const char **) &program_file, NULL, &err);
    err = clBuildProgram(program_cl, 1, &device[0], NULL, NULL, NULL);
    if (err != CL_SUCCESS)
    {
        size_t len;
        char buffer[4096];
        
        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(program_cl, device[1], CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);
        printf("%s\n", buffer);
        exit(1);
    }

    
    //kernel section
    kernel = clCreateKernel(program_cl, "Conv2D", &err);
    //kernel_2 = clCreateKernel(program_cl, "Conv2D", &err);
    kernel_3 = clCreateKernel(program_cl, "Avg_pool_2D", &err);
    kernel_4 = clCreateKernel(program_cl, "Global_avg_pool", &err);
    
    //dimension buffer section
    dim_buffer_0 = clCreateBuffer(context,CL_MEM_READ_WRITE,3 * sizeof(int),NULL,&err);
    dim_buffer_1 = clCreateBuffer(context,CL_MEM_READ_WRITE,3 * sizeof(int),NULL,&err);
    
    //data buffer
    
    data_in = clCreateBuffer(context, CL_MEM_READ_WRITE, Wn*Hn * image_ch *sizeof(float), NULL, &err);
    //after conv2d
    data_out = clCreateBuffer(context, CL_MEM_READ_WRITE, W*H * filter_ch * sizeof(float), NULL, &err);
    //after conv2d_n
    data_out_2 = clCreateBuffer(context, CL_MEM_READ_WRITE, (W-2)*(H-2) * filter_ch * sizeof(float), NULL, &err);
    //after avg_pool
    data_out_3 = clCreateBuffer(context, CL_MEM_READ_WRITE, (int)ceil((float)(W)/2) * (int)ceil((float)(H)/2)*filter_ch*sizeof(float), NULL, &err);
    //after global avg pool
    data_out_4 = clCreateBuffer(context, CL_MEM_READ_WRITE, filter_ch*sizeof(float), NULL, &err);

    //filter buffer
    filter_in_0 = clCreateBuffer(context, CL_MEM_READ_WRITE, K*K*filter_ch * sizeof(float), NULL, &err);
    
    //write image into buffer
    err = clEnqueueWriteBuffer(command_queue, data_in, CL_TRUE, 0, Wn*Hn * image_ch * sizeof(float), image_input, 0, NULL, NULL);
    //write first conv2d filter
    err = clEnqueueWriteBuffer(command_queue, filter_in_0, CL_TRUE, 0, K*K *filter_ch* sizeof(float), filter_coe_0, 0, NULL, NULL);

    //arg-kernel
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&data_in);
    err = clSetKernelArg(kernel, 1, sizeof(int), (void *)&image_ch);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&filter_in_0);
    err = clSetKernelArg(kernel, 3, sizeof(int), (void *)&K);
    err = clSetKernelArg(kernel, 4, sizeof(int), (void *)&active_fn);
    err = clSetKernelArg(kernel, 5, sizeof(cl_mem), (void *)&data_out);
    
    //arg-kernel_3
    err = clSetKernelArg(kernel_3, 0, sizeof(cl_mem), (void *)&data_out);
    err = clSetKernelArg(kernel_3, 1, sizeof(int), (void *)&K_maxpool);
    err = clSetKernelArg(kernel_3, 2, sizeof(cl_mem), (void *)&data_out_3);
    err = clSetKernelArg(kernel_3, 3, sizeof(int), (void *)&ori_x);
    err = clSetKernelArg(kernel_3, 4, sizeof(int), (void *)&ori_y);
    
    
    work_dim = 3;
    //global work size
    global_item_size[0] = W;
    global_item_size[1] = H;
    global_item_size[2] = filter_ch;
    //local work size
    local_item_size[0] = 1;
    local_item_size[1] = 1;
    local_item_size[2] = 1;
    
    err = clEnqueueNDRangeKernel(command_queue, kernel, work_dim, NULL,
            global_item_size, local_item_size, 0, NULL, &event);

    
    global_item_size[0] = (int)ceil((float)(W)/2);
    global_item_size[1] = (int)ceil((float)(H)/2);
    global_item_size[2] = 3;
    local_item_size[0] = 1;
    local_item_size[1] = 1;
    local_item_size[2] = 1;
    err = clEnqueueNDRangeKernel(command_queue, kernel_3, work_dim, NULL,
            global_item_size, local_item_size, 0, NULL, &event);
    
    clFinish(command_queue);

    err = clEnqueueReadBuffer(command_queue, data_out, CL_TRUE,  0,
            W*H * filter_ch * sizeof(float), result, 0, NULL, NULL);

    err = clEnqueueReadBuffer(command_queue, data_out_3, CL_TRUE, 0, (int)ceil((float)(W)/2) * (int)ceil((float)(H)/2)*filter_ch*sizeof(float), result2, 0, NULL, NULL);
    
    byte *export;
    export = (byte*)malloc(W*H*bytesPerPixel);
    for(int y=0;y<H;y++){
        for(int x=0;x<W;x++){
                
            //R
            *(export + bytesPerPixel*(y*W+x)+2) = (byte) *(result + W*H*0 + y*W+x);
            //G
            *(export + bytesPerPixel*(y*W+x)+1) = (byte) *(result + W*H*1 + y*W+x);
            //B
            *(export + bytesPerPixel*(y*W+x)+0) = (byte) *(result + W*H*2 + y*W+x);

            *(export + bytesPerPixel*(y*W+x)+3) =  (byte) 255;
    
        }
    }
    
    WriteImage("/Users/ethan/Downloads/Xcode/OpenCL_CNN_CLEAN/OpenCL_CNN_CLEAN/2_m.bmp", export, W, H, bytesPerPixel);

    byte *export2;
    int export2_h = (int)ceil((float)(H)/2);
    int export2_w = (int)ceil((float)(W)/2);
    
    export2 = (byte*)malloc(export2_h*export2_w*bytesPerPixel);
    for(int y=0;y<export2_h;y++){
        for(int x=0;x<export2_w;x++){
                
            //R
            *(export2 + bytesPerPixel*(y*export2_w+x)+2) = (byte) *(result2 + export2_w*export2_h*0 + y*export2_w+x);
            //G
            *(export2 + bytesPerPixel*(y*export2_w+x)+1) = (byte) *(result2 + export2_w*export2_h*1 + y*export2_w+x);
            //B
            *(export2 + bytesPerPixel*(y*export2_w+x)+0) = (byte) *(result2 + export2_w*export2_h*2 + y*export2_w+x);

            *(export2 + bytesPerPixel*(y*export2_w+x)+3) =  (byte) 255;
    
        }
    }
    
    WriteImage("/Users/ethan/Downloads/Xcode/OpenCL_CNN_CLEAN/OpenCL_CNN_CLEAN/avgpool.bmp", export2, export2_w, export2_h, bytesPerPixel);
    
    cl_ulong time_start;
    cl_ulong time_end;

    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(time_start), &time_start, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(time_end), &time_end, NULL);

    double nanoSeconds = time_end-time_start;
    printf("OpenCl Execution time is: %.3f micoseconds \n",nanoSeconds / 1000.0);

    err = clReleaseKernel(kernel);
    err = clReleaseKernel(kernel_3);

    err = clReleaseProgram(program_cl);
    err = clReleaseMemObject(data_in);
    err = clReleaseMemObject(data_out);
    err = clReleaseMemObject(filter_in_0);

    err = clReleaseCommandQueue(command_queue);
    err = clReleaseContext(context);
    free(result);
    free(result2);
    
    free(export);
    free(export2);
    free(image_input);
    free(pixels);
    free(device);
    //system("read");
    return 0;
}
