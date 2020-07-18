//
//  main.cpp
//  大球小球
//
//  Created by 彭文喜 on 2020/7/18.
//  Copyright © 2020 彭文喜. All rights reserved.
//



#include "GLTools.h"
#include "GLShaderManager.h"
#include "GLFrustum.h"
#include "GLBatch.h"
#include "GLMatrixStack.h"
#include "GLGeometryTransform.h"
#include "StopWatch.h"

#include <math.h>
#include <stdio.h>

#ifdef __APPLE__
#include <glut/glut.h>
#else
#define FREEGLUT_STATIC
#include <GL/glut.h>
#endif

GLShaderManager shaderManager;  //着色器管理器
GLMatrixStack modelViewMatrix;  //模型视图矩阵
GLMatrixStack projectionMatrix; //投影矩阵
GLFrustum viewFrustum;  //视景体
GLGeometryTransform transformPipeline;  //几何图形变换管道

GLTriangleBatch torusBatch; //大球
GLTriangleBatch sphereBatch;    //小球
GLBatch floorBatch; //地板

GLFrame cameraFrame;    //角色帧 照相机角色帧

#define NUM_SPHERES 50  //添加50个随机球
GLFrame spheres[NUM_SPHERES];



void ChangeSize(int w,int h){
    //1.设置视口
    glViewport(0, 0, w, h);
    //2.创建投影矩阵。
    viewFrustum.SetPerspective(35.0, float(w)/float(h), 1.0f, 100.0f);
    //viewFrustum.GetProjectionMatrix()  获取viewFrustum投影矩阵
    //并将其加载到投影矩阵堆栈上
    projectionMatrix.LoadMatrix(viewFrustum.GetProjectionMatrix());
    //3.设置变换管道以使用两个矩阵堆栈（变换矩阵modelViewMatrix ，投影矩阵projectionMatrix）
    //初始化GLGeometryTransform 的实例transformPipeline.通过将它的内部指针设置为模型视图矩阵堆栈 和 投影矩阵堆栈实例，来完成初始化
    //当然这个操作也可以在SetupRC函数中完成，但是在窗口大小改变时或者窗口创建时设置它们并没有坏处。而且这样可以一次性完成矩阵和管线的设置。
    transformPipeline.SetMatrixStacks(modelViewMatrix, projectionMatrix);
}

//加载顶点坐标
void SetupRC(){
    //初始化颜色
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    //初始化着色器
    shaderManager.InitializeStockShaders();
    
    //开启深度测试
    glEnable(GL_DEPTH_TEST);
    //开启正背面剔除
    glEnable(GL_CULL_FACE);
    
    //1、加载地板的坐标
    floorBatch.Begin(GL_LINES, 324);
    for (GLfloat x = -20; x<=20.0f; x+=0.5) {
        floorBatch.Vertex3f(x, -0.55f, 20.0f);
        floorBatch.Vertex3f(x, -0.55f, -20.0f);
        
        floorBatch.Vertex3f(20.0f, -0.55f, x);
        floorBatch.Vertex3f(-20.0f, -0.55f, x);
    }
    floorBatch.End();
    
    //2、设置大球模型
    gltMakeSphere(torusBatch, 0.4f, 40, 80);
    
    //3、设置小球模型
    gltMakeSphere(sphereBatch, 0.1f, 13, 26);
    //随机放置小球位置
    for (int i = 0; i<NUM_SPHERES; i++) {
        //y轴不变，x，z产生随机值
        GLfloat x = ((GLfloat)(rand() % 400) - 200) * 0.1f;
        GLfloat z = ((GLfloat)(rand() % 400) - 200) * 0.1f;
    
        //在y轴方向，将球体设置为0.0的位置，使得他们看起来是漂浮在眼睛的高度
        //对spheres数组中的每一个顶点，设置顶点数据
        spheres[i].SetOrigin(x, 0.0f, z);
    }
    
}

//场景绘制
void RendeScene(void){
    //设置颜色（小球，地板，大球）
    static GLfloat vFloorColor[] = {0.0f,1.0f,0.0f,1.0f};
    static GLfloat vtorusColor[] = {1.0f,0.0f,0.0f,1.0f};
    static GLfloat vsphereColor[] = {0.0f,0.0f,1.0f,1.0f};
    
    //设置时间动画
    static CStopWatch rotTmier;
    float yRot = rotTmier.GetElapsedSeconds() * 60;
    
    //清除颜色、深度缓冲区
    glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    
    //压栈(空栈)
    modelViewMatrix.PushMatrix();
    
    //4、加入观察者
    M3DMatrix44f mCamera;
    cameraFrame.GetCameraMatrix(mCamera);
    modelViewMatrix.PushMatrix(mCamera);
    
    //1、绘制地板
    shaderManager.UseStockShader(GLT_SHADER_FLAT,transformPipeline.GetModelViewProjectionMatrix(),vFloorColor);
    floorBatch.Draw();
    
    //2、绘制大球
    //获取光源位置
    M3DVector4f vLightPos = {0.0f,10.0f,5.0f,1.0f};
    //使大球位置平移3.0向屏幕里面
    modelViewMatrix.Translate(0.0f,0.0f, -3.0f);
    //压栈
    modelViewMatrix.PushMatrix();
    //大球自转
    modelViewMatrix.Rotate(yRot, 0.0f, 1.0f, 0.0f);
    //指定合适的着色器(点光源着色器)
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vtorusColor);
    torusBatch.Draw();
    //绘制完毕pop
    modelViewMatrix.PopMatrix();
    
    //3、绘制小球
    for (int i = 0; i<NUM_SPHERES; i++) {
        modelViewMatrix.PushMatrix();
        modelViewMatrix.MultMatrix(spheres[i]);
        shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vsphereColor);
        sphereBatch.Draw();
        modelViewMatrix.PopMatrix();
    }
    
    //让小球围绕大球自转
    modelViewMatrix.Rotate(yRot * -2.0f, 0.0f, 1.0f, 0.0f);
    modelViewMatrix.Translate(0.8f, 0.0f, 0.0f);
    shaderManager.UseStockShader(GLT_SHADER_POINT_LIGHT_DIFF,transformPipeline.GetModelViewMatrix(),transformPipeline.GetProjectionMatrix(),vLightPos,vsphereColor);
    sphereBatch.Draw();
    
    modelViewMatrix.PopMatrix();
    modelViewMatrix.PopMatrix();
    
    
    //执行缓冲区交换
    glutSwapBuffers();
    glutPostRedisplay();
}

void SpecialKeys(int key,int x,int y){
    //移动步长
    float linear = 0.1f;
    //旋转度数
    float angular = float(m3dDegToRad(5.0f));
    
    if(key == GLUT_KEY_UP)
        cameraFrame.MoveForward(linear);
    if(key == GLUT_KEY_DOWN)
        cameraFrame.MoveForward(-linear);
    if(key == GLUT_KEY_LEFT)
        cameraFrame.RotateWorld(angular, 0.0f, 1.0f, 0.0f);
    if(key == GLUT_KEY_RIGHT)
        cameraFrame.RotateWorld(-angular, 0.0f, 1.0f, 0.0f);
}

int main(int argc,char* argv[]){
    gltSetWorkingDirectory(argv[0]);
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_DEPTH|GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("综合案例");
    
    glutReshapeFunc(ChangeSize);
    glutDisplayFunc(RendeScene);
    glutSpecialFunc(SpecialKeys);
    
    
    GLenum err = glewInit();
    if(GLEW_OK!=err){
        fprintf(stderr, "GLEW Error: %s\n",glewGetErrorString(err));
        return 1;
    }
    
    
    SetupRC();
    glutMainLoop();
    
    return 0;
    
}

