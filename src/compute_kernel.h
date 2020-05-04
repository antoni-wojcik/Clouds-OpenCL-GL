//
//  compute_kernel.h
//  Clouds
//
//  Created by Antoni Wójcik on 22/03/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#ifndef compute_kernel_h
#define compute_kernel_h

#define CHANNELS 4
#define ITERATIONS 3

#include <fstream>
#include <string>
#include <sstream>
#include <random>
#include <cmath>
#include <vector>

//include the OpenCL library (C++ binding)
#define __CL_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include "cl2.hpp"

//include OpenGL libraries
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Clouds {
private:
    const int size = 320;
    const int nodes[ITERATIONS][CHANNELS] = {
        {1, 3,  6,  32},
        {2, 10,  30, 128},
        {2, 40, 60, 200}
    };
    const cl_float persistence[ITERATIONS][CHANNELS] = {
        {15.0f, 15.0f, 15.0f, 15.0f},
        {10.0f, 10.0f, 10.0f, 10.0f},
        {5.0f, 5.0f, 5.0f, 5.0f}
    };
    const cl_float blending[ITERATIONS][CHANNELS] = {
        {1.0f, 1.0f, 1.0f, 1.0f},
        {0.3f, 0.3f, 0.3f, 0.3f},
        {0.1f, 0.1f, 0.1f, 0.1f}
    };
    
    cl_GLuint cloud_texture_ID;
    GLuint texture_loc;
    
    struct pos{
        int x, y, z;
        pos() {}
        pos(int xx, int yy, int zz) {
            x = xx;
            y = yy;
            z = zz;
        }
    };
    
    std::string loadSource(const char* compute_path) {
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
            std::cerr << "ERROR: OpenCL KERNEL: CANNOT READ KERNEL CODE" << std::endl;
            exit(-1); //stop executing the program with the error code -1;
        }
        return compute_code;
    }
    
    // FOR NOW JUST SEND DATA IN THE RED CHANNEL
    
    void generateGLTexture(Shader& shader) {
        
        glEnable(GL_TEXTURE_3D);
        
        glGenTextures(1, &cloud_texture_ID);
        
        glBindTexture(GL_TEXTURE_3D, cloud_texture_ID);
        
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT); //GL_CLAMP_TO_EDGE or GL_REPEAT
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); //USE NEAREST TO SPEED UP
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // FOR RGBA: glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, size, size, size, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RG, size, size, size, 0, GL_RED, GL_FLOAT, NULL);
        
        texture_loc = glGetUniformLocation(shader.ID, "density_sampler");
        
        glFinish();
    }
    
public:
    Clouds(Shader& shader) {
        cl::Device device;
        cl::Program computing_program;
        
        try {
            std::vector<cl::Platform> platforms;
            std::vector<cl::Device> devices;
            cl::Platform::get(&platforms);
            if(platforms.size() == 0) {
                std::cerr << "ERROR: OpenCL: NO PLATFORMS FOUND" << std::endl;;
            }
            
            platforms[0].getDevices(CL_DEVICE_TYPE_GPU, &devices);
            if(devices.size() == 0) {
                std::cerr << "ERROR: OpenCL: NO DEVICES FOUND" << std::endl;
            }
            device = devices[1]; //choose the graphics card
            
            std::cout << "SUCCESS: OpenCL: USING A DEVICE: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
            
            // OpenCl - OpenGL interop
            // https://stackoverflow.com/questions/26802905/getting-opengl-buffers-using-opencl
            
            CGLContextObj CGLGetCurrentContext(void);
            CGLShareGroupObj CGLGetShareGroup(CGLContextObj);

            CGLContextObj kCGLContext = CGLGetCurrentContext();
            CGLShareGroupObj kCGLShareGroup = CGLGetShareGroup(kCGLContext);

            cl_context_properties properties[] = {
              CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE,
              (cl_context_properties) kCGLShareGroup,
              0
            };
            
            // https://stackoverflow.com/questions/22928704/opencl-opengl-interop-using-clcreatefromgltexture-fails-to-draw-to-texture-text
            
            cl::Context context(device, properties);
            cl::Program::Sources sources;
            std::string kernel_code = loadSource("src/kernels/generate_3d_cloud.ocl");
            sources.push_back({kernel_code.c_str(), kernel_code.length()});
            
            computing_program = cl::Program(context, sources);
            if (computing_program.build({device}) != CL_SUCCESS) {
                std::cout << "ERROR: OpenCL: CANNOT BUILD PROGRAM " << computing_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
                exit(-1);
            }
            
            
            
            std::random_device dev;
            std::mt19937 rng(dev()); //random number generator
            
            // PREPARE THE CHANNEL DATA IMAGE
            
            cl::ImageFormat image_format(CL_RGBA, CL_FLOAT);
            cl::Image3D cloud_3D_data(context, CL_MEM_READ_WRITE, image_format, size, size, size);
            
            // CALCULATE CHANNEL DATA
            
            cl::ImageFormat image_in_format(CL_RGBA, CL_UNSIGNED_INT32);
            cl::Image3D vertices_image[4];
            
            cl::Kernel generate_channels(computing_program, "generate_channels");
            
            for(int m = 0; m < ITERATIONS; m++) {
                cl::CommandQueue queue(context, device);
            
                cl_uint** vertices = new cl_uint*[CHANNELS];
                
                for(int k = 0; k < CHANNELS; k++) {
                    std::uniform_int_distribution<std::mt19937::result_type> distr(0,size/nodes[m][k]-1);
                    
                    int nodes_rep = nodes[m][k] + 2;
                    
                    pos*** v = new pos**[nodes_rep];
                    for(int i = 0; i < nodes_rep; i++) {
                        v[i] = new pos*[nodes_rep];
                        for(int j = 0; j < nodes_rep; j++) {
                            v[i][j] = new pos[nodes_rep];
                        }
                    }
                    
                    for(int n = 1; n <= nodes[m][k]; n++) for(int j = 1; j <= nodes[m][k]; j++) for(int i = 1; i <= nodes[m][k]; i++) {
                        v[i][j][n].x = distr(rng);
                        v[i][j][n].y = distr(rng);
                        v[i][j][n].z = distr(rng);
                    }
                    
                    for(int n = 0; n < nodes_rep; n++) {
                        for(int i = 0; i < nodes_rep; i++) {
                            int j = 1+(i-1+nodes[m][k])%nodes[m][k];
                            int u = 1+(n-1+nodes[m][k])%nodes[m][k];
                            v[i][0][n]             = v[j][nodes[m][k]][u];
                            v[i][nodes[m][k]+1][n] = v[j][1][u];
                            v[0][i][n]             = v[nodes[m][k]][j][u];
                            v[nodes[m][k]+1][i][n] = v[1][j][u];
                            v[i][n][0]             = v[j][u][nodes[m][k]];
                            v[i][n][nodes[m][k]+1] = v[j][u][1];
                        }
                    }
                    
                    vertices[k] = new cl_uint[nodes_rep*nodes_rep*nodes_rep*4];
                    
                    for(int n = 0; n < nodes_rep; n++) for(int j = 0; j < nodes_rep; j++) for(int i = 0; i < nodes_rep; i++) {
                        vertices[k][n*nodes_rep*nodes_rep*4 + j*nodes_rep*4 + i*4]     = v[i][j][n].x;
                        vertices[k][n*nodes_rep*nodes_rep*4 + j*nodes_rep*4 + i*4 + 1] = v[i][j][n].y;
                        vertices[k][n*nodes_rep*nodes_rep*4 + j*nodes_rep*4 + i*4 + 2] = v[i][j][n].y;
                    }
                    
                    for(int i = 0; i < nodes_rep; i++) {
                        for(int j = 0; j < nodes_rep; j++) {
                            delete [] v[i][j];
                        }
                        delete [] v[i];
                    }
                    delete [] v;
                    
                    vertices_image[k] = cl::Image3D(context, CL_MEM_READ_ONLY, image_in_format, nodes_rep, nodes_rep, nodes_rep);
                    generate_channels.setArg(k, vertices_image[k]);
                    queue.enqueueWriteImage(vertices_image[k], CL_TRUE, {0, 0, 0}, {size_t(nodes_rep), size_t(nodes_rep), size_t(nodes_rep)}, 0, 0, vertices[k]);
                }
                
                for(int k = 0; k < CHANNELS; k++) {
                    delete [] vertices[k];
                }
                delete [] vertices;
                    
                cl::Buffer persistence_buff(context, CL_MEM_READ_ONLY, sizeof(cl_float)*CHANNELS);
                cl::Buffer blending_buff(context, CL_MEM_READ_ONLY, sizeof(cl_float)*CHANNELS);
                    
                generate_channels.setArg(CHANNELS, persistence_buff);
                generate_channels.setArg(CHANNELS+1, blending_buff);
                generate_channels.setArg(CHANNELS+2, cloud_3D_data);
                generate_channels.setArg(CHANNELS+3, cloud_3D_data);
                    
                queue.enqueueWriteBuffer(persistence_buff, CL_TRUE, 0, sizeof(cl_float)*CHANNELS, persistence[m]);
                queue.enqueueWriteBuffer(blending_buff, CL_TRUE, 0, sizeof(cl_float)*CHANNELS, blending[m]);
                    
                queue.enqueueNDRangeKernel(generate_channels, cl::NullRange, cl::NDRange(size_t(size), size_t(size), size_t(size)), cl::NullRange);
                    
                // no need to do it as the GPU memory is shared between OpenCL and OpenGL now
                //if(m == ITERATIONS-1) queue.enqueueReadImage(cloud_3D_data, CL_TRUE, {0, 0, 0}, {size_t(size), size_t(size), size_t(size)}, 0, 0, image_result);
                
                queue.finish();
            }
            
            
            // CALCULATE DENSITY AND LIGHT DATA
            
            generateGLTexture(shader);
            cl::ImageGL image(context, CL_MEM_READ_WRITE, GL_TEXTURE_3D, 0, cloud_texture_ID);
            
            cl::Kernel generate_density(computing_program, "generate_density");
            
            cl::CommandQueue queue(context, device);
            
            generate_density.setArg(0, cloud_3D_data);
            generate_density.setArg(1, image);
            queue.enqueueNDRangeKernel(generate_density, cl::NullRange, cl::NDRange(size_t(size), size_t(size), size_t(size)), cl::NullRange);
            
            queue.finish();
            
        } catch(cl::Error e) {
            std::cerr << "ERROR: OpenCL: OTHER: " << e.what() << ": " << e.err() << std::endl;
            if(e.err() == CL_BUILD_PROGRAM_FAILURE) {
                std::cerr << "ERROR: OpenCL: CANNOT BUILD PROGRAM: " << computing_program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << std::endl;
            } else std::cerr << "USE:\nhttps://streamhpc.com/blog/2013-04-28/opencl-error-codes\nTO VERIFY ERROR TYPE" << std::endl;
            exit(-1);
        }
    }
    
    void transferData() {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_3D, cloud_texture_ID);
        glUniform1i(texture_loc, 0);
    }
    
};

#endif /* compute_kernel_h */
