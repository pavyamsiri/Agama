import _agama
from _agama import *
from _agama import __doc__, __version__

from . import schwarzlib
from .nemofile import NemoFile
from .pygama import *

try:
    import agamacolormaps  # initialize submodule and register some custom colormaps for matplotlib

    del agamacolormaps  # remove submodule from the namespace
except ImportError:
    pass  # no error in case this fails
del _agama  # remove the C++ library from the root namespace
del pygama  # and the same for the Python extension submodule
del schwarzlib
del nemofile

__all__ = [
    "ActionFinder",
    "ActionMapper",
    "Component",
    "CubicSpline",
    "Density",
    "DistributionFunction",
    "G",
    "GalaPotential",
    "GalaxyModel",
    "GalpyPotential",
    "NemoFile",
    "Potential",
    "SelectionFunction",
    "SelfConsistentModel",
    "Spline",
    "Target",
    "actions",
    "bsplineIntegrals",
    "bsplineInterp",
    "bsplineMatrix",
    "fromGalactictoICRS",
    "fromICRStoGalactic",
    "getCartesianCoords",
    "getCelestialCoords",
    "getGalacticFromGalactocentric",
    "getGalactocentricFromGalactic",
    "getIntrinsicShape",
    "getProjectedEllipse",
    "getUnits",
    "getViewingAngles",
    "ghInterp",
    "ghMoments",
    "integrateNdim",
    "makeCelestialRotationMatrix",
    "makeRotationMatrix",
    "nonuniformGrid",
    "orbit",
    "readSnapshot",
    "sampleNdim",
    "sampleOrbitLibrary",
    "setNumThreads",
    "setRandomSeed",
    "setUnits",
    "solveOpt",
    "splineApprox",
    "splineLogDensity",
    "symmetricGrid",
    "transformCelestialCoords",
    "writeSnapshot",
]
