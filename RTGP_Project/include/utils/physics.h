/*
Physics class:

This class manages the physics simulation of the scene developed for this project.
Each mesh drawn in the scene is also associated with a convex hull; all these convex hulls are stored inside
the collisionShapes object.
*/

#pragma once
#define GRAVITY -9.82f
#define CUT_IMPULSE 0.20f
#define X_BOUNDARY 3.f
#define X_IMPULSE_BOUNDARY 2.f
#define Y_IMPULSE_BOUNDARY 13.f

#include <glm/glm.hpp>
#include <btConvex2dShape.h>
#include <btConvexShape.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <unordered_map>
#include <utils/log.h>
#include <utils/mesh.h>
#include <utils/log.h>
#include <bullet/btBulletDynamicsCommon.h>

class Physics
{
public:

    btDiscreteDynamicsWorld* dynamicsWorld;
	btAlignedObjectArray<btCollisionShape*> collisionShapes;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btBroadphaseInterface* overlappingPairCache;
    btSequentialImpulseConstraintSolver* solver;

    //CONSTRUCTOR
    Physics()
    {
        this->collisionConfiguration = new btDefaultCollisionConfiguration();
        this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);
        this->overlappingPairCache = new btDbvtBroadphase();
        this->solver = new btSequentialImpulseConstraintSolver();
        this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher,this->overlappingPairCache,this->solver,this->collisionConfiguration);
        this->dynamicsWorld->setGravity(btVector3(0.0f,GRAVITY,0.0f));
    }
	//In scene class, meshes are stored inside a vector of meshes; so each one of them has un unique index inside that vector.
	//That index can be used here as well, to get the updated model transform of a mesh.
	glm::mat4 GetObjectModelMatrix(int i)
	{
		btCollisionObject* collisionObject = dynamicsWorld->getCollisionObjectArray()[i];
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
		return glm::make_mat4(glmTransform);
	}
	//Everytime a cut occurs, we must provide two new convex hulls to the physics engine, to simulate each piece of the cut mesh correctly.
	//This method adds the two new convex hulls generated to the simulation and applies an impulse to them, to make the physical behaviour of the cut more believable.
	void CutShapeWithImpulse(glm::vec3 cutNormal, int i, float negativeWeightFactor, glm::vec4 negativeMeshPosition, btConvexHullShape* negativeConvexHullShape, float positiveWeightFactor, glm::vec4 positiveMeshPosition, btConvexHullShape* positiveConvexHullShape)
	{
		btCollisionObject* cuttedCollisionObject = dynamicsWorld->getCollisionObjectArray()[i];
		btRigidBody* cuttedRigidBody = btRigidBody::upcast(cuttedCollisionObject);
		btTransform positiveTransform;
		btTransform negativeTransform;
		
		if (cuttedRigidBody && cuttedRigidBody->getMotionState())
		{
			cuttedRigidBody->getMotionState()->getWorldTransform(positiveTransform);
			cuttedRigidBody->getMotionState()->getWorldTransform(negativeTransform);
		}
		else
		{
			positiveTransform = cuttedCollisionObject->getWorldTransform();
			negativeTransform = cuttedCollisionObject->getWorldTransform();
		}
		
		RemoveRigidBodyAtIndex(i);
		
		positiveTransform.setOrigin(btVector3(positiveMeshPosition.x, positiveMeshPosition.y, positiveMeshPosition.z));
		negativeTransform.setOrigin(btVector3(negativeMeshPosition.x, negativeMeshPosition.y, negativeMeshPosition.z));
		
		btScalar positiveMass(positiveWeightFactor);
		btScalar negativeMass(negativeWeightFactor);
		btVector3 localInertia(0, 0, 0);
		btDefaultMotionState* positiveMotionState = new btDefaultMotionState(positiveTransform);
		btDefaultMotionState* negativeMotionState = new btDefaultMotionState(negativeTransform);
		
		positiveConvexHullShape->calculateLocalInertia(positiveWeightFactor, localInertia);
		negativeConvexHullShape->calculateLocalInertia(negativeWeightFactor, localInertia);
		
		btRigidBody::btRigidBodyConstructionInfo positiveRbInfo(positiveMass, positiveMotionState, positiveConvexHullShape, localInertia);
		btRigidBody::btRigidBodyConstructionInfo negativeRbInfo(negativeMass, negativeMotionState, negativeConvexHullShape, localInertia);
		positiveRbInfo.m_angularDamping = negativeRbInfo.m_angularDamping = 0.9f;
		btRigidBody* positiveRb = new btRigidBody(positiveRbInfo);
		btRigidBody* negativeRb = new btRigidBody(negativeRbInfo);
		glm::vec3 cutImpulseDirection=cutNormal;
		cutImpulseDirection*=CUT_IMPULSE;
		positiveRb->applyImpulse(btVector3(cutImpulseDirection.x, cutImpulseDirection.y, cutImpulseDirection.z), btVector3(0.5, 0.5, 0));
		negativeRb->applyImpulse(btVector3(-cutImpulseDirection.x, -cutImpulseDirection.y, cutImpulseDirection.z), btVector3(-0.5, 0.5, 0));
		collisionShapes.push_back(positiveConvexHullShape);
		collisionShapes.push_back(negativeConvexHullShape);
		dynamicsWorld->addRigidBody(positiveRb);
		dynamicsWorld->addRigidBody(negativeRb);
	}
	//This method adds the convex hull shape given to the simulation and gives an impulse to it,
	//in order to make the respective mesh appears in the view of the camera.
	void AddRigidBodyWithImpulse(btConvexHullShape* shape)
	{
		btTransform startTransform;
		startTransform.setIdentity();
		btScalar mass(1.f);
		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(mass, localInertia);
		btScalar xModel = ((rand()%101)/100.f)*X_BOUNDARY * ((rand()%2) == 0 ? 1.f : -1.f);
		btScalar yModel = -5.9;
		startTransform.setOrigin(btVector3(xModel, yModel, 0));
		btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
		rbInfo.m_angularDamping =0.90f;
		btRigidBody* body = new btRigidBody(rbInfo);
		collisionShapes.push_back(shape);
		dynamicsWorld->addRigidBody(body);
		btScalar xImpulse = ((rand()%101)/101.f)*X_IMPULSE_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
		btScalar yImpulse = Y_IMPULSE_BOUNDARY;
		body->applyImpulse(btVector3(xImpulse,yImpulse,0), btVector3(1.f,0,0));
	}

	void RemoveRigidBodyAtIndex(int i)
	{
		btCollisionObject* collisionObject = dynamicsWorld->getCollisionObjectArray()[i];
		dynamicsWorld->removeCollisionObject(collisionObject);
		delete collisionObject;
		btCollisionShape* shape=collisionShapes[i];
		collisionShapes.remove(shape);
		delete shape;
	}
	
	int GetCollisionShapeIndex(const btCollisionShape* collisionShape)
	{
		for(int i=0; i<collisionShapes.size(); i++)
		{
			if(collisionShape==collisionShapes[i])
				return i;
		}
		
		return -1;
	}

    void Clear()
    {
        for (int i=this->dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
        {
            btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState())
            {
                delete body->getMotionState();
            }
            this->dynamicsWorld->removeCollisionObject( obj );
            delete obj;
        }

        for (int j=0;j<this->collisionShapes.size();j++)
        {
            btCollisionShape* shape = this->collisionShapes[j];
            this->collisionShapes[j] = 0;
            delete shape;
        }

        delete this->dynamicsWorld;

        delete this->solver;

        delete this->overlappingPairCache;

        delete this->dispatcher;

        delete this->collisionConfiguration;

        this->collisionShapes.clear();
    }
};
