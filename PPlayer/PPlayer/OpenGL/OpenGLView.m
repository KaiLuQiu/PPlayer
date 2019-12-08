//
//  OpenGLView.m
//  PPlayer
//
//  Created by 邱开禄 on 2019/11/14.
//  Copyright © 2019 邱开禄. All rights reserved.
//

#import "OpenGLView.h"
#include "compileShaderTool.h"
#include <libavcodec/avcodec.h>


// TODO: 这部分代码只是先用于测试，后续我要对opengl模块进行进一步的改造
// 顶点坐标
static const GLfloat vertices[12] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
    1.0f,  1.0f, 0.0f
};

// 纹理坐标
static const GLfloat texcoords[8] = {
    0.0f, 1.0f,
    1.0f, 1.0f,
    0.0f, 0.0f,
    1.0f, 0.0f
};

@implementation OpenGLView{
    CAEAGLLayer *_eaglLayer;
    EAGLContext *_context;
    
    //GLSL
    GLuint _framebuffer;
    GLuint _renderbuffer;
    GLint _renderWidth;
    GLint _renderHeight;
    
    GLuint program;//program_id
    
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint plane_textures[GLES2_MAX_PLANE];//texture_id
    
    GLuint av4_position;
    GLuint av2_texcoord;
    GLuint um4_matrix;
    GLuint us2_sampler[GLES2_MAX_PLANE];
}




- (instancetype) initWithFrame:(CGRect)frame{
    self = [super initWithFrame:frame];
    if (self) {
        [self setupRender];
    }
    return self;
}

- (void)dealloc{
    if (_framebuffer) {
        glDeleteFramebuffers(1, &_framebuffer);
        _framebuffer = 0;
    }
    
    if (_renderbuffer) {
        glDeleteRenderbuffers(1, &_renderbuffer);
        _renderbuffer = 0;
    }
    glFinish();
    
}

- (void)setupRender{
    
    // 创建EGL环境
    _eaglLayer = (CAEAGLLayer*) self.layer;
    _eaglLayer.opaque = YES;
    _eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                     [NSNumber numberWithBool:NO], kEAGLDrawablePropertyRetainedBacking,
                                     kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                     nil];

    _context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
    [EAGLContext setCurrentContext:_context];
    
    // 绑定framebuffer
    glGenFramebuffers(1, &_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, _framebuffer);
    // 绑定渲染缓冲区
    glGenRenderbuffers(1, &_renderbuffer);
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    [_context renderbufferStorage:GL_RENDERBUFFER fromDrawable:_eaglLayer];
    
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &_renderWidth);
    glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &_renderHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, _renderbuffer);
    

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        NSLog(@"failed to make complete framebuffer object %x\n", status);
    }
    
    GLenum glError = glGetError();
    if (GL_NO_ERROR != glError) {
        NSLog(@"failed to setup GL %x\n", glError);
    }
}

- (void)loadShaders{
    // 创建program
    program = glCreateProgram();
    
    // 编译顶点着色器
    vertex_shader = compileShader(@"mvp_vsh", GL_VERTEX_SHADER);
    // 编译片段着色器
    fragment_shader = compileShader(@"YUV420P_fsh", GL_FRAGMENT_SHADER);
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
   
    glLinkProgram(program);
    GLint link_status;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (link_status == GL_FALSE) {
        GLchar messages[256];
        glGetProgramInfoLog(program, sizeof(messages), 0, &messages[0]);
        printf("%s\n", messages);
    }
    // 获取vertex shader中attribute变量的引用
    av4_position = glGetAttribLocation(program, "av4_position");
    av2_texcoord = glGetAttribLocation(program, "av2_texcoord");
    um4_matrix = glGetUniformLocation(program, "um4_matrix");
    // 获取fragment shader中uniform变量的引用
    us2_sampler[0] = glGetUniformLocation(program, "samplerY");
    us2_sampler[1] = glGetUniformLocation(program, "samplerU");
    us2_sampler[2] = glGetUniformLocation(program, "samplerV");
}



#pragma mark - 核心渲染代码
- (void)Render:(VideoFrame *)frame{
    //每次刷新画面都需要调用setCurrentContext
    [EAGLContext setCurrentContext:_context];
    [self loadShaders];
  
    // 激活program
    glUseProgram(program);
    //4.create the texture
    if (0 == plane_textures[0])
        glGenTextures(frame->planar , plane_textures);//创建texture对象


    int widths[frame->planar];
    int heights[frame->planar];
    if (frame->format == AV_PIX_FMT_YUV420P){
      
        widths[0] = frame->width;
        widths[1] = frame->width / 2;
        widths[2] = frame->width / 2;
        heights[0] = frame->height;
        heights[1] = frame->height / 2;
        heights[2] = frame->height / 2;
        
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (int i = 0; i < frame->planar; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);//设置当前活动的纹理单元
        glBindTexture(GL_TEXTURE_2D, plane_textures[i]);//texture绑定
        if (frame->format == AV_PIX_FMT_YUV420P){
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, widths[i], heights[i], 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, frame->pixels[i]);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glUniform1i(us2_sampler[i], i);
    }


    glEnableVertexAttribArray(av4_position);
    glEnableVertexAttribArray(av2_texcoord);

    glVertexAttribPointer(av4_position, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(av2_texcoord, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), texcoords);


    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, _renderWidth, _renderHeight);
    
    glUniformMatrix4fv(um4_matrix, 1, GL_FALSE, GLKMatrix4Identity.m);
    
    // 渲染绘制过程
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glBindRenderbuffer(GL_RENDERBUFFER, _renderbuffer);
    //present render
    [_context presentRenderbuffer:GL_RENDERBUFFER];
    
    if ((program)) {
        if (vertex_shader){
            glDetachShader(program, vertex_shader);
            glDeleteShader(vertex_shader);
            vertex_shader = 0;
        }

        if (fragment_shader){
            glDetachShader(program, fragment_shader);
            glDeleteShader(fragment_shader);
            fragment_shader = 0;
        }

        if (program){
            glDeleteProgram(program);
            program  = 0;
        }

        for (int i = 0; i < GLES2_MAX_PLANE; ++i) {
            if (plane_textures[i]) {
                glDeleteTextures(1, &plane_textures[i]);
                plane_textures[i] = 0;
            }
        }
    }
}




@end

