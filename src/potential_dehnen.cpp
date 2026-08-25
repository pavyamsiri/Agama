#include "potential_dehnen.h"
#include "math_core.h"
#include <cmath>
#include <stdexcept>
#include <cassert>

namespace potential {

Dehnen::Dehnen(double _mass, double _scaleRadius, double _gamma, double _axisRatioY, double _axisRatioZ): 
    BasePotentialCar(), mass(_mass), scaleRadius(_scaleRadius),
    gamma(_gamma), axisRatioY(_axisRatioY), axisRatioZ(_axisRatioZ)
{
    if(scaleRadius<=0)
        throw std::invalid_argument("Dehnen potential: scale radius must be positive");
    if(gamma<0 || gamma>2)
        throw std::invalid_argument("Dehnen potential: gamma must lie in the range [0:2]");
}

double Dehnen::densityCar(const coord::PosCar& pos, double /*time*/) const
{
    double m = sqrt(pow_2(pos.x) + pow_2(pos.y/axisRatioY) + pow_2(pos.z/axisRatioZ));
    return mass * scaleRadius * (3-gamma) * math::pow(m / (scaleRadius + m), -gamma) /
        (4*M_PI * axisRatioY * axisRatioZ * pow_2(pow_2(scaleRadius + m)));
}

namespace{  // internal

/// compute two auxiliary quantities in a way that avoids cancellations
void getAB(double gamma, double m, /*output*/ double& A, double& B)
{
    if(m < 2-gamma) {
        A = math::pow(m / (m+1), 2-gamma);  // A is small or at least not too close to 1
        B = (1 - A) / (2-gamma);            // B can be computed without severe cancellations
    } else {  // same here, but first computing B without severe cancellations, then A
        B = gamma<2 ? expm1( (gamma-2) * log1p(1/m) ) / (gamma-2) : log1p(1/m);
        A = 1 - B * (2-gamma);
    }
}

/// combined integrand for potential, gradient and hessian in the non-spherical case
class DehnenIntegrand: public math::IFunctionNdim {
    const double X2, Y2, Z2, gamma, p2, q2, c;
    const int NVAL;  // number of integrands to compute (1, 4 or 10)
public:
    DehnenIntegrand(const coord::PosCar& pos, double rscale, double gam, double p, double q, int nval) :
        X2(pow_2(pos.x/rscale)), Y2(pow_2(pos.y/rscale)), Z2(pow_2(pos.z/rscale)),
        gamma(gam), p2(p*p), q2(q*q), c((1 + sqrt(X2 + Y2 + Z2)) / 3), NVAL(nval)
    {}
    virtual unsigned int numVars()   const { return 1; }
    virtual unsigned int numValues() const { return NVAL; }
    virtual void eval(const double s[], double values[]) const
    {   // scaled integration variable "s" is in the range [0..1]
        double A, B,
        invs= 1 / s[0],
        u   = invs - 1,
        v   = u * (1 + u * u * c),
        tau = v * (2 + v),
        it1 = 1 / (tau + 1),
        itp = 1 / (tau + p2),
        itq = 1 / (tau + q2),
        m   = sqrt(X2 * it1 + Y2 * itp + Z2 * itq),
        dtds= (2 + u * u * c * 6) * (1 + v) * pow_2(invs),
        fac = dtds * sqrt(it1 * itp * itq);
        getAB(gamma, m, A, B);
        // note: severe cancellation in (B-A/(1+m)) at m>>1, but subdominant to integration error
        values[0] = fac * (B - A / (1+m));
        if(NVAL < 4)
            return;
        // components of the potential gradient
        fac *= A / pow_2((1+m) * m);
        values[1] = fac * it1;
        values[2] = fac * itp;
        values[3] = fac * itq;
        if(NVAL < 10)
            return;
        // components of the hessian
        fac *= (4 * m + gamma) / ((1+m) * m * m);
        values[4] = fac * it1 * it1;
        values[5] = fac * itp * itp;
        values[6] = fac * itq * itq;
        values[7] = fac * it1 * itp;
        values[8] = fac * it1 * itq;
        values[9] = fac * itp * itq;
    }
};

} // internal namespace

void Dehnen::evalCar(const coord::PosCar &pos,
    double* potential, coord::GradCar* deriv, coord::HessCar* deriv2, double /*time*/) const
{
    double m = sqrt(pos.x*pos.x + pos.y*pos.y + pos.z*pos.z) / scaleRadius;
    if(m == 0 || m == INFINITY) {  // special case
        if(potential) {
            if(m == INFINITY)
                *potential = std::signbit(mass) ? 0.0 : -0.0;
            else if(axisRatioY==1 && axisRatioZ==1)
                *potential = -mass / scaleRadius / (2-gamma);
            else {
                // there is an analytic expression for Phi(0) in terms of elliptic functions,
                // but for consistency we use the same numerical integration as for an arbitrary r
                math::integrateGL(DehnenIntegrand(pos, scaleRadius, gamma, axisRatioY, axisRatioZ, 1),
                    0, 1, 40, potential);
                *potential *= -0.5 * mass / scaleRadius;
            }
        }
        if(deriv)
            deriv->dx = deriv->dy = deriv->dz = 0;
        if(deriv2)
            deriv2->dx2 = deriv2->dy2 = deriv2->dz2 = deriv2->dxdy = deriv2->dydz = deriv2->dxdz = NAN;
        return;
    }
    if(axisRatioY==1 && axisRatioZ==1) {  // analytical expression for spherical potential
        double A, B;
        getAB(gamma, m, A, B);
        double fac = mass / scaleRadius;
        if(potential) {
            *potential = -fac * B;
        }
        fac *= A / ((1+m) * pow_2(m * scaleRadius));
        if(deriv) {
            deriv->dx = fac * pos.x;
            deriv->dy = fac * pos.y;
            deriv->dz = fac * pos.z;
        }
        if(deriv2) {
            double fac2 = -fac * (3 * m + gamma) / ((1+m) * pow_2(m * scaleRadius));
            deriv2->dx2 = fac2 * pos.x * pos.x + fac;
            deriv2->dy2 = fac2 * pos.y * pos.y + fac;
            deriv2->dz2 = fac2 * pos.z * pos.z + fac;
            deriv2->dxdy= fac2 * pos.x * pos.y;
            deriv2->dxdz= fac2 * pos.x * pos.z;
            deriv2->dydz= fac2 * pos.y * pos.z;
        }
        return;
    }
    DehnenIntegrand integrand(pos, scaleRadius, gamma, axisRatioY, axisRatioZ,
        /*number of integrands*/ deriv2 ? 10 : deriv ? 4 : 1);
    double result[10];
    // use a fixed-order quadrature, which reaches machine precision for gamma<=1 and r/a<200,
    // otherwise errors reach a level ~1e-9 at r/a=1e5 and are even worse at larger radii
    math::integrateGL(integrand, 0, 1, 40, result);  
    if(potential) {
        *potential = mass / scaleRadius * -0.5 * result[0];
    }
    double fac = mass / pow_3(scaleRadius) * 0.5 * (3-gamma);
    if(deriv) {
        deriv->dx = fac * pos.x * result[1];
        deriv->dy = fac * pos.y * result[2];
        deriv->dz = fac * pos.z * result[3];
    }
    if(deriv2) {
        double fac2 = -fac / pow_2(scaleRadius);
        deriv2->dx2 = fac2 * result[4] * pos.x * pos.x + fac * result[1];
        deriv2->dy2 = fac2 * result[5] * pos.y * pos.y + fac * result[2];
        deriv2->dz2 = fac2 * result[6] * pos.z * pos.z + fac * result[3];
        deriv2->dxdy= fac2 * result[7] * pos.x * pos.y;
        deriv2->dxdz= fac2 * result[8] * pos.x * pos.z;
        deriv2->dydz= fac2 * result[9] * pos.y * pos.z;
    }
}

}  // namespace potential
