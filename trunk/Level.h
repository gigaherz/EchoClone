//EchoClone - A Perspective Puzzle game
//Copyright (C) 2009  David Quintana <gigaherz@gmail.com>
//
//This program is free software; you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation; either version 2 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License along
//with this program; if not, write to the Free Software Foundation, Inc.,
//51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#pragma once

struct Vector {
	int X;
	int Y;
	int Z;
};

struct Start {
	int X;
	int Y;
	int Z;
	int D;
};

struct Cell {
	int X;
	int Y;
	int Z;
	int F;
};

struct Target {
	int X;
	int Y;
	int Z;
	int N; // number of targets required to show up
	bool A; // Acquired
};

extern char	LevelName[64];
extern char	Author[64];
extern Vector Size;
extern Start  Startpos;
extern Vector Camera;
extern Vector CameraMin;
extern Vector CameraMax;
extern int    Grid[16][16][16];
extern int    GridNumber[16][16][16];
extern int    GridNeighbor[16][16][16][6];
extern int    GridAdjacent[16][16][16][6];

extern int    NumCells;
extern Cell   Cells[32];

extern int		NumTargets;
extern Target	Targets[32];

extern char TBuffer[1024];

bool LoadLevel(char* filename);