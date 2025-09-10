from typing import Literal as L
from typing import overload

import numpy as np
from agama._potential_typing import _Potential
from optype import numpy as onp

type _Triplet = tuple[float, float, float]
type _Sextet = tuple[float, float, float, float, float, float]

type _TripletArray = onp.Array2D[np.float64]
type _SextetArray = onp.ToJustFloat16_2D

# 1 actions=False, angles=False, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[False],
    angles: L[False] = False,
    frequencies: L[False],
) -> None: ...

# 2 actions=False, angles=False, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[False],
    angles: L[False] = False,
    frequencies: L[True],
) -> tuple[_Triplet]: ...

# 3 actions=False, angles=False, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[False],
    angles: L[False] = False,
    frequencies: L[None] = None,
) -> None: ...

# 4 actions=False, angles=True, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[False],
    angles: L[True],
    frequencies: L[False],
) -> tuple[_Triplet]: ...

# 5 actions=False, angles=True, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[False],
    angles: L[True],
    frequencies: L[True],
) -> tuple[_Triplet, _Triplet]: ...

# 6 actions=False, angles=True, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[False],
    angles: L[True],
    frequencies: L[None] = None,
) -> tuple[_Triplet, _Triplet]: ...

# 7 actions=True, angles=False, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[False] = False,
    frequencies: L[False],
) -> tuple[_Triplet]: ...

# 8 actions=True, angles=False, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[False] = False,
    frequencies: L[True],
) -> tuple[_Triplet, _Triplet]: ...

# 9 actions=True, angles=False, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[False] = False,
    frequencies: L[None] = None,
) -> tuple[_Triplet]: ...

# 10 actions=True, angles=True, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[True],
    frequencies: L[False],
) -> tuple[_Triplet, _Triplet]: ...

# 11 actions=True, angles=True, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[True],
    frequencies: L[True],
) -> tuple[_Triplet, _Triplet, _Triplet]: ...

# 12 actions=True, angles=True, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _Sextet,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[True],
    frequencies: L[None] = None,
) -> tuple[_Triplet, _Triplet, _Triplet]: ...

# -- array version --

# 1 actions=False, angles=False, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[False],
    angles: L[False] = False,
    frequencies: L[False],
) -> None: ...

# 2 actions=False, angles=False, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[False],
    angles: L[False] = False,
    frequencies: L[True],
) -> tuple[_TripletArray]: ...

# 3 actions=False, angles=False, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[False],
    angles: L[False] = False,
    frequencies: L[None] = None,
) -> None: ...

# 4 actions=False, angles=True, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[False],
    angles: L[True],
    frequencies: L[False],
) -> tuple[_TripletArray]: ...

# 5 actions=False, angles=True, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[False],
    angles: L[True],
    frequencies: L[True],
) -> tuple[_TripletArray, _TripletArray]: ...

# 6 actions=False, angles=True, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[False],
    angles: L[True],
    frequencies: L[None] = None,
) -> tuple[_TripletArray, _TripletArray]: ...

# 7 actions=True, angles=False, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[False] = False,
    frequencies: L[False],
) -> tuple[_TripletArray]: ...

# 8 actions=True, angles=False, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[False] = False,
    frequencies: L[True],
) -> tuple[_TripletArray, _TripletArray]: ...

# 9 actions=True, angles=False, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[False] = False,
    frequencies: L[None] = None,
) -> tuple[_TripletArray]: ...

# 10 actions=True, angles=True, frequencies=False
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[True],
    frequencies: L[False],
) -> tuple[_TripletArray, _TripletArray]: ...

# 11 actions=True, angles=True, frequencies=True
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[True],
    frequencies: L[True],
) -> tuple[_TripletArray, _TripletArray, _TripletArray]: ...

# 12 actions=True, angles=True, frequencies=None
@overload
def actions(
    *,
    potential: _Potential,
    point: _SextetArray,
    fd: float = 0.0,
    actions: L[True] = True,
    angles: L[True],
    frequencies: L[None] = None,
) -> tuple[_TripletArray, _TripletArray, _TripletArray]: ...
