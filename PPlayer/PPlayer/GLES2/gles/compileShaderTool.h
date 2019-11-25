//
//  compileShaderTool.h
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#ifndef compileShaderTool_h
#define compileShaderTool_h

#include <stdio.h>
#include <OpenGLES/ES2/gl.h>
#include <Foundation/Foundation.h>

#define GLES2_MAX_PLANE 3

GLuint compileShader(NSString *shaderName, GLenum shaderType);

#endif /* compileShaderTool_h */
