//
//  main.cpp
//  Clouds
//
//  Created by Antoni Wójcik on 21/03/2020.
//  Copyright © 2020 Antoni Wójcik. All rights reserved.
//

#define RETINA
#define SCR_WIDTH 900
#define SCR_HEIGHT 900

#include <iostream>

// include OpenGL libraries
#include <glad/glad.h>
#include <GLFW/glfw3.h>
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

//screenshot variable
bool taking_screenshot = false;

//camera
Camera camera(60.0f, glm::vec3(0, 0, -2));


int main(int argc, const char* argv[]) {
    // initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES, 4);
    
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
    
    // set the viewport size and apply changes every time it is changed by a user
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, scrollCallback);
    
    // tell GLFW to capture the mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // initalize GLAD
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "ERROR: OpenGL: Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    Shader shader("src/shaders/screen.vs", "src/shaders/clouds.fs");
    
    Screen screen;
    
    Clouds clouds(shader);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    while(!glfwWindowShouldClose(window)) {
        #ifdef __APPLE__
        macWindowFix(window);
        #endif
        
        float currentFrameTime = glfwGetTime();
        delta_time = currentFrameTime - last_frame_time;
        last_frame_time = currentFrameTime;
        if(fps_steps_counter == fps_steps) {
            fps_steps_counter = 0;
            fps_sum = 0;
        }
        fps_sum += delta_time;
        fps_steps_counter++;
        
        processInput(window);
        
        camera.transferData(shader);

        clouds.transferData();
        
        shader.setFloat("time", currentFrameTime);
        
        screen.draw(shader);
        
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
    
    camera.setSize(scr_ratio);
}

void processInput(GLFWwindow* window) {
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.move(FORWARD, delta_time);
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.move(BACK, delta_time);
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.move(LEFT, delta_time);
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.move(RIGHT, delta_time);
    
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camera.setFasterSpeed(true);
    else if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera.setFasterSpeed(false);
    if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) camera.setSlowerSpeed(true);
    else if(glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_RELEASE && glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_RELEASE) camera.setSlowerSpeed(false);
    
    if(glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
        if(!taking_screenshot) camera.takeScreenshot(scr_width, scr_height);
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
    
    if(mouse_hidden) camera.rotate(offset_x, offset_y);
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
    camera.zoom(offset_y);
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
