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

/* gcc -I. $(pkg-config --cflags --libs byter) filer.c -shared -o libfiler.so */


#include <filer.h>


// Internal use only
char *filer_place_unsafe(char *path);
byte filer_status_unsafe(char *path, char *question);
char *filer_type_unsafe(char *path, byte rideLink);
long filer_size_unsafe(char *path, byte rideLink);
int filer_permission_unsafe(char *path);
char *filer_read_unsafe(char *path);
byte filer_write_unsafe(char *path, char *data, int permission);
byte filer_mkdir_unsafe(char *path, int permission);
byte filer_copy_unsafe(char *from, char *to);
byte filer_delete_unsafe(char *path);
byte filer_move_unsafe(char *from, char *to);


char *filer_normalize_path(char *path, char *here) {
	// NULL is NULL
	if (byter_check_empty(path) == 1) return NULL;
	char *_path = NULL;
	
	// Absolute
	if (path[0] == '/') _path = strdup(path);
	else {
		if ((strcmp(path, "~") == 0) || (strcmp(path, "~/") == 0)) {
			// Relative: Just home directory
			char *_home = getenv("HOME");
			if (byter_check_empty(_home) == 1) _home = "/";
			return strdup(_home);
		}
		else if ((strlen(path) >= 3) && (strncmp(path, "~/", 2) == 0)) {
			// Relative: Starts from home directory
			char *_home = getenv("HOME");
			if (byter_check_empty(_home) == 1) _home = "/";
			_path = byter_connect_strings(3, _home, "/", &path[2]);
		}
		else {
			// Relative: Starts from here
			char *_here = filer_normalize_path(here, NULL);
			if (_here == NULL) _here = strdup("/");
			_path = byter_connect_strings(3, _here, "/", path);
			if (_here == NULL) free(_here);
		}
	}
	
	// Create array for slices
	cell *_arraySlices = cell_create(NULL);
	if (_arraySlices == NULL) {
		if (_path != NULL) free(_path);
		return NULL;
	}
	
	// Split path into slices
	int _spotA = -1;
	int _spotB;
	for (_spotB = 0; _spotB < strlen(_path) + 1; _spotB++) {
		// Pass the non-slashes
		if ((_spotB < strlen(_path)) && (_path[_spotB] != '/')) continue;
		
		// Pass the slash-after-slash
		if (_spotB - _spotA <= 1) {_spotA = _spotB; continue;}
		
		// Dump the slice
		char *_slice = byter_take_part(_path, _spotA + 1, _spotB - 1);
		//printf("[filer] SAY slice %s\n", _slice);
		//printf("[filer] SAY filer_normalize_path 2 %s (%d + 1 ~ %d - 1) %s\n", _path, _spotA, _spotB, _slice);
		_spotA = _spotB;
		if (_slice == NULL) {cell_delete_all(_arraySlices, 1); return NULL;}
		_arraySlices->data = (void *)_slice;
		cell_insert(_arraySlices, -1, NULL);
		_arraySlices = _arraySlices->next;
	}
	
	// Remove single-dot and double-dots
	_arraySlices = cell_nth(_arraySlices, 0);
	while (_arraySlices->data != NULL) {
		char *_slice = (char *)_arraySlices->data;
		//printf("[filer] SAY filer_normalize_path 3 %s\n", _slice);
		if (strcmp(_slice, ".") == 0) {
			if (_arraySlices->previous != NULL) {
				// Case "/a/.": Go to previous, and burn the next
				_arraySlices = _arraySlices->previous;
				cell_delete(_arraySlices->next, 1);
			}
			else {
				// Case "/.": Go to next, and burn the previous
				_arraySlices = _arraySlices->next;
				cell_delete(_arraySlices->previous, 1);
			}
		}
		else if (strcmp(_slice, "..") == 0) {
			if (_arraySlices->previous != NULL) {
				_arraySlices = _arraySlices->previous;
				cell_delete(_arraySlices->next, 1);
				if (_arraySlices->previous != NULL) {
					// Case "/a/b/..": Once more, go to previous, and burn the next
					_arraySlices = _arraySlices->previous;
					cell_delete(_arraySlices->next, 1);
				}
				else {
					// Case "/a/..": Go to next, and burn the previous
					_arraySlices = _arraySlices->next;
					cell_delete(_arraySlices->previous, 1);
				}
			}
			else {
				// Case "/..": Go to next, and burn the previous
				_arraySlices = _arraySlices->next;
				cell_delete(_arraySlices->previous, 1);
			}
		}
		else _arraySlices = _arraySlices->next;
	}
	_arraySlices = cell_nth(_arraySlices, 0);
	
	// Now assemble the remaining slices
	char *_path1 = NULL;
	if ((cell_length(_arraySlices) == 1) && (_arraySlices->data == NULL)) _path1 = strdup("/");
	else {
		while (_arraySlices->data != NULL) {
			//printf("[filer] SAY filer_normalize_path 4a %s\n", _path1);
			if (_path1 == NULL) _path1 = byter_connect_strings(2, "/", (char *)_arraySlices->data);
			else {
				char *_path2 = byter_connect_strings(3, _path1, "/", (char *)_arraySlices->data);
				free(_path1);
				_path1 = _path2;
			}
			//printf("[filer] SAY filer_normalize_path 4b %s\n", _path1);
			_arraySlices = _arraySlices->next;
		}
	}
	//printf("[filer] SAY filer_normalize_path end2 %s\n", _path1);
	
	// Done!
	cell_delete_all(_arraySlices, 1);
	return _path1;
}
char *filer_place(char *path, char *here) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return NULL;
	
	// Get the place
	char *_place = filer_place_unsafe(_path);
	free(_path);
	return _place;
}
char *filer_name(char *path, char *here) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return NULL;
	
	// Root's name is root
	if (strcmp(_path, "/") == 0) return strdup("/");
	
	// Find the last slash
	int _nth;
	for (_nth = strlen(_path) - 1; _nth >= 0; _nth--) {if (_path[_nth] == '/') break;}
	
	// Get the name
	char *_name = strdup(&_path[_nth + 1]);
	free(_path);
	return _name;
}
byte filer_status(char *path, char *here, char *question) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return 0;
	
	// Answer to question
	byte _answer = filer_status_unsafe(_path, question);
	free(_path);
	return _answer;
}
char *filer_type(char *path, char *here, byte rideLink) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return NULL;
	
	// File exists?
	if (filer_status_unsafe(_path, "exists") != 1) {free(_path); return NULL;}
	
	// Get the type
	char *_type = filer_type_unsafe(_path, rideLink);
	free(_path);
	return _type;
}
long filer_size(char *path, char *here, byte rideLink) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return -1;
	
	long _size = filer_size_unsafe(_path, rideLink);
	free(_path);
	return _size;
}
int filer_permission(char *path, char *here) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return NULL;
	
	int _permission = filer_permission_unsafe(_path);
	free(_path);
	return _permission;
}
byte filer_create(char *path, char *here, char *type, char *linkTo, int permission) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return 0;
	
	// Prepare the place
	char *_place = filer_place_unsafe(_path);
	if (_place == NULL) {free(_path); return 0;}
	if (strcmp(_place, "/") != 0) {
		if (filer_mkdir_unsafe(_place, permission) != 1) {free(_place); return 0;}
	}
	free(_place);
	
	// Create the file
	byte _answer = 0;
	if (strcmp(type, "file") == 0) {
		int _answer0 = open(_path, O_RDWR | O_CREAT | O_EXCL | O_SYNC, permission);
		if (_answer0 == 0) {close(_answer0); _answer = 1;}
		else if (errno == EEXIST) _answer = 1;
	}
	else if (strcmp(type, "directory") == 0) _answer = filer_mkdir_unsafe(_path, permission);
	else if (strcmp(type, "link") == 0) {
		if (byter_check_empty(linkTo) != 1) {
			if (symlink(linkTo, _path) == 0) _answer = 1;
			else if (errno == EEXIST) {
				char *_type = filer_type_unsafe(_path, 0);
				if (strcmp(_type, "Symbolic link") == 0) _answer = 1;
				if (_type != NULL) free(_type);
			}
		}
	}
	
	// Done!
	free(_path);
	return _answer;
}
char *filer_read(char *path, char *here) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return NULL;
	
	// Read the contents
	char *_contents = filer_read_unsafe(path);
	free(_path);
	return _contents;
}
byte filer_write(char *path, char *here, char *data, int permission) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return NULL;
	
	// Write data to file
	byte _answer = filer_write_unsafe(_path, data, permission);
	free(_path);
	return _answer;
}
byte filer_copy(char *from, char *to, char *here) {
	// Normalize first
	char *_from = filer_normalize_path(from, here);
	if (_from == NULL) return 0;
	if (strcmp(_from, "/") == 0) {free(_from); return 0;}
	char *_to = filer_normalize_path(to, here);
	if (_to == NULL) {free(_from); return 0;}
	
	// Copy file
	byte _answer = filer_copy_unsafe(_from, _to);
	free(_from);
	free(_to);
	return _answer;
}
byte filer_delete(char *path, char *here) {
	// Normalize first
	char *_path = filer_normalize_path(path, here);
	if (_path == NULL) return 0;
	
	// Scary! Why you want to delete root directory?
	if (strcmp(_path, "/") == 0) {free(_path); return 0;}
	
	// Copy file
	byte _answer = filer_delete_unsafe(_path);
	free(_path);
	return _answer;
}
byte filer_move(char *from, char *to, char *here) {
	// Normalize first
	char *_from = filer_normalize_path(from, here);
	if (_from == NULL) return 0;
	if (strcmp(_from, "/") == 0) {free(_from); return 0;}
	char *_to = filer_normalize_path(to, here);
	if (_to == NULL) {free(_from); return 0;}
	
	// Copy file
	byte _answer = filer_move_unsafe(_from, _to);
	free(_from);
	free(_to);
	return _answer;
}

char *filer_place_unsafe(char *path) {
	// NULL is at NULL
	if (path == NULL) return NULL;
	
	// Root is at root
	if (strcmp(path, "/") == 0) return strdup("/");
	
	// Find the last slash
	int _nth;
	for (_nth = strlen(path) - 1; _nth >= 0; _nth--) {if (path[_nth] == '/') break;}
	
	if (_nth <= 0) return strdup("/");
	else return strndup(&path[0], _nth);
}
byte filer_status_unsafe(char *path, char *question) {
	int _answer = -1;
	if (strcmp(question, "exists") == 0) _answer = access(path, F_OK);
	else if (strcmp(question, "readable") == 0) _answer = access(path, R_OK);
	else if (strcmp(question, "writable") == 0) _answer = access(path, W_OK);
	else if (strcmp(question, "executable") == 0) _answer = access(path, X_OK);
	return (_answer == 0) ? 1 : 0;
}
char *filer_type_unsafe(char *path, byte rideLink) {
	// Get file profile
	struct stat _profile;
	if (rideLink == 1) {if (stat(path, &_profile) != 0) return NULL;}
	else {if (lstat(path, &_profile) != 0) return NULL;}
	
	// Get the type
	int _type = _profile.st_mode / 010000;
	if (_type == 01) return strdup("FIFO");
	else if (_type == 02) return strdup("Character device");
	else if (_type == 04) return strdup("Directory");
	else if (_type == 06) return strdup("Block device");
	else if (_type == 010) return strdup("Regular file");
	else if (_type == 012) return strdup("Symbolic link");
	else if (_type == 014) return strdup("Socket");
	else return NULL;
}
long filer_size_unsafe(char *path, byte rideLink) {
	// Get file profile
	struct stat _profile;
	if (rideLink == 1) {if (stat(path, &_profile) != 0) return -1;}
	else {if (lstat(path, &_profile) != 0) return -1;}
	
	// Get the size
	return _profile.st_size;
}
int filer_permission_unsafe(char *path) {
	struct stat _profile;
	if (stat(path, &_profile) != 0) return 0;
	
	return _profile.st_mode % 010000;
}
char *filer_read_unsafe(char *path) {
	// Get the size
	long _size = filer_size_unsafe(path, 1);
	
	// Open the file
	int _file = open(path, O_RDONLY);
	if (_file == -1) return NULL;
	
	// Read the contents
	char *_contents = (char *)malloc(sizeof(char) * (_size + 1));
	if (_contents == NULL) return NULL;
	memset(&_contents[0], 0, _size + 1);
	read(_file, &_contents[0], _size);
	
	// Done!
	close(_file);
	return _contents;
}
byte filer_write_unsafe(char *path, char *data, int permission) {
	// Open the file
	int _file = open(path, O_RDWR | O_CREAT | O_TRUNC, permission);
	if (_file == -1) return 0;
	
	// Write data to file
	byte _answer = (write(_file, &data[0], strlen(data)) != 1) ? 1 : 0;
	
	// Done!
	close(_file);
	return 1;
}
byte filer_mkdir_unsafe(char *path, int permission) {
	// Sanity check
	if (path == NULL) return 0;
	
	// Should we create root?
	if (strcmp(path, "/") == 0) return 1;
	
	// Prepare the place
	char *_place = filer_place_unsafe(path);
	byte _answer = filer_mkdir_unsafe(_place, permission);
	if (_place != NULL) free(_place);
	if (_answer != 1) return 0;
	
	// Already exists?
	if (filer_status_unsafe(path, "exists") == 1) {
		char *_type = filer_type_unsafe(path, 0);
		if (strcmp(_type, "Directory") == 0) {free(_type); return 1;}
		else if (strcmp(_type, "Symbolic link") == 0) {
			char *_type1 = filer_type_unsafe(path, 1);
			if (strcmp(_type1, "Directory") == 0) {free(_type1); free(_type); return 1;}
			else {free(_type1); free(_type); return 0;}
		}
		else {free(_type); return 0;}
	}
	
	// Calculate the proper permission
	int _permission = 0;
	if (permission % 010 != 0) _permission += 07;
	if ((permission / 010) % 010 != 0) _permission += 070;
	if ((permission / 010 / 010) % 010 != 0) _permission += 0700;
	
	// Create directory
	return (mkdir(path, _permission) == 0) ? 1 : 0;
}
byte filer_copy_unsafe(char *from, char *to) {
	// Prepare the room for answer
	byte _answer = 0;
	
	// Copy the file
	char *_type = filer_type_unsafe(from, 0);
	if (_type == NULL) return 0;
	if (strcmp(_type, "Directory") == 0) {
		// Create target directory
		if (filer_mkdir_unsafe(to, filer_permission_unsafe(from)) != 1) {free(_type); return 0;}
		
		// Copy children
		DIR *_directory = opendir(from);
		if (_directory == NULL) {free(_type); return 0;}
		struct dirent *_file = readdir(_directory);
		while (_file != NULL) {
			// Skip the dots
			if ((strcmp(_file->d_name, ".") == 0) || (strcmp(_file->d_name, "..") == 0)) {_file = readdir(_directory); continue;}
			
			// Copy the file
			char *_from = byter_connect_strings(3, from, "/", _file->d_name);
			char *_to = (strcmp(to, "/") == 0) ? byter_connect_strings(2, to, _file->d_name) : byter_connect_strings(3, to, "/", _file->d_name);
			_answer = filer_copy_unsafe(_from, _to);
			if (_from != NULL) free(_from);
			if (_to != NULL) free(_to);
			if (_answer != 1) break;
			
			// Next!
			_file = readdir(_directory);
		}
		closedir(_directory);
	}
	else if (strcmp(_type, "Regular file") == 0) {
		// Only do copy if destination is vacant or same regular file
		char *_typeTo = filer_type_unsafe(to, 1);
		if (_typeTo != NULL) {
			if (strcmp(_typeTo, "Regular file") != 0) {free(_typeTo); free(_type); return 0;}
			free(_typeTo);
		}
		
		// Copy the file
		char *_contents = filer_read_unsafe(from);
		if (byter_check_empty(_contents) == 1) {
			int _fileTo = open(to, O_RDWR | O_CREAT | O_TRUNC, filer_permission_unsafe(from));
			if (_fileTo != -1) {close(_fileTo); _answer = 1;}
		}
		else _answer = filer_write_unsafe(to, _contents, filer_permission_unsafe(from));
		if (_contents != NULL) free(_contents);
	}
	else if (strcmp(_type, "Symbolic link") == 0) {
		// Get the path linked to
		char _path[PATH_MAX + 1];
		memset(&_path[0], 0, PATH_MAX + 1);
		readlink(from, &_path[0], PATH_MAX);
		char *_pathLinkTo = strdup(_path);
		if (_pathLinkTo == NULL) {free(_type); return 0;}
		
		// Only create symbolic link if destination is vacant or same symbolic link
		char *_typeTo = filer_type_unsafe(to, 1);
		if (_typeTo != NULL) {
			if (strcmp(_typeTo, "Symbolic link") != 0) {free(_typeTo); free(_pathLinkTo); free(_type); return 0;}
			if (unlink(to) != 0) {free(_typeTo); free(_pathLinkTo); free(_type); return 0;}
			free(_typeTo);
		}
		
		// Create the link
		if (symlink(_pathLinkTo, to) == 0) _answer = 1;
		free(_pathLinkTo);
	}
	/*else if (strcmp(_type, "FIFO") == 0) {
		// Only create FIFO if destination is vacant or same FIFO
		char *_typeTo = filer_type_unsafe(to, 1);
		if (_typeTo != NULL) {
			if (strcmp(_typeTo, "FIFO") != 0) {free(_typeTo); free(_type); return 0;}
			if (unlink(to) != 0) {free(_typeTo); free(_type); return 0;}
			free(_typeTo);
		}
		
		// Create FIFO
		if (mkfifo(to, filer_permission_unsafe(from)) == 0) _answer = 1;
	}*/
	else {
		// Copying FIFO, socket, device files are not supported yet. We are finding how should we copy these files correctly.
		if (filer_status_unsafe(to, "exists") == 0) {
			int _file = open(to, O_RDWR | O_CREAT, filer_permission_unsafe(from));
			if (_file != -1) close(_file);
			_answer = 1;
		}
	}
	free(_type);
	
	// Done!
	return _answer;
}
byte filer_delete_unsafe(char *path) {
	printf("[filer] SAY delete_unsafe %s\n", path);
	
	// Prepare the room for answer
	byte _answer = 0;
	
	char *_type = filer_type_unsafe(path, 0);
	if (_type == NULL) return 0;
	if (strcmp(_type, "Directory") == 0) {
		byte _answer1 = 1;
		
		// Delete children
		DIR *_directory = opendir(path);
		if (_directory == NULL) {free(_type); return 0;}
		struct dirent *_file = readdir(_directory);
		while (_file != NULL) {
			// Skip the dots
			if ((strcmp(_file->d_name, ".") == 0) || (strcmp(_file->d_name, "..") == 0)) {_file = readdir(_directory); continue;}
			
			// Delete the file
			char *_file1 = byter_connect_strings(3, path, "/", _file->d_name);
			_answer1 = filer_delete_unsafe(_file1);
			if (_file1 != NULL) free(_file1);
			if (_answer1 != 1) break;
			
			// Next!
			_file = readdir(_directory);
		}
		closedir(_directory);
		
		// Delete directory
		if (_answer1 == 1) _answer = (rmdir(path) == 0) ? 1 : 0;
	}
	else _answer = (unlink(path) == 0) ? 1 : 0; // Delete the file
	free(_type);
	
	printf("[filer] SAY delete_unsafe %s (%d)\n", path, _answer);
	
	// Done!
	return _answer;
}
byte filer_move_unsafe(char *from, char *to) {
	// Try simply renaming
	byte _answer = (rename(from, to) == 0) ? 1 : 0;
	int _error = errno;
	if (_answer == 1) return 1;
	if (_error != EXDEV) return 0;
	
	// Try copying
	_answer = filer_copy_unsafe(from, to);
	if (_answer != 1) return 0;
	
	// Delete original files
	return filer_delete_unsafe(from);
}

