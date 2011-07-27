#include "dynamics/BodyNodeDynamics.h"
#include "dynamics/SkeletonDynamics.h"
#include "model3d/FileInfoSkel.hpp"
#include "model3d/FileInfoDof.h"
#include "model3d/BodyNode.h"

#include "MyWindow.h"

#include <iostream>
#include <Eigen/dense>

using namespace std;
using namespace Eigen;
using namespace model3d;
using namespace dynamics;

#include "utils/UtilsMath.h"
#include "utils/Paths.h"

int main(int argc, char* argv[])
{
    const char* modelfile;
    const char* doffile;
    if(argc!=3){
		modelfile = GROUNDZERO_DATA_PATH"skel/YutingEuler.skel";
		doffile = GROUNDZERO_DATA_PATH"dof/RHand.dof";
    }else{
        modelfile = argv[1];
        doffile = argv[2];
    }
	
    model3d::FileInfoSkel<dynamics::SkeletonDynamics> skel;
    skel.loadFile(modelfile, model3d::SKEL);

    SkeletonDynamics *skelDyn = static_cast<SkeletonDynamics*>(skel.getSkel());

    model3d::FileInfoDof motion(skel.getSkel());
    motion.loadFile(doffile);

    // test the velocity computation using the two methods: inverse dynamics and regular
    VectorXd q = VectorXd::Zero(skel.getSkel()->getNumDofs());
    VectorXd qdot = VectorXd::Zero(skel.getSkel()->getNumDofs());
    for(int i=0; i<skel.getSkel()->getNumDofs(); i++){
        q[i] = utils::random(-1.0, 1.0);
        qdot[i] = utils::random(-5.0, 5.0);
    }
    // set the pose
    skel.getSkel()->setPose(q);
    Vector3d gravity(0.0, -9.81, 0.0);
    // solve the inverse dynamics

    //// test the velocities computed by the two methods
    //skelDyn->computeInverseDynamicsLinear(gravity, &qdot);
    //for(int i=0; i<skel.getSkel()->getNumNodes(); i++){
    //    BodyNodeDynamics *nodei = static_cast<BodyNodeDynamics*>(skelDyn->getNode(i));
    //    // compute velocities using regular method
    //    nodei->updateTransform();
    //    nodei->updateFirstDerivatives();
    //    nodei->evalJacLin();
    //    nodei->evalJacAng();
    //    Vector3d v1 = Vector3d::Zero();
    //    Vector3d w1 = Vector3d::Zero();
    //    for(int j=0; j<nodei->getNumDependantDofs(); j++){
    //        int dj = nodei->getDependantDof(j);
    //        for(int k=0; k<3; k++) {
    //            v1[k] += nodei->mJv(k, j)*qdot[dj];
    //            w1[k] += nodei->mJw(k, j)*qdot[dj];
    //        }
    //    }

    //    // compute velocities using inverse dynamics routine
    //    Vector3d v2 = nodei->mW.topLeftCorner(3,3)*nodei->mVelBody;
    //    Vector3d w2 = nodei->mW.topLeftCorner(3,3)*nodei->mOmegaBody;

    //    cout<<"Node: "<<nodei->getName()<<endl;
    //    //cout<<"Angular Jacobian regular: \n"<<(nodei->mJw).rightCols(nodei->getParentJoint()->getNumDofsRot())<<endl;
    //    //MatrixXd WJw = nodei->mJwJoint;
    //    //if(nodei->getParentNode()) WJw = nodei->getParentNode()->mW.topLeftCorner(3,3)*WJw;
    //    //cout<<"Angular Jacobian InvDyn : \n"<<WJw<<endl;
    //    cout<<"Linear velocity regular: \n"<<v1<<endl;
    //    cout<<"Linear velocity InvDyn : \n"<<v2<<endl;
    //    cout<<endl;
    //    cout<<"Angular velocity regular: \n"<<w1<<endl;
    //    cout<<"Angular velocity InvDyn : \n"<<w2<<endl;
    //    cout<<endl;

    //    getchar();
    //}
    //exit(1);

    // test the Jwdot using finite differences
    double dt = 1.0e-3;
    VectorXd origq = q;
    VectorXd newq = q + qdot*dt;
    for(int i=0; i<skel.getSkel()->getNumNodes(); i++){
        BodyNodeDynamics *nodei = static_cast<BodyNodeDynamics*>(skelDyn->getNode(i));

        skel.getSkel()->setPose(origq);
        skelDyn->computeInverseDynamicsLinear(gravity, &qdot);
        Matrix3d Ri = nodei->mW.topLeftCorner(3,3);

        MatrixXd JwOrig = nodei->mJwJoint;
        MatrixXd JwDotOrig = nodei->mJwDotJoint;
        Vector3d wOrig = Ri*nodei->mOmegaBody;
        Vector3d wDotOrig = Ri*nodei->mOmegaDotBody;
        Vector3d vOrig = Ri*nodei->mVelBody;
        Vector3d vDotOrig = Ri*nodei->mVelDotBody;

        skel.getSkel()->setPose(newq);
        skelDyn->computeInverseDynamicsLinear(gravity, &qdot);
        Matrix3d Rinew = nodei->mW.topLeftCorner(3,3);

        MatrixXd JwNew = nodei->mJwJoint;
        MatrixXd JwDotApprox = (JwNew-JwOrig)/dt;
        Vector3d wNew = Rinew*nodei->mOmegaBody;
        Vector3d wDotApprox = (wNew-wOrig)/dt;
        Vector3d vNew = Rinew*nodei->mVelBody;
        Vector3d vDotApprox = (vNew-vOrig)/dt;

        cout<<"JwDot: \n"<<JwDotOrig<<endl;
        cout<<"JwDot approx: \n"<<JwDotApprox<<endl;
        cout<<"wDot: \n"<<wDotOrig<<endl;
        cout<<"wDot approx: \n"<<wDotApprox<<endl;
        cout<<"vDot: \n"<<vDotOrig<<endl;
        cout<<"vDot approx: \n"<<vDotApprox<<endl;

        cout<<endl;
        //getchar();
    }

    // test the dynamics
    VectorXd Cginvdyn = skelDyn->computeInverseDynamicsLinear(gravity, &qdot);
    skelDyn->computeDynamics(gravity, qdot, false); // compute dynamics by not using inverse dynamics

    cout<<"C+g term inverse dynamics: "<<Cginvdyn<<endl;
    cout<<"C+g term regular dynamics: "<<skelDyn->mCg<<endl;
    cout<<"Difference: \n"<<Cginvdyn - skelDyn->mCg<<endl;
    
    exit(1);


    MyWindow window(motion);
    window.computeMax();

    glutInit(&argc, argv);
    window.initWindow(640, 480, modelfile);
    glutMainLoop();

    return 0;
}
