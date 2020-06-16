/*
	Copyright (C) 2020 Minho Jo <whitestone8214@gmail.com>
	
	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU Lesser General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.
	
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Lesser General Public License for more details.
	
	You should have received a copy of the GNU Lesser General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


// Internal header(s)
#include <byter.h>

// Standard C header(s)
#include <fcntl.h>


#ifndef TURNED_ON_FILER
#define TURNED_ON_FILER

char *filer_normalize_path(char *path, char *here);
char *filer_place(char *path, char *here);
char *filer_name(char *path, char *here);
byte filer_status(char *path, char *here, char *question);
char *filer_type(char *path, char *here, byte rideLink);
long filer_size(char *path, char *here, byte rideLink);
int filer_permission(char *path, char *here);
byte filer_create(char *path, char *here, char *type, char *linkTo, int permission);
char *filer_read(char *path, char *here);
byte filer_write(char *path, char *here, char *data, int permission);
byte filer_copy(char *from, char *to, char *here);
byte filer_delete(char *path, char *here);
byte filer_move(char *from, char *to, char *here);

#endif
