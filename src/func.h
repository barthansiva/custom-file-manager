#ifndef FUNC_h
#define FUNC_h
// this header is ideally for functionality based... functions
// stuff like moving, creating or removing files

int removeFile();
int createFile();
int moveFile();
int renameFile();

// this might be called by our search bar
int searchFile();

// opens file with whatever viewer is available
// (installs a virus if no viewer is available)
int viewFile();

#endif