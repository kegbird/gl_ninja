/*
Scene class:

This class is responsible about everything rendered in the application.
In particular it handles:

-the cut operation
-the updates of all cuttable objects position and rotation through the physic class
-the allocation of cuttable objects
-the deallocation of all cuttable objects that fall outside the camera view
-the rendering of cuttable objects and the background plane
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
	Shader objectShader;
	Mesh planeMesh;
	GLint planeTexture;
	GLint objectTexture;
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
	GLfloat repeat = 80.0;
	glm::mat4 projection;
	glm::mat4 view;
	GLfloat currentFrame;
	GLfloat lastFrame;
	GLfloat specularColor[3] = {1.0,1.0,1.0};
	GLfloat ambientColor[3] = {0.1,0.1,0.1};
	glm::vec3 lightPositions[3] = {glm::vec3(5.0f, 10.0f, 10.0f), glm::vec3(-5.0f, 10.0f, 10.0f), glm::vec3(5.0f, 10.0f, -10.0f)};		
	GLfloat objectDiffuseColor[3] = {1.0f,1.0f,1.0f};
	float cutDepthNDC=0.0f;

public:
	//CONSTRUCTOR
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
		objectShader=Shader("lambert.vert", "lambert.frag");
		planeShader=Shader("phong_tex_multiplelights.vert", "blinnphong_tex_multiplelights.frag");
		planeTexture=LoadTexture("../../textures/SoilCracked.png");
		glm::vec4 origin=glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		origin=projection*view*origin;
		origin/=origin.w;
		cutDepthNDC=origin.z;
	}
	//This method is used only to load the plane's texture.
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
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		glGenerateMipmap(GL_TEXTURE_2D);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST_MIPMAP_NEAREST);
		stbi_image_free(image);
		return textureImage;
	}
	//Each time there are no more cuttable meshes visible in the scene,
	//a new mesh is added by calling this method.
	//The new mesh will have a random diffuse color and will be added to the cuttable mesh array and
	//in the physics simulation.
	void AddMesh(string meshPath)
	{
		GLfloat red=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		GLfloat green=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		GLfloat blue=(GLfloat)((GLfloat)(rand()%COLOR_LIMIT)/(GLfloat)COLOR_LIMIT);
		objectDiffuseColor[0]=red;
		objectDiffuseColor[1]=green;
		objectDiffuseColor[2]=blue;
		Model* object = new Model(meshPath);
		std::vector<Mesh>::iterator cuttableMeshesIt=cuttableMeshes.begin();
		cuttableMeshes.insert(cuttableMeshesIt, object->meshes.begin(), object->meshes.end());
		engine.AddRigidBodyWithImpulse(object->shape);
	}
	//This is a method used in the main class; whether all cuttable meshes are deallocated,
	//this method will return a true value, implying that a new mesh will be added.
	bool AllMeshRemoved()
	{
		return cuttableMeshes.size()==0;
	}
	//This function perform the cut of all meshes that intersect the segment defined by the given positions;
	//each mesh cut will generate two new independent meshes are subsequentialy added to the scene.
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
											  positiveConvexHullShape, 
											  negativeConvexHullShape, 
											  positiveWeightFactor, 
											  negativeWeightFactor);
				
				glm::vec3 cutNormal=glm::vec3(-1*(cutEndPointWS.y-cutStartPointWS.y), cutEndPointWS.x-cutStartPointWS.x, 0.0f);
				engine.CutShapeWithImpulse(cutNormal, meshIndex, negativeWeightFactor, negativeMeshPositionWS, negativeConvexHullShape, positiveWeightFactor, positiveMeshPositionWS, positiveConvexHullShape);
				//Delete the old mesh
				cuttableMeshes[meshIndex].Delete();
				iter_swap(cuttableMeshes.begin()+meshIndex, cuttableMeshes.end()-1);
				cuttableMeshes.pop_back();
				cuttableMeshes.push_back(positiveMesh);
				cuttableMeshes.push_back(negativeMesh);	
			}
		}
	}
	//This method just render the background plane and all cuttable meshes; each cuttable mesh gets its model transform,
	//from the simulation class.
	void DrawScene()
	{
		planeShader.Use();
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, planeTexture);
        glUniformMatrix4fv(glGetUniformLocation(planeShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(planeShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
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
        glUniform1i(textureLocation, planeTexture);
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
			objectShader.Use();
			glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
			glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));
			GLint pointLightLocation = glGetUniformLocation(objectShader.Program, "pointLightPosition");
			GLint objectDiffuseLocation = glGetUniformLocation(objectShader.Program, "diffuseColor");
			GLint kdObjectLocation = glGetUniformLocation(objectShader.Program, "Kd");
			glUniform3fv(pointLightLocation, 1, glm::value_ptr(lightPositions[0]));
			glUniform3fv(objectDiffuseLocation, 1, objectDiffuseColor);
			glUniform1f(kdObjectLocation, Kd);
			glm::mat4 objectModelMatrix=engine.GetObjectModelMatrix(i);
			glm::mat3 objectNormalMatrix;
			objectNormalMatrix = glm::inverseTranspose(glm::mat3(view*objectModelMatrix));
			glUniformMatrix4fv(glGetUniformLocation(objectShader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(objectModelMatrix));
			glUniformMatrix3fv(glGetUniformLocation(objectShader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(objectNormalMatrix));
			cuttableMeshesIt->Draw(objectShader);
			i++;
		}
	}
	//This method is called to update the physics simulation.
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
	
			if(transform.getOrigin().getY()<=Y_KILL)
			{
				cuttableMeshes[i].Delete();
				iter_swap(cuttableMeshes.begin()+i, cuttableMeshes.end()-1);
				cuttableMeshes.pop_back();
				engine.RemoveRigidBodyAtIndex(i);
			}
		}
	}
	//This function is called only during the application shutdown, to remove everything the scene object has allocated.
	void Clear()
	{
		engine.Clear();
		objectShader.Delete();
		planeMesh.Delete();
		std::vector<Mesh>::iterator cuttableMeshesIt;
		for (cuttableMeshesIt = cuttableMeshes.begin(); cuttableMeshesIt != cuttableMeshes.end(); ++cuttableMeshesIt)
			cuttableMeshesIt->Delete();
		cuttableMeshes.clear();		
	}    
};
