#ifndef HANDLERS_h
#define HANDLERS_h
// I assume we'll need handlers to deal with individual elements, so this is for stuff
// like a sidebar handler
// These will presumably be threads

// the master handler incase we need such frivolities
// this would then call all the other handlers instead of having UI call them all individually
void handlerHandler();

void sideBarHandler();
void searchBarHandler();
void toolbarHandler();

#endif