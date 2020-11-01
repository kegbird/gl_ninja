/*
Physics class - v1:
- initialization of the physics simulation using the Bullet librarty

The class sets up the collision manager and the resolver of the constraints, using basic general-purposes methods provided by the library. Advanced and multithread methods are available, please consult Bullet documentation and examples

createRigidBody method sets up a  Box or Sphere Collision Shape. For other Shapes, you must extend the method.

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in Computer Science
Universita' degli Studi di Milano
*/

#pragma once
#include <time.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <bullet/btBulletDynamicsCommon.h>
#include <btConvexShape.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>
#define N_LIGHTS 3
#define COLOR_LIMIT 256
#define Y_KILL -6

class Scene
{
private:
	Physics engine;
	Shader planeShader;
	Mesh planeMesh;
	GLint planeTexture;
	vector<Mesh> cuttableMeshes;
	GLfloat deltaTime;
	const GLfloat maxSecPerFrame=1.0f / 60.0f;
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
	GLfloat repeat = 80.0;
	glm::mat4 projection;
	glm::mat4 view;
	GLfloat currentFrame;
	GLfloat lastFrame;
	Shader meshShader;
	GLfloat meshDiffuseColor[3]={0.0f, 0.0f, 0.0f};
	glm::vec3 lightPositions[3] = {glm::vec3(5.0f, 10.0f, 10.0f), glm::vec3(-5.0f, 10.0f, 10.0f), glm::vec3(5.0f, 10.0f, -10.0f)};		
	GLfloat specularColor[3] = {1.0,1.0,1.0};
	GLfloat ambientColor[3] = {0.1,0.1,0.1};
	GLfloat diffuseColor[3] = {1.0f,0.0f,0.0f};
	float cutDepthNDC=0.0f;

public:
	Scene(glm::mat4 projection, glm::mat4 view)
	{
		Model* object = new Model("../../models/plane.obj");
		planeMesh=object->meshes[0];
		engine=Physics();
		deltaTime=0.0f;
		currentFrame=0.0f;
		lastFrame=0.0f;
		this->projection=projection;
		this->view=view;
		meshShader=Shader("lambert.vert", "lambert.frag");
		planeShader=Shader("18_phong_tex_multiplelights.vert", "19a_blinnphong_tex_multiplelights.frag");
		planeTexture=LoadTexture("../../textures/SoilCracked.png");
		
		glm::vec4 origin=glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		origin=projection*view*origin;
		origin/=origin.w;
		cutDepthNDC=origin.z;
	}
	
	static GLint LoadTexture(const char* path)
	{
		GLuint textureImage;
		int w, h, channels;
		unsigned char* image;
		image = stbi_load(path, &w, &h, &channels, STBI_rgb);
		if (image == nullptr)
			std::cout << "Failed to load texture!" << std::endl;
		glGenTextures(1, &textureImage);
		glBindTexture(GL_TEXTURE_2D, textureImage);
		if (channels==3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels==4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		stbi_image_free(image);
		return textureImage;
	}
	
	void AddMesh(int meshIndex, string meshPath)
	{
		GLfloat red=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		GLfloat green=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		GLfloat blue=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		meshDiffuseColor[0]=red;
		meshDiffuseColor[1]=green;
		meshDiffuseColor[2]=blue;
		Model* object = new Model(meshPath);
		std::vector<Mesh>::iterator cuttableMeshesIt=cuttableMeshes.begin();
		cuttableMeshes.insert(cuttableMeshesIt, object->meshes.begin(), object->meshes.end());
		engine.AddRigidBodyWithImpulse(cuttableMeshes.back());
	}
	
	bool AllMeshRemoved()
	{
		return cuttableMeshes.size()==0;
	}
	
	void Cut(glm::vec3 startCutPointNDC, glm::vec3 endCutPointNDC)
	{
		glm::vec4 cutStartPointWS=glm::vec4(startCutPointNDC.x, startCutPointNDC.y, cutDepthNDC, 1.);
		glm::vec4 cutEndPointWS=glm::vec4(endCutPointNDC.x, endCutPointNDC.y, cutDepthNDC, 1.);
		//Converting cut segment from ndc to world space to perform the cut check
		glm::mat4 projViewInv = glm::inverse(projection * view);
		cutStartPointWS = projViewInv*cutStartPointWS;
		cutStartPointWS/=cutStartPointWS.w;
		cutEndPointWS = projViewInv*cutEndPointWS;
		cutEndPointWS/=cutEndPointWS.w;

		btCollisionWorld::AllHitsRayResultCallback callback(btVector3(cutStartPointWS.x, cutStartPointWS.y, cutStartPointWS.z), btVector3(cutEndPointWS.x, cutEndPointWS.y, cutEndPointWS.z));
		engine.dynamicsWorld->rayTest(btVector3(cutStartPointWS.x, cutStartPointWS.y, cutStartPointWS.z), btVector3(cutEndPointWS.x, cutEndPointWS.y, cutEndPointWS.z), callback);
									
		if(callback.hasHit())
		{
			for(int i=0;i<callback.m_collisionObjects.size();i++)
			{
				const btCollisionObject* object=callback.m_collisionObjects[i];
				const btCollisionShape* collisionShape=object->getCollisionShape();
				int meshIndex=engine.GetCollisionShapeIndex(collisionShape);
				if(meshIndex<0)
					continue;
					
				printf("La mesh tagliata: %d.\n",meshIndex);
				glm::mat4 model=engine.GetObjectModelMatrix(meshIndex);
				
				btConvexHullShape* positiveConvexHullShape;
				btConvexHullShape* negativeConvexHullShape;
				glm::vec4 positiveMeshPositionWS;
				glm::vec4 negativeMeshPositionWS;
				Mesh positiveMesh;
				Mesh negativeMesh;
				float positiveWeightFactor;
				float negativeWeightFactor;
				cuttableMeshes[meshIndex].Cut(positiveMesh,
											  negativeMesh,
											  positiveMeshPositionWS, 
											  negativeMeshPositionWS, 
											  cutStartPointWS, 
											  cutEndPointWS, 
											  model, 
											  view, 
											  projection, 
											  positiveConvexHullShape, 
											  negativeConvexHullShape, 
											  positiveWeightFactor, 
											  negativeWeightFactor);
				
				glm::vec3 cutNormal=glm::vec3(-1*(cutEndPointWS.y-cutStartPointWS.y), cutEndPointWS.x-cutStartPointWS.x, 0.0f);
				engine.CutShapeWithImpulse(cutNormal, meshIndex, model, negativeWeightFactor, negativeMeshPositionWS, negativeConvexHullShape, positiveWeightFactor, positiveMeshPositionWS, positiveConvexHullShape);
				cuttableMeshes.push_back(positiveMesh);
				cuttableMeshes.push_back(negativeMesh);	
				cuttableMeshes[meshIndex].Delete();
				cuttableMeshes.erase(cuttableMeshes.begin()+meshIndex);
			}
		}
	}
	
	void DrawScene()
	{
		planeShader.Use();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, planeTexture);
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
        for (GLuint i = 0; i < N_LIGHTS; i++)
        {
            string number = to_string(i);
            glUniform3fv(glGetUniformLocation(planeShader.Program, ("lights[" + number + "]").c_str()), 1, glm::value_ptr(lightPositions[i]));
        }
        glUniform1f(kdLocation, Kd);
        glUniform1i(textureLocation, 1);
        glUniform1f(repeatLocation, repeat);
        glUniform3fv(matAmbientLocation, 1, ambientColor);
        glUniform3fv(matSpecularLocation, 1, specularColor);
        glUniform1f(kaLocation, Ka);
        glUniform1f(ksLocation, 0.0f);
        glUniform1f(shineLocation, 1.0f);
        glUniform1f(constantLocation, constant);
        glUniform1f(linearLocation, linear);
        glUniform1f(quadraticLocation, quadratic);
        glm::mat4 planeModelMatrix;
        glm::mat3 planeNormalMatrix;
        planeModelMatrix = glm::scale(planeModelMatrix, glm::vec3(10.0f, 10.0f, 1.0f));
		planeModelMatrix = glm::rotate(planeModelMatrix, glm::radians(90.f),glm::vec3(1.f, 0.f, 0.f));
        planeModelMatrix = glm::translate(planeModelMatrix, glm::vec3(0.0f, -5.0f, 0.0f));
        planeNormalMatrix = glm::inverseTranspose(glm::mat3(view*planeModelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(planeShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(planeModelMatrix));
        glUniformMatrix3fv(glGetUniformLocation(planeShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(planeNormalMatrix));
        planeMesh.Draw(planeShader);
		
		int i=0;
		std::vector<Mesh>::iterator cuttableMeshesIt;
		for (cuttableMeshesIt = cuttableMeshes.begin(); cuttableMeshesIt != cuttableMeshes.end(); ++cuttableMeshesIt)
		{
			meshShader.Use();
			glUniformMatrix4fv(glGetUniformLocation(meshShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(meshShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
			// we determine the position in the Shader Program of the uniform variables
			GLint pointLightLocation = glGetUniformLocation(meshShader.Program, "pointLightPosition");
			GLint objectDiffuseLocation = glGetUniformLocation(meshShader.Program, "diffuseColor");
			GLint kdObjectLocation = glGetUniformLocation(meshShader.Program, "Kd");
			// we assign the value to the uniform variables
			glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPositions[0]));
			glUniform3fv(objectDiffuseLocation, 1, meshDiffuseColor);
			glUniform1f(kdObjectLocation, Kd);
			glm::mat4 objectModelMatrix=engine.GetObjectModelMatrix(i);
			glm::mat3 objectNormalMatrix;
			objectNormalMatrix = glm::inverseTranspose(glm::mat3(view*objectModelMatrix));
			glUniformMatrix4fv(glGetUniformLocation(meshShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objectModelMatrix));
			glUniformMatrix3fv(glGetUniformLocation(meshShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
			cuttableMeshesIt->Draw(meshShader);
			i++;
		}
	}
	
	void SimulationStep()
	{		
		currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		engine.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);
		
		for(int i=engine.dynamicsWorld->getCollisionObjectArray().size()-1;i>=0;i--)
		{
			btCollisionObject* collisionObject = engine.dynamicsWorld->getCollisionObjectArray()[i];
			btRigidBody* rigidBody = btRigidBody::upcast(collisionObject);
			btTransform transform;
			if (rigidBody && rigidBody->getMotionState())
			{
				rigidBody->getMotionState()->getWorldTransform(transform);
			}
			else
			{
				transform = collisionObject->getWorldTransform();
			}
			
			float glmTransform[16];
			transform.getOpenGLMatrix(glmTransform);
	
			if(transform.getOrigin().getY()<=Y_KILL)
			{
				cuttableMeshes[i].Delete();
				iter_swap(cuttableMeshes.begin()+i, cuttableMeshes.end()-1);
				cuttableMeshes.pop_back();
				engine.RemoveRigidBodyAtIndex(i);
			}
		}
	}
	
	void RemoveAllMeshAndColliders()
	{
		for(int i=cuttableMeshes.size()-1; 0<=i;i--)
		{
			cuttableMeshes[i].Delete();
			cuttableMeshes.erase(cuttableMeshes.begin()+i);
			engine.RemoveRigidBodyAtIndex(i);
		}
	}
	
	void Clear()
	{
		engine.Clear();
		planeShader.Delete();
		meshShader.Delete();
		planeMesh.Delete();
		std::vector<Mesh>::iterator cuttableMeshesIt;
		for (cuttableMeshesIt = cuttableMeshes.begin(); cuttableMeshesIt != cuttableMeshes.end(); ++cuttableMeshesIt)
			cuttableMeshesIt->Delete();
		cuttableMeshes.clear();		
	}    
};
