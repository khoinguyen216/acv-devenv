#pragma once

// Define a macro for exporting and importing DLL.
// LBPDETECTOR_API is used for exporting DLL, when LBPDETECTOR_EXPORTS has been defined.
// LBPDETECTOR_API is used for importing DLL, when LBPDETECTOR_EXPORTS hasn't been defined.
#ifdef WIN32
#ifdef LBPDETECTOR_EXPORTS
#define LBPDETECTOR_API __declspec(dllexport)
#else
#define LBPDETECTOR_API __declspec(dllimport)
#endif
#else
//For cygwin
#define LBPDETECTOR_API
#endif

#ifdef LBPDETECTOR_API_STATIC
#define LBPDETECTOR_API
#endif
