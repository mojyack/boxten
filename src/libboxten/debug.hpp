/* This is an internal header, which will not be installed. */
#pragma once
#include <iostream>
#if defined(DEBUG)
#define DEBUG_OUT(message) std::cout << message << __FILE__ << " : " << __func__ << "(" << __LINE__ << ")" << std::endl;
#else
#define DEBUG_OUT(message) 
#endif
