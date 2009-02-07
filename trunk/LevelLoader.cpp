#include <stdio.h>
#include <windows.h>		
#include <gl\gl.h>			
#include <gl\glu.h>			
#include "Level.h"

char	LevelName[64];
char	Author[64];
Vector	Size;
Start   Startpos;
Vector  Camera;
Vector  CameraMin;
Vector  CameraMax;
int		Grid[16][16][16];
int		GridNumber[16][16][16];
int     GridNeighbor[16][16][16][6];
int     GridAdjacent[16][16][16][6];

int		NumCells;
Cell	Cells[32];

int		NumTargets;
Target	Targets[32];

char TBuffer[1024];

int ReadLineIgnoreComments(FILE*f, char* dest, int maxChars)
{
	int charsRead=0;
	while(maxChars>1)
	{
		int ch = fgetc(f);

		if(ch=='#')
		{
			while(ch!='\n')
			{
				ch = fgetc(f);
				if((ch==EOF)||(ch==0))
				{
					*(dest)=0;
					return charsRead;
				}
			}
		}
		else if(((ch=='\n')&&(charsRead!=0))||(ch==EOF)||(ch==0))
		{
			*(dest)=0;
			return charsRead;
		}
		else
		{
			if(ch==9) ch=32;
			if(ch>=32)
			{
				*(dest++) = ch;
				maxChars--;
				charsRead++;
			}
		}
	}
	*(dest)=0;
	return charsRead;
}

bool StrBegins(const char* a, const char*b) // check if a and b match until one of them ends
{
	while((*a!=NULL)&&(*b!=NULL))
	{
		int ca = *a;
		int cb = *b;
		if(ca!=cb) return false;
		a++;
		b++;
	}
	return true;
}

bool ParseCamera(char *src)
{
	Vector s;

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	s.Y = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	s.X = atoi(src);

	Camera = s;

	return true;
}

bool ParseLimits(char *src)
{
	Vector mn;
	Vector mx;

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	mn.Y = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	mx.Y = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	mn.X = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	mx.X = atoi(src);

	CameraMin = mn;
	CameraMax = mx;

	return true;
}

bool ParseStart(char *src)
{
	Start s;

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	s.X = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	s.Y = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	s.Z = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	s.D = atoi(src);

	Startpos = s;

	return true;
}

bool ParseTarget(char *src)
{
	Target t;

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	t.X = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	t.Y = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	t.Z = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	t.N = atoi(src);

	t.A = false;

	Targets[NumTargets++] = t;

	return true;
}

bool ParseCell(char *src)
{
	Cell c;

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	c.X = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	c.Y = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	c.Z = atoi(src);

	while((*src!=0)&&(*src!=' '))
	{
		src++;
	}

	if(*src==0) return false;
	if(*src==' ') src++;

	c.F = atoi(src);
	Grid[c.X][c.Y][c.Z] = c.F;
	GridNumber[c.X][c.Y][c.Z] = NumCells;

	Cells[NumCells++] = c;

	Size.X = max(Size.X,c.X+1);
	Size.Y = max(Size.Y,c.Y+1);
	Size.Z = max(Size.Z,c.Z+1);

	return true;
}

bool LoadLevel(char* filename)
{
	FILE *fin = fopen(filename,"r");

	if(!fin) return false;

	bool VersionOk = false;

	while(ReadLineIgnoreComments(fin,TBuffer,1024))
	{
		if(StrBegins(TBuffer,"ECHO CLONE LEVEL VERSION "))
		{
			if(strcmp(TBuffer,"ECHO CLONE LEVEL VERSION 1")==0)
			{
				VersionOk = true;
			}
		}
		else
		{
			if(!VersionOk)
			{
				fclose(fin);
				return false;
			}

			if(StrBegins(TBuffer,"LEVEL "))
			{
				strncpy(LevelName,TBuffer+6,63);

				// avoid buffer corruption
				LevelName[63]=0;
			}
			else if(StrBegins(TBuffer,"AUTHOR "))
			{
				strncpy(Author,TBuffer+7,63);

				// avoid buffer corruption
				Author[63]=0;
			}
			else if(StrBegins(TBuffer,"START "))
			{
				if(!ParseStart(TBuffer))
				{
					fclose(fin);
					return false;
				}
			}
			else if(StrBegins(TBuffer,"CAMERA "))
			{
				if(!ParseCamera(TBuffer))
				{
					fclose(fin);
					return false;
				}
			}
			else if(StrBegins(TBuffer,"LIMITS "))
			{
				if(!ParseLimits(TBuffer))
				{
					fclose(fin);
					return false;
				}
			}
			else if(StrBegins(TBuffer,"TARGET "))
			{
				if(!ParseTarget(TBuffer))
				{
					fclose(fin);
					return false;
				}
			}
			else if(StrBegins(TBuffer,"CELL "))
			{
				if(!ParseCell(TBuffer))
				{
					fclose(fin);
					return false;
				}
			}
		}
	}

	fclose(fin);

	if(VersionOk)
	{
		// find gaps & mark them

		for(int z=0;z<=Size.Z;z++)
		{
			for(int y=0;y<=Size.Y;y++)
			{
				for(int x=0;x<=Size.X;x++)
				{
					if(Grid[x][y][z]==0)
					{
						bool added = false;

						if((x>0)&&(x<Size.X))
						{
							if ((Grid[x-1][y][z]>0)&&(Grid[x+1][y][z]>0)&&
								(Grid[x-1][y][z]!=128)&&(Grid[x+1][y][z]!=128))
							{
								if( (y==Size.Y)||(
									((Grid[x-1][y+1][z]==0)||(Grid[x-1][y+1][z]==128))&&
									((Grid[x+1][y+1][z]==0)||(Grid[x+1][y+1][z]==128))))
								{
									Cell c;

									c.X=x;
									c.Y=y;
									c.Z=z;
									c.F=128;
									Grid[x][y][z] = c.F;
									GridNumber[x][y][z] = NumCells;
									Cells[NumCells++] = c;
									added=true;
								}
							}
						}

						if(!added)
						{
							if((z>0)&&(z<Size.Z))
							{
								if ((Grid[x][y][z-1]>0)&&(Grid[x][y][z+1]>0)&&
									(Grid[x][y][z-1]!=128)&&(Grid[x][y][z+1]!=128))
								{
									if( (y==Size.Y)||(
										((Grid[x][y+1][z-1]==0)||(Grid[x][y+1][z-1]==128))&&
										((Grid[x][y+1][z+1]==0)||(Grid[x][y+1][z+1]==128))))
									{
										Cell c;

										c.X=x;
										c.Y=y;
										c.Z=z;
										c.F=128;
										Grid[x][y][z] = c.F;
										GridNumber[x][y][z] = NumCells;
										Cells[NumCells++] = c;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return VersionOk;
}
