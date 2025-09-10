from collections.abc import Callable
from typing import Any, Literal, overload

import numpy as np
from optype import numpy as onp

type _Potential = Potential | Callable[[onp.Array2D[np.floating[Any]]], onp.Array1D[np.bool_ | np.float32 | np.float64]]
type _Density = (
    Density | dict[str, object] | Callable[[onp.Array2D[np.floating[Any]]], onp.Array1D[np.bool_ | np.float32 | np.float64]]
)

type _PotentialType = Literal[
    "logarithmic",
    "harmonic",
    "keplerbinary",
    "nfw",
    "plummer",
    "dehnen",
    "ferrers",
    "isochrone",
    "nuker",
    "basisSet",
    "multipole",
    "cylspline",
    "miyamotonagai",
    "king",
    "evolving",
    "uniformacceleration",
    "perfectellipsoid",
    "densitysphericalharmonic",
    "densityazimuthalharmonic",
]
type _DensityType = Literal[
    "dehnen",
    "plummer",
    "perfectellipsoid",
    "ferrers",
    "miyamotonagai",
    "nfw",
    "disk",
    "spheroid",
    "nuker",
    "sersic",
    "king",
]

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
