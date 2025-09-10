from collections.abc import Callable, Sequence
from types import TracebackType
from typing import Any, Final, Protocol, Self, overload
from typing import Literal as L

import numpy as np
from optype import numpy as onp

from ._potential_typing import _DensityType

type _Array1D[_SCT: np.generic] = np.ndarray[tuple[int], np.dtype[_SCT]]
type _Array2D[_SCT: np.generic] = np.ndarray[tuple[int, int], np.dtype[_SCT]]
type _Potential = Potential | Callable[[_Array2D[np.floating[Any]]], _Array1D[np.bool_ | np.float32 | np.float64]]
type _Density = Density | dict[str, object] | Callable[[_Array2D[np.floating[Any]]], _Array1D[np.bool_ | np.float32 | np.float64]]

class _CanArray1D[_SCT: np.generic](Protocol):
    def __len__(self, /) -> int: ...
    def __array__(self, /) -> np.ndarray[tuple[int], np.dtype[_SCT]]: ...

class _CanArray2D[_SCT: np.generic](Protocol):
    def __len__(self, /) -> int: ...
    def __array__(self, /) -> np.ndarray[tuple[int, int], np.dtype[_SCT]]: ...

type _To1D1[_SCT: np.generic] = _CanArray1D[_SCT] | Sequence[_SCT]
type _To1D2[_SCT: np.generic] = _CanArray2D[_SCT] | Sequence[_SCT]
type _ToArray1D[_SCT: np.generic] = _CanArray1D[_SCT] | Sequence[_To1D1[_SCT]]
type _ToArray2D[_SCT: np.generic] = _CanArray2D[_SCT] | Sequence[_To1D2[_SCT]]

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

class ActionFinder:
    def __init__(self, potential: _Potential, interp: bool = False) -> None: ...
    def __call__(
        self,
        point: object,
        actions: bool = True,
        angles: bool = False,
        frequencies: bool = False,
    ) -> None: ...

class Density:
    @overload
    def __init__(self, cumulmass: _ToArray2D[np.float64]) -> None: ...
    @overload
    def __init__(self, filename: str) -> None: ...
    @overload
    def __init__(self, *args: _Density) -> None: ...
    @overload
    def __init__(
        self,
        *,
        type: _DensityType | None = None,
        density: _DensityType | None = None,
        mass: float | None = None,
        scaleradius: float | None = None,
        scaleheight: float | None = None,
        p: float | None = None,
        q: float | None = None,
        gamma: float | None = None,
        beta: float | None = None,
        alpha: float | None = None,
        sersicIndex: float | None = None,
        innercutoffradius: float | None = None,
        outercutoffradius: float | None = None,
        cutoffstrength: float | None = None,
        surfacedensity: float | None = None,
        densitynorm: float | None = None,
        w0: float | None = None,
        trunc: float | None = None,
        center: tuple[float, float, float] | str | None = None,
        orientation: tuple[float, float, float] | None = None,
        rotation: float | str | None = None,
        scale: tuple[float, float] | str | None = None,
    ) -> None: ...
    def density(
        self,
        xyz: tuple[float, float, float] | _ToArray2D[np.float64],
        t: float | _ToArray1D[np.float64] | None = None,
    ) -> float | _Array1D[np.float64]: ...
    def projectedDensity(  # noqa: N802
        self,
        xyz: float | _ToArray1D[np.float64],
    ) -> float | _Array1D[np.float64]: ...
    def export(self, filename: str) -> None: ...
    def sample(
        self,
        n: int,
        potential: Potential | None = None,
        beta: float | None = None,
        kappa: float | None = None,
    ) -> tuple[list[list[float]], list[float]]: ...
    def totalMass(self) -> float: ...
    def enclosedMass(self, r: float | Sequence[float]) -> float | list[float]: ...
    def principalAxes(self, r: float | _ToArray1D[np.float64] | None = None) -> tuple[list[float], list[float]]: ...
    def name(self) -> str: ...
    def __getitem__(self, index: int) -> Density | Potential: ...
    def __len__(self) -> int: ...
    def __add__(self, other: Density) -> Density: ...

class Potential(Density):
    def __init__(
        self,
        *args: Any,
        type: str | None = None,
        density: str | Density | Callable[[Any], Any] | None = None,
        potential: Potential | Callable[[Any], Any] | None = None,
        file: str | None = None,
        particles: tuple[Any, Any] | None = None,
        symmetry: str | None = None,
        gridSizeR: int | None = None,  # noqa: N803
        gridSizeZ: int | None = None,  # noqa: N803
        rmin: float | None = None,
        rmax: float | None = None,
        zmin: float | None = None,
        zmax: float | None = None,
        lmax: int | None = None,
        mmax: int | None = None,
        smoothing: float | None = None,
        nmax: int | None = None,
        eta: float | None = None,
        r0: float | None = None,
        center: Sequence[float] | str | None = None,
        orientation: Sequence[float] | None = None,
        rotation: float | Sequence[float] | str | None = None,
        scale: Sequence[float] | str | None = None,
    ) -> None: ...
    def potential(
        self,
        x: float | Sequence[float] | Sequence[Sequence[float]],
        y: float | None = None,
        z: float | None = None,
        t: float | Sequence[float] | None = None,
    ) -> float | list[float]: ...
    def force(
        self,
        x: float | Sequence[float] | Sequence[Sequence[float]],
        y: float | None = None,
        z: float | None = None,
        t: float | Sequence[float] | None = None,
    ) -> list[float] | list[list[float]]: ...
    def forceDeriv(
        self,
        x: float | Sequence[float] | Sequence[Sequence[float]],
        y: float | None = None,
        z: float | None = None,
        t: float | Sequence[float] | None = None,
    ) -> Any: ...
    def eval(
        self,
        x: float | Sequence[float] | Sequence[Sequence[float]],
        y: float | None = None,
        z: float | None = None,
        t: float | Sequence[float] | None = None,
    ) -> tuple[Any, Any, Any]: ...
    def projectedEval(
        self,
        X: float | Sequence[float] | Sequence[Sequence[float]],  # noqa: N803
        Y: float | None = None,  # noqa: N803
        alpha: float | Sequence[float] | None = 0,
        beta: float | Sequence[float] | None = 0,
        gamma: float | Sequence[float] | None = 0,
        t: float | Sequence[float] | None = 0,
    ) -> tuple[Any, Any, Any]: ...
    def Rcirc(self, E: float | Sequence[float], Lz: float | Sequence[float] | None = None) -> float | list[float]: ...  # noqa: N803
    def Tcirc(self, E: float | Sequence[float], Lz: float | Sequence[float] | None = None) -> float | list[float]: ...  # noqa: N803
    def Rmax(self, E: float | Sequence[float], Lz: float | Sequence[float] | None = None) -> float | list[float]: ...  # noqa: N803
    def Rperiapo(
        self,
        E: float | Sequence[float],
        Lz: float | Sequence[float] | None = None,  # noqa: N803
    ) -> tuple[float | list[float], float | list[float]]: ...

class Spline: ...
