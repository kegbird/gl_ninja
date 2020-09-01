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
#include<btBoxShape.h>
#include<btCylinderShape.h>
#include<btSphereShape.h>

// number of lights in the scene
#define NR_LIGHTS 3
#define N_MODELS 5
#define COLOR_LIMIT 256
#define X_BOUNDARY 6
#define Y_KILL -6
#define X_IMPULSE_BOUNDARY 2
#define Y_IMPULSE_BOUNDARY 13

GLuint screenWidth = 1280, screenHeight = 720;

void drawIndicatorLine(Shader lineShader);
void setupIndicatorLineBuffers();
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void setupPhysics();
void addRigidBody(int i);
void removeRigidBody(int i);
void deletePhysics();
GLint loadTexture(const char* path);

bool keys[1024];

GLfloat lastX = 400, lastY = 300;
bool firstMouse = true;
bool pressing = false;
bool cut=false;
const GLfloat maxSecPerFrame = 1.0f / 60.0f;

GLboolean wireframe = GL_FALSE;

glm::vec3 lightPositions[] = {
    glm::vec3(5.0f, 10.0f, 10.0f),
    glm::vec3(-5.0f, 10.0f, 10.0f),
    glm::vec3(5.0f, 10.0f, -10.0f),
};

GLfloat specularColor[] = {1.0,1.0,1.0};
GLfloat ambientColor[] = {0.1,0.1,0.1};
GLfloat diffuseColor[] = {1.0f,0.0f,0.0f};

GLfloat Kd = 0.8f;
GLfloat Ks = 0.5f;
GLfloat Ka = 0.1f;
GLfloat shininess = 25.0f;
GLfloat constant = 1.0f;
GLfloat linear = 0.02f;
GLfloat quadratic = 0.001f;
GLfloat alpha = 0.2f;
GLfloat F0 = 0.9f;
vector<GLint> textureID;
GLfloat repeat = 1.0;

GLFWcursor* cursor;

unsigned int VAOSegment, VBOSegment;
glm::vec3 segmentVertices[] = {glm::vec3(0.f, 0.f, 1.f), glm::vec3(0.f, 0.f, 1.f)};
GLuint segmentIndices[] = { 0, 1 };

GLFWwindow* window;
float gravity=-9.82f;
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
	
    window = glfwCreateWindow(screenWidth, screenHeight, "GL_Ninja", nullptr, nullptr);
    if (!window)
    {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	
    glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glViewport(0, 0, screenWidth, screenHeight);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.26f, 0.46f, 0.98f, 1.0f);
		
	Shader planeShader("18_phong_tex_multiplelights.vert", "19a_blinnphong_tex_multiplelights.frag");
	Shader objectShader("lambert.vert", "lambert.frag");
	Shader lineShader("lineShader.vert", "lineShader.frag");

	array<Model, N_MODELS> models=
			{Model("../../models/cube.obj"),
			Model("../../models/cone.obj"),
			Model("../../models/torus.obj"),
			Model("../../models/icosphere.obj"),
			Model("../../models/sphere.obj")};
	
    Model planeModel("../../models/plane.obj");
	Model cubeModel("../../models./cube.obj");		

    // we load the images and store them in a vector
    textureID.push_back(loadTexture("../../textures/SoilCracked.png"));
	
	setupPhysics();

    // Projection matrix: FOV angle, aspect ratio, near and far planes
    glm::mat4 projection = glm::perspective(45.0f, (float)screenWidth/(float)screenHeight, 0.1f, 15.0f);
	glm::mat4 view = glm::lookAt(glm::vec3(0.f, 0.f, 7.f), glm::vec3(0.f, 0.f, 6.f), glm::vec3(0.f, 1.f, 0.f));
    
	int modelIndex=0;
	
	btRigidBody* rigidBody;
	float glmTransform[16];
	
	addRigidBody(modelIndex);
	
	GLfloat deltaTime;
	GLfloat currentFrame;
	GLfloat lastFrame;

	setupIndicatorLineBuffers();
	
    while(!glfwWindowShouldClose(window))
    {
        // Check is an I/O event is happening
        glfwPollEvents();
        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		
		dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);
		
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
	
		if(transform.getOrigin().getY()<Y_KILL)
		{
			removeRigidBody(0);
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
        glm::mat4 objectModelMatrix=glm::make_mat4(glmTransform);
        glm::mat3 objectNormalMatrix;
        objectNormalMatrix = glm::inverseTranspose(glm::mat3(view*objectModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objectModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(objectShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
		
		models.at(modelIndex).Draw(objectShader);
	
		if(pressing)
		{
			lineShader.Use();
			double xCursor, yCursor;
			glfwGetCursorPos(window, &xCursor, &yCursor);
			float x=(float)xCursor;
			float y=(float)yCursor;
			segmentVertices[1]=glm::vec3(2.0f*(x/screenWidth) - 1.0f, -2.0f*(y/screenHeight) + 1.0f, 1.0);
			glBindVertexArray(VAOSegment);
			glBindBuffer(GL_ARRAY_BUFFER, VBOSegment);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(segmentVertices), segmentVertices);	
			glDrawArrays(GL_LINES, 0, 2);
			glm::vec4 cutPlaneEndPoint=glm::vec4(segmentVertices[1].x,-segmentVertices[1].y,0,1.0);
			cutPlaneEndPoint= inverse(projection*view)*cutPlaneEndPoint;
			cutPlaneEndPoint.x*=cutPlaneEndPoint.w;
			cutPlaneEndPoint.y*=cutPlaneEndPoint.w;
			cutPlaneEndPoint.z*=cutPlaneEndPoint.w;
			objectShader.Use();
			/*glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
			
			glm::mat4 cubeModelMatrix;
			glm::mat3 cubeNormalMatrix;
			cubeModelMatrix = glm::translate(glm::mat4(1.0), glm::vec3(cutPlaneEndPoint.x, -cutPlaneEndPoint.y, 0));
			cubeNormalMatrix = glm::inverseTranspose(glm::mat3(view*cubeModelMatrix));
			
			glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(cubeModelMatrix));
			glUniformMatrix3fv(glGetUniformLocation(objectShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(cubeNormalMatrix));
			cubeModel.Draw(objectShader);*/
		}
		else if(cut)
		{
			//Converting ndc coordinates to world coordinates
			cut=false;
			glm::vec4 cutPlaneStartPoint=glm::vec4(segmentVertices[0].x,-segmentVertices[0].y,-segmentVertices[0].z,1.);
			glm::vec4 cutPlaneEndPoint=glm::vec4(segmentVertices[1].x,-segmentVertices[1].y,-segmentVertices[1].z,1.);
			cutPlaneStartPoint = inverse(projection*view)*cutPlaneStartPoint;
			cutPlaneEndPoint= inverse(projection*view)*cutPlaneEndPoint;
			cutPlaneStartPoint.x*=cutPlaneStartPoint.w;
			cutPlaneStartPoint.y*=cutPlaneStartPoint.w*(-1);
			cutPlaneStartPoint.z*=0;
			cutPlaneEndPoint.x*=cutPlaneEndPoint.w;
			cutPlaneEndPoint.y*=cutPlaneEndPoint.w*(-1);
			cutPlaneEndPoint.z*=0;
			printf("Starting cut position:%f,%f,%f\n",cutPlaneStartPoint.x,cutPlaneStartPoint.y,cutPlaneStartPoint.z);
			printf("End cut position:%f,%f,%f\n",cutPlaneEndPoint.x,cutPlaneEndPoint.y,cutPlaneEndPoint.z);
			btCollisionWorld::ClosestRayResultCallback RayCallback(btVector3(cutPlaneStartPoint.x, cutPlaneStartPoint.y, cutPlaneStartPoint.z),
									btVector3(cutPlaneEndPoint.x, cutPlaneEndPoint.y, cutPlaneEndPoint.z));
				
			dynamicsWorld->rayTest(btVector3(cutPlaneStartPoint.x, cutPlaneStartPoint.y, cutPlaneStartPoint.z),
									btVector3(cutPlaneEndPoint.x, cutPlaneEndPoint.y, cutPlaneEndPoint.z),
									RayCallback);
									
			if(RayCallback.hasHit()) 
			{
				printf("You have cut something!\n");
			}
		}
        glfwSwapBuffers(window);
    }
	
	glDeleteVertexArrays(1, &VAOSegment);
    glDeleteBuffers(1, &VBOSegment);
	planeShader.Delete();
	objectShader.Delete();
	lineShader.Delete();
	glfwDestroyCursor(cursor);
	deletePhysics();
    glfwTerminate();
    return 0;
}

void setupIndicatorLineBuffers()
{
	glGenVertexArrays(1, &VAOSegment);
	glGenBuffers(1, &VBOSegment);
	glBindVertexArray(VAOSegment);
	glBindBuffer(GL_ARRAY_BUFFER, VBOSegment);
	glBufferData(GL_ARRAY_BUFFER, sizeof(segmentVertices), segmentVertices, GL_DYNAMIC_DRAW);		
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);  
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void setupPhysics()
{
	collisionConfiguration = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(collisionConfiguration);
	overlappingPairCache = new btDbvtBroadphase();
	solver = new btSequentialImpulseConstraintSolver;
	dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
	dynamicsWorld->setGravity(btVector3(0, gravity, 0));	
	
	btCollisionShape* cube=new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));
	collisionShapes.push_back(cube);
	btCollisionShape* cone=new btConeShape(1.0f, 2.0f);
	collisionShapes.push_back(cone);
	btCollisionShape* torus=new btCylinderShape(btVector3(0.25f, 6.25f, 0.0f));
	collisionShapes.push_back(torus);
	btCollisionShape* iconsphere = new btSphereShape(1.0f);
	collisionShapes.push_back(iconsphere);
	btCollisionShape* sphere=new btSphereShape(1.0f);
	collisionShapes.push_back(sphere);
}

void addRigidBody(int i)
{
	btTransform startTransform;
	startTransform.setIdentity();
	btScalar mass(1.f);
	btVector3 localInertia(0, 0, 0);
	btCollisionShape* meshCollisionShape = collisionShapes[i];
	meshCollisionShape->calculateLocalInertia(mass, localInertia);
	btScalar xModel = ((rand()%101)/101.f)*X_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
	btScalar yModel = -6;
	startTransform.setOrigin(btVector3(xModel, yModel, 0));
	btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, meshCollisionShape, localInertia);
	btRigidBody* body = new btRigidBody(rbInfo);
	dynamicsWorld->addRigidBody(body);
	
	btScalar xImpulse = ((rand()%101)/101.f)*X_IMPULSE_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
	btScalar yImpulse = Y_IMPULSE_BOUNDARY;
	body->applyImpulse(btVector3(xImpulse,yImpulse,0), btVector3(1,0,0));
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

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		float x=(float)xpos;
		float y=(float)ypos;
		segmentVertices[0]=glm::vec3(2.0f*(x/screenWidth) - 1.0f, (-1)*2.0f*(y/screenHeight) + 1.0f, 0);
		pressing=true;
	}
	else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE )
	{
		pressing=false;
		cut=true;
	}
}