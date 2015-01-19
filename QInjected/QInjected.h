#ifndef __QINJECTED_H__
#define __QINJECTED_H__

#define UNICODE
#define _UNICODE
#include <Winsock2.h>
#include "../QInjector/Tools.h"

/*  To use this exported function of dll, include this header
 *  in your project.
 */

#ifdef BUILD_DLL
    #define DLL_EXPORT __declspec(dllexport)
#else
    #define DLL_EXPORT __declspec(dllimport)
#endif


#ifdef __cplusplus
extern "C"
{
#endif

//void DLL_EXPORT SomeFunction();

#ifdef __cplusplus
}
#endif

#endif // __MAIN_H__
