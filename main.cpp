//
//  main.cpp
//  Clouds
//
//  Created by Antoni Wójcik on 21/03/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#define RETINA
#define PROCESSING_UNIT 1 // 1 - AMD GRAPHCIS CARD

#define SCR_WIDTH 800
#define SCR_HEIGHT 800

#include <iostream>

// include OpenGL libraries
#include <GL/glew.h>
#define GLFW_COCOA_GRAPHICS_SWITCHING 0x00023003
#include <GLFW/glfw3.h>
#include <OpenGL/OpenGL.h>
#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"

#include "shader.h"
#include "screen.h"
#include "camera.h"
#include "compute_kernel.h"


// function declarations
void framebufferSizeCallback(GLFWwindow*, int, int);
void processInput(GLFWwindow*);
void mouseCallback(GLFWwindow*, double, double);
void mouseButtonCallback(GLFWwindow*, int, int, int);
void scrollCallback(GLFWwindow*, double, double);

#ifdef __APPLE__
void macWindowFix(GLFWwindow* window);

bool mac_fixed = false;
#endif

#ifdef RETINA
// dimensions of the viewport (they have to be multiplied by 2 at the retina displays)
unsigned int scr_width = SCR_WIDTH*2;
unsigned int scr_height = SCR_HEIGHT*2;
#else
unsigned int scr_width = SCR_WIDTH;
unsigned int scr_height = SCR_HEIGHT;
#endif

float scr_ratio = (float)scr_width/(float)scr_height;

// variables used in the main loop
float delta_time = 0.0f;
float last_frame_time = 0.0f;

// variables used in callbacks
bool mouse_first_check = true;
bool mouse_hidden = true;
float mouse_last_x = scr_width / 2.0f;
float mouse_last_y = scr_width / 2.0f;

// fps counter variables
float fps_sum = 0.0f;
const int fps_steps = 5;
int fps_steps_counter = 0;

// screenshot variable
bool taking_screenshot = false;

// camera pointer
Camera* camera_ptr;

// screen pointer
Screen* screen_ptr;


void processTime(float time) {
    delta_time = time - last_frame_time;
    last_frame_time = time;
    if(fps_steps_counter == fps_steps) {
        fps_steps_counter = 0;
        fps_sum = 0;
    }
    fps_sum += delta_time;
    fps_steps_counter++;
}

struct GLRendererInfo {
  GLint rendererID;       // RendererID number
  GLint accelerated;      // Whether Hardware accelerated
  GLint online;           // Whether renderer (/GPU) is onilne. Second GPU on Mac Pro is offline
  GLint virtualScreen;    // Virtual screen number
  GLint videoMemoryMB;
  GLint textureMemoryMB;
  const GLubyte *vendor;
};


int main(int argc, const char* argv[]) {
    // initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 0);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_COCOA_GRAPHICS_SWITCHING, GL_TRUE);
    #endif
    
    #ifdef RETINA
    GLFWwindow* window = glfwCreateWindow(scr_width/2, scr_height/2, "Clouds", NULL, NULL);
    #else
    GLFWwindow* window = glfwCreateWindow(scr_width, scr_height, "Clouds", NULL, NULL);
    #endif
    if(window == NULL) {
        std::cout << "ERROR: OpenGL: Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    #ifdef __APPLE__
    // Grab the GLFW context and pixel format for future calls
    CGLContextObj contextObject = CGLGetCurrentContext();
    CGLRendererInfoObj rend;
    GLint nRenderers = 0;
    CGLQueryRendererInfo(0xffffffff, &rend, &nRenderers);
    
    GLRendererInfo renderer;
    CGLDescribeRenderer(rend, PROCESSING_UNIT, kCGLRPOnline, &(renderer.online));
    CGLDescribeRenderer(rend, PROCESSING_UNIT, kCGLRPAcceleratedCompute, &(renderer.accelerated));
    CGLDescribeRenderer(rend, PROCESSING_UNIT, kCGLRPRendererID,  &(renderer.rendererID));
    CGLDescribeRenderer(rend, PROCESSING_UNIT, kCGLRPVideoMemoryMegabytes, &(renderer.videoMemoryMB));
    CGLDescribeRenderer(rend, PROCESSING_UNIT, kCGLRPTextureMemoryMegabytes, &(renderer.textureMemoryMB));

    CGLSetVirtualScreen(contextObject, PROCESSING_UNIT-1); //https://gist.github.com/dbarnes/94ded353e16a579ba3da52d2c6261173
    
    GLint r;
    CGLGetParameter(contextObject, kCGLCPCurrentRendererID, &r);
    renderer.vendor = glGetString(GL_VENDOR);
    
    std::cout << "SUCCESS: OpenGL: USING A DEVICE: Vendor: " << renderer.vendor << ", Video Memory MB: " << renderer.videoMemoryMB << " Texture Memory MB: " << renderer.textureMemoryMB << "\n";
    
    (void)glGetError();
    #endif
    
    // set the viewport size and apply changes every time it is changed by a user
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // tell GLFW to capture the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // initialise GLEW
    if(glewInit() != GLEW_OK) {
        std::cerr << "ERROR: OpenGL: Failed to initialize GLEW" << std::endl;
        glGetError();
        return -1;
    }
    
    Shader shader("src/shaders/screen_clouds.vs", "src/shaders/clouds_fast.fs");
    
    Screen screen("src/shaders/screen.vs", "src/shaders/screen.fs", SCR_WIDTH, SCR_HEIGHT);
    screen_ptr = &screen;
    
    Camera camera(60.0f, glm::vec3(0.5, 0.5, -2));
    camera_ptr = &camera;
    
    Clouds clouds(shader);

    clouds.transferData();
    
    //glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    while(!glfwWindowShouldClose(window)) {
        #ifdef __APPLE__
        macWindowFix(window);
        #endif
        
        float currentFrameTime = glfwGetTime();
        processTime(currentFrameTime);
        
        processInput(window);
        
        shader.use();
        
        camera.transferData(shader);
        
        shader.setFloat("time", currentFrameTime);
        
        screen.drawToBuffer(shader);
        screen.drawScreen(shader, scr_width, scr_height);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    scr_width = width;
    scr_height = height;
    scr_ratio = (float)width/(float)height;
    
    camera_ptr->setSize(scr_ratio);
}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera_ptr->move(FORWARD, delta_time);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera_ptr->move(BACK, delta_time);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera_ptr->move(LEFT, delta_time);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera_ptr->move(RIGHT, delta_time);
    
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera_ptr->setFasterSpeed(true);
    else if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera_ptr->setFasterSpeed(false);
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera_ptr->setSlowerSpeed(true);
    else if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera_ptr->setSlowerSpeed(false);
    
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        if(!taking_screenshot) screen_ptr->takeScreenshot(scr_width, scr_height);
        taking_screenshot = true;
    } else if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_RELEASE) {
        taking_screenshot = false;
    }
}

void mouseCallback(GLFWwindow* window, double x_pos, double y_pos) {
    if(mouse_first_check) {
        mouse_last_x = x_pos;
        mouse_last_y = y_pos;
        mouse_first_check = false;
    }
    
    float offset_x = x_pos - mouse_last_x;
    float offset_y = mouse_last_y - y_pos;
    
    mouse_last_x = x_pos;
    mouse_last_y = y_pos;
    
    if(mouse_hidden) camera_ptr->rotate(offset_x, offset_y);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        if(mouse_hidden) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        else glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        
        mouse_hidden = !mouse_hidden;
    }
}

void scrollCallback(GLFWwindow* window, double offset_x, double offset_y) {
    camera_ptr->zoom(offset_y);
}


//for some reason on Mac OS 10.14+ OpenGL window will only display black color until it is resized for the first time. This function does it automatically
#ifdef __APPLE__
void macWindowFix(GLFWwindow* window) {
    if(!mac_fixed) {
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        glfwSetWindowPos(window, ++x, y);
        
        glViewport(0, 0, scr_width, scr_height);
        mac_fixed = true;
    }
}
#endif
