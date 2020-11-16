#pragma once
#define GRAVITY -9.82f
#define CUT_IMPULSE 0.20f
#define X_BOUNDARY 6
#define X_IMPULSE_BOUNDARY 2
#define Y_IMPULSE_BOUNDARY 13

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

///////////////////  Physics class ///////////////////////
class Physics
{
public:

    btDiscreteDynamicsWorld* dynamicsWorld; // the main physical simulation class
    btAlignedObjectArray<btCollisionShape*> collisionShapes; // a vector for all the Collision Shapes of the scene
    btDefaultCollisionConfiguration* collisionConfiguration; // setup for the collision manager
    btCollisionDispatcher* dispatcher; // collision manager
    btBroadphaseInterface* overlappingPairCache; // method for the broadphase collision detection
    btSequentialImpulseConstraintSolver* solver; // constraints solver

    //////////////////////////////////////////
    // constructor
    // we set all the classes needed for the physical simulation
    Physics()
    {
        // Collision configuration, to be used by the collision detection class
        //collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
        this->collisionConfiguration = new btDefaultCollisionConfiguration();

        //default collision dispatcher (=collision detection method). For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);

        //btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
        this->overlappingPairCache = new btDbvtBroadphase();

        // we set a ODE solver, which considers forces, constraints, collisions etc., to calculate positions and rotations of the rigid bodies.
        //the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        this->solver = new btSequentialImpulseConstraintSolver();

        //  DynamicsWorld is the main class for the physical simulation
        this->dynamicsWorld = new btDiscreteDynamicsWorld(this->dispatcher,this->overlappingPairCache,this->solver,this->collisionConfiguration);

        // we set the gravity force
        this->dynamicsWorld->setGravity(btVector3(0.0f,GRAVITY,0.0f));
    }
	
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
	
	void CutShapeWithImpulse(glm::vec3 cutNormal, glm::vec3 cutDirection, int i, float negativeWeightFactor, glm::vec4 negativeMeshPosition, btConvexHullShape* negativeConvexHullShape, float positiveWeightFactor, glm::vec4 positiveMeshPosition, btConvexHullShape* positiveConvexHullShape)
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
	
	void AddRigidBodyWithImpulse(btConvexHullShape* shape)
	{
		Log log=Log();
		log.InitLog("AddRigidBodyWithImpulse");
		btTransform startTransform;
		startTransform.setIdentity();
		btScalar mass(1.f);
		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(mass, localInertia);
		btScalar xModel = ((rand()%101)/100.f)*X_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
		btScalar yModel = -5.9;
		startTransform.setOrigin(btVector3(xModel, yModel, 0));
		btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
		rbInfo.m_angularDamping =0.90f;
        // we create the rigid body
		btRigidBody* body = new btRigidBody(rbInfo);
		collisionShapes.push_back(shape);
		dynamicsWorld->addRigidBody(body);
		btScalar xImpulse = ((rand()%101)/101.f)*X_IMPULSE_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
		btScalar yImpulse = Y_IMPULSE_BOUNDARY;
		body->applyImpulse(btVector3(xImpulse,yImpulse,0), btVector3(1.f,0,0));
		log.EndLog();
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

    //////////////////////////////////////////
    // We delete the data of the physical simulation when the program ends
    void Clear()
    {
        //we remove the rigid bodies from the dynamics world and delete them
        for (int i=this->dynamicsWorld->getNumCollisionObjects()-1; i>=0 ;i--)
        {
            // we remove all the Motion States
            btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[i];
            // we upcast in order to use the methods of the main class RigidBody
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState())
            {
                delete body->getMotionState();
            }
            this->dynamicsWorld->removeCollisionObject( obj );
            delete obj;
        }

        // we remove all the Collision Shapes
        for (int j=0;j<this->collisionShapes.size();j++)
        {
            btCollisionShape* shape = this->collisionShapes[j];
            this->collisionShapes[j] = 0;
            delete shape;
        }

        //delete dynamics world
        delete this->dynamicsWorld;

        //delete solver
        delete this->solver;

        //delete broadphase
        delete this->overlappingPairCache;

        //delete dispatcher
        delete this->dispatcher;

        delete this->collisionConfiguration;

        this->collisionShapes.clear();
    }
};
