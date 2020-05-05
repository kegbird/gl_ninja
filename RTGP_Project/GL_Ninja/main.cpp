/*
Es04a: as Es03c, but textures are used for the diffusive component of materials
- swapping (pressing keys from 1 to 2) between a shader with texturing applied to the Blinn-Phong and GGX illumination models

N.B. 1) The presented shader swapping method is used in OpenGL 3.3 .
        For OpenGL 4.x, shader subroutines can be used : http://www.geeks3d.com/20140701/opengl-4-shader-subroutines-introduction-3d-programming-tutorial/

N.B. 2) we have considered point lights only, the code must be modifies in case of lights of different nature

N.B. 3) there are other methods (more efficient) to pass multiple data to the shaders (search for Uniform Buffer Objects)

N.B. 4) with last versions of OpenGL, using structures like the one cited above, it is possible to pass a dynamic number of lights

N.B. 5) Updated versions, including texturing, of Model (model_v2.h) and Mesh (mesh_v2.h) classes are used.

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

/*
OpenGL coordinate system (right-handed)
positive X axis points right
positive Y axis points up
positive Z axis points "outside" the screen


                              Y
                              |
                              |
                              |________X
                             /
                            /
                           /
                          Z
*/

#ifdef _WIN32
    #define __USE_MINGW_ANSI_STDIO 0
#endif
// Std. Includes
#include <time.h>
#include <string>
#include <cstdlib>

#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, and for FPS camera
// in this example, the Model and Mesh classes support texturing
#include <utils/shader.h>
#include <utils/model.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

#include<btBulletDynamicsCommon.h>

// number of lights in the scene
#define NR_LIGHTS 3
#define N_MODELS 5
#define DELAY 1.f
#define COLOR_LIMIT 256
#define X_BOUNDARY 7

// dimensions of application's window
GLuint screenWidth = 1280, screenHeight = 720;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void setupPhysics();
void addRigidBody(int i);
void removeRigidBody(int i);
void deletePhysics();
GLint loadTexture(const char* path);

// we initialize an array of booleans for each keybord key
bool keys[1024];

// we set the initial position of mouse cursor in the application window
GLfloat lastX = 400, lastY = 300;
// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

glm::vec3 lightPositions[] = {
    glm::vec3(5.0f, 10.0f, 10.0f),
    glm::vec3(-5.0f, 10.0f, 10.0f),
    glm::vec3(5.0f, 10.0f, -10.0f),
};

// specular and ambient components
GLfloat specularColor[] = {1.0,1.0,1.0};
GLfloat ambientColor[] = {0.1,0.1,0.1};
GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};

// weights for the diffusive, specular and ambient components
GLfloat Kd = 0.8f;
GLfloat Ks = 0.5f;
GLfloat Ka = 0.1f;
// shininess coefficient for Blinn-Phong shader
GLfloat shininess = 25.0f;

// attenuation parameters for Blinn-Phong shader
GLfloat constant = 1.0f;
GLfloat linear = 0.02f;
GLfloat quadratic = 0.001f;

// roughness index for Cook-Torrance shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;

// vector for the textures IDs
vector<GLint> textureID;
// UV repetitions
GLfloat repeat = 1.0;

float gravity=-10;
btDefaultCollisionConfiguration* collisionConfiguration;
btCollisionDispatcher* dispatcher;
btBroadphaseInterface* overlappingPairCache;
btSequentialImpulseConstraintSolver* solver;
btDiscreteDynamicsWorld* dynamicsWorld; 
btAlignedObjectArray<btCollisionShape*> collisionShapes;

int main()
{
	srand(time(0));
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "GL_Ninja", nullptr, nullptr);
    if (!window)
    {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	
    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }


    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);

	array<Model, N_MODELS> models={Model("../../models/cube.obj"),
			Model("../../models/cone.obj"),
			Model("../../models/torus.obj"),
			Model("../../models/icosphere.obj"),
			Model("../../models/sphere.obj")};

    // we create the Shader Program used for the plane (fixed)
    Shader planeShader("18_phong_tex_multiplelights.vert", "19a_blinnphong_tex_multiplelights.frag");
	Shader objectShader("lambert.vert", "lambert.frag");
	
    Model planeModel("../../models/plane.obj");

    // we load the images and store them in a vector
    textureID.push_back(loadTexture("../../textures/SoilCracked.png"));
	
	setupPhysics();
	
	for(unsigned int i=0;i<models.size();i++)
	{
		for(unsigned int j=0;j<models[i].meshes.size();j++)
		{
			btConvexHullShape* meshCollisionShape = new btConvexHullShape();
			for(unsigned int k=0;k<models[i].meshes[j].vertices.size();k++)
			{
				Vertex vertex = models[i].meshes[j].vertices[k];
				meshCollisionShape->addPoint(btVector3(vertex.Position.x, vertex.Position.y, vertex.Position.z));
			}
			
			collisionShapes.push_back(meshCollisionShape);
		}
	}

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 10000.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.f, 0.f, 7.f), glm::vec3(0.f, 0.f, 6.f), glm::vec3(0.f, 1.f, 0.f));
    
	GLfloat currentTime;
	GLfloat lastTime=glfwGetTime();
	GLfloat deltaTime;
	int modelIndex=0;
	
	btRigidBody* rigidBody;
	float glmTransform[16];
	
	addRigidBody(modelIndex);
	
    while(!glfwWindowShouldClose(window))
    {
        // Check is an I/O event is happening
        glfwPollEvents();
        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		dynamicsWorld->stepSimulation(1.f / 60.f, 10);
		
        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			
        /////////////////// PLANE ////////////////////////////////////////////////
        // We render a plane under the objects. We apply the fullcolor shader to the plane, and we do not apply the rotation applied to the other objects.
        planeShader.Use();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, textureID[0]);

        // we pass projection and view matrices to the Shader Program of the plane
        glUniformMatrix4fv(glGetUniformLocation(planeShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(planeShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint kdLocation = glGetUniformLocation(planeShader.Program, "Kd");
        GLint textureLocation = glGetUniformLocation(planeShader.Program, "tex");
        GLint repeatLocation = glGetUniformLocation(planeShader.Program, "repeat");
        GLint matAmbientLocation = glGetUniformLocation(planeShader.Program, "ambientColor");
        GLint matSpecularLocation = glGetUniformLocation(planeShader.Program, "specularColor");
        GLint kaLocation = glGetUniformLocation(planeShader.Program, "Ka");
        GLint ksLocation = glGetUniformLocation(planeShader.Program, "Ks");
        GLint shineLocation = glGetUniformLocation(planeShader.Program, "shininess");
        GLint constantLocation = glGetUniformLocation(planeShader.Program, "constant");
        GLint linearLocation = glGetUniformLocation(planeShader.Program, "linear");
        GLint quadraticLocation = glGetUniformLocation(planeShader.Program, "quadratic");

        // we pass each light position to the shader
        for (GLuint i = 0; i < NR_LIGHTS; i++)
        {
            string number = to_string(i);
            glUniform3fv(glGetUniformLocation(planeShader.Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
        }

        // we assign the value to the uniform variables
        glUniform1f(kdLocation, Kd);
        glUniform1i(textureLocation, 1);
        glUniform1f(repeatLocation, 80.0);
        glUniform3fv(matAmbientLocation, 1, ambientColor);
        glUniform3fv(matSpecularLocation, 1, specularColor);
        glUniform1f(kaLocation, Ka);
        glUniform1f(ksLocation, 0.0f);
        glUniform1f(shineLocation, 1.0f);
        glUniform1f(constantLocation, constant);
        glUniform1f(linearLocation, linear);
        glUniform1f(quadraticLocation, quadratic);

        // we create the transformation matrix by defining the Euler's matrices, and the matrix for normals transformation
        glm::mat4 planeModelMatrix;
        glm::mat3 planeNormalMatrix;
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 10.0f, 1.0f));
		planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(90.f),glm::vec3(1.f, 0.f, 0.f));
        planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -5.0f, 0.0f));
        planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(planeShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(planeShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));

        // we render the plane
        planeModel.Draw(planeShader);
		
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
		
		if(deltaTime>DELAY)
		{
			removeRigidBody(0);
			lastTime=glfwGetTime();
			currentTime=lastTime;
			GLfloat red=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
			GLfloat green=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
			GLfloat blue=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
			
			diffuseColor[0]=red;
			diffuseColor[1]=green;
			diffuseColor[2]=blue;
			
			modelIndex++;
			modelIndex = modelIndex%N_MODELS;
			addRigidBody(modelIndex);
		}
		
		objectShader.Use();
        // we pass projection and view matrices to the Shader Program of the plane
        glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // we determine the position in the Shader Program of the uniform variables
        GLint pointLightLocation = glGetUniformLocation(objectShader.Program, "pointLightPosition");
        GLint objectDiffuseLocation = glGetUniformLocation(objectShader.Program, "diffuseColor");
        GLint kdObjectLocation = glGetUniformLocation(objectShader.Program, "Kd");

        // we assign the value to the uniform variables
        glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPositions[0]));
        glUniform3fv(objectDiffuseLocation, 1, diffuseColor);
        glUniform1f(kdObjectLocation, Kd);

		btCollisionObject* collisionObject = dynamicsWorld->getCollisionObjectArray()[0];
		rigidBody = btRigidBody::upcast(collisionObject);
		
		btTransform transform;
		if (rigidBody && rigidBody->getMotionState())
		{
			rigidBody->getMotionState()->getWorldTransform(transform);
		}
		else
		{
			transform = collisionObject->getWorldTransform();
		}
		
		transform.getOpenGLMatrix(glmTransform);
		
        // we create the transformation matrix by defining the Euler's matrices, and the matrix for normals transformation
        glm::mat4 objectModelMatrix=glm::mat4(glmTransform[0],
												glmTransform[1],
												glmTransform[2],
												glmTransform[3],
												glmTransform[4],
												glmTransform[5],
												glmTransform[6],
												glmTransform[7],
												glmTransform[8],
												glmTransform[9],
												glmTransform[10],
												glmTransform[11],
												glmTransform[12],
												glmTransform[13],
												glmTransform[14],
												glmTransform[15]);
        glm::mat3 objectNormalMatrix;
        objectNormalMatrix = glm::inverseTranspose(glm::mat3(view*objectModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objectModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(objectShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
		
		models.at(modelIndex).Draw(objectShader);
		
        glfwSwapBuffers(window);
    }
	
	planeShader.Delete();
	objectShader.Delete();
	deletePhysics();
    glfwTerminate();
    return 0;
}

void setupPhysics()
{
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, gravity, 0));	
}

void addRigidBody(int i)
{
	btTransform startTransform;
	startTransform.setIdentity();
	btScalar mass(1.f);
	btVector3 localInertia(0, 0, 0);
	btCollisionShape* meshCollisionShape = collisionShapes[i];
	meshCollisionShape->calculateLocalInertia(mass, localInertia);
	
	btScalar x = ((rand()%101)/101.f)*X_BOUNDARY;
	btScalar y = -6;
	btScalar z = 0;
	
	startTransform.setOrigin(btVector3(x, y, z));
	btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, meshCollisionShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	dynamicsWorld->addRigidBody(body);
}

void removeRigidBody(int i)
{
	btCollisionObject* collisionObject = dynamicsWorld->getCollisionObjectArray()[i];
	btRigidBody* rigidBody = btRigidBody::upcast(collisionObject);
	dynamicsWorld->removeRigidBody(rigidBody);
}

void deletePhysics()
{
	delete dynamicsWorld;
	delete solver;
	delete overlappingPairCache;
	delete dispatcher;
	delete collisionConfiguration;
	collisionShapes.clear();
}

GLint loadTexture(const char* path)
{
    GLuint textureImage;
    int w, h, channels;
    unsigned char* image;
    image = stbi_load(path, &w, &h, &channels, STBI_rgb);

    if (image == nullptr)
        std::cout << "Failed to load texture!" << std::endl;

    glGenTextures(1, &textureImage);
    glBindTexture(GL_TEXTURE_2D, textureImage);
    // 3 channels = RGB ; 4 channel = RGBA
    if (channels==3)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
    else if (channels==4)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    // we set how to consider UVs outside [0,1] range
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // we set the filtering for minification and magnification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    // we free the memory once we have created an OpenGL texture
    stbi_image_free(image);

    return textureImage;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;
		
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
      // we move the camera view following the mouse cursor
      // we calculate the offset of the mouse cursor from the position in the last frame
      // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
      if(firstMouse)
      {
          lastX = xpos;
          lastY = ypos;
          firstMouse = false;
      }

      // offset of mouse cursor position
      GLfloat xoffset = xpos - lastX;
      GLfloat yoffset = lastY - ypos;

      // the new position will be the previous one for the next frame
      lastX = xpos;
      lastY = ypos;
}
