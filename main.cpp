//
//  main.cpp
//  Clouds
//
//  Created by Antoni Wójcik on 21/03/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <random>
#include <cmath>
#include <vector>

#define __CL_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "cl2.hpp"

struct pos{
    int x, y, z;
    pos() {}
    pos(int xx, int yy, int zz) {
        x = xx;
        y = yy;
        z = zz;
    }
};

std::string load_source(const char* compute_path) {
    std::string compute_code;
    std::ifstream compute_file;
    compute_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        compute_file.open(compute_path);
        std::ostringstream compute_stream;
        compute_stream << compute_file.rdbuf();
        compute_file.close();
        compute_code = compute_stream.str();
    } catch(std::ifstream::failure e) {
        std::cerr << "ERROR: CANNOT READ COMPUTE CODE" << std::endl;
        exit(-1); //stop executing the program with the error code -1;
    }
    return compute_code;
}

int main(int argc, const char* argv[]) {
    int size = 100;
    int iterations = 5;
    int nodes[5] = {5, 10, 20, 25, 50};
    cl_float persistence[5] = {5.0f, 10.0f, 10.0f, 10.0f, 10.0f};
    cl_float blending[5] = {1.0f, 0.3f, 0.1f, 0.05f, 0.04f};
    
    float* image_result = new float[size*size*size];
    
    try {
        cl::Device device;
        std::vector<cl::Platform> platforms;
        std::vector<cl::Device> devices;
        cl::Platform::get(&platforms);
        if(platforms.size() == 0) {
            std::cerr << "ERROR: OPENCL: NO PLATFORMS FOUND" << std::endl;;
        }
        
        platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
        if(devices.size() == 0) {
            std::cerr << "ERROR: OPENCL: NO DEVICES FOUND" << std::endl;
        }
        device = devices[1]; //choose the graphics card
        
        std::cout << "SUCCESS: OPENCL: USING A DEVICE: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
        
        cl::Context context({device});
        cl::Program::Sources sources;
        std::string kernel_code = load_source("src/compute_gpu/generate_3d_cloud.ocl");
        sources.push_back({kernel_code.c_str(), kernel_code.length()});
        
        cl::Program computing_program(context, sources);
        if (computing_program.build({device}) != CL_SUCCESS)
        {
            std::cout << "ERROR: OPENCL: CANNOT BUILD PROGRAM " << computing_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
            return -1;
        }
        
        std::random_device dev;
        std::mt19937 rng(dev()); //random number generator
        
        cl::ImageFormat image_format(CL_R, CL_FLOAT);
        cl::Image3D image(context, CL_MEM_READ_WRITE, image_format, size, size, size); //CL_MEM_READ_WRITE
        
        cl::Buffer persistence_buff;
        cl::Buffer blending_buff;
        
        cl::ImageFormat image_in_format(CL_RGBA, CL_UNSIGNED_INT32);
        cl::Image3D vertices_image;
        
        cl::Kernel generate(computing_program, "generate");
        
        
        for(int k = 0; k < iterations; k++) {
            std::uniform_int_distribution<std::mt19937::result_type> distr(0,size/nodes[k]-1);
            
            int nodes_rep = nodes[k] + 2;
            
            pos*** v = new pos**[nodes_rep];
            for(int i = 0; i < nodes_rep; i++) {
                v[i] = new pos*[nodes_rep];
                for(int j = 0; j < nodes_rep; j++) {
                    v[i][j] = new pos[nodes_rep];
                }
            }
            
            for(int n = 1; n <= nodes[k]; n++) for(int j = 1; j <= nodes[k]; j++) for(int i = 1; i <= nodes[k]; i++) {
                v[i][j][n].x = distr(rng);
                v[i][j][n].y = distr(rng);
                v[i][j][n].z = distr(rng);
            }
            
            for(int n = 0; n < nodes_rep; n++) {
                for(int i = 0; i < nodes_rep; i++) {
                    int j = 1+(i-1+nodes[k])%nodes[k];
                    int u = 1+(n-1+nodes[k])%nodes[k];
                    v[i][0][n] =          v[j][nodes[k]][u];
                    v[i][nodes[k]+1][n] = v[j][1][u];
                    v[0][i][n] =          v[nodes[k]][j][u];
                    v[nodes[k]+1][i][n] = v[1][j][u];
                    v[i][n][0] =          v[j][u][nodes[k]];
                    v[i][n][nodes[k]+1] = v[j][u][1];
                }
            }
            
            
            cl_uint* vertices = new cl_uint[nodes_rep*nodes_rep*nodes_rep*4];
            for(int n = 0; n < nodes_rep; n++) for(int j = 0; j < nodes_rep; j++) for(int i = 0; i < nodes_rep; i++) {
                vertices[n*nodes_rep*nodes_rep*4 + j*nodes_rep*4 + i*4]     = v[i][j][n].x;
                vertices[n*nodes_rep*nodes_rep*4 + j*nodes_rep*4 + i*4 + 1] = v[i][j][n].y;
                vertices[n*nodes_rep*nodes_rep*4 + j*nodes_rep*4 + i*4 + 2] = v[i][j][n].y;
            }
            
            for(int i = 0; i < nodes_rep; i++) {
                for(int j = 0; j < nodes_rep; j++) {
                    delete [] v[i][j];
                }
                delete [] v[i];
            }
            delete [] v;
            
            
            vertices_image = cl::Image3D(context, CL_MEM_READ_ONLY, image_in_format, nodes_rep, nodes_rep, nodes_rep);
            persistence_buff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float));
            blending_buff = cl::Buffer(context, CL_MEM_READ_ONLY, sizeof(cl_float));
            
            generate.setArg(0, vertices_image);
            generate.setArg(1, image);
            generate.setArg(2, image);
            generate.setArg(3, blending[k]);
            generate.setArg(4, persistence[k]);
            
            cl::CommandQueue queue(context, device);
            
            queue.enqueueWriteImage(vertices_image, CL_TRUE, {0, 0, 0}, {size_t(nodes_rep), size_t(nodes_rep), size_t(nodes_rep)}, 0, 0, vertices);
            queue.enqueueWriteBuffer(blending_buff, CL_TRUE, 0, sizeof(cl_float), &blending[k]);
            queue.enqueueWriteBuffer(persistence_buff, CL_TRUE, 0, sizeof(cl_float), &persistence[k]);
            
            queue.enqueueNDRangeKernel(generate, cl::NullRange, cl::NDRange(size_t(size), size_t(size), size_t(size)), cl::NullRange);
            
            if(k == iterations-1) queue.enqueueReadImage(image, CL_TRUE, {0, 0, 0}, {size_t(size), size_t(size), size_t(size)}, 0, 0, image_result);

            queue.finish();
            
            delete [] vertices;
        }
        
    } catch(cl::Error e) {
        std::cerr << "ERROR: OPENCL: OTHER: " << e.what() << ": " << e.err() << std::endl;
        if(e.err() == -11) {
            std::cerr << "COMPILATION ERROR, CHECK THE SYNTAX\nUSE:\nhttps://streamhpc.com/blog/2013-05-13/verify-your-opencl-kernel-online/\nTO FIND ERRORS" << std::endl;
        } else std::cerr << "USE:\nhttps://www.khronos.org/registry/OpenCL/sdk/1.2/docs/man/xhtml/scalarDataTypes.html\nTO VERIFY ERROR TYPE" << std::endl;
        return -1;
    }
    
    for(int k = 0; k < size; k++) {
        std::string name("output/sim");
        if(k < 10) name += "00";
        else if(k < 100) name += "0";
        name += std::to_string(k) + ".ppm";
        std::ofstream img(name.c_str());
        img << "P3\n" << size << " " << size << "\n255\n";
        
        for(int j = 0; j < size; j++) for(int i = 0; i < size; i++) {
            int ir = int(255.99f*image_result[k*size*size + j*size + i]);
            int ig = int(255.99f*image_result[k*size*size + j*size + i]);
            int ib = int(255.99f*image_result[k*size*size + j*size + i]);
            
            img << ir << " " << ig << " " << ib << "\n";
        }

        img.close();
    }
    
    delete [] image_result;
    
    system("open output/sim000.ppm");
    
    //use "convert -delay 5 -loop 0 *.ppm  gif.gif" (ImageMagick) in terminal to generate gif from the files
    
    return 0;
}
