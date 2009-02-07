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