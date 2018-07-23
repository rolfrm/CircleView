#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <iron/types.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iron/log.h>
#include <iron/mem.h> 
#include <iron/fileio.h>
#include <iron/time.h>
#include <iron/utils.h>
#include <iron/linmath.h>
#include <iron/math.h>
#include <iron/image.h>
#include "gl_utils.h"

bool use_framebuffer = true;
int image_width = 1000;//15000;
int image_height =1000;//10000;


void printError(const char * file, int line ){
  u32 err = glGetError();
  if(err != 0) logd("%s:%i : GL ERROR  %i\n", file, line, err);
}

#define PRINTERR() printError(__FILE__, __LINE__);

vec2 glfwGetNormalizedCursorPos(GLFWwindow * window){
  double xpos, ypos;
  glfwGetCursorPos(window, &xpos, &ypos);
  int win_width, win_height;
  glfwGetWindowSize(window, &win_width, &win_height);
  return vec2_new((xpos / win_width * 2 - 1), -(ypos / win_height * 2 - 1));
}


// detects a previously existing jump related bug.
int main(int argc, char ** argv){  
  if(argc > 3){
    logd("ARGS: %i\n", argc);
    sscanf(argv[1], "%i", &image_width);
    sscanf(argv[2], "%i", &image_height);
    sscanf(argv[3], "%i", &use_framebuffer);

  }
  glfwInit();
  
  glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, true);

  int w_width = 256, w_height = 256;
  if(use_framebuffer == false){
    w_width = image_width;
    w_height = image_height;
  }
  
  GLFWwindow * win = glfwCreateWindow(w_width, w_height, "Octree Rendering", NULL, NULL);
  glfwMakeContextCurrent(win);
  glfwSwapInterval(2);
  glViewport(0, 0, image_width, image_height);
  ASSERT(glewInit() == GLEW_OK);

  gl_init_debug_calls();
  glClearColor(1.0, 1.0, 1.0, 0.0);

  glEnable(GL_STENCIL_TEST);

  //glEnable(GL_BLEND);
  u32 framefb = 0;
  u32 frame_tex = 0;
  if(use_framebuffer){ // creating a framebuffer
    glGenTextures(1, &frame_tex);
    glBindTexture(GL_TEXTURE_2D, frame_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, image_width, image_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
    
    
    glGenFramebuffers(1, &framefb);
    glBindFramebuffer(GL_FRAMEBUFFER, framefb);  
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frame_tex, 0);

  }
  glBindFramebuffer(GL_FRAMEBUFFER, framefb);
  var circ = load_circle_shader();
  glUseProgram(circ.prog);

  size_t scaling = 5;
  while(glfwWindowShouldClose(win) == false){
    srand(2321);
    vec2 * circs = alloc0(sizeof(vec2));
    float * radius = alloc0(sizeof(float));
    vec2 * circs_next = NULL;
    float * radius_next = NULL;
    size_t n_circs = 1;

    circs[0] = vec2_new(0.5, 0.5);
    radius[0] = 0.5;
  
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    u64 ts = timestamp();
    
    for(int i = 1; i < 8; i++){
      glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
      glStencilFunc(GL_EQUAL, i - 1,  0xFF);
  
      glStencilMask(0xFF);
      circs_next = realloc(circs_next, sizeof(vec2) * n_circs * scaling);
      radius_next = realloc(radius_next, sizeof(float) * n_circs * scaling);
      for(size_t j = 0; j < n_circs; j++){
	vec2 v = circs[j];

	float r = radius[j];
	//vec2_print(v);logd("%f \n", r);
	glUniform2f(circ.offset_loc, v.x, v.y);
	glUniform1f(circ.radius_loc, r);
	glUniform3f(circ.color_loc, 1 - ((float)i) * 0.10, 1 - ((float)i) * 0.10, 1 - ((float)i) * 0.10);	
	glDrawArrays(GL_TRIANGLE_STRIP,0, 4);

	for(size_t k = 0; k < scaling; k++){
	  float phase = (1.0f + (float)k) / ((float)scaling) * 2 * M_PI;
	  float dist = 0.5 * r;
	  float x = cos(phase) * dist * 0.9;
	  float y = sin(phase) * dist * 0.9;

	  circs_next[j * scaling + k] = vec2_add(v, vec2_new(x,y));
	  radius_next[j * scaling + k] = r * 0.40;
	}
      }
      SWAP(circs_next, circs);
      SWAP(radius_next, radius);
      n_circs *= scaling;
    }

    dealloc(circs);
    dealloc(radius);
    dealloc(circs_next);
    dealloc(radius_next);
      
    
    
    if(!use_framebuffer){
      glfwSwapBuffers(win);
    }
    iron_usleep(100000);
    u64 ts2 = timestamp();
    var seconds_spent = ((double)(ts2 - ts) * 1e-6);
    
    logd("%f s \n", seconds_spent);
    if(use_framebuffer){
      glReadBuffer(GL_COLOR_ATTACHMENT0);
      {
	int x =0, y = 0, width = image_width, height = image_height;
	char * buffer = alloc0(width * height * 4);
	glReadPixels(x, y, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid*)buffer);
	UNUSED(x);UNUSED(y);
	image img = {.type = PIXEL_RGB, .buffer = buffer, .width = width, .height = height};
	image_save(&img, "out_img.png");
      }
      
      return 0;
    }
    
    if(seconds_spent < 0.016){
      iron_sleep(0.016 - seconds_spent);
    }

    glfwPollEvents();

  }
}

