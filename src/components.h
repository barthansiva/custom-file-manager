#ifndef COMPONENTS_h
#define COMPONENTS_h
// this is meant for the smaller components like buttons, idk if we'll need this with Glade 
// but I'll at least put it here for now anyways

typedef struct {
    char* label;
    int x;
    int y;
    int width;
    int height;
} Button;

// like for a search bar or smn
typedef struct {
    char* input;
    int x;
    int y;
    int width;
    int height;
} Textspace;

// The ''''Object'''' for showing files in our file menu
typedef struct {
    char* fileName;
    char* pathToPreviewImage;
} Filepreview;


#endif