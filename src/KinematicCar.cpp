#include <ompl/control/SpaceInformation.h>
#include <ompl/base/spaces/SE2StateSpace.h>
#include <ompl/control/ODESolver.h>
#include <ompl/control/spaces/RealVectorControlSpace.h>
#include <ompl/control/SimpleSetup.h>
#include <ompl/config.h>
// std includes
#include <iostream>
#include <valarray>
#include <limits>
#include <boost/geometry.hpp>
#include <boost/geometry/geometries/geometries.hpp>
// my includes
#include "../includes/KinematicCar.h"

namespace ob = ompl::base;
namespace oc = ompl::control;
namespace bg = boost::geometry;
namespace trans = boost::geometry::strategy::transform;


// this function is used for any 2D projection needed
// include but not limited to: obs checking, and path segmenting
std::vector<polygon> TwoKinematicCarsModel::GetPolygons()
{
    std::vector<polygon> Vehicles;
    // *********************
    // ***** vehicle 1 *****
    // *********************

    // create the shape of the vehicle centered at (0, 0)
    // point A1 = [-0.5l, -0.5w]
    point BackR1( -0.5 * carLength_, -0.5 * carWidth_);
    // point B1 = [-0.5l, +0.5w]
    point BackL1( (-0.5 * carLength_), (0.5 * carWidth_));
    // point C1 = [+0.5l, +0.5w]
    point FrontL1( (0.5 * carLength_), (0.5 * carWidth_));
    // point B1 = [+0.5l, -0.5w]
    point FrontR1( (0.5 * carLength_), (-0.5 * carWidth_));

    // note that for some reason, boost rotation is not conventional to counter clockwise = positive angle
    // to counter this, I use a negative angle
    trans::rotate_transformer<bg::radian, double, 2, 2>rotate1(-(rot1_->value));

    boost::geometry::transform(BackR1, BackR1, rotate1);
    boost::geometry::transform(BackL1, BackL1, rotate1);
    boost::geometry::transform(FrontL1, FrontL1, rotate1);
    boost::geometry::transform(FrontR1, FrontR1, rotate1);

    // now, translate the polygon to the state location
    trans::translate_transformer<double, 2, 2> translate1(xyState1_->values[0], xyState1_->values[1]);

    boost::geometry::transform(BackR1, BackR1, translate1);
    boost::geometry::transform(BackL1, BackL1, translate1);
    boost::geometry::transform(FrontL1, FrontL1, translate1);
    boost::geometry::transform(FrontR1, FrontR1, translate1);

    // create instance of polygon
    polygon v1;
    // // add the outer points to the shape
    v1.outer().push_back(BackR1);
    v1.outer().push_back(BackL1);
    v1.outer().push_back(FrontL1);
    v1.outer().push_back(FrontR1);
    v1.outer().push_back(BackR1);

    Vehicles.push_back(v1);

    // // *********************
    // // ***** vehicle 2 *****
    // // *********************

    point BackR2( -0.5 * carLength_, -0.5 * carWidth_);
    // point B1 = [-0.5l, +0.5w]
    point BackL2( (-0.5 * carLength_), (0.5 * carWidth_));
    // point C1 = [+0.5l, +0.5w]
    point FrontL2( (0.5 * carLength_), (0.5 * carWidth_));
    // point B1 = [+0.5l, -0.5w]
    point FrontR2( (0.5 * carLength_), (-0.5 * carWidth_));

    trans::rotate_transformer<bg::radian, double, 2, 2>rotate2(-(rot2_->value));

    boost::geometry::transform(BackR2, BackR2, rotate2);
    boost::geometry::transform(BackL2, BackL2, rotate2);
    boost::geometry::transform(FrontL2, FrontL2, rotate2);
    boost::geometry::transform(FrontR2, FrontR2, rotate2);

    // now, translate the polygon to the state location
    trans::translate_transformer<double, 2, 2> translate2(xyState2_->values[0], xyState2_->values[1]);

    boost::geometry::transform(BackR2, BackR2, translate2);
    boost::geometry::transform(BackL2, BackL2, translate2);
    boost::geometry::transform(FrontL2, FrontL2, translate2);
    boost::geometry::transform(FrontR2, FrontR2, translate2);

    // create instance of polygon
    polygon v2;
    // // add the outer points to the shape
    v2.outer().push_back(BackR1);
    v2.outer().push_back(BackL1);
    v2.outer().push_back(FrontL1);
    v2.outer().push_back(FrontR1);
    v2.outer().push_back(BackR1);

    Vehicles.push_back(v2);

    return Vehicles;
}


void list_coordinates(point const& p) 
{ 
    using boost::geometry::get;
    
    std::cout << "x = " << get<0>(p) << " y = " << get<1>(p) << std::endl; 

} 



// Definition of the ODE for the kinematic car.
// This method is analogous to the above KinematicCarModel::ode function.
void KinematicCarODE (const oc::ODESolver::StateType& q, const oc::Control* control, oc::ODESolver::StateType& qdot)
{
    const double *u = control->as<oc::RealVectorControlSpace::ControlType>()->values;
    const double theta = q[2];
    double carLength = 0.2;

    // Zero out qdot
    qdot.resize (q.size (), 0);

    qdot[0] = u[0] * cos(theta);
    qdot[1] = u[0] * sin(theta);
    qdot[2] = u[0] * tan(u[1]) / carLength;
}

// This is a callback method invoked after numerical integration.
void KinematicCarPostIntegration (const ob::State* /*state*/, const oc::Control* /*control*/, const double /*duration*/, ob::State *result)
{
    // Normalize orientation between 0 and 2*pi
    ob::SO2StateSpace SO2;
    SO2.enforceBounds(result->as<ob::SE2StateSpace::StateType>()->as<ob::SO2StateSpace::StateType>(1));
}

// multi agent ODE function
void TwoKinematicCarsODE (const oc::ODESolver::StateType& q, const oc::Control* control, oc::ODESolver::StateType& qdot)
{
    // std::cout << "here" << std::endl;
    const double *u = control->as<oc::RealVectorControlSpace::ControlType>()->values;
    // q = x1, y1, theta1, x2, y2, theta2
    // c = v1, phi1, v2, phi2 
    const double theta1 = q[2];
    const double theta2 = q[5];
    double carLength = 0.2;

    // Zero out qdot
    qdot.resize (q.size (), 0);
    // vehicle 1
    qdot[0] = u[0] * cos(theta1);
    qdot[1] = u[0] * sin(theta1);
    qdot[2] = u[0] * tan(u[1]) / carLength;
    // vehicle 2
    qdot[3] = u[2] * cos(theta2);
    qdot[4] = u[2] * sin(theta2);
    qdot[5] = u[2] * tan(u[3]) / carLength;
}
// multi agent callback function
void postProp_TwoKinematicCars(const ob::State *q, const oc::Control *ctl, 
    const double duration, ob::State *qnext)
{
    //pull the angles from both cars
    ob::CompoundState* cs = qnext->as<ob::CompoundState>();

    ob::SO2StateSpace::StateType* angleState1 = cs->as<ob::SO2StateSpace::StateType>(1);
    ob::SO2StateSpace::StateType* angleState2 = cs->as<ob::SO2StateSpace::StateType>(3);

    //use ompl to normalize theta
    ob::SO2StateSpace SO2;
    SO2.enforceBounds(angleState1);
    SO2.enforceBounds(angleState2);

}








