#include "galaxymodel_densitygrid.h"
#include "galaxymodel_base.h"
#include "math_core.h"
#include "math_sphharm.h"
#include "utils.h"
#include <cmath>
#include <algorithm>
#include <cassert>
#include <stdexcept>
#ifndef _MSC_VER
#include <alloca.h>
#else
#include <malloc.h>
#endif

/** The integration in computeDensityProjection() methods can be implemented using
    either fixed-order Gauss-Legendre schemes or the adaptive routine integrateNdim.
    The former case has predictable cost, but unpredictable accuracy, which can get very bad
    for density profiles that vary strongly within one grid segment.
    The latter case, by contrast, tries to achieve (and usually far exceeds) the prescribed accuracy,
    but a downside is that it does not produce exact result for spherically- or axisymmetric
    density profiles; nevertheless, it is generally recommended.
*/
#define ADAPTIVE_INTEGRATION

namespace galaxymodel{

namespace{

/// relative accuracy of computing the projection of density onto basis functions
static const double EPSREL_DENSITY_INT = 1e-4;

/// max number of evaluations for integrating the density times basis functions in each cell
static const unsigned int MAX_NUM_EVAL = 1e4;

#ifndef ADAPTIVE_INTEGRATION
/// order of Gauss-Legendre integration in radial (or vertical) directions for all schemes
static const unsigned int GLORDER_RAD  = 8;

/// order of Gauss-Legendre integration in angular direction for the classical grid scheme
static const unsigned int GLORDER_ANG  = 4;

/// minimum order of spherical-harmonic or Fourier expansion for computing the density projection
static const int LMIN_SPHHARM = 12;

/// number of additional sph.-harm. or Fourier terms on top of the requested output order of expansion
/// (to improve the accuracy of integration in angles)
static const int LADD_SPHHARM = 6;

/// eliminate spherical-harmonic or Fourier terms whose relative amplitude is less than this number
static const double EPS_COEF = 1e-12;
#endif

// decode the index of the cell for TargetDensityClassic<0>,
// or its four corners for TargetDensityClassic<1>
template<int N> void getCornerIndicesClassic(
    /*input*/  int pane, int ind1, int ind2, int stripsPerPane,
    /*output*/ int& indll, int& indul, int& indlu, int& induu);

template<> inline void getCornerIndicesClassic<0>(
    /*input*/  int pane, int ind1, int ind2, int stripsPerPane,
    /*output*/ int& indCell, int&, int&, int&)
{
    indCell = ind1 + (ind2 + pane * stripsPerPane) * stripsPerPane;
}
template<> inline void getCornerIndicesClassic<1>(
    /*input*/  int pane, int ind1, int ind2, int stripsPerPane,
    /*output*/ int& indll, int& indul, int& indlu, int& induu)
{
    indll = ind1 + (ind2 + pane * stripsPerPane) * (stripsPerPane+1);
    indul = indll + 1;
    if(ind2 < stripsPerPane-1) {
        indlu = indll + stripsPerPane+1;
        induu = indlu + 1;
    } else {
        int adjpane = (pane+2) % 3;   // index of the adjacent pane
        indlu = (ind1 + adjpane * stripsPerPane + 1) * (stripsPerPane+1) - 1;
        if(ind1 < stripsPerPane-1)
            induu = indlu + stripsPerPane + 1;
        else
            induu = 3 * stripsPerPane * (stripsPerPane + 1);  // the central node where three panes join
    }
}

// decode the index of the basis element with the given index of angular harmonic m
// for TargetDensityCylindrical<0>, or the indices of four basis elements for TargetDensityCylindrical<1>
template<int N> void getCornerIndicesCylindrical(
    /*input*/  int m, int indR, int indz, int gridRsize, int gridzsize,
    /*output*/ int& indll, int& indul, int& indlu, int& induu);

template<> inline void getCornerIndicesCylindrical<0>(
    /*input*/  int m, int indR, int indz, int gridRsize, int gridzsize,
    /*output*/ int& indCell, int&, int&, int&)
{
    indCell = indR + (indz + m/2 * gridzsize) * gridRsize;
}
template<> inline void getCornerIndicesCylindrical<1>(
    /*input*/  int m, int indR, int indz, int gridRsize, int gridzsize,
    /*output*/ int& indll, int& indul, int& indlu, int& induu)
{
    int offsetm = 0;
    if(m>0) {
        offsetm = (1 + m/2 * gridRsize) * (gridzsize + 1);
        // higher harmonics do not have any basis functions at the grid node R=0
        gridRsize -= 1;
        indR -= 1;
    }
    indll = offsetm + indR + indz * (gridRsize + 1);
    indul = indll + 1;
    indlu = indll + gridRsize + 1;
    induu = indlu + 1;
}

#ifdef ADAPTIVE_INTEGRATION

/** Helper class for computing the projection of a density profile onto the basis functions
    of a DensityClassic discretization scheme, integrating the product of the input density
    times all nontrivial basis functions within one radial grid segment.
*/
template<int N>
class DensityClassicIntegrand: public math::IFunctionNdim {
    const potential::BaseDensity& density; ///< input density profile
    const unsigned int stripsPerPane;
    const double axisX, axisY, axisZ;      ///< flattening of the grid in each cartesian direction
    const int indShell, indPane, ind1, ind2;
    const double rlow, rupp;               ///< boundaries of the current radial grid segment
public:
    DensityClassicIntegrand(
        const potential::BaseDensity& _density,
        const unsigned int _stripsPerPane,
        const std::vector<double> &gridr,
        double _axisX, double _axisY, double _axisZ,
        unsigned int indCell)
    :
        density(_density),
        stripsPerPane(_stripsPerPane),
        axisX(_axisX), axisY(_axisY), axisZ(_axisZ),
        indShell(indCell / (3 * pow_2(stripsPerPane))),
        indPane(indCell % (3 * pow_2(stripsPerPane)) / pow_2(stripsPerPane)),
        ind1(indCell % pow_2(stripsPerPane) / stripsPerPane),
        ind2(indCell % stripsPerPane),
        rlow(indShell>0 ? gridr[indShell-1] : 0), rupp(gridr[indShell])
    {}
    virtual unsigned int numVars() const { return 3; }
    virtual unsigned int numValues() const { return N==0 ? 1 : rlow==0 ? 5 : 8; }
    virtual void eval(const double vars[], double values[]) const { evalMany(1, vars, values); }
    virtual void evalMany(const size_t npoints, const double vars[], double values[]) const
    {
        if(npoints == 0)  // this should never happen, but silences an unjustified compiler warning
            return;
        // collect the density values at all points
        coord::PosCar* points = static_cast<coord::PosCar*>(alloca(npoints * sizeof(coord::PosCar)));
        double* jac = static_cast<double*>(alloca(npoints * sizeof(double)));
        double prefact = 0.5*M_PI*M_PI / pow_2(stripsPerPane) * (rupp-rlow);
        for(size_t i=0; i<npoints; i++) {
            double r = rlow + vars[i*3] * (rupp - rlow);
            double u = tan(M_PI/4 * (ind1 + vars[i*3+1]) / stripsPerPane);
            double v = tan(M_PI/4 * (ind2 + vars[i*3+2]) / stripsPerPane);
            double denom = 1. / sqrt(1 + pow_2(u) + pow_2(v));
            double coord[3] = {r * denom, r * denom * u, r * denom * v};
            points[i].x = coord[(3-indPane)%3] * axisX;
            points[i].y = coord[(4-indPane)%3] * axisY;
            points[i].z = coord[(5-indPane)%3] * axisZ;
            jac[i] = prefact * r * r * (1 + u*u) * (1 + v*v) * pow_3(denom);
        }
        double* densval = static_cast<double*>(alloca(npoints * sizeof(double)));
        density.evalManyDensityCar(npoints, points, densval);
        for(size_t i=0, o=0; i<npoints; i++) {
            double mult = jac[i] * densval[i];
            if(N == 0) {  // entire cell is the single basis function
                values[o++] = mult;
            } else if(N == 1) {  // four corners of the 3d cell (at rupp)
                values[o++] = mult * vars[i*3] * (1-vars[i*3+1]) * (1-vars[i*3+2]);
                values[o++] = mult * vars[i*3] *    vars[i*3+1]  * (1-vars[i*3+2]);
                values[o++] = mult * vars[i*3] * (1-vars[i*3+1]) *    vars[i*3+2];
                values[o++] = mult * vars[i*3] *    vars[i*3+1]  *    vars[i*3+2];
                if(rlow == 0) {  // fifth corner is a single node at origin
                    values[o++] = mult * (1-vars[i*3]);
                } else {  // four other corners of the 3d cell (at rlow)
                    values[o++] = mult * (1-vars[i*3]) * (1-vars[i*3+1]) * (1-vars[i*3+2]);
                    values[o++] = mult * (1-vars[i*3]) *    vars[i*3+1]  * (1-vars[i*3+2]);
                    values[o++] = mult * (1-vars[i*3]) * (1-vars[i*3+1]) *    vars[i*3+2];
                    values[o++] = mult * (1-vars[i*3]) *    vars[i*3+1]  *    vars[i*3+2];
                }
            } else
                assert(!"TargetDensityClassic: unimplemented N");
        }
    }

    /// compute the contributions of all nontrivial basis functions in the current grid segment
    /// and add them to the `result` array of coefficients returned by computeDensityProjection
    /// \param[in/out] result - add the contributions of integrals to this array
    void run(std::vector<double>& result) const
    {
        double tmpresult[8];
        double xlower[3] = {0, 0, 0}, xupper[3] = {1, 1, 1};
        math::integrateNdim(*this, xlower, xupper, EPSREL_DENSITY_INT, MAX_NUM_EVAL, tmpresult);
        int indll, indul, indlu, induu, valuesPerShell = 3 * stripsPerPane * (stripsPerPane + N) + N;
        getCornerIndicesClassic<N>(indPane, ind1, ind2, stripsPerPane,
            /*output*/indll, indul, indlu, induu);
        if(N == 0) {
            // one cell = one basis function, no overlap and no need for a critical section
            result[indll + indShell * valuesPerShell] = tmpresult[0];
        } else {
            // when N==1, each coefficient has contributions from multiple cells,
            // thus the accumulation step operating on a shared variable must be mutex-protected
#ifdef _OPENMP
#pragma omp critical
#endif
            {
                result[indll + indShell * valuesPerShell + 1] += tmpresult[0];
                result[indul + indShell * valuesPerShell + 1] += tmpresult[1];
                result[indlu + indShell * valuesPerShell + 1] += tmpresult[2];
                result[induu + indShell * valuesPerShell + 1] += tmpresult[3];
                if(indShell == 0) {
                    result[0] += tmpresult[4];
                } else {
                    result[indll + (indShell-1) * valuesPerShell + 1] += tmpresult[4];
                    result[indul + (indShell-1) * valuesPerShell + 1] += tmpresult[5];
                    result[indlu + (indShell-1) * valuesPerShell + 1] += tmpresult[6];
                    result[induu + (indShell-1) * valuesPerShell + 1] += tmpresult[7];
                }
            }
        }
    }
};

/** Helper class for computing the projection of a density profile onto the basis functions
    of a DensitySphHarm discretization scheme, integrating the product of the input density
    times all nontrivial basis functions within one radial grid segment.
*/
class DensitySphHarmIntegrand: public math::IFunctionNdim {
    const potential::BaseDensity& density; ///< input density profile
    const int lmax, mmax;                  ///< order of angular expansion in theta and phi
    const unsigned int angularCoefs;       ///< number of angular coefs at each radius
    const unsigned int ndim;               ///< dimensions of integration (2 or 3)
    const double axisX, axisY, axisZ;      ///< flattening of the grid in each cartesian direction
    const double rlow, rupp;               ///< boundaries of the current radial grid segment
    const size_t gridSize, gridIndex;      ///< size of the radial grid and index of this segment
public:
    DensitySphHarmIntegrand(
        const potential::BaseDensity& _density,
        int _lmax, int _mmax,
        double _axisX, double _axisY, double _axisZ,
        const std::vector<double> &gridr, unsigned int indexr)
    :
        density(_density),
        lmax(_lmax), mmax(_mmax),
        angularCoefs( (lmax/2+1) * (mmax/2+1) - mmax/2 * (mmax/2+1) / 2 ),
        ndim(isZRotSymmetric(density) && _axisX==_axisY && mmax==0 ? 2 : 3),
        axisX(_axisX), axisY(_axisY), axisZ(_axisZ),
        rlow(indexr>0 ? gridr[indexr-1] : 0), rupp(gridr[indexr]),
        gridSize(gridr.size()), gridIndex(indexr)
    {}
    virtual unsigned int numVars() const { return ndim; }
    virtual unsigned int numValues() const { return rlow==0 ? angularCoefs + 1 : angularCoefs * 2; }
    virtual void eval(const double vars[], double values[]) const { evalMany(1, vars, values); }
    virtual void evalMany(const size_t npoints, const double vars[], double values[]) const
    {
        // collect the density values at all points
        coord::PosCar* points = static_cast<coord::PosCar*>(alloca(npoints * sizeof(coord::PosCar)));
        for(size_t i=0; i<npoints; i++) {
            double r = rlow + vars[i*ndim] * (rupp - rlow);
            double costh = vars[i*ndim+1], sinth = sqrt(1 - pow_2(costh));
            double cosph = 1, sinph = 0;
            if(ndim == 3)
                math::sincos(M_PI*0.5 * vars[i*ndim+2], sinph, cosph);
            points[i].x = r * sinth * cosph * axisX;
            points[i].y = r * sinth * sinph * axisY;
            points[i].z = r * costh * axisZ;
        }
        double* densval = static_cast<double*>(alloca(npoints * sizeof(double)));
        density.evalManyDensityCar(npoints, points, densval);
        // temporary array for storing the values of Legendre and trigonometric functions
        double* leg  = static_cast<double*>(alloca((1 + lmax + mmax) * sizeof(double)));
        double* trig = leg + lmax+1;
        for(size_t i=0; i<npoints; i++) {
            double r = rlow + vars[i*ndim] * (rupp - rlow);
            double costh = vars[i*ndim+1], sinth = sqrt(1 - pow_2(costh)), tau = costh / (1 + sinth);
            double mult = 4*M_PI * (rupp - rlow) * r * r * densval[i];
            math::trigMultiAngle(ndim==3 ? M_PI*0.5 * vars[i*ndim+2] : 0, mmax, false, trig);
            // storage scheme for the output:
            // first `angularCoefs` elements contain the contributions of the integrals for C_{lm}
            // at rupp, in the following order:
            // (l=0,m=0), (l=2,m=0), ..., (l=lmax,m=0), (l=2,m=2), (l=4,m=2), ..., (l=lmax,m=mmax);
            // then the remaining elements contain the same quantities at rlow,
            // but if rlow=0, then only one term (l=0,m=0) is stored instead of all `angularCoefs`.
            // This is different from the final storage order of all coefficients,
            // which are reordered once all integrals are computed.
            // In addition, since the integrals for higher-order terms in the sph-harm expansion
            // can be very close to zero, it may be inefficient to evaluate them with the same
            // relative precision as the main term. Therefore, for l>0 the stored values are
            // actually C_{lm} + C_{00}, and the contribution of C_{00} is subtracted later.
            double val0 = 0;  // C_{00}
            for(int m=0, offset=i*numValues(); m<=mmax; m+=2) {
                math::sphHarmArray(lmax, m, tau, leg);
                for(int l=m; l<=lmax; l+=2, offset++) {
                    double val = mult * leg[l-m] * 2*M_SQRTPI * (m==0 ? 1. : M_SQRT2 * trig[m-1]);
                    values[offset] = (val + val0) * vars[i*ndim];  // contributions at rupp
                    if(rlow>0 || l==0)
                        values[offset + angularCoefs] = (val + val0) * (1 - vars[i*ndim]);  // rlow
                    if(l==0)
                        val0 = val;
                }
            }
        }
    }

    /// compute the contributions of all nontrivial basis functions in the current grid segment
    /// and add them to the `result` array of coefficients returned by computeDensityProjection
    /// \param[in/out] result - add the contributions of integrals to this array
    void run(std::vector<double>& result) const
    {
        double* tmpresult = static_cast<double*>(alloca(numValues() * sizeof(double)));
        double xlower[3] = {0, 0, 0}, xupper[3] = {1, 1, 1};
        math::integrateNdim(*this, xlower, xupper, EPSREL_DENSITY_INT, MAX_NUM_EVAL, tmpresult);
        // The order of coefficients computed by the integration routine differs from their
        // storage order in the result array: first `angularCoefs` values are contributions
        // of integrals to coefficients at rupp (=gridr[ir]), remaining terms are the same quantities
        // at rlow (=gridr[ir-1]); when ir=0, there is only one term (l=0,m=0), otherwise there
        // are `angularCoefs` terms.
        // A further complication is that the integrals for l>0 terms contain the contribution
        // of the l=0 term (to avoid wasting time trying to compute them with high relative accuracy
        // when the values are very close to zero), which needs to be subtracted.
        // This accumulation step operates on a shared variable and thus must be mutex-protected.
#ifdef _OPENMP
#pragma omp critical
#endif
        {
            for(unsigned int ia=0; ia<angularCoefs; ia++)
                // contributions to coefs at rupp, subtracting the C_{00} term
                result[gridIndex + 1 + ia * gridSize] += tmpresult[ia] - (ia>0) * tmpresult[0];
            // contributions to C_{00} at rlow
            result[gridIndex] += tmpresult[angularCoefs];
            // contributions to higher-order coefs at rlow, subtracting C_{00}(rlow)
            for(unsigned int ia=1; gridIndex>0 && ia<angularCoefs; ia++)
                result[gridIndex + ia * gridSize] +=
                    tmpresult[ia + angularCoefs] - tmpresult[angularCoefs];
        }
    }
};

/** Helper class for computing the projection of a density profile onto the basis functions
    of a DensityCylindrical discretization scheme, integrating the product of the input density
    times all nontrivial basis functions within one cell of the 2d meridional grid
*/
template<int N>
class DensityCylindricalIntegrand: public math::IFunctionNdim {
    const potential::BaseDensity& density; ///< input density profile
    const int mmax;                        ///< order of Fourier expansion in phi
    const unsigned int ndim;               ///< dimensions of integration (2 or 3)
    const double Rlow, Rupp, zlow, zupp;   ///< boundaries of the current grid cell
    const int gridRsize, gridzsize;
    const int indR, indz;
public:
    DensityCylindricalIntegrand(
        const potential::BaseDensity& _density,
        int _mmax,
        const std::vector<double> &gridR, const std::vector<double> &gridz,
        unsigned int indexR, unsigned int indexz)
    :
        density(_density),
        mmax(_mmax),
        ndim(isZRotSymmetric(density) && mmax==0 ? 2 : 3),
        Rlow(indexR>0 ? gridR[indexR-1] : 0), Rupp(gridR[indexR]),
        zlow(indexz>0 ? gridz[indexz-1] : 0), zupp(gridz[indexz]),
        gridRsize(gridR.size()), gridzsize(gridz.size()), indR(indexR), indz(indexz)
    {}
    virtual unsigned int numVars() const { return ndim; }
    virtual unsigned int numValues() const {
        return N==0 ?  1+mmax/2 :          // one basis function for each Fourier term in one grid cell
            Rlow==0 ? (1+mmax/2) * 2 + 2 : // two functions per term at Rupp, plus two for m=0 at Rlow
                      (1+mmax/2) * 4;      // four functions per term, bilinear form in {R,z}_{low,Rupp}
    }
    virtual void eval(const double vars[], double values[]) const { evalMany(1, vars, values); }
    virtual void evalMany(const size_t npoints, const double vars[], double values[]) const
    {
        if(npoints == 0)  // this should never happen, but silences an unjustified compiler warning
            return;
        // collect the density values at all points
        coord::PosCyl* points = static_cast<coord::PosCyl*>(alloca(npoints * sizeof(coord::PosCyl)));
        for(size_t i=0; i<npoints; i++) {
            points[i].R = Rlow + vars[i*ndim  ] * (Rupp-Rlow);
            points[i].z = zlow + vars[i*ndim+1] * (zupp-zlow);
            points[i].phi = ndim==3 ? 0.5*M_PI * vars[i*ndim+2] : 0;
        }
        double* densval = static_cast<double*>(alloca(npoints * sizeof(double)));
        density.evalManyDensityCyl(npoints, points, densval);
        // temporary array for storing the values of trigonometric functions
        double* trig = static_cast<double*>(alloca(mmax * sizeof(double)));
        for(size_t i=0, o=0; i<npoints; i++) {
            math::trigMultiAngle(points[i].phi, mmax, false, trig);
            double mult = 4*M_PI * (Rupp - Rlow) * (zupp - zlow) * points[i].R * densval[i];
            if(N==0) {
                values[o++] = mult;
                // add the value of the m=0 term to all higher-m terms to speed up convergence
                for(int m=2; m<=mmax; m+=2)
                    values[o++] = mult * (1 + 2 * trig[m-1]);
            } else if(N==1) {
                // storage scheme for the N=1 case: for each input point, the output array elements are
                // [0] - contribution of the m=0 harmonic to the basis function centered at (Rupp,zlow);
                // [1] - m=0, (Rupp,zupp);
                // [2] - m=2, (Rupp,zlow);
                // [3] - m=2, (Rupp,zupp);
                // continue for all even m values up to mmax;
                // [2 * (mmax/2+1)    ] - m=0, (Rlow,zlow);
                // [2 * (mmax/2+1) + 1] - m=0, (Rlow,zupp);
                // remaining elements continue to higher m, but only if Rlow>0, otherwise end here.
                // This is different from the final storage order of all coefficients,
                // which are reordered once all integrals are computed.
                // Moreover, the m=0 term (or four such terms at each corner of the grid when N=1)
                // is added to all terms with m>0 to avoid wasting time in computing these m>0
                // terms with high relative accuracy when their magnitude might be very small;
                // the contribution of the m=0 term is subtracted at the end of the integration.
                double offR = vars[i*ndim], offz = vars[i*ndim+1];
                for(int m=0; m<=mmax; m+=2) {
                    double val = mult * (m==0 ? 1 : 1 + 2 * trig[m-1]);
                    values[o++] = val * offR * (1-offz);
                    values[o++] = val * offR *    offz;
                }
                values[o++] = mult * (1-offR) * (1-offz);
                values[o++] = mult * (1-offR) *    offz;
                for(int m=2; Rlow>0 && m<=mmax; m+=2) {
                    double val = mult * (1 + 2 * trig[m-1]);
                    values[o++] = val * (1-offR) * (1-offz);
                    values[o++] = val * (1-offR) *    offz;
                }
            } else
                assert(!"TargetDensityCylindrical: unimplemented N");
        }
    }

    /// compute the contributions of all nontrivial basis functions in the current grid segment
    /// and add them to the `result` array of coefficients returned by computeDensityProjection
    /// \param[in/out] result - add the contributions of integrals to this array
    void run(std::vector<double>& result) const
    {
        double* tmpresult = static_cast<double*>(alloca(numValues() * sizeof(double)));
        double xlower[3] = {0, 0, 0}, xupper[3] = {1, 1, 1};
        math::integrateNdim(*this, xlower, xupper, EPSREL_DENSITY_INT, MAX_NUM_EVAL, tmpresult);
        // Accumulate the coefficients, reordering them according to the output storage convention
        // and undoing the addition of the m=0 terms to all higher-m terms.
        // This accumulation step operates on a shared variable and thus must be mutex-protected.
#ifdef _OPENMP
#pragma omp critical
#endif
        {
            for(int m=0; m<=mmax; m+=2) {
                int indll, indul, indlu, induu;
                getCornerIndicesCylindrical<N>(m, indR, indz, gridRsize, gridzsize,
                    /*output*/ indll, indul, indlu, induu);
                if(N==0) {
                    result.at(indll) += m==0 ? tmpresult[0] : tmpresult[m / 2] - tmpresult[0];
                } else {
                    result.at(indul) += m==0 ? tmpresult[0] : tmpresult[m    ] - tmpresult[0];
                    result.at(induu) += m==0 ? tmpresult[1] : tmpresult[m + 1] - tmpresult[1];
                    if(m==0) {
                        result.at(indll) += tmpresult[mmax + 2];
                        result.at(indlu) += tmpresult[mmax + 3];
                    } else if(indR>0) {
                        result.at(indll) += tmpresult[mmax + 2 + m] - tmpresult[mmax + 2];
                        result.at(indlu) += tmpresult[mmax + 3 + m] - tmpresult[mmax + 3];
                    }   // otherwise there is no such term in the basis set
                }
            }
        }
    }
};
#endif

} // internal ns


//----- Classic grid-based density representation -----//

template<int N>
TargetDensityClassic<N>::TargetDensityClassic(
    const unsigned int _stripsPerPane,
    const std::vector<double>& _gridr,
    const double axisYtoX, const double axisZtoX)
:
    stripsPerPane(_stripsPerPane),
    valuesPerShell(3 * stripsPerPane * (stripsPerPane + N) + N),
    // if the input grid starts from zero, skip this first array element, otherwise take the whole array
    gridr(_gridr.empty() || _gridr[0] > 0 ? _gridr.begin() : _gridr.begin()+1, _gridr.end()),
    axisX(1. / cbrt(axisYtoX*axisZtoX)),
    axisY(axisYtoX * axisX),
    axisZ(axisZtoX * axisX)   // the product axisX*axisY*axisZ is unity
{
    bool ok = gridr.size() >= 1 && gridr[0] > 0;
    for(unsigned int i=1; i<gridr.size(); i++)
        ok &= gridr[i] > gridr[i-1];
    if(!ok || stripsPerPane<1 || axisYtoX<=0 || axisZtoX<=0)
        throw std::invalid_argument("TargetDensityClassic: invalid grid parameters");
}

template<int N>
void TargetDensityClassic<N>::addPoint(const double point[3], const double mult, double values[]) const
{
    const int numShells = gridr.size();
    double X = fabs(point[0] / axisX), Y = fabs(point[1]) / axisY, Z = fabs(point[2]) / axisZ;
    double r = sqrt(X*X + Y*Y + Z*Z);
    int indShell = math::binSearch(r, &gridr[0], numShells) + 1;
    assert(indShell>=0);
    if(indShell >= numShells || mult == 0) {
        return;  // outside the grid
    }
    int pane;
    double coord0, coord1, coord2;
    if(X>=Y && X>Z) {
        pane=0;
        coord0=X;
        coord1=Y;
        coord2=Z;
    } else if(Y>=Z && Y>X) {
        pane=1;
        coord0=Y;
        coord1=Z;
        coord2=X;
    } else if(Z>=X && Z>=Y) {
        pane=2;
        coord0=Z;
        coord1=X;
        coord2=Y;
    } else
        throw std::runtime_error("TargetDensityClassic: cannot determine the cell index");
    // fractional index of the grid cell in both directions:
    // ratio1 between 0 and 1 - inside the first row, between 1 and 2 - inside the second row, etc.;
    // ratio2 between 0 and 1 - inside the first column, etc.
    double ratio1 = fmin(atan(coord1/coord0) * (4/M_PI), 1.) * stripsPerPane;
    double ratio2 = fmin(atan(coord2/coord0) * (4/M_PI), 1.) * stripsPerPane;
    int ind1 = std::min<int>(ratio1, stripsPerPane-1);
    int ind2 = std::min<int>(ratio2, stripsPerPane-1);

    // further details differ depending on the interpolation order of basis elements
    int indll, indul, indlu, induu;
    getCornerIndicesClassic<N>(pane, ind1, ind2, stripsPerPane, /*output*/indll, indul, indlu, induu);
    if(N == 0) {
        values[indll + indShell * valuesPerShell] += mult;
    } else if(N==1) {
        // convert ratio1,ratio2 and r into fractional coordinates within the current cell (between 0 and 1)
        ratio1 -= ind1;
        ratio2 -= ind2;
        double val = indShell == 0 ?
            mult *  r / gridr[0] :
            mult * (r - gridr[indShell-1]) / (gridr[indShell] - gridr[indShell-1]);
        // contribution to the basis functions at the upper end of the radial segment
        double* valOff = &values[indShell * valuesPerShell + 1];  // offset in the output array
        valOff[indll] += (1-ratio1) * (1-ratio2) * val;
        valOff[indul] +=    ratio1  * (1-ratio2) * val;
        valOff[indlu] += (1-ratio1) *    ratio2  * val;
        valOff[induu] +=    ratio1  *    ratio2  * val;
        // contributions to the lower end of the radial segment
        if(indShell   == 0) {          // if this is the innermost segment, then there is only
            values[0] += mult - val;   // a single basis function at origin
        } else {                       // otherwise a full set of four functions
            valOff -= valuesPerShell;  // another offset in the output array
            val = mult - val;          // remaining contribution of the input point
            valOff[indll] += (1-ratio1) * (1-ratio2) * val;
            valOff[indul] +=    ratio1  * (1-ratio2) * val;
            valOff[indlu] += (1-ratio1) *    ratio2  * val;
            valOff[induu] +=    ratio1  *    ratio2  * val;
        }
    } else
        assert(!"TargetDensityClassic: unimplemented N");
}

template<int N>
std::vector<double> TargetDensityClassic<N>::computeDensityProjection(
    const potential::BaseDensity& density) const
{
    if(!isTriaxial(density))
        throw std::runtime_error("TargetDensityClassic: input density should have triaxial symmetry");
#ifndef ADAPTIVE_INTEGRATION
    const double *glnodesRad = math::GLPOINTS[GLORDER_RAD], *glweightsRad = math::GLWEIGHTS[GLORDER_RAD];
    const double *glnodesAng = math::GLPOINTS[GLORDER_ANG], *glweightsAng = math::GLWEIGHTS[GLORDER_ANG];
    // pre-compute nodes and weigths for all strips in the integration over angles
    std::vector<double> nodesAng(GLORDER_ANG * stripsPerPane), weightsAng(GLORDER_ANG * stripsPerPane);
    for(unsigned int iA=0; iA < GLORDER_ANG * stripsPerPane; iA++) {
        int strip = iA / GLORDER_ANG, glindex = iA % GLORDER_ANG;
        nodesAng  [iA] = tan(M_PI/4 * (strip + glnodesAng[glindex]) / stripsPerPane);
        weightsAng[iA] = glweightsAng[glindex] * (1 + pow_2(nodesAng[iA])) / stripsPerPane * M_PI/4;
    }
    std::vector<double> result(numValues());

    // 1. prepare coordinates of all points where the input density values should be collected
    std::vector<coord::PosCar> pos(gridr.size() * GLORDER_RAD * pow_2(GLORDER_ANG * stripsPerPane) * 3);
    for(unsigned int iR=0, ip=0; iR < gridr.size() * GLORDER_RAD; iR++) {
        int indShell = iR / GLORDER_RAD, offShell = iR % GLORDER_RAD;
        double
        offsetR = glnodesRad[offShell],
        radius  = (indShell == 0 ? 0. : gridr[indShell-1]) * (1-offsetR) + gridr[indShell] * offsetR;
        for(unsigned int i1 = 0; i1 < GLORDER_ANG * stripsPerPane; i1++) {
            for(unsigned int i2 = 0; i2 < GLORDER_ANG * stripsPerPane; i2++) {
                double
                denom    = 1. / sqrt(1 + pow_2(nodesAng[i1]) + pow_2(nodesAng[i2])),
                coord[3] = {radius * denom, radius * denom * nodesAng[i1], radius * denom * nodesAng[i2]};
                for(int pane=0; pane<3; pane++, ip++)
                    pos.at(ip) = coord::PosCar(
                        coord[(3-pane)%3] * axisX, coord[(4-pane)%3] * axisY, coord[(5-pane)%3] * axisZ);
            }
        }
    }

    // 2. collect density values at all points at once
    std::vector<double> densValues(pos.size());
    density.evalManyDensityCar(pos.size(), &pos.front(), &densValues.front());

    // 3. convert these values into the array of cell masses
    for(unsigned int iR=0, ip=0; iR < gridr.size() * GLORDER_RAD; iR++) {
        int indShell = iR / GLORDER_RAD, offShell = iR % GLORDER_RAD;
        double
        radius1 = indShell == 0 ? 0. : gridr[indShell-1],
        radius2 = gridr[indShell],
        offsetR = glnodesRad[offShell],
        radius  = radius1 * (1-offsetR) + radius2 * offsetR,
        weightRad = 8/*octants*/ * pow_2(radius) * glweightsRad[offShell] * (radius2 - radius1);
        for(unsigned int i1 = 0; i1 < GLORDER_ANG * stripsPerPane; i1++) {
            for(unsigned int i2 = 0; i2 < GLORDER_ANG * stripsPerPane; i2++) {
                double
                offset1  = glnodesAng[i1 % GLORDER_ANG],  // fractional coords within the current cell
                offset2  = glnodesAng[i2 % GLORDER_ANG],
                denom    = 1. / sqrt(1 + pow_2(nodesAng[i1]) + pow_2(nodesAng[i2])),
                weight   = weightRad * weightsAng[i1] * weightsAng[i2] * pow_3(denom);
                int ind1 = i1 / GLORDER_ANG, ind2 = i2 / GLORDER_ANG;
                for(int pane=0; pane<3; pane++, ip++) {
                    double value = weight * densValues[ip];
                    int indll, indul, indlu, induu;
                    getCornerIndicesClassic<N>(pane, ind1, ind2, stripsPerPane,
                        /*output*/indll, indul, indlu, induu);
                    // further details depend on the shape of basis elements
                    if(N == 0) {
                        result[indll + indShell * valuesPerShell] += value;
                    } else if(N==1) {
                        double* resOff = &result[indShell * valuesPerShell + 1];
                        resOff[indll] += value * offsetR * (1-offset1) * (1-offset2);
                        resOff[indul] += value * offsetR *    offset1  * (1-offset2);
                        resOff[indlu] += value * offsetR * (1-offset1) *    offset2;
                        resOff[induu] += value * offsetR *    offset1  *    offset2;
                        if(indShell   == 0) {  // a single node at origin
                            result[0] += value * (1-offsetR);
                        } else {
                            resOff -= valuesPerShell;
                            resOff[indll] += value * (1-offsetR) * (1-offset1) * (1-offset2);
                            resOff[indul] += value * (1-offsetR) *    offset1  * (1-offset2);
                            resOff[indlu] += value * (1-offsetR) * (1-offset1) *    offset2;
                            resOff[induu] += value * (1-offsetR) *    offset1  *    offset2;
                        }
                    } else
                        assert(!"TargetDensityClassic: unimplemented N");
                }
            }
        }
    }
#else
    std::vector<double> result(numValues());
    int numCells = 3 * pow_2(stripsPerPane) * gridr.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for(int indCell=0; indCell<numCells; indCell++) {
        DensityClassicIntegrand<N> fnc(density, stripsPerPane, gridr, axisX, axisY, axisZ, indCell);
        fnc.run(result);
    }
#endif
    return result;
}

template<int N>
std::string TargetDensityClassic<N>::coefName(unsigned int index) const
{
    if(index >= numValues())
        throw std::out_of_range("TargetDensityClassic: index out of range");
    double coord[3] = {1.}, radius;
    unsigned int pane;
    if(N==0) {
        unsigned int
        indShell = index / valuesPerShell,
        ind1     = index % stripsPerPane,
        ind2     = index / stripsPerPane % stripsPerPane;
        pane     = index % valuesPerShell / (stripsPerPane * stripsPerPane);
        coord[1] = tan(M_PI/4 * (ind1 + 0.5) / stripsPerPane);
        coord[2] = tan(M_PI/4 * (ind2 + 0.5) / stripsPerPane);
        radius   = 0.5 * (gridr.at(indShell) + (indShell>0 ? gridr[indShell-1] : 0.));
    } else if(N==1) {
        if(index==0) {  // the node at origin
            coord[1] = coord[2] = radius = 0.;
            pane = 0;
        } else {
            index--;
            radius = gridr.at(index / valuesPerShell);
            index %= valuesPerShell;
            if(index == valuesPerShell-1) {  // the central node
                pane  = 0;
                coord[1] = coord[2] = 1.;
            } else {  // ordinary nodes
                pane   = index / (stripsPerPane * (stripsPerPane+1));
                index %= stripsPerPane * (stripsPerPane+1);
                coord[1] = tan(M_PI/4 * (index % (stripsPerPane+1)) / stripsPerPane);
                coord[2] = tan(M_PI/4 * (index / (stripsPerPane+1)) / stripsPerPane);
            }
        }
    } else
        assert(!"TargetDensityClassic: unimplemented N");
    double denom = 1. / sqrt(pow_2(coord[0]) + pow_2(coord[1]) + pow_2(coord[2]));
    // pane=0:  coord = {x, y, z};  pane=1:  coord = {y, z, x};  pane=2:  coord = {z, x, y}
    return "x=" + utils::toString(radius * denom * coord[(3-pane)%3] * axisX) +
         ", y=" + utils::toString(radius * denom * coord[(4-pane)%3] * axisY) +
         ", z=" + utils::toString(radius * denom * coord[(5-pane)%3] * axisZ);
}

template<> const char* TargetDensityClassic<0>::name() const { return "DensityClassicTopHat"; }
template<> const char* TargetDensityClassic<1>::name() const { return "DensityClassicLinear"; }

template class TargetDensityClassic<0>;
template class TargetDensityClassic<1>;


//----- Spherical-harmonic density representation -----//

TargetDensitySphHarm::TargetDensitySphHarm(
    const int _lmax, const int _mmax,
    const std::vector<double>& _gridr,
    const double axisYtoX, const double axisZtoX)
:
    lmax(_lmax), mmax(_mmax),
    angularCoefs( (lmax/2+1) * (mmax/2+1) - mmax/2 * (mmax/2+1) / 2 ),
    // if the input grid starts from zero, skip this first array element, otherwise take the whole array
    gridr(_gridr.empty() || _gridr[0] > 0 ? _gridr.begin() : _gridr.begin()+1, _gridr.end()),
    axisX(1. / cbrt(axisYtoX*axisZtoX)),
    axisY(axisYtoX * axisX),
    axisZ(axisZtoX * axisX)   // the product axisX*axisY*axisZ is unity
{
    bool ok = gridr.size() >= 1 && gridr[0] > 0;
    for(unsigned int i=1; i<gridr.size(); i++)
        ok &= gridr[i] > gridr[i-1];
    if(!ok || axisYtoX<=0 || axisZtoX<=0 ||
        lmax < 0 || lmax%2 != 0 || mmax < 0 || mmax%2 != 0 || mmax > lmax)
        throw std::invalid_argument("TargetDensitySphHarm: invalid grid parameters");
}

void TargetDensitySphHarm::addPoint(const double point[3], double mult, double values[]) const
{
    const coord::PosCyl pcyl = toPosCyl(
        coord::PosCar(point[0] / axisX, point[1] / axisY, point[2] / axisZ));
    double r   = sqrt(pow_2(pcyl.R) + pow_2(pcyl.z));
    double tau = pcyl.z / (r + pcyl.R);
    if(r==0) {
        values[0] += mult;
        return;
    }
    const int gridrsize = gridr.size();
    int indr = math::binSearch(r, &gridr[0], gridrsize) + 1;
    assert(indr>=0);
    if(indr >= gridrsize || mult == 0)
        return;  // outside the grid
    // convert r into a fractional offset within the shell [0..1]
    double offr = indr==0 ? r / gridr[0] : (r - gridr[indr-1]) / (gridr[indr] - gridr[indr-1]);

    // temporary array for storing the values of Legendre and trigonometric functions
    unsigned int size = 1 + lmax + mmax;
    double* leg = static_cast<double*>(alloca(size * sizeof(double))), *trig = leg + lmax+1;
    math::trigMultiAngle(pcyl.phi, mmax, false, trig);

    // storage scheme for the output: the radial variation of each harmonic coefficient
    // is represented by a continuous array of `gridrsize` numbers
    // (except l=0, which has one extra term at r=0); of these numbers, only two may be nonzero,
    // corresponding to the radial basis functions associated to the grid nodes 
    // gridr[indr] and gridr[indr+1], which enclose the radius of the input point.
    // The arrays for each harmonic term are stored one after another in the following order: 
    // first all terms with l=0,2,4,...,lmax and m=0,
    // then l=2,4,...,lmax and m=2, then l=4,...,lmax and m=4, up to mmax.
    // offset is the index of the coefficient with the given (l,m) in the output array.
    for(int m=0, offset=indr+1; m<=mmax; m+=2) {
        math::sphHarmArray(lmax, m, tau, leg);
        for(int l=m; l<=lmax; l+=2, offset+=gridrsize) {
            double val = mult * leg[l-m] * 2*M_SQRTPI * (m==0 ? 1. : M_SQRT2 * trig[m-1]);
            values[offset] += val * offr;
            if(indr>0 || l==0)
                values[offset-1] += val * (1-offr);
        }
    }
}

std::vector<double> TargetDensitySphHarm::computeDensityProjection(
    const potential::BaseDensity& density) const
{
    if(!isTriaxial(density))
        throw std::runtime_error("TargetDensitySphHarm: input density should have triaxial symmetry");
#ifndef ADAPTIVE_INTEGRATION
    // the integration in radius follows the Gauss-Legendre rule
    const double *glnodesRad = math::GLPOINTS[GLORDER_RAD], *glweightsRad = math::GLWEIGHTS[GLORDER_RAD];

    // to improve accuracy of SH coefficient computation, we may increase the order of expansion
    // that determines the number of integration points in angles
    int lmax_tmp = std::max<int>(lmax+LADD_SPHHARM, LMIN_SPHHARM);
    int mmax_tmp = std::max<int>(mmax+LADD_SPHHARM, LMIN_SPHHARM);
    if(isSpherical(density) && axisX==1 && axisY==1) lmax_tmp = 0;
    if(isZRotSymmetric(density) && axisX==axisY)     mmax_tmp = 0;
    math::SphHarmIndices ind(lmax_tmp, mmax_tmp, coord::ST_TRIAXIAL);
    math::SphHarmTransformForward trans(ind);
    const unsigned int numSamplesAngles = trans.size();  // size of array of density values at each r
    const int gridrsize = gridr.size();

    // 1. prepare coordinates of all points where the input density values should be collected
    std::vector<coord::PosCar> pos(gridrsize * GLORDER_RAD * numSamplesAngles);
    for(unsigned int ir=0, ip=0; ir < gridrsize * GLORDER_RAD; ir++) {
        // 0. assign the radius of this point and its weigth in the total integral
        int indr = ir / GLORDER_RAD, subr = ir % GLORDER_RAD;
        double
        offr    = glnodesRad[subr],  // fractional offset [0..1] within the current grid segment
        radius  = (indr == 0 ? 0. : gridr[indr-1]) * (1-offr) + gridr[indr] * offr;
        for(unsigned int ia=0; ia<numSamplesAngles; ia++, ip++)  {
            double z   = radius * trans.costheta(ia);
            double R   = sqrt(pow_2(radius) - z*z);
            double phi = trans.phi(ia);
            coord::PosCar tp = toPosCar(coord::PosCyl(R, z, phi));
            pos.at(ip) = coord::PosCar(tp.x * axisX, tp.y * axisY, tp.z * axisZ);
        }
    }

    // 2. collect density values at all points at once
    std::vector<double> densValues(pos.size());
    density.evalManyDensityCar(pos.size(), &pos.front(), &densValues.front());

    // 3. convert these values into the array of expansion coefficients
    std::vector<double> shcoefs(std::max<int>(ind.size(), pow_2(lmax+1)));
    std::vector<double> result(numValues());
    for(unsigned int ir=0; ir < gridrsize * GLORDER_RAD; ir++) {
        // transform the array of values at each radius to spherical-harmonic expansion coefficients
        trans.transform(&densValues.at(ir * numSamplesAngles), /*output*/ &shcoefs[0]);
        math::eliminateNearZeros(shcoefs, EPS_COEF);

        int indr = ir / GLORDER_RAD, subr = ir % GLORDER_RAD;
        double
        radius1 = indr == 0 ? 0. : gridr[indr-1],
        radius2 = gridr[indr],
        offr    = glnodesRad[subr],
        radius  = radius1 * (1-offr) + radius2 * offr,
        weight  = 4*M_PI * pow_2(radius) * glweightsRad[subr] * (radius2 - radius1);

        // store the contribution of each SH coef to the relevant radial basis functions
        // (they are simply triangular-shaped blocks, so at most two of them are used at each radius)
        for(int m=0, offset=indr+1; m<=mmax; m+=2) {
            for(int l=m; l<=lmax; l+=2, offset+=gridrsize) {
                double val = shcoefs[ind.index(l, m)] * weight;
                result.at(offset) += val * offr;
                if(indr>0 || l==0)  // for the grid node at origin, only one angular term (l=0) is used
                    result.at(offset-1) += val * (1-offr);
            }
        }
    }
#else
    std::vector<double> result(numValues());
    int lmax_tmp = isSpherical(density) && axisX==1 && axisY==1 ? 0 : lmax;
    int mmax_tmp = isZRotSymmetric(density) && axisX==axisY ? 0 : mmax;
    // loop over all segments of the radial grid, and in each segment,
    // accumulate the contributions of integrals for all nontrivial coefficients in the `result` array
    int size = gridr.size();
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for(int ir=0; ir<size; ir++) {
        DensitySphHarmIntegrand fnc(density, lmax_tmp, mmax_tmp, axisX, axisY, axisZ, gridr, ir);
        fnc.run(result);
    }
#endif
    return result;
}

std::string TargetDensitySphHarm::coefName(unsigned int index) const
{
    if(index >= numValues())
        throw std::out_of_range("TargetDensitySphHarm: index out of range");
    if(index == 0)
        return "r=0, l=0, m=0";
    index--;
    unsigned int gridrsize = gridr.size(), m = 0;
    while(index >= gridrsize * (1 + lmax/2 - m/2)) {
        index -= gridrsize * (1 + lmax/2 - m/2);
        m += 2;
    }
    unsigned int l = index / gridrsize * 2 + m, indr = index % gridrsize;
    return "r=" + utils::toString(gridr[indr]) + ", l=" + utils::toString(l) + ", m=" + utils::toString(m);
}

const char* TargetDensitySphHarm::name() const { return "DensitySphHarm"; }


//----- Cylindrical grid + azimuthal Fourier density representation -----//

template<int N>
TargetDensityCylindrical<N>::TargetDensityCylindrical(const int _mmax,
    const std::vector<double>& _gridR, const std::vector<double>& _gridz)
:
    mmax(_mmax),
    // if the input grids start from zero, skip this first array element, otherwise take the whole array
    gridR(_gridR.empty() || _gridR[0] > 0 ? _gridR.begin() : _gridR.begin()+1, _gridR.end()),
    gridz(_gridz.empty() || _gridz[0] > 0 ? _gridz.begin() : _gridz.begin()+1, _gridz.end()),
    totalNumValues( (gridz.size() + N) * (gridR.size() * (mmax/2+1) + N) )
{
    bool ok = mmax >= 0 && gridR.size() >= 1 && gridz.size() >= 1 && gridR[0] > 0 && gridz[0] > 0;
    for(unsigned int i=1; i<gridR.size(); i++)
        ok &= gridR[i] > gridR[i-1];
    for(unsigned int i=1; i<gridz.size(); i++)
        ok &= gridz[i] > gridz[i-1];
    if(!ok)
        throw std::invalid_argument("TargetDensityCylindrical: invalid grid parameters");
}

template<int N>
void TargetDensityCylindrical<N>::addPoint(const double point[3], double mult, double values[]) const
{
    const coord::PosCyl pcyl = toPosCyl(coord::PosCar(point[0], point[1], fabs(point[2])));
    const int gridRsize = gridR.size(), gridzsize = gridz.size();
    int indR = math::binSearch(pcyl.R, &gridR[0], gridRsize) + 1;
    int indz = math::binSearch(pcyl.z, &gridz[0], gridzsize) + 1;
    assert(indR>=0 && indz>=0);
    if(indR >= gridRsize || indz >= gridzsize || mult == 0)
        return;  // outside the grid
    // convert R,z into fractional coordinates within the 2d cell [0..1]
    double prevR = indR>0 ? gridR[indR-1] : 0.;
    double prevz = indz>0 ? gridz[indz-1] : 0.;
    double offR  = (pcyl.R - prevR) / ( gridR[indR] - prevR );
    double offz  = (pcyl.z - prevz) / ( gridz[indz] - prevz );

    // temporary array for storing the values of trigonometric functions (cosines only)
    double* trig = static_cast<double*>(alloca(mmax * sizeof(double)));
    math::trigMultiAngle(pcyl.phi, mmax, false, trig);

    for(int m=0; m<=mmax; m+=2) {
        double val = mult * (m==0 ? 1. : 2*trig[m-1]);
        int indll, indul, indlu, induu;
        getCornerIndicesCylindrical<N>(m, indR, indz, gridRsize, gridzsize,
            /*output*/ indll, indul, indlu, induu);
        if(N==0) {  // only one term
            assert(indll < (int)totalNumValues);
            values[indll] += val;
        } else if(N==1) {  // up to four terms
            assert(induu < (int)totalNumValues);
            values[indul] += val * offR * (1-offz);
            values[induu] += val * offR *    offz;
            if(m==0 || indR>0) {
                values[indll] += val * (1-offR) * (1-offz);
                values[indlu] += val * (1-offR) *    offz;
            }   // otherwise there is no such term in the basis set
        } else
            assert(!"TargetDensityCylindrical: unimplemented N");
    }
}

template<int N>
std::vector<double> TargetDensityCylindrical<N>::computeDensityProjection(
    const potential::BaseDensity& density) const
{
    if(!isTriaxial(density))
        throw std::runtime_error("TargetDensityCylindrical: input density should have triaxial symmetry");
#ifndef ADAPTIVE_INTEGRATION
    // the integration in R and z follows the Gauss-Legendre rule
    const double *glnodesRad = math::GLPOINTS[GLORDER_RAD], *glweightsRad = math::GLWEIGHTS[GLORDER_RAD];
    // select a sufficiently high order of integration in angles
    int mmax_tmp = isZRotSymmetric(density) ? 0 : std::max(mmax+LADD_SPHHARM, LMIN_SPHHARM);
    math::FourierTransformForward trans(mmax_tmp, false/*no odd terms*/);
    const unsigned int numSamplesAngles = trans.size();  // size of array of density values at each (R,z)
    const int gridRsize = gridR.size(), gridzsize = gridz.size();

    // 1. prepare coordinates of all points where the input density values should be collected
    std::vector<coord::PosCyl> pos(gridRsize * gridzsize * pow_2(GLORDER_RAD) * numSamplesAngles);
    for(unsigned int iz=0, ip=0; iz < gridzsize * GLORDER_RAD; iz++) {
        int indz = iz / GLORDER_RAD, subz = iz % GLORDER_RAD;
        double
        offz = glnodesRad[subz],  // fractional offset [0..1] in z within the current grid segment
        z    = (indz == 0 ? 0. : gridz[indz-1]) * (1-offz) + gridz[indz] * offz;
        for(unsigned int iR=0; iR < gridRsize * GLORDER_RAD; iR++) {
            int indR = iR / GLORDER_RAD, subR = iR % GLORDER_RAD;
            double
            offR  = glnodesRad[subR],  // fractional offset in R
            R     = (indR == 0 ? 0. : gridR[indR-1]) * (1-offR) + gridR[indR] * offR;
            for(unsigned int iphi=0; iphi<numSamplesAngles; iphi++, ip++)
                pos.at(ip) = coord::PosCyl(R, z, trans.phi(iphi));
        }
    }

    // 2. collect density values at all points at once
    std::vector<double> densValues(pos.size());
    density.evalManyDensityCyl(pos.size(), &pos.front(), &densValues.front());

    // 3. convert these values into the array of Fourier coefficients
    std::vector<double> coefs(std::max<int>(trans.size(), mmax+1));  // temp.storage for transformed coefs
    std::vector<double> result(numValues());  // output array
    for(unsigned int iz=0; iz < gridzsize * GLORDER_RAD; iz++) {
        int indz = iz / GLORDER_RAD, subz = iz % GLORDER_RAD;
        double
        offz    = glnodesRad[subz],  // fractional offset in z
        weightz = glweightsRad[subz] * (gridz[indz] - (indz == 0 ? 0. : gridz[indz-1]));

        for(unsigned int iR=0; iR < gridRsize * GLORDER_RAD; iR++) {
            int indR = iR / GLORDER_RAD, subR = iR % GLORDER_RAD;
            double
            R1     = indR == 0 ? 0. : gridR[indR-1],
            R2     = gridR[indR],
            offR   = glnodesRad[subR],
            R      = R1 * (1-offR) + R2 * offR,
            weight = weightz * 2 * R * glweightsRad[subR] * (R2 - R1);

            // transform the array of density values at each annulus in R,z into Fourier coefficients
            trans.transform(&densValues.at((iz * gridRsize * GLORDER_RAD + iR) * numSamplesAngles),
                /*output*/ &coefs[0]);
            math::eliminateNearZeros(coefs, EPS_COEF);

            // add the contribution to each basis function in the azimuthal plane
            for(int m=0; m<=mmax; m+=2) {
                double val = coefs[m] * weight * (1 + (m>0));
                int indll, indul, indlu, induu;
                getCornerIndicesCylindrical<N>(m, indR, indz, gridRsize, gridzsize,
                    /*output*/ indll, indul, indlu, induu);
                if(N==0) {  // only one term
                    result.at(indll) += val;
                } else if(N==1) {  // up to four terms
                    result.at(indul) += val * offR * (1-offz);
                    result.at(induu) += val * offR *    offz;
                    if(m==0 || indR>0) {
                        result.at(indll) += val * (1-offR) * (1-offz);
                        result.at(indlu) += val * (1-offR) *    offz;
                    }   // otherwise there is no such term in the basis set
                } else
                    assert(!"TargetDensityCylindrical: unimplemented N");
            }
        }
    }
#else
    std::vector<double> result(numValues());
    const int mmax_tmp = isZRotSymmetric(density) ? 0 : mmax;
    // loop over all cells of the 2d grid in the meridional plane, and in each cell,
    // accumulate the contributions of integrals for all nontrivial coefficients in the `result` array
    const int sizeR = gridR.size(), size = gridz.size() * sizeR;
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
    for(int i=0; i<size; i++) {
        DensityCylindricalIntegrand<N> fnc(density, mmax_tmp, gridR, gridz, i % sizeR, i / sizeR);
        fnc.run(result);
    }
#endif
    return result;
}

template<> std::string TargetDensityCylindrical<0>::coefName(unsigned int index) const
{
    if(index >= numValues())
        throw std::out_of_range("TargetDensityCylindrical: index out of range");
    unsigned int gridRsize = gridR.size(), gridzsize = gridz.size();
    unsigned int
    indR  = index % gridRsize,
    indmz = index / gridRsize,
    indz  = indmz % gridzsize,
    m     = indmz / gridzsize * 2;
    double R = 0.5 * (gridR[indR] + (indR>0 ? gridR[indR-1] : 0.));
    double z = 0.5 * (gridz[indz] + (indz>0 ? gridz[indz-1] : 0.));
    return "R=" + utils::toString(R) + ", z=" + utils::toString(z) + ", m=" + utils::toString(m);
}

template<> std::string TargetDensityCylindrical<1>::coefName(unsigned int index) const
{
    if(index >= numValues())
        throw std::out_of_range("TargetDensityCylindrical: index out of range");
    unsigned int gridRsize = gridR.size(), gridzsize = gridz.size();
    // # of meridional-plane elements for m=0 is larger than for higher m
    unsigned int
    m0size = (gridRsize+1) * (gridzsize+1),
    msize  =  gridRsize    * (gridzsize+1),
    m      = index < m0size ? 0 : ((index-m0size) / msize + 1) * 2,
    indz   = index < m0size ? index / (gridRsize+1) : (index-m0size) / gridRsize % (gridzsize+1),
    indR   = index < m0size ? index % (gridRsize+1) : (index-m0size) % gridRsize + 1;
    return "R=" + utils::toString(indR==0 ? 0. : gridR.at(indR-1)) +
         ", z=" + utils::toString(indz==0 ? 0. : gridz.at(indz-1)) +
         ", m=" + utils::toString(m);
}

template<> const char* TargetDensityCylindrical<0>::name() const { return "DensityCylindricalTopHat"; }
template<> const char* TargetDensityCylindrical<1>::name() const { return "DensityCylindricalLinear"; }

template class TargetDensityCylindrical<0>;
template class TargetDensityCylindrical<1>;

}  // namespace
