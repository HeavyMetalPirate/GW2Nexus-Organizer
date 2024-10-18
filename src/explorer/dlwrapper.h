#pragma once
#ifdef __linux__
#define LIBTYPE void*
#define OPENLIB(libname) dlopen((libname), RTLD_LAZY)
#define LIBFUNC(lib, fn) dlsym((lib), (fn))
#define CLOSELIB(lib) dlclose((lib))
#elif defined(WINVER)
#define LIBTYPE HINSTANCE
#define OPENLIB(libname) LoadLibraryW(L ## libname)
#define LIBFUNC(lib, fn) GetProcAddress((lib), (fn))
#define CLOSELIB(lib) FreeLibrary((lib))
#endif