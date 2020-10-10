/*
Physics class - v1:
- initialization of the physics simulation using the Bullet librarty

The class sets up the collision manager and the resolver of the constraints, using basic general-purposes methods provided by the library. Advanced and multithread methods are available, please consult Bullet documentation and examples

createRigidBody method sets up a  Box or Sphere Collision Shape. For other Shapes, you must extend the method.

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2018/2019
Master degree in ComputclTabCtrler Science
Universita' degli Studi di Milano
*/

#pragma once
//#define GRAVITY -9.82f
#define GRAVITY 0.0f
#define X_BOUNDARY 6
#define X_IMPULSE_BOUNDARY 2
#define Y_IMPULSE_BOUNDARY 13

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
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
		btCollisionObject* collisionObject = dynamicsWorld->getCollisionObjectArray()[0];
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
	
	void AddRigidBodyWithImpulse(int i)
	{
		btCollisionShape* shape;
		switch(i)
		{
			case 0:
				shape=new btBoxShape(btVector3(btScalar(1.0f), btScalar(1.0f), btScalar(1.0f)));
				break;
			case 1:
				shape=new btConeShape(btScalar(1.0f), btScalar(2.0f));
				break;
			case 2:
				shape=new btCylinderShape(btVector3(btScalar(1.0f), btScalar(2.0f), btScalar(0.0)));
				break;
			default:
				shape=new btSphereShape(btScalar(1.0f));
				break;
		}
		collisionShapes.push_back(shape);
		btTransform startTransform;
		startTransform.setIdentity();
		btScalar mass(1.f);
		btVector3 localInertia(0, 0, 0);
		shape->calculateLocalInertia(mass, localInertia);
		btScalar xModel = ((rand()%101)/101.f)*X_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
		btScalar yModel = -5.9;
		startTransform.setOrigin(btVector3(0, -2, 0));
		btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
		btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
		rbInfo.m_angularDamping =0.90f;
        // we create the rigid body
		btRigidBody* body = new btRigidBody(rbInfo);
		dynamicsWorld->addRigidBody(body);
		btScalar xImpulse = ((rand()%101)/101.f)*X_IMPULSE_BOUNDARY * ((rand()%2)>0) ? 1 : -1;
		btScalar yImpulse = Y_IMPULSE_BOUNDARY;
		//body->applyImpulse(btVector3(xImpulse,yImpulse,0), btVector3(1,0,0));
	}
	
	void RemoveRigidBodyAtIndex(int i)
	{
		printf("Coll. objects: %d.\n", dynamicsWorld->getCollisionObjectArray().size());
		btCollisionObject* collisionObject = dynamicsWorld->getCollisionObjectArray()[i];
		dynamicsWorld->removeCollisionObject(collisionObject);
		delete collisionObject;
		
		printf("Coll. shapes: %d.\n", collisionShapes.size());
		btCollisionShape* shape=collisionShapes[i];
		collisionShapes.removeAtIndex(i);
		delete shape;
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
