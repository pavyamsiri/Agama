from collections.abc import Callable, Sequence
from types import TracebackType
from typing import Any, Final, Self, overload
from typing import Literal as L

import numpy as np
from agama._actions import ActionFinder, actions
from agama._potential_typing import Density, Potential, Spline, _Potential
from optype import numpy as onp

type _StorageType = np.float32

__all__ = [
    "ActionFinder",
    "ActionMapper",
    "Component",
    "Density",
    "DistributionFunction",
    "G",
    "GalaxyModel",
    "Potential",
    "SelectionFunction",
    "SelfConsistentModel",
    "Spline",
    "Target",
    "__version__",
    "actions",
    "getUnits",
    "ghMoments",
    "integrateNdim",
    "orbit",
    "readSnapshot",
    "sampleNdim",
    "setNumThreads",
    "setRandomSeed",
    "setUnits",
    "solveOpt",
    "splineApprox",
    "splineLogDensity",
    "writeSnapshot",
]

__version__: Final[str]
G: Final[float]

# -- IO --
type _WriteFormat = L["t", "n", "g"]

def readSnapshot(filename: str, /) -> tuple[onp.Array2D[np.float64], onp.Array1D[np.float64]]: ...
def writeSnapshot(
    filename: str,
    particles: tuple[onp.ToJustFloat64_2D, onp.ToJustFloat64_1D],
    format: _WriteFormat | str = "t",
) -> None: ...

# -- Misc --
# - `setRandomSeed` -
def setRandomSeed(seed: int) -> None: ...

# - `setUnits` -
# Reset units
@overload
def setUnits() -> None: ...

# Specify mass, length and time
@overload
def setUnits(*, mass: float, length: float, time: float) -> None: ...

# Specify mass, length and velocity
@overload
def setUnits(*, mass: float, length: float, velocity: float) -> None: ...

# Specify mass, time and velocity
@overload
def setUnits(*, mass: float, time: float, velocity: float) -> None: ...

# - `getUnits` -
type _UnitDimension = L["mass", "length", "time", "velocity"]

def getUnits() -> dict[_UnitDimension, float]: ...

# - `sampleNdim` -
type _SampleNdimCallable = Callable[
    [onp.Array2D[np.float64]], onp.Array2D[np.float32] | onp.Array2D[np.float64] | onp.Array2D[np.bool_]
]

# `lower` is the number of dimensions
@overload
def sampleNdim(fnc: _SampleNdimCallable, nsamples: int, lower: int) -> tuple[onp.Array2D[np.float64], float, float, int]: ...

# Both `lower` and `upper` are provided as arrays
@overload
def sampleNdim(
    fnc: _SampleNdimCallable, nsamples: int, lower: onp.ToJustFloat64_1D, upper: onp.ToJustFloat64_1D
) -> tuple[onp.Array2D[np.float64], float, float, int]: ...

# - `integrateNdim` -
type _IntegrateNdimCallable = Callable[
    [onp.Array2D[np.float64]], onp.Array2D[np.float32] | onp.Array2D[np.float64] | onp.Array2D[np.bool_]
]

# `lower` is the number of dimensions
@overload
def integrateNdim(
    fnc: _IntegrateNdimCallable, nsamples: int, lower: int, toler: float, maxeval: int
) -> tuple[float, float, int]: ...

# Both `lower` and `upper` are provided as arrays
@overload
def integrateNdim(
    fnc: _IntegrateNdimCallable,
    nsamples: int,
    lower: onp.ToJustFloat64_1D,
    upper: onp.ToJustFloat64_1D,
    toler: float,
    maxeval: float,
) -> tuple[float, float, int]: ...

# - `setNumThreads `-
class setNumThreads:
    currNumThreads: int
    prevNumThreads: int

    def __init__(self, num_threads: int) -> None: ...
    def __enter__(self) -> Self: ...
    def __exit__(
        self,
        exc_type: type[BaseException] | None,
        exc_val: BaseException | None,
        exc_tb: TracebackType | None,
    ) -> bool | None: ...

# - `solveOpt`-
# single argument for matrix
@overload
def solveOpt(
    matrix: onp.Array2D[np.floating[Any]],
    rhs: onp.ToJustFloat64_1D,
    rpenl: onp.ToJustFloat64_1D | None = None,
    rpenq: onp.ToJustFloat64_1D | None = None,
    xpenl: onp.ToJustFloat64_1D | None = None,
    xpenq: onp.ToJustFloat64_1D | None = None,
    xmin: onp.ToJustFloat64_1D | None = None,
    xmax: onp.ToJustFloat64_1D | None = None,
) -> onp.Array1D[np.float64]: ...

# list or tuple of matrices
@overload
def solveOpt(
    matrix: Sequence[onp.Array2D[np.floating[Any]]],
    rhs: Sequence[onp.ToJustFloat64_1D],
    rpenl: Sequence[onp.ToJustFloat64_1D] | None = None,
    rpenq: Sequence[onp.ToJustFloat64_1D] | None = None,
    xpenl: onp.ToJustFloat64_1D | None = None,
    xpenq: onp.ToJustFloat64_1D | None = None,
    xmin: onp.ToJustFloat64_1D | None = None,
    xmax: onp.ToJustFloat64_1D | None = None,
) -> onp.Array1D[np.float64]: ...

# - `splineApprox`-
def splineApprox(
    knots: onp.ToJustFloat64_1D,
    x: onp.ToJustFloat64_1D,
    y: onp.ToJustFloat64_1D,
    w: onp.ToJustFloat64_1D | None = None,
    smooth: float | None = None,
) -> Spline: ...

# - `splineLogDensity`-
def splineLogDensity(
    knots: onp.ToJustFloat64_1D,
    x: onp.ToJustFloat64_1D,
    w: onp.ToJustFloat64_1D | None = None,
    infLeft: float | None = None,
    infRight: float | None = None,
    der3: int | None = None,
    smooth: float | None = None,
) -> Spline: ...

# - `ghMoments`-
# matrix is 1D
@overload
def ghMoments(
    degree: int,
    gridv: onp.ToJustFloat64_1D,
    matrix: onp.Array1D[np.float64],
    ghorder: int,
    ghbasis: onp.ToJustFloat64_2D | None = None,
) -> onp.Array1D[_StorageType]: ...

# matrix is 2D
@overload
def ghMoments(
    degree: int,
    gridv: onp.ToJustFloat64_1D,
    matrix: onp.Array2D[np.float64],
    ghorder: int,
    ghbasis: onp.ToJustFloat64_2D | None = None,
) -> onp.Array2D[_StorageType]: ...

# catch all for convertible to array types
@overload
def ghMoments(
    degree: int,
    gridv: onp.ToJustFloat64_1D,
    matrix: onp.ToJustFloat64_1D | onp.ToJustFloat64_2D,
    ghorder: int,
    ghbasis: onp.ToJustFloat64_2D | None = None,
) -> onp.Array1D[_StorageType] | onp.Array2D[_StorageType]: ...

# - `orbit` -
# TODO: This has a bunch of overloads
def orbit(
    *,
    ic: onp.ToJustFloat64_1D | onp.ToJustFloat64_2D,
    time: onp.ToJustFloat64 | onp.ToJustFloat64_1D,
    timestart: onp.ToJustFloat64 | onp.ToJustFloat64_1D | None = None,
    potential: _Potential,
    targets: Sequence[Target] | None = None,
    trajsize: onp.ToJustInt | onp.ToJustInt1D | None = None,
    der: bool = False,
    lyapunov: bool = False,
    Omega: float = 0.0,
    accuracy: float = 1e-8,
    maxNumSteps: int = 100000000,
    dtype: np.dtype[np.float32 | np.float64 | np.complex64 | np.complex128 | np.object_] | None = None,
    method: str | None = None,
    verbose: bool = True,
) -> onp.Array2D[np.float64] | tuple[onp.Array2D[np.float64], ...]: ...

class Target: ...
